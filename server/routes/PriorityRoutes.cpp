#include "PriorityRoutes.h"
#include "AuthMiddleware.h"

#include "Passenger.h"
#include "PriorityQueue.h"

#include "httplib.h"
#include <mysqlx/xdevapi.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

const char* getDbUser();
const char* getDbPassword();

void setCorsHeaders(httplib::Response& response);
bool databaseCredentialsAvailable();

std::string trim(const std::string& value);
std::string toUpper(std::string value);
std::string jsonEscape(const std::string& value);

std::string passengerToJson(const Passenger& passenger);

void registerPriorityRoutes(httplib::Server& server)
{
    // Display PriorityQueue reconstructed from MySQL.
server.Get(
    "/api/priority/queue",
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
            if (!authorizeRequest(session, request, response, authenticatedUser))
                return;

            PriorityQueue priorityQueue;

            auto result = session.sql(
                "SELECT a.priority, a.reason, "
                "p.passenger_id, p.name, p.age, p.gender, "
                "p.phone_number, p.flight_number, "
                "p.seat_number, p.assistance_type, p.status "
                "FROM assistance_queue a "
                "JOIN passengers p "
                "ON a.passenger_id = p.passenger_id "
                "ORDER BY a.priority ASC, a.request_id ASC"
            ).execute();

            auto row = result.fetchOne();

            while (row)
            {
                int priority = row[0].get<int>();
                std::string reason = row[1].get<std::string>();

                Passenger passenger(
                    row[2].get<std::string>(),
                    row[3].get<std::string>(),
                    row[4].get<int>(),
                    row[5].get<std::string>(),
                    row[6].get<std::string>(),
                    row[7].get<std::string>(),
                    row[8].get<std::string>(),
                    row[9].get<std::string>(),
                    row[10].get<std::string>()
                );

                if (!priorityQueue.enqueueRecord(
                        passenger, priority, reason))
                {
                    throw std::runtime_error(
                        "Could not reconstruct PriorityQueue"
                    );
                }

                row = result.fetchOne();
            }

            std::ostringstream body;
            body << "[";
            bool firstVisibleRequest = true;

            for (std::size_t i = 0;
                 i < priorityQueue.getQueueSize(); ++i)
            {
                Passenger passenger;
                int priority;
                std::string reason;

                if (!priorityQueue.getPassengerAt(
                        i, passenger, priority, reason))
                {
                    throw std::runtime_error(
                        "Could not retrieve priority request"
                    );
                }

                if (!canAccessPassenger(authenticatedUser, passenger.getPassengerId()))
                    continue;

                if (!firstVisibleRequest)
                    body << ",";
                firstVisibleRequest = false;

                body
                    << "{\"queuePosition\":" << (i + 1)
                    << ",\"sequence\":" << (i + 1)
                    << ",\"priority\":" << priority
                    << ",\"reason\":\"" << jsonEscape(reason) << "\","
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
                << "Could not load PriorityQueue: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not load PriorityQueue"})",
                "application/json"
            );
        }
    }
);

// Add an assistance request to the C++ PriorityQueue.
server.Post(
    "/api/priority/enqueue",
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
                !request.has_param("priority"))
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Passenger ID and priority are required"})",
                    "application/json"
                );
                return;
            }

            std::string passengerId =
                trim(request.get_param_value("passengerId"));

            std::string priorityText =
                trim(request.get_param_value("priority"));

            std::string reason = request.has_param("reason")
                ? trim(request.get_param_value("reason"))
                : "";

            int priority = 0;

            try
            {
                std::size_t parsed = 0;
                priority = std::stoi(priorityText, &parsed);

                if (parsed != priorityText.size())
                    throw std::invalid_argument("Invalid priority");
            }
            catch (...)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Priority must be an integer from 1 to 5"})",
                    "application/json"
                );
                return;
            }

            if (passengerId.empty() ||
                priority < 1 || priority > 5 ||
                reason.size() > 255)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid passenger ID, priority or reason"})",
                    "application/json"
                );
                return;
            }

            if (reason.empty())
                reason = "Assistance request";

            mysqlx::Session session(
                "localhost", 33060,
                getDbUser(), getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AuthUser authenticatedUser;
            if (!authorizeRequest(session, request, response, authenticatedUser))
                return;
            if (!canAccessPassenger(authenticatedUser, passengerId))
            {
                response.status = 403;
                response.set_content(R"({"status":"error","message":"You can only request assistance for your own passenger record"})", "application/json");
                return;
            }
            session.startTransaction();

            try
            {
                auto passengerResult = session.sql(
                    "SELECT passenger_id, name, age, gender, "
                    "phone_number, flight_number, seat_number, "
                    "assistance_type, status "
                    "FROM passengers WHERE passenger_id = ?"
                ).bind(passengerId).execute();

                auto passengerRow = passengerResult.fetchOne();

                if (!passengerRow)
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
                        R"({"status":"error","message":"Assistance request denied for cancelled flight"})",
                        "application/json"
                    );
                    return;
                }

                // Reconstruct the C++ PriorityQueue.
                PriorityQueue priorityQueue;

                auto queueResult = session.sql(
                    "SELECT a.priority, a.reason, "
                    "p.passenger_id, p.name, p.age, p.gender, "
                    "p.phone_number, p.flight_number, "
                    "p.seat_number, p.assistance_type, p.status "
                    "FROM assistance_queue a "
                    "JOIN passengers p "
                    "ON a.passenger_id = p.passenger_id "
                    "ORDER BY a.priority ASC, a.request_id ASC "
                    "FOR UPDATE"
                ).execute();

                auto queueRow = queueResult.fetchOne();

                while (queueRow)
                {
                    Passenger existingPassenger(
                        queueRow[2].get<std::string>(),
                        queueRow[3].get<std::string>(),
                        queueRow[4].get<int>(),
                        queueRow[5].get<std::string>(),
                        queueRow[6].get<std::string>(),
                        queueRow[7].get<std::string>(),
                        queueRow[8].get<std::string>(),
                        queueRow[9].get<std::string>(),
                        queueRow[10].get<std::string>()
                    );

                    if (!priorityQueue.enqueueRecord(
                            existingPassenger,
                            queueRow[0].get<int>(),
                            queueRow[1].get<std::string>()))
                    {
                        throw std::runtime_error(
                            "Could not reconstruct PriorityQueue"
                        );
                    }

                    queueRow = queueResult.fetchOne();
                }

                if (priorityQueue.containsPassenger(passengerId))
                {
                    session.rollback();
                    response.status = 409;
                    response.set_content(
                        R"({"status":"error","message":"Passenger already has an active assistance request"})",
                        "application/json"
                    );
                    return;
                }

                if (!priorityQueue.enqueueRecord(
                        passenger, priority, reason))
                {
                    throw std::runtime_error(
                        "Could not enqueue assistance request"
                    );
                }

                session.sql(
                    "INSERT INTO assistance_queue "
                    "(passenger_id, priority, reason) "
                    "VALUES (?, ?, ?)"
                )
                .bind(passengerId, priority, reason)
                .execute();

                session.commit();

                std::ostringstream body;

                body
                    << "{\"status\":\"success\","
                    << "\"message\":\"Assistance request added\","
                    << "\"priority\":" << priority
                    << ",\"reason\":\"" << jsonEscape(reason) << "\","
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
                << "Could not enqueue assistance request: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not enqueue assistance request"})",
                "application/json"
            );
        }
    }
);

// Optional search by passenger ID.
server.Get(
    "/api/assistance/history",
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
            if (!authorizeRequest(session, request, response, authenticatedUser))
                return;

            std::string passengerId =
                request.has_param("passengerId")
                ? trim(request.get_param_value("passengerId"))
                : "";

            if (authenticatedUser.role == "passenger")
            {
                if (!passengerId.empty() && !canAccessPassenger(authenticatedUser, passengerId))
                {
                    response.status = 403;
                    response.set_content(R"({"status":"error","message":"You can only view your own assistance history"})", "application/json");
                    return;
                }
                passengerId = authenticatedUser.passengerId;
            }

            auto result = session.sql(
                "SELECT history_id, passenger_id, priority, reason, "
                "COALESCE(DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s'), ''), "
                "DATE_FORMAT(processed_at, '%Y-%m-%d %H:%i:%s'), "
                "status "
                "FROM assistance_request_history "
                "WHERE (? = '' OR passenger_id = ?) "
                "ORDER BY processed_at DESC, history_id DESC"
            )
            .bind(passengerId, passengerId)
            .execute();

            std::ostringstream body;
            body << "[";

            auto row = result.fetchOne();
            bool first = true;

            while (row)
            {
                if (!first)
                    body << ",";

                first = false;

                unsigned long long historyId =
                    row[0].get<unsigned long long>();

                int priority = row[2].get<int>();

                body
                    << "{"
                    << "\"historyId\":" << historyId << ","
                    << "\"passengerId\":\""
                    << jsonEscape(row[1].get<std::string>()) << "\","
                    << "\"priority\":" << priority << ","
                    << "\"reason\":\""
                    << jsonEscape(row[3].get<std::string>()) << "\","
                    << "\"createdAt\":\""
                    << jsonEscape(row[4].get<std::string>()) << "\","
                    << "\"processedAt\":\""
                    << jsonEscape(row[5].get<std::string>()) << "\","
                    << "\"status\":\""
                    << jsonEscape(row[6].get<std::string>()) << "\""
                    << "}";

                row = result.fetchOne();
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
                << "Could not load assistance history: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not load assistance history"})",
                "application/json"
            );
        }
    }
);

// Process the highest-priority assistance request.
server.Post(
    "/api/priority/dequeue",
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
                PriorityQueue priorityQueue;

                auto result = session.sql(
                    "SELECT a.priority, a.reason, "
                    "p.passenger_id, p.name, p.age, p.gender, "
                    "p.phone_number, p.flight_number, "
                    "p.seat_number, p.assistance_type, p.status "
                    "FROM assistance_queue a "
                    "JOIN passengers p "
                    "ON a.passenger_id = p.passenger_id "
                    "ORDER BY a.priority ASC, a.request_id ASC "
                    "FOR UPDATE"
                ).execute();

                auto row = result.fetchOne();

                while (row)
                {
                    Passenger passenger(
                        row[2].get<std::string>(),
                        row[3].get<std::string>(),
                        row[4].get<int>(),
                        row[5].get<std::string>(),
                        row[6].get<std::string>(),
                        row[7].get<std::string>(),
                        row[8].get<std::string>(),
                        row[9].get<std::string>(),
                        row[10].get<std::string>()
                    );

                    if (!priorityQueue.enqueueRecord(
                            passenger,
                            row[0].get<int>(),
                            row[1].get<std::string>()))
                    {
                        throw std::runtime_error(
                            "Could not reconstruct PriorityQueue"
                        );
                    }

                    row = result.fetchOne();
                }

                if (priorityQueue.isEmpty())
                {
                    session.rollback();
                    response.status = 409;
                    response.set_content(
                        R"({"status":"error","message":"Priority queue is empty"})",
                        "application/json"
                    );
                    return;
                }

                Passenger passenger;
                int priority = 0;
                std::string reason;

                
                if (!priorityQueue.dequeueRecord(passenger, priority, reason))
                {
                    throw std::runtime_error("Could not process priority request");
                }

                // First, preserve the completed request in MySQL history.
                auto historyResult = session.sql("INSERT INTO assistance_request_history "
                "(passenger_id, priority, reason, created_at, "
                "processed_at, status) "
                "SELECT passenger_id, priority, reason, created_at, "
                "CURRENT_TIMESTAMP, 'Completed' "
                "FROM assistance_queue "
                "WHERE passenger_id = ?"
                ).bind(passenger.getPassengerId()).execute();

                if (historyResult.getAffectedItemsCount() != 1)
                {
                    throw std::runtime_error("Could not save assistance request history");
            }

// Now remove the request from the active queue.
auto deleteResult = session.sql(
    "DELETE FROM assistance_queue "
    "WHERE passenger_id = ?"
).bind(passenger.getPassengerId()).execute();

if (deleteResult.getAffectedItemsCount() != 1)
{
    throw std::runtime_error(
        "Could not remove assistance request"
    );
}


                session.commit();

                std::ostringstream body;

                body
                    << "{\"status\":\"success\","
                    << "\"message\":\"Priority request processed\","
                    << "\"priority\":" << priority
                    << ",\"reason\":\"" << jsonEscape(reason) << "\","
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
                << "Could not process PriorityQueue: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not process PriorityQueue"})",
                "application/json"
            );
        }
    }
);

}
