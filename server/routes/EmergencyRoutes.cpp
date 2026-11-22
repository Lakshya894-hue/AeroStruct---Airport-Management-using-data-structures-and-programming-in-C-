#include "EmergencyRoutes.h"
#include "AuthMiddleware.h"
#include "EmergencyAssistanceManager.h"

#include "httplib.h"
#include <mysqlx/xdevapi.h>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>


// Database helpers
const char* getDbUser();
const char* getDbPassword();
bool databaseCredentialsAvailable();

// HTTP helper
void setCorsHeaders(httplib::Response& response);

// String and JSON helpers
std::string trim(const std::string& value);
std::string toUpper(std::string value);
std::string jsonEscape(const std::string& value);

// Emergency-specific helpers
bool loadEmergencyCasesIntoManager(
    mysqlx::Session& session,
    EmergencyAssistanceManager& manager
);

std::string emergencyIncidentToJson(
    const EmergencyIncidentRecord& incident
);

std::vector<std::string> parseJsonStringArray(
    const std::string& json
);

std::string stringVectorToJson(
    const std::vector<std::string>& values
);
void registerEmergencyRoutes(httplib::Server& server)
{
    server.Get(
    "/api/emergencies",
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
            EmergencyAssistanceManager emergencyManager;

            if (!loadEmergencyCasesIntoManager(
                    session, emergencyManager))
            {
                throw std::runtime_error(
                    "Could not reconstruct EmergencyAssistanceManager"
                );
            }

            std::ostringstream body;
            body << "[";
            bool firstVisibleIncident = true;

            for (std::size_t i = 0;
                 i < emergencyManager.getIncidentCount(); ++i)
            {
                EmergencyIncidentRecord incident;

                if (!emergencyManager.getIncidentAt(i, incident))
                {
                    throw std::runtime_error(
                        "Could not retrieve emergency incident"
                    );
                }

                if (authenticatedUser.role == "passenger" &&
                    !canAccessPassenger(authenticatedUser, incident.primaryPassengerId))
                {
                    continue;
                }

                // A passenger response must not expose other affected passengers.
                if (authenticatedUser.role == "passenger")
                {
                    incident.affectedPassengers.erase(
                        std::remove_if(
                            incident.affectedPassengers.begin(),
                            incident.affectedPassengers.end(),
                            [&](const auto& affected)
                            {
                                return !canAccessPassenger(
                                    authenticatedUser,
                                    affected.passengerId
                                );
                            }
                        ),
                        incident.affectedPassengers.end()
                    );
                }

                if (!firstVisibleIncident)
                    body << ",";
                firstVisibleIncident = false;

                body << emergencyIncidentToJson(incident);
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
                << "Could not load emergency cases: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not load emergency cases"})",
                "application/json"
            );
        }
    }
);

// Register an emergency case in MySQL and EmergencyAssistanceManager.
server.Post(
    "/api/emergencies",
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

        std::string caseId = toUpper(readField("caseId"));
        std::string primaryPassengerId =
            readField("primaryPassengerId");
        std::string emergencyType = readField("emergencyType");
        std::string severityText = readField("severity");
        std::string incidentLocation =
            readField("incidentLocation");
        std::string description = readField("description");
        std::string affectedIdsText =
            readField("affectedPassengerIds");

        // Validate the basic incident fields.
        if (caseId.size() < 4 || caseId.size() > 20 ||
            caseId.substr(0, 3) != "EMR" ||
            primaryPassengerId.empty() ||
            primaryPassengerId.size() > 30 ||
            incidentLocation.empty() ||
            incidentLocation.size() > 100 ||
            description.size() < 3 ||
            description.size() > 300)
        {
            response.status = 400;
            response.set_content(
                R"({"status":"error","message":"One or more emergency fields are invalid"})",
                "application/json"
            );
            return;
        }

        // Only the supported emergency types are accepted.
        const std::vector<std::string> validTypes = {
            "Medical Emergency",
            "Fire / Smoke",
            "Security Threat",
            "Missing / Separated Passenger",
            "Other Emergency"
        };

        if (std::find(
                validTypes.begin(),
                validTypes.end(),
                emergencyType) == validTypes.end())
        {
            response.status = 400;
            response.set_content(
                R"({"status":"error","message":"Invalid emergency type"})",
                "application/json"
            );
            return;
        }

        if (severityText != "1" &&
            severityText != "2" &&
            severityText != "3")
        {
            response.status = 400;
            response.set_content(
                R"({"status":"error","message":"Severity must be 1, 2 or 3"})",
                "application/json"
            );
            return;
        }

        const int severity = std::stoi(severityText);

        // Read the additional affected passenger IDs.
        std::vector<std::string> affectedIds;
        std::stringstream affectedStream(affectedIdsText);
        std::string affectedId;

        while (std::getline(affectedStream, affectedId, ','))
        {
            affectedId = trim(affectedId);

            if (affectedId.empty())
                continue;

            affectedIds.push_back(affectedId);
        }

        if (affectedIds.size() > 20)
        {
            response.status = 400;
            response.set_content(
                R"({"status":"error","message":"A maximum of 20 additional passengers is allowed"})",
                "application/json"
            );
            return;
        }

        // Reject duplicate IDs, including the primary passenger.
        std::vector<std::string> seenIds;
        seenIds.push_back(toUpper(primaryPassengerId));

        for (const auto& id : affectedIds)
        {
            const std::string normalizedId = toUpper(id);

            if (std::find(
                    seenIds.begin(),
                    seenIds.end(),
                    normalizedId) != seenIds.end())
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Duplicate affected passenger ID"})",
                    "application/json"
                );
                return;
            }

            seenIds.push_back(normalizedId);
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

            if (!canAccessPassenger(authenticatedUser, primaryPassengerId))
            {
                response.status = 403;
                response.set_content(R"({"status":"error","message":"You can only report an emergency for your own passenger record"})", "application/json");
                return;
            }
            if (authenticatedUser.role == "passenger" && !affectedIds.empty())
            {
                response.status = 403;
                response.set_content(R"({"status":"error","message":"Passenger emergency reports cannot add other affected passengers"})", "application/json");
                return;
            }

            session.startTransaction();

            try
            {
                // Reconstruct the existing emergency linked list.
                EmergencyAssistanceManager emergencyManager;

                if (!loadEmergencyCasesIntoManager(
                        session, emergencyManager))
                {
                    throw std::runtime_error(
                        "Could not reconstruct EmergencyAssistanceManager"
                    );
                }

                // Verify the primary passenger and their flight.
                auto primaryResult = session.sql(
                    "SELECT p.name, p.flight_number, f.status "
                    "FROM passengers p "
                    "INNER JOIN flights f "
                    "ON p.flight_number = f.flight_number "
                    "WHERE p.passenger_id = ?"
                ).bind(primaryPassengerId).execute();

                auto primaryRow = primaryResult.fetchOne();

                if (!primaryRow)
                {
                    session.rollback();
                    response.status = 404;
                    response.set_content(
                        R"({"status":"error","message":"Primary passenger or assigned flight was not found"})",
                        "application/json"
                    );
                    return;
                }

                const std::string primaryPassengerName =
                    primaryRow[0].get<std::string>();

                const std::string flightNumber =
                    primaryRow[1].get<std::string>();

                const std::string flightStatus =
                    toUpper(primaryRow[2].get<std::string>());

                if (flightStatus == "CANCELLED" ||
                    flightStatus == "CANCELED")
                {
                    session.rollback();
                    response.status = 400;
                    response.set_content(
                        R"({"status":"error","message":"Cannot register an emergency case for a passenger on a cancelled flight"})",
                        "application/json"
                    );
                    return;
                }

                // Construct the record for the C++ manager.
                EmergencyIncidentRecord incident;

                incident.caseId = caseId;
                incident.primaryPassengerId = primaryPassengerId;
                incident.primaryPassengerName = primaryPassengerName;
                incident.flightNumber = flightNumber;
                incident.emergencyType = emergencyType;
                incident.severity = severity;
                incident.incidentLocation = incidentLocation;
                incident.description = description;
                incident.status = "Open";
                incident.recommendedFacility = "";
                incident.route.clear();
                incident.routeMinutes = -1;
                incident.routeMeters = -1.0;
                incident.routeMessage =
                    "Find a suitable facility route when needed.";

                // Use the database timestamp for the record.
                auto timeResult = session.sql(
                    "SELECT DATE_FORMAT("
                    "CURRENT_TIMESTAMP(3), "
                    "'%Y-%m-%dT%H:%i:%s.%f')"
                ).execute();

                auto timeRow = timeResult.fetchOne();

                if (!timeRow)
                    throw std::runtime_error(
                        "Could not obtain the incident timestamp"
                    );

                incident.reportedAt =
                    timeRow[0].get<std::string>();

                // The primary passenger is always first.
                incident.affectedPassengers.push_back({
                    primaryPassengerId,
                    primaryPassengerName
                });

                // Verify and add each additional passenger.
                for (const auto& id : affectedIds)
                {
                    auto passengerResult = session.sql(
                        "SELECT name FROM passengers "
                        "WHERE passenger_id = ?"
                    ).bind(id).execute();

                    auto passengerRow =
                        passengerResult.fetchOne();

                    if (!passengerRow)
                    {
                        session.rollback();
                        response.status = 404;
                        response.set_content(
                            R"({"status":"error","message":"One or more affected passengers were not found"})",
                            "application/json"
                        );
                        return;
                    }

                    incident.affectedPassengers.push_back({
                        id,
                        passengerRow[0].get<std::string>()
                    });
                }

                // Insert into the severity-ordered C++ linked list.
                std::string managerMessage;

                if (!emergencyManager.addIncidentRecord(
                        incident, managerMessage))
                {
                    session.rollback();
                    response.status = 409;
                    response.set_content(
                        "{\"status\":\"error\",\"message\":\"" +
                        jsonEscape(managerMessage) + "\"}",
                        "application/json"
                    );
                    return;
                }

                // Save the main incident record.
                session.sql(
                    "INSERT INTO emergency_cases "
                    "(case_id, primary_passenger_id, "
                    "primary_passenger_name, flight_number, "
                    "emergency_type, severity, incident_location, "
                    "description, status, recommended_facility, "
                    "route_json, route_meters, route_minutes, "
                    "route_message) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, "
                    "'Open', '', JSON_ARRAY(), NULL, NULL, ?)"
                )
                .bind(
                    incident.caseId,
                    incident.primaryPassengerId,
                    incident.primaryPassengerName,
                    incident.flightNumber,
                    incident.emergencyType,
                    incident.severity,
                    incident.incidentLocation,
                    incident.description,
                    incident.routeMessage
                )
                .execute();

                // Save the primary and additional passengers.
                for (std::size_t i = 0;
                     i < incident.affectedPassengers.size(); ++i)
                {
                    const auto& passenger =
                        incident.affectedPassengers[i];

                    session.sql(
                        "INSERT INTO emergency_case_passengers "
                        "(case_id, passenger_id, passenger_name, "
                        "is_primary) VALUES (?, ?, ?, ?)"
                    )
                    .bind(
                        incident.caseId,
                        passenger.passengerId,
                        passenger.passengerName,
                        i == 0
                    )
                    .execute();
                }

                session.commit();

                response.set_content(
                    "{\"status\":\"success\","
                    "\"message\":\"Emergency case registered\","
                    "\"incident\":" +
                    emergencyIncidentToJson(incident) +
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
                << "Could not register emergency case: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not register emergency case"})",
                "application/json"
            );
        }
    }
);


// Update an emergency case status in MySQL and the C++ linked list.
server.Post(
    "/api/emergencies/status",
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

        if (!request.has_param("caseId") ||
            !request.has_param("status"))
        {
            response.status = 400;
            response.set_content(
                R"({"status":"error","message":"Case ID and status are required"})",
                "application/json"
            );
            return;
        }

        const std::string caseId =
            toUpper(trim(request.get_param_value("caseId")));

        const std::string requestedStatus =
            toUpper(trim(request.get_param_value("status")));

        std::string nextStatus;

        if (requestedStatus == "OPEN")
            nextStatus = "Open";
        else if (requestedStatus == "IN PROGRESS")
            nextStatus = "In Progress";
        else if (requestedStatus == "RESOLVED")
            nextStatus = "Resolved";
        else
        {
            response.status = 400;
            response.set_content(
                R"({"status":"error","message":"Invalid emergency status"})",
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
                EmergencyAssistanceManager emergencyManager;

                if (!loadEmergencyCasesIntoManager(
                        session, emergencyManager))
                {
                    throw std::runtime_error(
                        "Could not reconstruct EmergencyAssistanceManager"
                    );
                }

                std::string managerMessage;

                if (!emergencyManager.updateIncidentStatusRecord(
                        caseId, nextStatus, managerMessage))
                {
                    session.rollback();

                    response.status = 400;
                    response.set_content(
                        "{\"status\":\"error\",\"message\":\"" +
                        jsonEscape(managerMessage) + "\"}",
                        "application/json"
                    );
                    return;
                }

                // Persist the status change in MySQL.
                session.sql(
                    "UPDATE emergency_cases "
                    "SET status = ? "
                    "WHERE case_id = ?"
                )
                .bind(nextStatus, caseId)
                .execute();

                // Retrieve the updated record from the C++ manager.
                EmergencyIncidentRecord updatedIncident;
                bool incidentFound = false;

                for (std::size_t i = 0;
                     i < emergencyManager.getIncidentCount(); ++i)
                {
                    EmergencyIncidentRecord currentIncident;

                    if (emergencyManager.getIncidentAt(
                            i, currentIncident) &&
                        currentIncident.caseId == caseId)
                    {
                        updatedIncident = currentIncident;
                        incidentFound = true;
                        break;
                    }
                }

                if (!incidentFound)
                {
                    throw std::runtime_error(
                        "Updated emergency incident could not be retrieved"
                    );
                }

                session.commit();

                response.set_content(
                    "{\"status\":\"success\","
                    "\"message\":\"Emergency status updated\","
                    "\"incident\":" +
                    emergencyIncidentToJson(updatedIncident) +
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
                << "Could not update emergency status: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not update emergency status"})",
                "application/json"
            );
        }
    }
);

// Save the recommended facility and calculated route in MySQL.
server.Post(
    "/api/emergencies/route",
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

        const std::string caseId =
            toUpper(readField("caseId"));

        std::string facility =
            readField("facility");

        const std::string routeJson =
            readField("routeJson");

        const std::string routeMetersText =
            readField("routeMeters");

        std::string routeMessage =
            readField("routeMessage");

        if (caseId.size() < 4 ||
            caseId.size() > 20 ||
            caseId.substr(0, 3) != "EMR")
        {
            response.status = 400;
            response.set_content(
                R"({"status":"error","message":"Invalid emergency case ID"})",
                "application/json"
            );
            return;
        }

        if (facility.size() > 150 ||
            routeMessage.size() > 255 ||
            routeJson.empty() ||
            routeJson.size() > 10000 ||
            routeJson.front() != '[' ||
            routeJson.back() != ']')
        {
            response.status = 400;
            response.set_content(
                R"({"status":"error","message":"Invalid route information"})",
                "application/json"
            );
            return;
        }

        // Reconstruct the route from the JSON array sent by JavaScript.
        std::vector<std::string> route =
            parseJsonStringArray(routeJson);

        // An empty route is valid only when the request contains [].
        if (route.empty())
        {
            std::string compactJson;

            for (char ch : routeJson)
            {
                if (!std::isspace(static_cast<unsigned char>(ch)))
                    compactJson.push_back(ch);
            }

            if (compactJson != "[]")
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid route path"})",
                    "application/json"
                );
                return;
            }
        }

        if (route.size() > 100)
        {
            response.status = 400;
            response.set_content(
                R"({"status":"error","message":"Route contains too many locations"})",
                "application/json"
            );
            return;
        }

        for (const auto& location : route)
        {
            if (location.empty() || location.size() > 120)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid location in route"})",
                    "application/json"
                );
                return;
            }
        }

        double routeMeters = -1.0;

        if (!routeMetersText.empty())
        {
            try
            {
                std::size_t parsedLength = 0;

                routeMeters = std::stod(
                    routeMetersText,
                    &parsedLength
                );

                if (parsedLength != routeMetersText.size() ||
                    !std::isfinite(routeMeters) ||
                    routeMeters < 0)
                {
                    throw std::invalid_argument(
                        "Invalid route distance"
                    );
                }
            }
            catch (...)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Route distance must be a valid non-negative number"})",
                    "application/json"
                );
                return;
            }
        }

        // A route without a facility or distance is not considered valid.
        // Empty routes represent cases where no suitable route was found.
        if (!route.empty())
        {
            if (facility.empty() || routeMeters < 0)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"A facility and route distance are required for a calculated route"})",
                    "application/json"
                );
                return;
            }
        }
        else
        {
            facility.clear();
            routeMeters = -1.0;
        }

        if (routeMessage.empty())
        {
            routeMessage =
                "Find a suitable facility route when needed.";
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
                EmergencyAssistanceManager emergencyManager;

                if (!loadEmergencyCasesIntoManager(
                        session, emergencyManager))
                {
                    throw std::runtime_error(
                        "Could not reconstruct EmergencyAssistanceManager"
                    );
                }

                // Update the route through the C++ linked-list manager.
                // Browser-calculated distance is in metres.
                // C++ walking minutes remain separate and unavailable here.
                const long long routeMinutes = -1;

                std::string managerMessage;

                if (!emergencyManager.updateIncidentRouteRecord(
                        caseId,
                        facility,
                        route,
                        routeMinutes,
                        routeMeters,
                        routeMessage,
                        managerMessage))
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

                // Persist the route and its measurements.
                session.sql(
                    "UPDATE emergency_cases SET "
                    "recommended_facility = ?, "
                    "route_json = CAST(? AS JSON), "
                    "route_meters = CASE WHEN ? < 0 "
                    "THEN NULL ELSE ? END, "
                    "route_minutes = CASE WHEN ? < 0 "
                    "THEN NULL ELSE ? END, "
                    "route_message = ? "
                    "WHERE case_id = ?"
                )
                .bind(
                    facility,
                    stringVectorToJson(route),
                    routeMeters,
                    routeMeters,
                    routeMinutes,
                    routeMinutes,
                    routeMessage,
                    caseId
                )
                .execute();

                // Retrieve the updated incident from the C++ manager.
                EmergencyIncidentRecord updatedIncident;
                bool incidentFound = false;

                for (std::size_t i = 0;
                     i < emergencyManager.getIncidentCount(); ++i)
                {
                    EmergencyIncidentRecord currentIncident;

                    if (emergencyManager.getIncidentAt(
                            i, currentIncident) &&
                        currentIncident.caseId == caseId)
                    {
                        updatedIncident = currentIncident;
                        incidentFound = true;
                        break;
                    }
                }

                if (!incidentFound)
                {
                    throw std::runtime_error(
                        "Updated emergency incident could not be retrieved"
                    );
                }

                session.commit();

                response.set_content(
                    "{\"status\":\"success\","
                    "\"message\":\"Emergency facility route saved\","
                    "\"incident\":" +
                    emergencyIncidentToJson(updatedIncident) +
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
                << "Could not save emergency route: "
                << error.what() << '\n';

            response.status = 503;
            response.set_content(
                R"({"status":"error","message":"Could not save emergency route"})",
                "application/json"
            );
        }
    }
);
}
