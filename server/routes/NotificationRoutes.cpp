#include "NotificationRoutes.h"
#include "AuthMiddleware.h"

#include "NotificationManager.h"
#include "httplib.h"

#include <mysqlx/xdevapi.h>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>

// Existing shared helpers implemented in api_main.cpp.
const char* getDbUser();
const char* getDbPassword();

bool databaseCredentialsAvailable();

void setCorsHeaders(httplib::Response& response);

std::string trim(const std::string& value);
std::string jsonEscape(const std::string& value);

bool loadNotificationsIntoManager(
    mysqlx::Session& session,
    NotificationManager& manager
);

std::string notificationRecordToJson(
    const NotificationRecord& notification
);


void registerNotificationRoutes(httplib::Server& server)
{
    // =================================================
    // GET: DISPLAY ALL NOTIFICATIONS
    // =================================================

    server.Get(
        "/api/notifications",
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

                NotificationManager notificationManager;

                if (!loadNotificationsIntoManager(
                        session, notificationManager))
                {
                    throw std::runtime_error(
                        "Could not reconstruct NotificationManager"
                    );
                }

                std::ostringstream body;
                body << "[";
                bool firstVisibleNotification = true;

                for (std::size_t i = 0;
                     i < notificationManager.getSize(); ++i)
                {
                    NotificationRecord notification;

                    if (!notificationManager.getNotificationAt(
                            i, notification))
                    {
                        throw std::runtime_error(
                            "Could not retrieve notification"
                        );
                    }

                    if (!canAccessPassenger(authenticatedUser, notification.passengerId))
                        continue;

                    if (!firstVisibleNotification)
                        body << ",";
                    firstVisibleNotification = false;

                    body << notificationRecordToJson(notification);
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
                    << "Could not load notifications: "
                    << error.what() << '\n';

                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Could not load notifications"})",
                    "application/json"
                );
            }
        }
    );


    // =================================================
    // POST: CREATE A NOTIFICATION
    // =================================================

    server.Post(
        "/api/notifications",
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

            auto readField = [&](const std::string& name)
            {
                if (!request.has_param(name))
                    return std::string();

                return trim(request.get_param_value(name));
            };

            const std::string passengerId =
                readField("passengerId");

            std::string flightNumber =
                readField("flightNumber");

            const std::string type =
                readField("type");

            const std::string message =
                readField("message");

            const std::vector<std::string> validTypes = {
                "Check-In Reminder",
                "Boarding Reminder",
                "Gate Change",
                "Flight Status",
                "Delay",
                "Cancellation",
                "Connection Risk",
                "Assistance Request"
            };

            if (passengerId.empty() ||
                passengerId.size() > 30 ||
                flightNumber.size() > 20 ||
                type.empty() ||
                type.size() > 50 ||
                message.empty() ||
                message.size() > 1000 ||
                std::find(
                    validTypes.begin(),
                    validTypes.end(),
                    type
                ) == validTypes.end())
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid notification details"})",
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
            if (!canAccessPassenger(authenticatedUser, passengerId))
            {
                response.status = 403;
                response.set_content(R"({"status":"error","message":"You can only create notifications for your own passenger account"})", "application/json");
                return;
            }
                session.startTransaction();

                try
                {
                    NotificationManager notificationManager;

                    if (!loadNotificationsIntoManager(
                            session, notificationManager))
                    {
                        throw std::runtime_error(
                            "Could not reconstruct NotificationManager"
                        );
                    }

                    // Retrieve the passenger's actual database details.
                    auto passengerResult = session.sql(
                        "SELECT name, flight_number "
                        "FROM passengers "
                        "WHERE passenger_id = ?"
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

                    const std::string passengerName =
                        passengerRow[0].get<std::string>();

                    if (flightNumber.empty())
                    {
                        flightNumber =
                            passengerRow[1].get<std::string>();
                    }

                    // Verify that the notification's flight exists.
                    auto flightResult = session.sql(
                        "SELECT flight_number FROM flights "
                        "WHERE flight_number = ?"
                    ).bind(flightNumber).execute();

                    if (!flightResult.fetchOne())
                    {
                        session.rollback();

                        response.status = 404;
                        response.set_content(
                            R"({"status":"error","message":"Notification flight not found"})",
                            "application/json"
                        );
                        return;
                    }

                    // Insert the notification.
                    session.sql(
                        "INSERT INTO notifications "
                        "(passenger_id, passenger_name, flight_number, "
                        "notification_type, message, is_read) "
                        "VALUES (?, ?, ?, ?, ?, FALSE)"
                    )
                    .bind(
                        passengerId,
                        passengerName,
                        flightNumber,
                        type,
                        message
                    )
                    .execute();

                    // Obtain the database-generated notification ID.
                    auto idResult = session.sql(
                        "SELECT LAST_INSERT_ID()"
                    ).execute();

                    auto idRow = idResult.fetchOne();

                    if (!idRow)
                    {
                        throw std::runtime_error(
                            "Could not obtain notification ID"
                        );
                    }

                    const long long notificationId =
                        idRow[0].get<long long>();

                    // Obtain the actual database timestamp.
                    auto timeResult = session.sql(
                        "SELECT DATE_FORMAT(created_at, "
                        "'%Y-%m-%dT%H:%i:%s.%f') "
                        "FROM notifications "
                        "WHERE notification_id = ?"
                    ).bind(notificationId).execute();

                    auto timeRow = timeResult.fetchOne();

                    if (!timeRow)
                    {
                        throw std::runtime_error(
                            "Could not obtain notification timestamp"
                        );
                    }

                    // Add the record to the C++ NotificationManager.
                    NotificationRecord notification;

                    notification.notificationId = notificationId;
                    notification.passengerId = passengerId;
                    notification.passengerName = passengerName;
                    notification.flightNumber = flightNumber;
                    notification.type = type;
                    notification.message = message;
                    notification.isRead = false;
                    notification.createdAt =
                        timeRow[0].get<std::string>();

                    std::string managerMessage;

                    if (!notificationManager.appendNotificationRecord(
                            notification, managerMessage))
                    {
                        throw std::runtime_error(managerMessage);
                    }

                    session.commit();

                    response.set_content(
                        "{\"status\":\"success\","
                        "\"message\":\"Notification created\","
                        "\"notification\":" +
                        notificationRecordToJson(notification) +
                        "}",
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
                    << "Could not create notification: "
                    << error.what() << '\n';

                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Could not create notification"})",
                    "application/json"
                );
            }
        }
    );


    // =================================================
    // POST: MARK ONE NOTIFICATION AS READ
    // =================================================

    server.Post(
        "/api/notifications/read",
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

            if (!request.has_param("notificationId"))
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Notification ID is required"})",
                    "application/json"
                );
                return;
            }

            long long notificationId = 0;

            try
            {
                const std::string idText =
                    trim(request.get_param_value("notificationId"));

                std::size_t parsedLength = 0;

                notificationId = std::stoll(
                    idText, &parsedLength
                );

                if (parsedLength != idText.size() ||
                    notificationId <= 0)
                {
                    throw std::invalid_argument("Invalid ID");
                }
            }
            catch (...)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid notification ID"})",
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

            auto ownerResult = session.sql(
                "SELECT passenger_id FROM notifications WHERE notification_id = ? LIMIT 1"
            ).bind(notificationId).execute();
            auto ownerRow = ownerResult.fetchOne();
            if (!ownerRow)
            {
                response.status = 404;
                response.set_content(R"({"status":"error","message":"Notification not found"})", "application/json");
                return;
            }
            const std::string ownerPassengerId = ownerRow[0].get<std::string>();
            if (!canAccessPassenger(authenticatedUser, ownerPassengerId))
            {
                response.status = 403;
                response.set_content(R"({"status":"error","message":"You can only update your own notifications"})", "application/json");
                return;
            }

            session.startTransaction();

                try
                {
                    NotificationManager notificationManager;

                    if (!loadNotificationsIntoManager(
                            session, notificationManager))
                    {
                        throw std::runtime_error(
                            "Could not reconstruct NotificationManager"
                        );
                    }

                    std::string managerMessage;

                    if (!notificationManager.setNotificationReadRecord(
                            notificationId, true, managerMessage))
                    {
                        session.rollback();

                        response.status = 404;
                        response.set_content(
                            "{\"status\":\"error\",\"message\":\"" +
                            jsonEscape(managerMessage) + "\"}",
                            "application/json"
                        );
                        return;
                    }

                    // Persist the read status in MySQL.
                    session.sql(
                        "UPDATE notifications "
                        "SET is_read = TRUE "
                        "WHERE notification_id = ?"
                    ).bind(notificationId).execute();

                    // Retrieve the updated linked-list record.
                    NotificationRecord updatedNotification;
                    bool found = false;

                    for (int i = 0;
                         i < notificationManager.getSize(); ++i)
                    {
                        NotificationRecord current;

                        if (notificationManager.getNotificationAt(
                                static_cast<std::size_t>(i), current) &&
                            current.notificationId == notificationId)
                        {
                            updatedNotification = current;
                            found = true;
                            break;
                        }
                    }

                    if (!found)
                    {
                        throw std::runtime_error(
                            "Updated notification could not be retrieved"
                        );
                    }

                    session.commit();

                    response.set_content(
                        "{\"status\":\"success\","
                        "\"message\":\"Notification marked as read\","
                        "\"notification\":" +
                        notificationRecordToJson(updatedNotification) +
                        "}",
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
                    << "Could not mark notification as read: "
                    << error.what() << '\n';

                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Could not update notification"})",
                    "application/json"
                );
            }
        }
    );


    // =================================================
    // POST: MARK ALL NOTIFICATIONS AS READ
    // =================================================

    server.Post(
        "/api/notifications/read-all",
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

            std::string passengerId;

            if (request.has_param("passengerId"))
            {
                passengerId =
                    trim(request.get_param_value("passengerId"));
            }

            if (passengerId.size() > 30)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid passenger ID"})",
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

            if (authenticatedUser.role == "passenger")
            {
                if (!passengerId.empty() && !canAccessPassenger(authenticatedUser, passengerId))
                {
                    response.status = 403;
                    response.set_content(R"({"status":"error","message":"You can only update your own notifications"})", "application/json");
                    return;
                }
                passengerId = authenticatedUser.passengerId;
            }

            session.startTransaction();

                try
                {
                    NotificationManager notificationManager;

                    if (!loadNotificationsIntoManager(
                            session, notificationManager))
                    {
                        throw std::runtime_error(
                            "Could not reconstruct NotificationManager"
                        );
                    }

                    // Update the C++ linked list first.
                    const std::size_t markedCount =
                        notificationManager.markAllNotificationRecordsRead(
                            passengerId
                        );

                    // Persist the same operation in MySQL.
                    if (passengerId.empty())
                    {
                        session.sql(
                            "UPDATE notifications "
                            "SET is_read = TRUE "
                            "WHERE is_read = FALSE"
                        ).execute();
                    }
                    else
                    {
                        session.sql(
                            "UPDATE notifications "
                            "SET is_read = TRUE "
                            "WHERE passenger_id = ? "
                            "AND is_read = FALSE"
                        ).bind(passengerId).execute();
                    }

                    session.commit();

                    response.set_content(
                        "{\"status\":\"success\","
                        "\"message\":\"Notifications updated\","
                        "\"markedCount\":" +
                        std::to_string(markedCount) +
                        "}",
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
                    << "Could not mark notifications as read: "
                    << error.what() << '\n';

                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Could not update notifications"})",
                    "application/json"
                );
            }
        }
    );
}
