#include "GroupTravelRoutes.h"
#include "AuthMiddleware.h"

#include "GroupTravelManager.h"
#include "FlightManager.h"
#include "httplib.h"

#include <mysqlx/xdevapi.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Existing helper functions implemented in api_main.cpp.
const char* getDbUser();
const char* getDbPassword();

bool databaseCredentialsAvailable();

void setCorsHeaders(httplib::Response& response);

void setNavigatorErrorResponse(
    httplib::Response& response,
    int status,
    const std::string& message
);

std::string trim(const std::string& value);
std::string toUpper(std::string value);
std::string jsonEscape(const std::string& value);

bool loadFlightsIntoManager(
    mysqlx::Session& session,
    FlightManager& manager
);

std::string travelGroupRecordToJson(
    const TravelGroupRecord& record
);

void registerGroupTravelRoutes(httplib::Server& server)
{
    // ================================================
    // GROUP TRAVEL: GET ALL GROUPS
    // ================================================

    server.Get(
        "/api/groups",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            if (!databaseCredentialsAvailable())
            {
                setNavigatorErrorResponse(
                    response, 500,
                    "Database credentials are not configured.");
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

                auto groupResult = session.sql(
                    "SELECT group_id, group_name, flight_number "
                    "FROM travel_groups "
                    "ORDER BY created_at DESC, group_id"
                ).execute();

                auto groupRow = groupResult.fetchOne();

                std::ostringstream body;
                body << "[";

                bool firstGroup = true;

                while (groupRow)
                {
                    TravelGroupRecord record;

                    record.groupId =
                        groupRow[0].get<std::string>();

                    record.groupName =
                        groupRow[1].get<std::string>();

                    record.flightNumber =
                        groupRow[2].get<std::string>();

                    // Load the members belonging to this group.
                    auto memberResult = session.sql(
                        "SELECT passenger_id "
                        "FROM travel_group_members "
                        "WHERE group_id = ? "
                        "ORDER BY member_id"
                    ).bind(record.groupId).execute();

                    auto memberRow = memberResult.fetchOne();

                    while (memberRow)
                    {
                        record.passengerIds.push_back(
                            memberRow[0].get<std::string>()
                        );

                        memberRow = memberResult.fetchOne();
                    }

                    if (authenticatedUser.role == "passenger")
                    {
                        bool isMember = false;
                        std::vector<std::string> ownMembership;
                        for (const auto& memberId : record.passengerIds)
                        {
                            if (canAccessPassenger(authenticatedUser, memberId))
                            {
                                isMember = true;
                                ownMembership.push_back(memberId);
                            }
                        }
                        if (!isMember)
                        {
                            groupRow = groupResult.fetchOne();
                            continue;
                        }
                        record.passengerIds = std::move(ownMembership);
                    }

                    if (!firstGroup)
                        body << ",";

                    firstGroup = false;

                    body << travelGroupRecordToJson(record);

                    groupRow = groupResult.fetchOne();
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
                    << "Group listing failed: "
                    << error.what() << '\n';

                setNavigatorErrorResponse(
                    response, 503,
                    "Could not retrieve travel groups.");
            }
        }
    );

    // ================================================
    // GROUP TRAVEL: CREATE GROUP
    // ================================================

    server.Post(
        "/api/groups",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            auto readField = [&](const std::string& name)
            {
                if (!request.has_param(name))
                    return std::string();

                return trim(request.get_param_value(name));
            };

            const std::string groupId =
                toUpper(readField("groupId"));

            const std::string groupName =
                readField("groupName");

            const std::string flightNumber =
                toUpper(readField("flightNumber"));

            if (groupId.empty() ||
                groupName.empty() ||
                flightNumber.empty())
            {
                setNavigatorErrorResponse(
                    response, 400,
                    "Group ID, group name and flight number are required.");
                return;
            }

            if (groupId.size() > 24 ||
                groupName.size() > 80 ||
                flightNumber.size() > 20)
            {
                setNavigatorErrorResponse(
                    response, 400,
                    "One or more group fields exceed the allowed length.");
                return;
            }

            if (!databaseCredentialsAvailable())
            {
                setNavigatorErrorResponse(
                    response, 500,
                    "Database credentials are not configured.");
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

                // Check for a duplicate group ID in MySQL.
                auto duplicateResult = session.sql(
                    "SELECT group_id "
                    "FROM travel_groups "
                    "WHERE group_id = ?"
                ).bind(groupId).execute();

                if (duplicateResult.fetchOne())
                {
                    setNavigatorErrorResponse(
                        response, 409,
                        "This group ID already exists.");
                    return;
                }

                // Load database flights into the C++ FlightManager.
                FlightManager flightManager(10000);

                if (!loadFlightsIntoManager(
                        session, flightManager))
                {
                    throw std::runtime_error(
                        "Could not load flights into FlightManager.");
                }

                // Validate and create the group through the
                // C++ GroupTravelManager linked-list module.
                GroupTravelManager groupManager;

                TravelGroupRecord record;
                record.groupId = groupId;
                record.groupName = groupName;
                record.flightNumber = flightNumber;

                std::string message;

                if (!groupManager.createGroupRecord(
                        record, flightManager, message))
                {
                    setNavigatorErrorResponse(
                        response, 400, message);
                    return;
                }

                // Persist only after C++ validation succeeds.
                session.sql(
                    "INSERT INTO travel_groups "
                    "(group_id, group_name, flight_number) "
                    "VALUES (?, ?, ?)"
                ).bind(
                    groupId,
                    groupName,
                    flightNumber
                ).execute();

                response.status = 201;

                response.set_content(
                    "{\"success\":true,\"message\":\"" +
                    jsonEscape(message) +
                    "\",\"group\":" +
                    travelGroupRecordToJson(record) +
                    "}",
                    "application/json"
                );
            }
            catch (const std::exception& error)
            {
                std::cerr
                    << "Group creation failed: "
                    << error.what() << '\n';

                setNavigatorErrorResponse(
                    response, 500,
                    "Could not create the travel group.");
            }
        }
    );

    // ================================================
    // GROUP TRAVEL: ADD PASSENGER
    // ================================================

    server.Post(
        "/api/groups/add-passenger",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            auto readField = [&](const std::string& name)
            {
                if (!request.has_param(name))
                    return std::string();

                return trim(request.get_param_value(name));
            };

            const std::string groupId =
                toUpper(readField("groupId"));

            const std::string passengerId =
                toUpper(readField("passengerId"));

            if (groupId.empty() || passengerId.empty())
            {
                setNavigatorErrorResponse(
                    response, 400,
                    "Group ID and passenger ID are required.");
                return;
            }

            if (groupId.size() > 24 ||
                passengerId.size() > 30)
            {
                setNavigatorErrorResponse(
                    response, 400,
                    "Group ID or passenger ID exceeds the allowed length.");
                return;
            }

            if (!databaseCredentialsAvailable())
            {
                setNavigatorErrorResponse(
                    response, 500,
                    "Database credentials are not configured.");
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

                // Retrieve the selected group.
                auto groupResult = session.sql(
                    "SELECT group_name, flight_number "
                    "FROM travel_groups "
                    "WHERE group_id = ?"
                ).bind(groupId).execute();

                auto groupRow = groupResult.fetchOne();

                if (!groupRow)
                {
                    setNavigatorErrorResponse(
                        response, 404,
                        "Travel group not found.");
                    return;
                }

                const std::string groupName =
                    groupRow[0].get<std::string>();

                const std::string flightNumber =
                    groupRow[1].get<std::string>();

                // Check whether this passenger already belongs
                // to any travel group.
                auto existingMember = session.sql(
                    "SELECT group_id "
                    "FROM travel_group_members "
                    "WHERE passenger_id = ?"
                ).bind(passengerId).execute();

                if (existingMember.fetchOne())
                {
                    setNavigatorErrorResponse(
                        response, 409,
                        "This passenger already belongs to a travel group.");
                    return;
                }

                // Verify that the passenger exists and obtain
                // their assigned flight.
                auto passengerResult = session.sql(
                    "SELECT flight_number "
                    "FROM passengers "
                    "WHERE passenger_id = ?"
                ).bind(passengerId).execute();

                auto passengerRow = passengerResult.fetchOne();

                if (!passengerRow)
                {
                    setNavigatorErrorResponse(
                        response, 404,
                        "Passenger record not found.");
                    return;
                }

                const std::string passengerFlight =
                    passengerRow[0].get<std::string>();

                // Load flights into the C++ FlightManager.
                FlightManager flightManager(10000);

                if (!loadFlightsIntoManager(
                        session, flightManager))
                {
                    throw std::runtime_error(
                        "Could not load flights into FlightManager.");
                }

                // Recreate the selected group in the C++
                // linked-list manager for validation.
                GroupTravelManager groupManager;

                TravelGroupRecord groupRecord;
                groupRecord.groupId = groupId;
                groupRecord.groupName = groupName;
                groupRecord.flightNumber = flightNumber;

                std::string message;

                if (!groupManager.createGroupRecord(
                        groupRecord, flightManager, message))
                {
                    setNavigatorErrorResponse(
                        response, 400, message);
                    return;
                }

                // C++ validation checks:
                // 1. Group exists.
                // 2. Flight is available.
                // 3. Passenger's flight matches the group flight.
                // 4. Passenger is not already in this manager.
                if (!groupManager.addPassengerRecord(
                        groupId,
                        passengerId,
                        passengerFlight,
                        flightManager,
                        message))
                {
                    setNavigatorErrorResponse(
                        response, 400, message);
                    return;
                }

                // Persist the member after validation.
                session.sql(
                    "INSERT INTO travel_group_members "
                    "(group_id, passenger_id) "
                    "VALUES (?, ?)"
                ).bind(
                    groupId,
                    passengerId
                ).execute();

                response.status = 201;

                response.set_content(
                    "{\"success\":true,"
                    "\"message\":\"" + jsonEscape(message) + "\","
                    "\"groupId\":\"" + jsonEscape(groupId) + "\","
                    "\"passengerId\":\"" + jsonEscape(passengerId) + "\""
                    "}",
                    "application/json"
                );
            }
            catch (const std::exception& error)
            {
                std::cerr
                    << "Adding group passenger failed: "
                    << error.what() << '\n';

                setNavigatorErrorResponse(
                    response, 500,
                    "Could not add the passenger to the travel group.");
            }
        }
    );
}
