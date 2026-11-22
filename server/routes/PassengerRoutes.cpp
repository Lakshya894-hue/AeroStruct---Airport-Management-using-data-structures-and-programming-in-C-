#include "PassengerRoutes.h"
#include "AuthMiddleware.h"

#include "Passenger.h"
#include "PassengerManager.h"
#include "Flight.h"
#include "FlightManager.h"

#include "httplib.h"
#include <mysqlx/xdevapi.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// Existing helper functions defined in api_main.cpp.
const char* getDbUser();
const char* getDbPassword();

void setCorsHeaders(httplib::Response& response);
bool databaseCredentialsAvailable();

std::string trim(const std::string& value);
std::string toUpper(std::string value);
std::string jsonEscape(const std::string& value);

std::string passengerToJson(const Passenger& passenger);

bool loadPassengersIntoManager(
    mysqlx::Session& session,
    PassengerManager& manager
);

bool loadFlightsIntoManager(
    mysqlx::Session& session,
    FlightManager& manager
);

void registerPassengerRoutes(httplib::Server& server)
{
    server.Get("/api/passengers",[](const httplib::Request& request,
       httplib::Response& response)
    {
        setCorsHeaders(response);

        if (!databaseCredentialsAvailable())
        {
            response.status = 500;
            response.set_content(
                R"({"status":"error","message":"Database credentials are not configured"})",
                "application/json"
            );
            return;
        }

        try
        {
            mysqlx::Session session(
                "localhost",
                33060,
                getDbUser(),
                getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AuthUser authenticatedUser;
            if (!authorizeRequest(session, request, response, authenticatedUser))
                return;

            // Load database records into the linked list.
            PassengerManager passengerManager;

            if (!loadPassengersIntoManager(
                    session, passengerManager))
            {
                throw std::runtime_error(
                    "PassengerManager could not load a database record"
                );
            }

            // Convert linked-list records into JSON.
            std::ostringstream body;
            body << "[";
            bool firstVisiblePassenger = true;

            for (std::size_t i = 0;
                 i < passengerManager.getPassengerCount();
                 ++i)
            {
                Passenger passenger;

                if (!passengerManager.getPassengerAt(i, passenger))
                {
                    throw std::runtime_error(
                        "Could not read passenger from PassengerManager"
                    );
                }

                if (!canAccessPassenger(authenticatedUser, passenger.getPassengerId()))
                    continue;

                if (!firstVisiblePassenger)
                    body << ",";
                firstVisiblePassenger = false;

                body << passengerToJson(passenger);
            }

            body << "]";

            response.set_content(
                body.str(),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Could not load passengers: "
                << error.what() << '\n';

            response.status = 503;

            response.set_content(
                R"({"status":"error","message":"Could not load passengers from the database"})",
                "application/json"
            );
        }
    }
);


server.Post(
    "/api/passengers",
    [](const httplib::Request& request,
       httplib::Response& response)
    {
        setCorsHeaders(response);

        if (!databaseCredentialsAvailable())
        {
            response.status = 500;
            response.set_content(
                R"({"status":"error","message":"Database credentials are not configured"})",
                "application/json"
            );
            return;
        }

        try
        {
            // Read form-urlencoded parameters.
            auto readField = [&](const std::string& name)
            {
                if (!request.has_param(name))
                    return std::string();

                return trim(request.get_param_value(name));
            };

            std::string passengerId =
                toUpper(readField("passengerId"));

            std::string name = readField("name");
            std::string ageText = readField("age");
            std::string gender = readField("gender");
            std::string phoneNumber = readField("phoneNumber");

            std::string flightNumber =
                toUpper(readField("flightNumber"));

            std::string seatNumber =
                toUpper(readField("seatNumber"));

            std::string assistanceType =
                readField("assistanceType");

            if (assistanceType.empty())
                assistanceType = "None";

            // Validate required fields.
            if (passengerId.empty() ||
                name.empty() ||
                ageText.empty() ||
                gender.empty() ||
                phoneNumber.empty() ||
                flightNumber.empty() ||
                seatNumber.empty())
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"All required passenger fields must be provided"})",
                    "application/json"
                );
                return;
            }

            // Validate maximum database field lengths.
            if (passengerId.size() > 30 ||
                name.size() > 100 ||
                gender.size() > 20 ||
                phoneNumber.size() > 20 ||
                flightNumber.size() > 20 ||
                seatNumber.size() > 10 ||
                assistanceType.size() > 50)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"One or more passenger fields exceed the allowed length"})",
                    "application/json"
                );
                return;
            }

            // Passenger ID: letters, numbers, hyphens and underscores.
            bool validPassengerId = std::all_of(
                passengerId.begin(),
                passengerId.end(),
                [](unsigned char ch)
                {
                    return (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9') ||
                           ch == '-' || ch == '_';
                }
            );

            if (!validPassengerId || name.size() < 2)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid passenger ID or name"})",
                    "application/json"
                );
                return;
            }

            // Validate age.
            int age = 0;

            try
            {
                std::size_t used = 0;
                age = std::stoi(ageText, &used);

                if (used != ageText.size() ||
                    age < 0 || age > 120)
                {
                    throw std::invalid_argument("Invalid age");
                }
            }
            catch (const std::exception&)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Age must be a whole number between 0 and 120"})",
                    "application/json"
                );
                return;
            }

            // Validate phone number.
            bool validPhone = std::all_of(
                phoneNumber.begin(),
                phoneNumber.end(),
                [](unsigned char ch)
                {
                    return std::isdigit(ch) ||
                           ch == '+' || ch == '(' ||
                           ch == ')' || ch == '-' ||
                           ch == ' ';
                }
            );

            int digitCount = static_cast<int>(
                std::count_if(
                    phoneNumber.begin(),
                    phoneNumber.end(),
                    [](unsigned char ch)
                    {
                        return std::isdigit(ch);
                    }
                )
            );

            if (!validPhone ||
                phoneNumber.size() < 7 ||
                phoneNumber.size() > 16 ||
                digitCount < 7)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid phone number"})",
                    "application/json"
                );
                return;
            }

            // Connect to MySQL.
            mysqlx::Session session(
                "localhost",
                33060,
                getDbUser(),
                getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AuthUser authenticatedUser;
            if (!authorizeRequest(session, request, response, authenticatedUser, "admin"))
                return;

            // Load existing flights into the original FlightManager.
            FlightManager flightManager(10000);

            if (!loadFlightsIntoManager(session, flightManager))
            {
                throw std::runtime_error(
                    "Could not load existing flights into FlightManager"
                );
            }

            // Verify that the selected flight exists.
            Flight selectedFlight;

            if (!flightManager.findFlightRecord(
                    flightNumber, selectedFlight))
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Selected flight does not exist"})",
                    "application/json"
                );
                return;
            }

            // Prevent registration on cancelled flights.
            std::string flightStatus = selectedFlight.getStatus();

            std::transform(
                flightStatus.begin(),
                flightStatus.end(),
                flightStatus.begin(),
                [](unsigned char ch)
                {
                    return static_cast<char>(std::tolower(ch));
                }
            );

            if (flightStatus == "cancelled" ||
                flightStatus == "canceled")
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Cannot register a passenger on a cancelled flight"})",
                    "application/json"
                );
                return;
            }

            // Check whether the seat is already assigned.
            auto seatResult = session.sql(
                "SELECT passenger_id FROM passengers "
                "WHERE flight_number = ? AND seat_number = ? LIMIT 1"
            ).bind(
                flightNumber,
                seatNumber
            ).execute();

            if (seatResult.fetchOne())
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"This seat is already assigned on the selected flight"})",
                    "application/json"
                );
                return;
            }

            // Load existing passengers into PassengerManager.
            PassengerManager passengerManager;

            if (!loadPassengersIntoManager(
                    session, passengerManager))
            {
                throw std::runtime_error(
                    "Could not load existing passengers into PassengerManager"
                );
            }

            // Construct the new Passenger object.
            Passenger passenger(
                passengerId,
                name,
                age,
                gender,
                phoneNumber,
                flightNumber,
                seatNumber,
                assistanceType,
                "Registered"
            );

            // Reuse PassengerManager's duplicate-ID check.
            if (!passengerManager.addPassengerRecord(passenger))
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"Passenger ID already exists"})",
                    "application/json"
                );
                return;
            }

            // Save the accepted passenger permanently in MySQL.
            session.sql(
                "INSERT INTO passengers "
                "(passenger_id, name, age, gender, phone_number, "
                "flight_number, seat_number, assistance_type, status) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
            ).bind(
                passengerId,
                name,
                age,
                gender,
                phoneNumber,
                flightNumber,
                seatNumber,
                assistanceType,
                passenger.getStatus()
            ).execute();

            response.status = 201;

            response.set_content(
                passengerToJson(passenger),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Passenger creation failed: "
                << error.what() << '\n';

            response.status = 500;

            response.set_content(
                R"({"status":"error","message":"Could not create passenger; check server console"})",
                "application/json"
            );
        }
    }
);


// Update an existing passenger in MySQL.
server.Post(
    "/api/passengers/update",
    [](const httplib::Request& request,
       httplib::Response& response)
    {
        setCorsHeaders(response);

        if (!databaseCredentialsAvailable())
        {
            response.status = 500;
            response.set_content(
                R"({"status":"error","message":"Database credentials are not configured"})",
                "application/json"
            );
            return;
        }

        try
        {
            // Read form-urlencoded parameters.
            auto readField = [&](const std::string& field)
            {
                if (!request.has_param(field))
                    return std::string();

                return trim(request.get_param_value(field));
            };

            std::string passengerId =
                toUpper(readField("passengerId"));

            std::string name = readField("name");
            std::string ageText = readField("age");
            std::string gender = readField("gender");
            std::string phoneNumber = readField("phoneNumber");

            std::string flightNumber =
                toUpper(readField("flightNumber"));

            std::string seatNumber =
                toUpper(readField("seatNumber"));

            std::string assistanceType =
                readField("assistanceType");

            if (assistanceType.empty())
                assistanceType = "None";

            // Validate required fields.
            if (passengerId.empty() ||
                name.empty() ||
                ageText.empty() ||
                gender.empty() ||
                phoneNumber.empty() ||
                flightNumber.empty() ||
                seatNumber.empty())
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"All required passenger fields must be provided"})",
                    "application/json"
                );
                return;
            }

            // Validate maximum field lengths.
            if (passengerId.size() > 30 ||
                name.size() > 100 ||
                gender.size() > 20 ||
                phoneNumber.size() > 20 ||
                flightNumber.size() > 20 ||
                seatNumber.size() > 10 ||
                assistanceType.size() > 50)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"One or more fields exceed the allowed length"})",
                    "application/json"
                );
                return;
            }

            // Validate Passenger ID.
            bool validPassengerId = std::all_of(
                passengerId.begin(),
                passengerId.end(),
                [](unsigned char ch)
                {
                    return (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9') ||
                           ch == '-' || ch == '_';
                }
            );

            if (!validPassengerId || name.size() < 2)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid passenger ID or name"})",
                    "application/json"
                );
                return;
            }

            // Validate age.
            int age = 0;

            try
            {
                std::size_t used = 0;
                age = std::stoi(ageText, &used);

                if (used != ageText.size() ||
                    age < 0 || age > 120)
                {
                    throw std::invalid_argument("Invalid age");
                }
            }
            catch (const std::exception&)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Age must be between 0 and 120"})",
                    "application/json"
                );
                return;
            }

            // Validate phone number.
            bool validPhone = std::all_of(
                phoneNumber.begin(),
                phoneNumber.end(),
                [](unsigned char ch)
                {
                    return std::isdigit(ch) ||
                           ch == '+' || ch == '(' ||
                           ch == ')' || ch == '-' ||
                           ch == ' ';
                }
            );

            int digitCount = static_cast<int>(
                std::count_if(
                    phoneNumber.begin(),
                    phoneNumber.end(),
                    [](unsigned char ch)
                    {
                        return std::isdigit(ch);
                    }
                )
            );

            if (!validPhone ||
                phoneNumber.size() < 7 ||
                phoneNumber.size() > 16 ||
                digitCount < 7)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid phone number"})",
                    "application/json"
                );
                return;
            }

            // Connect to MySQL.
            mysqlx::Session session(
                "localhost",
                33060,
                getDbUser(),
                getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AuthUser authenticatedUser;
            if (!authorizeRequest(session, request, response, authenticatedUser, "admin"))
                return;

            // Load existing passengers into PassengerManager.
            PassengerManager passengerManager;

            if (!loadPassengersIntoManager(
                    session, passengerManager))
            {
                throw std::runtime_error(
                    "Could not load passengers into PassengerManager"
                );
            }

            // Find the passenger to update.
            Passenger existingPassenger;

            if (!passengerManager.findPassengerRecord(
                    passengerId, existingPassenger))
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Passenger not found"})",
                    "application/json"
                );
                return;
            }

            // Only Registered passengers can be edited.
            if (toUpper(existingPassenger.getStatus()) != "REGISTERED")
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"Only Registered passengers can be updated"})",
                    "application/json"
                );
                return;
            }

            // Load flights into FlightManager.
            FlightManager flightManager(10000);

            if (!loadFlightsIntoManager(session, flightManager))
            {
                throw std::runtime_error(
                    "Could not load flights into FlightManager"
                );
            }

            // Verify the new flight.
            Flight selectedFlight;

            if (!flightManager.findFlightRecord(
                    flightNumber, selectedFlight))
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Selected flight does not exist"})",
                    "application/json"
                );
                return;
            }

            std::string selectedStatus =
                toUpper(selectedFlight.getStatus());

            if (selectedStatus == "CANCELLED" ||
                selectedStatus == "CANCELED" ||
                selectedStatus == "COMPLETED")
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Cannot assign passenger to an unavailable flight"})",
                    "application/json"
                );
                return;
            }

            // Check for duplicate seat assignment.
            auto seatResult = session.sql(
                "SELECT passenger_id FROM passengers "
                "WHERE flight_number = ? "
                "AND seat_number = ? "
                "AND passenger_id <> ? LIMIT 1"
            ).bind(
                flightNumber,
                seatNumber,
                passengerId
            ).execute();

            if (seatResult.fetchOne())
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"This seat is already assigned on the selected flight"})",
                    "application/json"
                );
                return;
            }

            // Construct updated passenger.
            // Preserve the original passenger status.
            Passenger updatedPassenger(
                passengerId,
                name,
                age,
                gender,
                phoneNumber,
                flightNumber,
                seatNumber,
                assistanceType,
                existingPassenger.getStatus()
            );

            // Update through the original linked list.
            if (!passengerManager.updatePassengerRecord(
                    passengerId, updatedPassenger))
            {
                throw std::runtime_error(
                    "PassengerManager could not update the record"
                );
            }

            // Update the MySQL database.
            session.sql(
                "UPDATE passengers SET "
                "name = ?, age = ?, gender = ?, "
                "phone_number = ?, flight_number = ?, "
                "seat_number = ?, assistance_type = ? "
                "WHERE passenger_id = ?"
            ).bind(
                name,
                age,
                gender,
                phoneNumber,
                flightNumber,
                seatNumber,
                assistanceType,
                passengerId
            ).execute();

            response.status = 200;

            response.set_content(
                passengerToJson(updatedPassenger),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Passenger update failed: "
                << error.what() << '\n';

            response.status = 500;

            response.set_content(
                R"({"status":"error","message":"Could not update passenger; check server console"})",
                "application/json"
            );
        }
    }
);

// Delete a passenger from MySQL through PassengerManager.
server.Post(
    "/api/passengers/delete",
    [](const httplib::Request& request,
       httplib::Response& response)
    {
        setCorsHeaders(response);

        if (!databaseCredentialsAvailable())
        {
            response.status = 500;
            response.set_content(
                R"({"status":"error","message":"Database credentials are not configured"})",
                "application/json"
            );
            return;
        }

        try
        {
            // Read Passenger ID.
            std::string passengerId;

            if (request.has_param("passengerId"))
            {
                passengerId = toUpper(
                    trim(request.get_param_value("passengerId"))
                );
            }

            if (passengerId.empty())
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Passenger ID is required"})",
                    "application/json"
                );
                return;
            }

            // Connect to MySQL.
            mysqlx::Session session(
                "localhost",
                33060,
                getDbUser(),
                getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AuthUser authenticatedUser;
            if (!authorizeRequest(session, request, response, authenticatedUser, "admin"))
                return;

            // Load existing passengers into PassengerManager.
            PassengerManager passengerManager;

            if (!loadPassengersIntoManager(
                    session, passengerManager))
            {
                throw std::runtime_error(
                    "Could not load passengers into PassengerManager"
                );
            }

            // Find the passenger before deleting.
            Passenger removedPassenger;

            if (!passengerManager.findPassengerRecord(
                    passengerId, removedPassenger))
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Passenger not found"})",
                    "application/json"
                );
                return;
            }

            // Remove the passenger from the linked list.
            if (!passengerManager.removePassengerRecord(
                    passengerId, &removedPassenger))
            {
                throw std::runtime_error(
                    "PassengerManager could not remove the passenger"
                );
            }

            // Delete the passenger permanently from MySQL.
            session.sql(
                "DELETE FROM passengers WHERE passenger_id = ?"
            ).bind(
                passengerId
            ).execute();

            response.status = 200;

            response.set_content(
                passengerToJson(removedPassenger),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Passenger deletion failed: "
                << error.what() << '\n';

            response.status = 500;

            response.set_content(
                R"({"status":"error","message":"Could not delete passenger; check server console"})",
                "application/json"
            );
        }
    }
);


// Restore a previously deleted passenger.
server.Post(
    "/api/passengers/restore",
    [](const httplib::Request& request,
       httplib::Response& response)
    {
        setCorsHeaders(response);

        if (!databaseCredentialsAvailable())
        {
            response.status = 500;
            response.set_content(
                R"({"status":"error","message":"Database credentials are not configured"})",
                "application/json"
            );
            return;
        }

        try
        {
            // Read passenger details.
            auto readField = [&](const std::string& field)
            {
                if (!request.has_param(field))
                    return std::string();

                return trim(request.get_param_value(field));
            };

            std::string passengerId =
                toUpper(readField("passengerId"));

            std::string name = readField("name");
            std::string ageText = readField("age");
            std::string gender = readField("gender");
            std::string phoneNumber = readField("phoneNumber");

            std::string flightNumber =
                toUpper(readField("flightNumber"));

            std::string seatNumber =
                toUpper(readField("seatNumber"));

            std::string assistanceType =
                readField("assistanceType");

            std::string status = readField("status");

            if (assistanceType.empty())
                assistanceType = "None";

            // Validate required fields.
            if (passengerId.empty() ||
                name.empty() ||
                ageText.empty() ||
                gender.empty() ||
                phoneNumber.empty() ||
                flightNumber.empty() ||
                seatNumber.empty() ||
                status.empty())
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Complete passenger details are required for restoration"})",
                    "application/json"
                );
                return;
            }

            // Validate field lengths.
            if (passengerId.size() > 30 ||
                name.size() > 100 ||
                gender.size() > 20 ||
                phoneNumber.size() > 20 ||
                flightNumber.size() > 20 ||
                seatNumber.size() > 10 ||
                assistanceType.size() > 50 ||
                status.size() > 30 ||
                name.size() < 2)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid passenger field length"})",
                    "application/json"
                );
                return;
            }

            // Validate passenger ID.
            bool validPassengerId = std::all_of(
                passengerId.begin(),
                passengerId.end(),
                [](unsigned char ch)
                {
                    return (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9') ||
                           ch == '-' || ch == '_';
                }
            );

            if (!validPassengerId)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid passenger ID"})",
                    "application/json"
                );
                return;
            }

            // Validate age.
            int age = 0;

            try
            {
                std::size_t used = 0;
                age = std::stoi(ageText, &used);

                if (used != ageText.size() ||
                    age < 0 || age > 120)
                {
                    throw std::invalid_argument("Invalid age");
                }
            }
            catch (const std::exception&)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid passenger age"})",
                    "application/json"
                );
                return;
            }

            // Connect to MySQL.
            mysqlx::Session session(
                "localhost",
                33060,
                getDbUser(),
                getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AuthUser authenticatedUser;
            if (!authorizeRequest(session, request, response, authenticatedUser, "admin"))
                return;

            // Check whether the assigned flight exists.
            FlightManager flightManager(10000);

            if (!loadFlightsIntoManager(session, flightManager))
            {
                throw std::runtime_error(
                    "Could not load flights into FlightManager"
                );
            }

            Flight selectedFlight;

            if (!flightManager.findFlightRecord(
                    flightNumber, selectedFlight))
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Original flight no longer exists"})",
                    "application/json"
                );
                return;
            }

            // Check whether this passenger already exists.
            PassengerManager passengerManager;

            if (!loadPassengersIntoManager(
                    session, passengerManager))
            {
                throw std::runtime_error(
                    "Could not load passengers into PassengerManager"
                );
            }

            Passenger passenger(
                passengerId,
                name,
                age,
                gender,
                phoneNumber,
                flightNumber,
                seatNumber,
                assistanceType,
                status
            );

            // Prevent duplicate passenger IDs.
            if (!passengerManager.addPassengerRecord(passenger))
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"Passenger already exists"})",
                    "application/json"
                );
                return;
            }

            // Prevent conflicting seat assignments.
            auto seatResult = session.sql(
                "SELECT passenger_id FROM passengers "
                "WHERE flight_number = ? "
                "AND seat_number = ? LIMIT 1"
            ).bind(
                flightNumber,
                seatNumber
            ).execute();

            if (seatResult.fetchOne())
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"Original seat is now occupied"})",
                    "application/json"
                );
                return;
            }

            // Restore the original record in MySQL.
            session.sql(
                "INSERT INTO passengers "
                "(passenger_id, name, age, gender, phone_number, "
                "flight_number, seat_number, assistance_type, status) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
            ).bind(
                passengerId,
                name,
                age,
                gender,
                phoneNumber,
                flightNumber,
                seatNumber,
                assistanceType,
                status
            ).execute();

            response.status = 201;

            response.set_content(
                passengerToJson(passenger),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Passenger restoration failed: "
                << error.what() << '\n';

            response.status = 500;

            response.set_content(
                R"({"status":"error","message":"Could not restore passenger; check server console"})",
                "application/json"
            );
        }
    }
);

}
