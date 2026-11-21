#include "CheckInRoutes.h"
#include "AuthMiddleware.h"

#include "Passenger.h"
#include "CheckInQueue.h"

#include "httplib.h"
#include <mysqlx/xdevapi.h>

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
std::string passengerToJson(const Passenger& passenger);

void registerCheckInRoutes(httplib::Server& server)
{
    server.Get(
    "/api/checkin/queue",
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

            // Create the original C++ linked-list queue.
            CheckInQueue checkInQueue;

            // Retrieve passengers in FIFO order.
            auto result = session.sql(
                "SELECT p.passenger_id, p.name, p.age, "
                "p.gender, p.phone_number, p.flight_number, "
                "p.seat_number, p.assistance_type, p.status "
                "FROM checkin_queue q "
                "JOIN passengers p "
                "ON q.passenger_id = p.passenger_id "
                "ORDER BY q.queue_position ASC"
            ).execute();

            auto row = result.fetchOne();

            // Reconstruct the C++ queue from MySQL.
            while (row)
            {
                Passenger passenger(
                    row[0].get<std::string>(),
                    row[1].get<std::string>(),
                    row[2].get<int>(),
                    row[3].get<std::string>(),
                    row[4].get<std::string>(),
                    row[5].get<std::string>(),
                    row[6].get<std::string>(),
                    row[7].get<std::string>(),
                    row[8].get<std::string>()
                );

                if (!checkInQueue.enqueueRecord(passenger))
                {
                    throw std::runtime_error(
                        "Could not load passenger into CheckInQueue"
                    );
                }

                row = result.fetchOne();
            }

            // Convert the linked-list queue into JSON.
            std::ostringstream body;
            body << "[";
            bool firstVisiblePassenger = true;

            for (std::size_t i = 0;
                 i < checkInQueue.getQueueSize();
                 ++i)
            {
                Passenger passenger;

                if (!checkInQueue.getPassengerAt(i, passenger))
                {
                    throw std::runtime_error(
                        "Could not retrieve passenger from CheckInQueue"
                    );
                }

                if (!canAccessPassenger(authenticatedUser, passenger.getPassengerId()))
                    continue;

                if (!firstVisiblePassenger)
                    body << ",";
                firstVisiblePassenger = false;

                body
                    << "{\"queuePosition\":" << (i + 1)
                    << ",\"passenger\":"
                    << passengerToJson(passenger)
                    << "}";
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
                << "Could not load check-in queue: "
                << error.what() << '\n';

            response.status = 503;

            response.set_content(
                R"({"status":"error","message":"Could not load check-in queue"})",
                "application/json"
            );
        }
    }
);


// Add a passenger to the check-in queue and save it in MySQL.
server.Post(
    "/api/checkin/enqueue",
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
            // Read the passenger ID from the request.
            if (!request.has_param("passengerId"))
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Passenger ID is required"})",
                    "application/json"
                );
                return;
            }

            std::string passengerId =
                trim(request.get_param_value("passengerId"));

            if (passengerId.empty())
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Passenger ID cannot be empty"})",
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
            if (!authorizeRequest(session, request, response, authenticatedUser))
                return;
            if (!canAccessPassenger(authenticatedUser, passengerId))
            {
                response.status = 403;
                response.set_content(R"({"status":"error","message":"You can only join your own check-in queue"})", "application/json");
                return;
            }

            // Find the passenger in MySQL.
            auto passengerResult = session.sql(
                "SELECT passenger_id, name, age, gender, "
                "phone_number, flight_number, seat_number, "
                "assistance_type, status "
                "FROM passengers "
                "WHERE passenger_id = ?"
                ).bind(passengerId).execute();

            auto passengerRow = passengerResult.fetchOne();

            if (!passengerRow)
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Passenger not found"})",
                    "application/json"
                );
                return;
            }

            // Reconstruct the Passenger object.
            Passenger passenger(
                passengerRow[0].get<std::string>(),
                passengerRow[1].get<std::string>(),
                passengerRow[2].get<int>(),
                passengerRow[3].get<std::string>(),
                passengerRow[4].get<std::string>(),
                passengerRow[5].get<std::string>(),
                passengerRow[6].get<std::string>(),
                passengerRow[7].get<std::string>(),
                passengerRow[8].get<std::string>()
            );

            std::string passengerStatus =
                toUpper(trim(passengerRow[8].get<std::string>()));

            std::string flightNumber =
                passengerRow[5].get<std::string>();

            // Only registered passengers can enter this queue.
            if (passengerStatus != "REGISTERED")
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"Only registered passengers can enter the check-in queue"})",
                    "application/json"
                );
                return;
            }

            // Check whether the passenger's flight exists.
            auto flightResult = session.sql(
                "SELECT status FROM flights "
                "WHERE flight_number = ?"
            ).bind(flightNumber).execute();

            auto flightRow = flightResult.fetchOne();

            if (!flightRow)
            {
                response.status = 404;
                response.set_content(
                    R"({"status":"error","message":"Passenger flight not found"})",
                    "application/json"
                );
                return;
            }

            std::string flightStatus =
                toUpper(trim(flightRow[0].get<std::string>()));

            if (flightStatus == "CANCELLED" ||
                flightStatus == "CANCELED")
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"Passengers cannot check in for a cancelled flight"})",
                    "application/json"
                );
                return;
            }

            // Reconstruct the existing C++ linked-list queue.
            CheckInQueue checkInQueue;

            auto queueResult = session.sql(
                "SELECT p.passenger_id, p.name, p.age, "
                "p.gender, p.phone_number, p.flight_number, "
                "p.seat_number, p.assistance_type, p.status "
                "FROM checkin_queue q "
                "JOIN passengers p "
                "ON q.passenger_id = p.passenger_id "
                "ORDER BY q.queue_position ASC"
            ).execute();

            auto queueRow = queueResult.fetchOne();

            while (queueRow)
            {
                Passenger existingPassenger(
                    queueRow[0].get<std::string>(),
                    queueRow[1].get<std::string>(),
                    queueRow[2].get<int>(),
                    queueRow[3].get<std::string>(),
                    queueRow[4].get<std::string>(),
                    queueRow[5].get<std::string>(),
                    queueRow[6].get<std::string>(),
                    queueRow[7].get<std::string>(),
                    queueRow[8].get<std::string>()
                );

                if (!checkInQueue.enqueueRecord(existingPassenger))
                {
                    throw std::runtime_error(
                        "Could not reconstruct the check-in queue"
                    );
                }

                queueRow = queueResult.fetchOne();
            }

            // Prevent duplicate queue entries.
            if (checkInQueue.containsPassenger(passengerId))
            {
                response.status = 409;
                response.set_content(
                    R"({"status":"error","message":"Passenger is already in the check-in queue"})",
                    "application/json"
                );
                return;
            }

            // Add the new passenger to the C++ queue.
            if (!checkInQueue.enqueueRecord(passenger))
            {
                response.status = 500;
                response.set_content(
                    R"({"status":"error","message":"Could not enqueue passenger"})",
                    "application/json"
                );
                return;
            }

            // Find the next available queue position.
            auto positionResult = session.sql(
                "SELECT COALESCE(MAX(queue_position), 0) + 1 "
                "FROM checkin_queue"
            ).execute();

            auto positionRow = positionResult.fetchOne();

            if (!positionRow)
            {
                throw std::runtime_error(
                    "Could not determine the next queue position"
                );
            }

            long long queuePosition =
                positionRow[0].get<long long>();

            // Save the queue entry in MySQL.
            session.sql(
                "INSERT INTO checkin_queue "
                "(passenger_id, queue_position) "
                "VALUES (?, ?)"
                )
                .bind(passengerId, queuePosition)
                .execute();

            // Return the new queue entry.
            std::ostringstream body;

            body
                << "{\"status\":\"success\","
                << "\"message\":\"Passenger added to check-in queue\","
                << "\"queuePosition\":" << queuePosition
                << ",\"passenger\":"
                << passengerToJson(passenger)
                << "}";

            response.set_content(
                body.str(),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Could not enqueue passenger: "
                << error.what() << '\n';

            response.status = 503;

            response.set_content(
                R"({"status":"error","message":"Could not enqueue passenger"})",
                "application/json"
            );
        }
    }
);

// Process the first passenger in the check-in queue.
server.Post(
    "/api/checkin/dequeue",
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

            session.startTransaction();

            try
            {
                // Reconstruct the C++ linked-list queue from MySQL.
                CheckInQueue checkInQueue;

                auto queueResult = session.sql(
                    "SELECT p.passenger_id, p.name, p.age, "
                    "p.gender, p.phone_number, p.flight_number, "
                    "p.seat_number, p.assistance_type, p.status "
                    "FROM checkin_queue q "
                    "JOIN passengers p "
                    "ON q.passenger_id = p.passenger_id "
                    "ORDER BY q.queue_position ASC "
                    "FOR UPDATE"
                ).execute();

                auto row = queueResult.fetchOne();

                while (row)
                {
                    Passenger passenger(
                        row[0].get<std::string>(),
                        row[1].get<std::string>(),
                        row[2].get<int>(),
                        row[3].get<std::string>(),
                        row[4].get<std::string>(),
                        row[5].get<std::string>(),
                        row[6].get<std::string>(),
                        row[7].get<std::string>(),
                        row[8].get<std::string>()
                    );

                    if (!checkInQueue.enqueueRecord(passenger))
                    {
                        throw std::runtime_error(
                            "Could not reconstruct the check-in queue"
                        );
                    }

                    row = queueResult.fetchOne();
                }

                // Reject processing if the queue is empty.
                if (checkInQueue.isEmpty())
                {
                    session.rollback();

                    response.status = 409;
                    response.set_content(
                        R"({"status":"error","message":"Check-in queue is empty"})",
                        "application/json"
                    );
                    return;
                }

                // Remove the front passenger using the C++ Queue.
                Passenger processedPassenger;

                if (!checkInQueue.dequeueRecord(processedPassenger))
                {
                    throw std::runtime_error(
                        "Could not dequeue the first passenger"
                    );
                }

                std::string passengerId =
                    processedPassenger.getPassengerId();

                std::string currentStatus =
                    toUpper(trim(processedPassenger.getStatus()));

                if (currentStatus != "REGISTERED")
                {
                    session.rollback();

                    response.status = 409;
                    response.set_content(
                        R"({"status":"error","message":"Passenger is not eligible for check-in"})",
                        "application/json"
                    );
                    return;
                }

                // Update the passenger status in MySQL.
                auto updateResult = session.sql(
                    "UPDATE passengers "
                    "SET status = 'Checked-In' "
                    "WHERE passenger_id = ? "
                    "AND status = 'Registered'"
                ).bind(passengerId).execute();

                if (updateResult.getAffectedItemsCount() != 1)
                {
                    throw std::runtime_error(
                        "Could not update passenger check-in status"
                    );
                }

                // Remove the processed passenger from MySQL queue.
                auto deleteResult = session.sql(
                    "DELETE FROM checkin_queue "
                    "WHERE passenger_id = ?"
                ).bind(passengerId).execute();

                if (deleteResult.getAffectedItemsCount() != 1)
                {
                    throw std::runtime_error(
                        "Could not remove passenger from check-in queue"
                    );
                }

                // Complete both database operations together.
                session.commit();

                processedPassenger.setStatus("Checked-In");

                std::ostringstream body;

                body
                    << "{\"status\":\"success\","
                    << "\"message\":\"Passenger checked in successfully\","
                    << "\"passenger\":"
                    << passengerToJson(processedPassenger)
                    << "}";

                response.set_content(
                    body.str(),
                    "application/json"
                );
            }
            catch (...)
            {
                session.rollback();
                throw;
            }
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Could not process check-in queue: "
                << error.what() << '\n';

            response.status = 503;

            response.set_content(
                R"({"status":"error","message":"Could not process check-in queue"})",
                "application/json"
            );
        }
    }
);
}
