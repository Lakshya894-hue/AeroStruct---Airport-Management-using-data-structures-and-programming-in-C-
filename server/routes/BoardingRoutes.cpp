#include "BoardingRoutes.h"
#include "AuthMiddleware.h"

#include "Passenger.h"
#include "BoardingQueue.h"

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
std::string jsonEscape(const std::string& value);
std::string passengerToJson(const Passenger& passenger);

void registerBoardingRoutes(httplib::Server& server)
{
    // Clear security and add the passenger to the BoardingQueue.
server.Post(
    "/api/security/clear",
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
            if (!request.has_param("passengerId") ||
                !request.has_param("gate"))
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Passenger ID and gate are required"})",
                    "application/json"
                );
                return;
            }

            std::string passengerId =
                trim(request.get_param_value("passengerId"));

            std::string gate =
                toUpper(trim(request.get_param_value("gate")));

            if (passengerId.empty() || gate.empty() || gate.size() > 10)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid passenger ID or gate"})",
                    "application/json"
                );
                return;
            }

            mysqlx::Session session(
                "localhost", 33060,
                getDbUser(), getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AuthUser authenticatedUser;
            if (!authorizeRequest(session, request, response, authenticatedUser, "admin"))
                return;
            session.startTransaction();

            try
            {
                // Lock and retrieve the passenger record.
                auto result = session.sql(
                    "SELECT passenger_id, name, age, gender, "
                    "phone_number, flight_number, seat_number, "
                    "assistance_type, status "
                    "FROM passengers "
                    "WHERE passenger_id = ? FOR UPDATE"
                ).bind(passengerId).execute();

                auto row = result.fetchOne();

                if (!row)
                {
                    session.rollback();
                    response.status = 404;
                    response.set_content(
                        R"({"status":"error","message":"Passenger not found"})",
                        "application/json"
                    );
                    return;
                }

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

                if (toUpper(trim(passenger.getStatus())) != "CHECKED-IN")
                {
                    session.rollback();
                    response.status = 409;
                    response.set_content(
                        R"({"status":"error","message":"Only Checked-In passengers can clear security"})",
                        "application/json"
                    );
                    return;
                }

                // Verify that the passenger's flight is active.
                auto flightResult = session.sql(
                    "SELECT status FROM flights WHERE flight_number = ?"
                ).bind(passenger.getFlightNumber()).execute();

                auto flightRow = flightResult.fetchOne();

                if (!flightRow)
                {
                    session.rollback();
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
                    session.rollback();
                    response.status = 409;
                    response.set_content(
                        R"({"status":"error","message":"Security clearance denied for cancelled flight"})",
                        "application/json"
                    );
                    return;
                }

                // Reconstruct the existing C++ BoardingQueue.
                BoardingQueue boardingQueue;

                auto queueResult = session.sql(
                    "SELECT p.passenger_id, p.name, p.age, "
                    "p.gender, p.phone_number, p.flight_number, "
                    "p.seat_number, p.assistance_type, "
                    "p.status, b.gate "
                    "FROM boarding_queue b "
                    "JOIN passengers p "
                    "ON b.passenger_id = p.passenger_id "
                    "ORDER BY b.queue_position ASC FOR UPDATE"
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

                    if (!boardingQueue.enqueueRecord(
                            existingPassenger,
                            queueRow[9].get<std::string>()))
                    {
                        throw std::runtime_error(
                            "Could not reconstruct BoardingQueue"
                        );
                    }

                    queueRow = queueResult.fetchOne();
                }

                if (boardingQueue.containsPassenger(passengerId))
                {
                    session.rollback();
                    response.status = 409;
                    response.set_content(
                        R"({"status":"error","message":"Passenger is already in Boarding Queue"})",
                        "application/json"
                    );
                    return;
                }

                if (!boardingQueue.enqueueRecord(passenger, gate))
                {
                    throw std::runtime_error(
                        "Could not add passenger to BoardingQueue"
                    );
                }

                // Find the next boarding position.
                auto positionResult = session.sql(
                    "SELECT COALESCE(MAX(queue_position), 0) + 1 "
                    "FROM boarding_queue"
                ).execute();

                auto positionRow = positionResult.fetchOne();

                if (!positionRow)
                {
                    throw std::runtime_error(
                        "Could not determine boarding position"
                    );
                }

                long long queuePosition =
                    positionRow[0].get<long long>();

                // Save the boarding entry.
                session.sql(
                    "INSERT INTO boarding_queue "
                    "(passenger_id, queue_position, gate) "
                    "VALUES (?, ?, ?)"
                )
                .bind(passengerId, queuePosition, gate)
                .execute();

                // Update passenger status.
                auto updateResult = session.sql(
                    "UPDATE passengers "
                    "SET status = 'Security-Cleared' "
                    "WHERE passenger_id = ? "
                    "AND status = 'Checked-In'"
                ).bind(passengerId).execute();

                if (updateResult.getAffectedItemsCount() != 1)
                {
                    throw std::runtime_error(
                        "Could not update security clearance status"
                    );
                }

                session.commit();

                passenger.setStatus("Security-Cleared");

                std::ostringstream body;

                body
                    << "{\"status\":\"success\","
                    << "\"message\":\"Security clearance completed\","
                    << "\"queuePosition\":" << queuePosition
                    << ",\"gate\":\"" << jsonEscape(gate) << "\","
                    << "\"passenger\":"
                    << passengerToJson(passenger)
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
                << "Security clearance error: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not complete security clearance"})",
                "application/json"
            );
        }
    }
);

// Display BoardingQueue reconstructed from MySQL.
server.Get(
    "/api/boarding/queue",
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
                "localhost", 33060,
                getDbUser(), getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AuthUser authenticatedUser;
            if (!authorizeRequest(session, request, response, authenticatedUser, "admin"))
                return;

            BoardingQueue boardingQueue;

            auto result = session.sql(
                "SELECT p.passenger_id, p.name, p.age, "
                "p.gender, p.phone_number, p.flight_number, "
                "p.seat_number, p.assistance_type, "
                "p.status, b.gate "
                "FROM boarding_queue b "
                "JOIN passengers p "
                "ON b.passenger_id = p.passenger_id "
                "ORDER BY b.queue_position ASC"
            ).execute();

            auto row = result.fetchOne();

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

                std::string gate =
                    row[9].get<std::string>();

                if (!boardingQueue.enqueueRecord(passenger, gate))
                {
                    throw std::runtime_error(
                        "Could not reconstruct BoardingQueue"
                    );
                }

                row = result.fetchOne();
            }

            std::ostringstream body;
            body << "[";

            for (std::size_t i = 0;
                 i < boardingQueue.getQueueSize(); ++i)
            {
                Passenger passenger;
                std::string gate;

                if (!boardingQueue.getPassengerAt(
                        i, passenger, gate))
                {
                    throw std::runtime_error(
                        "Could not retrieve boarding passenger"
                    );
                }

                if (i > 0)
                    body << ",";

                body
                    << "{\"queuePosition\":" << (i + 1)
                    << ",\"gate\":\"" << jsonEscape(gate) << "\","
                    << "\"passenger\":"
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
                << "Could not load BoardingQueue: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not load BoardingQueue"})",
                "application/json"
            );
        }
    }
);

// Board the first passenger in the FIFO BoardingQueue.
server.Post(
    "/api/boarding/dequeue",
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
                "localhost", 33060,
                getDbUser(), getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AuthUser authenticatedUser;
            if (!authorizeRequest(session, request, response, authenticatedUser, "admin"))
                return;
            session.startTransaction();

            try
            {
                BoardingQueue boardingQueue;

                auto result = session.sql(
                    "SELECT p.passenger_id, p.name, p.age, "
                    "p.gender, p.phone_number, p.flight_number, "
                    "p.seat_number, p.assistance_type, "
                    "p.status, b.gate "
                    "FROM boarding_queue b "
                    "JOIN passengers p "
                    "ON b.passenger_id = p.passenger_id "
                    "ORDER BY b.queue_position ASC FOR UPDATE"
                ).execute();

                auto row = result.fetchOne();

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

                    if (!boardingQueue.enqueueRecord(
                            passenger,
                            row[9].get<std::string>()))
                    {
                        throw std::runtime_error(
                            "Could not reconstruct BoardingQueue"
                        );
                    }

                    row = result.fetchOne();
                }

                if (boardingQueue.isEmpty())
                {
                    session.rollback();
                    response.status = 409;
                    response.set_content(
                        R"({"status":"error","message":"Boarding queue is empty"})",
                        "application/json"
                    );
                    return;
                }

                Passenger passenger;
                std::string gate;

                if (!boardingQueue.dequeueRecord(passenger, gate))
                {
                    throw std::runtime_error(
                        "Could not dequeue passenger"
                    );
                }

                std::string passengerId =
                    passenger.getPassengerId();

                if (toUpper(trim(passenger.getStatus())) !=
                    "SECURITY-CLEARED")
                {
                    session.rollback();
                    response.status = 409;
                    response.set_content(
                        R"({"status":"error","message":"Passenger has not cleared security"})",
                        "application/json"
                    );
                    return;
                }

                auto updateResult = session.sql(
                    "UPDATE passengers "
                    "SET status = 'Boarded' "
                    "WHERE passenger_id = ? "
                    "AND status = 'Security-Cleared'"
                ).bind(passengerId).execute();

                if (updateResult.getAffectedItemsCount() != 1)
                {
                    throw std::runtime_error(
                        "Could not update passenger boarding status"
                    );
                }

                auto deleteResult = session.sql(
                    "DELETE FROM boarding_queue "
                    "WHERE passenger_id = ?"
                ).bind(passengerId).execute();

                if (deleteResult.getAffectedItemsCount() != 1)
                {
                    throw std::runtime_error(
                        "Could not remove passenger from BoardingQueue"
                    );
                }

                session.commit();

                passenger.setStatus("Boarded");

                std::ostringstream body;

                body
                    << "{\"status\":\"success\","
                    << "\"message\":\"Passenger boarded successfully\","
                    << "\"gate\":\"" << jsonEscape(gate) << "\","
                    << "\"passenger\":"
                    << passengerToJson(passenger)
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
                << "Could not process BoardingQueue: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not process BoardingQueue"})",
                "application/json"
            );
        }
    }
);

}
