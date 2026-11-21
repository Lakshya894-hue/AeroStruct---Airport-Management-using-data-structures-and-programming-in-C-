#include "FlightRoutes.h"

#include "Flight.h"
#include "FlightManager.h"
#include "AuthMiddleware.h"

#include <mysqlx/xdevapi.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// Existing helper functions from api_main.cpp

const char* getDbUser();
const char* getDbPassword();

bool databaseCredentialsAvailable();

void setCorsHeaders(httplib::Response& response);

std::string trim(const std::string& value);
std::string toUpper(std::string value);

bool isValidFlightTime(const std::string& value);

bool loadFlightsIntoManager(
    mysqlx::Session& session,
    FlightManager& manager
);

std::string flightToJson(const Flight& flight);
std::string jsonEscape(const std::string& value);

void registerFlightRoutes(httplib::Server& server)
{
    // Flight route registrations will go here.
    // 3. List flights from MySQL through FlightManager.
    server.Get(
        "/api/flights",
        [](const httplib::Request&,
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

                FlightManager flightManager(10000);

                auto result = session.sql(
                    "SELECT flight_number, airline, source, "
                    "destination, departure_time, arrival_time, "
                    "gate, status, fare "
                    "FROM flights ORDER BY flight_number"
                ).execute();

                auto row = result.fetchOne();

                while (row)
                {
                    std::ostringstream fareText;
                    fareText << row[8];

                    Flight flight(
                        row[0].get<std::string>(),
                        row[1].get<std::string>(),
                        row[2].get<std::string>(),
                        row[3].get<std::string>(),
                        row[4].get<std::string>(),
                        row[5].get<std::string>(),
                        row[7].get<std::string>(),
                        std::stod(fareText.str())
                    );

                    flight.setGate(row[6].get<std::string>());

                    if (!flightManager.addFlightRecord(flight))
                    {
                        throw std::runtime_error(
                            "FlightManager could not load a database record"
                        );
                    }

                    row = result.fetchOne();
                }

                std::ostringstream body;
                body << "[";

                for (int i = 0;
                     i < flightManager.getFlightCount();
                     ++i)
                {
                    Flight flight;

                    if (!flightManager.getFlightAt(i, flight))
                    {
                        throw std::runtime_error(
                            "Could not read flight from FlightManager"
                        );
                    }

                    if (i > 0)
                        body << ",";

                    body << flightToJson(flight);
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
                    << "Could not load flights: "
                    << error.what() << '\n';

                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Could not load flights from the database"})",
                    "application/json"
                );
            }
        }
    );

    server.Post(
    "/api/flights",
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

            std::string flightNumber =
                toUpper(readField("flightNumber"));

            std::string airline = readField("airline");
            std::string source = readField("source");
            std::string destination = readField("destination");

            std::string departureTime =
                readField("departureTime");

            std::string arrivalTime =
                readField("arrivalTime");

            std::string gate =
                toUpper(readField("gate"));

            std::string status = readField("status");
            std::string fareText = readField("fare");

            // Validate required fields.
            if (flightNumber.empty() ||
                airline.empty() ||
                source.empty() ||
                destination.empty() ||
                departureTime.empty() ||
                arrivalTime.empty() ||
                gate.empty() ||
                status.empty() ||
                fareText.empty())
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"All flight fields are required"})",
                    "application/json"
                );
                return;
            }

            // Validate maximum database field lengths.
            if (flightNumber.size() > 20 ||
                airline.size() > 100 ||
                source.size() > 100 ||
                destination.size() > 100 ||
                gate.size() > 20 ||
                status.size() > 30)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"One or more fields exceed the allowed length"})",
                    "application/json"
                );
                return;
            }

            if (source == destination)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Source and destination must be different"})",
                    "application/json"
                );
                return;
            }

            if (!isValidFlightTime(departureTime) ||
                !isValidFlightTime(arrivalTime))
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid time. Use HH:MM format"})",
                    "application/json"
                );
                return;
            }

            if (status != "On Time" &&
                status != "Boarding" &&
                status != "Delayed")
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid initial flight status"})",
                    "application/json"
                );
                return;
            }

            double fare = 0.0;

            try
            {
                std::size_t used = 0;
                fare = std::stod(fareText, &used);

                if (used != fareText.size() ||
                    !std::isfinite(fare) ||
                    fare < 0 ||
                    fare > 99999999.99)
                {
                    throw std::invalid_argument("Invalid fare");
                }
            }
            catch (const std::exception&)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Fare must be a valid non-negative number"})",
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
            // Only authenticated administrators can add flights.
AuthUser authenticatedUser;

if (!authorizeRequest(
        session,
        request,
        response,
        authenticatedUser,
        "admin"))
{
    return;
}

            // Load existing flights into the original C++ manager.
            FlightManager flightManager(10000);

            if (!loadFlightsIntoManager(session, flightManager))
            {
                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Could not load existing flights into FlightManager"})",
                    "application/json"
                );
                return;
            }

            // Construct the new Flight object.
            Flight flight(
                flightNumber,
                airline,
                source,
                destination,
                departureTime,
                arrivalTime,
                status,
                fare
            );

            flight.setGate(gate);

            // Reuse FlightManager's duplicate and capacity checks.
            if (!flightManager.addFlightRecord(flight))
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"Flight number already exists or flight capacity is full"})",
                    "application/json"
                );
                return;
            }

            // Save the accepted flight permanently in MySQL.
            session.sql(
                "INSERT INTO flights "
                "(flight_number, airline, source, destination, "
                "departure_time, arrival_time, gate, status, fare) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
            ).bind(
                flightNumber,
                airline,
                source,
                destination,
                departureTime,
                arrivalTime,
                gate,
                status,
                fare
            ).execute();

            response.status = 201;

            response.set_content(
                flightToJson(flight),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Flight creation failed: "
                << error.what() << '\n';

            response.status = 500;

            response.set_content(
                R"({"status":"error","message":"Could not create flight; check server console"})",
                "application/json"
            );
        }
    }
   );

     server.Post(
    "/api/flights/update",
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
            auto readField = [&](const std::string& name)
            {
                if (!request.has_param(name))
                    return std::string();

                return trim(request.get_param_value(name));
            };

            std::string flightNumber =
                toUpper(readField("flightNumber"));

            std::string airline = readField("airline");
            std::string source = readField("source");
            std::string destination = readField("destination");

            std::string departureTime = readField("departureTime");
            std::string arrivalTime = readField("arrivalTime");

            std::string gate = toUpper(readField("gate"));
            std::string fareText = readField("fare");

            if (flightNumber.empty() ||
                airline.empty() ||
                source.empty() ||
                destination.empty() ||
                departureTime.empty() ||
                arrivalTime.empty() ||
                gate.empty() ||
                fareText.empty())
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Flight number and all editable fields are required"})",
                    "application/json"
                );
                return;
            }

            if (flightNumber.size() > 20 ||
                airline.size() > 100 ||
                source.size() > 100 ||
                destination.size() > 100 ||
                gate.size() > 20)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"One or more fields exceed the allowed length"})",
                    "application/json"
                );
                return;
            }

            if (source == destination)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Source and destination must be different"})",
                    "application/json"
                );
                return;
            }

            if (!isValidFlightTime(departureTime) ||
                !isValidFlightTime(arrivalTime))
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid time. Use HH:MM format"})",
                    "application/json"
                );
                return;
            }

            double fare = 0.0;

            try
            {
                std::size_t used = 0;
                fare = std::stod(fareText, &used);

                if (used != fareText.size() ||
                    !std::isfinite(fare) ||
                    fare < 0 ||
                    fare > 99999999.99)
                {
                    throw std::invalid_argument("Invalid fare");
                }
            }
            catch (const std::exception&)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid fare"})",
                    "application/json"
                );
                return;
            }

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

            FlightManager flightManager(10000);

            if (!loadFlightsIntoManager(session, flightManager))
            {
                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Could not load flights"})",
                    "application/json"
                );
                return;
            }

            // Check whether the flight exists.
            Flight existingFlight;

            if (!flightManager.findFlightRecord(
                    flightNumber, existingFlight))
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Flight not found"})",
                    "application/json"
                );
                return;
            }

            // Preserve the existing status unless a new one is supplied.
            std::string status =
                request.has_param("status")
                ? readField("status")
                : existingFlight.getStatus();

            if (status.empty() || status.size() > 30)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid flight status"})",
                    "application/json"
                );
                return;
            }

            Flight updatedFlight(
                flightNumber,
                airline,
                source,
                destination,
                departureTime,
                arrivalTime,
                status,
                fare
            );

            updatedFlight.setGate(gate);

            // Reuse the existing FlightManager operation.
            if (!flightManager.updateFlightRecord(
                    flightNumber, updatedFlight))
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Flight not found"})",
                    "application/json"
                );
                return;
            }

            // Update the persistent MySQL record.
            session.sql(
                "UPDATE flights SET "
                "airline = ?, "
                "source = ?, "
                "destination = ?, "
                "departure_time = ?, "
                "arrival_time = ?, "
                "gate = ?, "
                "status = ?, "
                "fare = ? "
                "WHERE flight_number = ?"
            ).bind(
                airline,
                source,
                destination,
                departureTime,
                arrivalTime,
                gate,
                status,
                fare,
                flightNumber
            ).execute();

            response.status = 200;
            response.set_content(
                flightToJson(updatedFlight),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Flight update failed: "
                << error.what() << '\n';

            response.status = 500;
            response.set_content(
                R"({"status":"error","message":"Could not update flight; check server console"})",
                "application/json"
            );
        }
    }
);

        server.Post(
    "/api/flights/delete",
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
            if (!request.has_param("flightNumber"))
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Flight number is required"})",
                    "application/json"
                );
                return;
            }

            std::string flightNumber =
                toUpper(trim(request.get_param_value("flightNumber")));

            if (flightNumber.empty() || flightNumber.size() > 20)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid flight number"})",
                    "application/json"
                );
                return;
            }

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

            FlightManager flightManager(10000);

            if (!loadFlightsIntoManager(session, flightManager))
            {
                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Could not load flights"})",
                    "application/json"
                );
                return;
            }

            // Reuse FlightManager's deletion operation.
            if (!flightManager.removeFlightRecord(flightNumber))
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Flight not found"})",
                    "application/json"
                );
                return;
            }

            // Delete the persistent database record.
            session.sql(
                "DELETE FROM flights WHERE flight_number = ?"
            ).bind(flightNumber).execute();

            response.status = 200;

            response.set_content(
                "{\"status\":\"ok\","
                "\"message\":\"Flight deleted successfully\","
                "\"flightNumber\":\"" +
                jsonEscape(flightNumber) + "\"}",
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Flight deletion failed: "
                << error.what() << '\n';

            response.status = 500;
            response.set_content(
                R"({"status":"error","message":"Could not delete flight; check server console"})",
                "application/json"
            );
        }
    }
);
}
