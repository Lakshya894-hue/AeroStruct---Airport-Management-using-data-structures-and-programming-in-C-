#include "httplib.h"
#include <mysqlx/xdevapi.h>
#include "Flight.h"
#include "FlightManager.h"
#include "Passenger.h"
#include "PassengerManager.h"
#include "CheckInQueue.h"
#include "BoardingQueue.h"
#include "PriorityQueue.h"
#include "EmergencyAssistanceManager.h"
#include "NotificationManager.h"
#include "AirportNavigator.h"
#include "GroupTravelManager.h"
#include "BaggageManager.h"
#include "AeroAssist.h"
#include "SmartFlightSuggestion.h"
#include "routes/HealthRoutes.h"
#include "routes/FlightRoutes.h"
#include "routes/PassengerRoutes.h"
#include "routes/CheckInRoutes.h"
#include "routes/BoardingRoutes.h"
#include "routes/BaggageRoutes.h"
#include "routes/PriorityRoutes.h"
#include "routes/EmergencyRoutes.h"
#include "routes/SmartFlightRoutes.h"
#include "routes/NavigationRoutes.h"
#include "routes/GroupTravelRoutes.h"
#include "routes/NotificationRoutes.h"
#include "routes/AeroAssistRoutes.h"
#include "routes/AuthRoutes.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <string>
#include <limits>
#include <vector>

const char* getDbUser()
{
    return std::getenv("AEROSTRUCT_DB_USER");
}

const char* getDbPassword()
{
    return std::getenv("AEROSTRUCT_DB_PASSWORD");
}

void setCorsHeaders(httplib::Response& response)
{
    response.set_header(
        "Access-Control-Allow-Origin", "*"
    );

    response.set_header(
        "Access-Control-Allow-Methods",
        "GET, POST, OPTIONS"
    );

    response.set_header(
    "Access-Control-Allow-Headers",
    "Content-Type, Authorization"
    );
}

std::string jsonEscape(const std::string& value)
{
    std::ostringstream out;

    for (unsigned char ch : value)
    {
        switch (ch)
        {
            case '"':  out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b";  break;
            case '\f': out << "\\f";  break;
            case '\n': out << "\\n";  break;
            case '\r': out << "\\r";  break;
            case '\t': out << "\\t";  break;

            default:
                if (ch < 0x20)
                {
                    out << "\\u"
                        << std::hex << std::setw(4)
                        << std::setfill('0')
                        << static_cast<int>(ch)
                        << std::dec;
                }
                else
                {
                    out << static_cast<char>(ch);
                }
        }
    }

    return out.str();
}

std::string flightToJson(const Flight& flight)
{
    std::ostringstream out;

    out << "{\"flightNumber\":\""
        << jsonEscape(flight.getFlightNumber())
        << "\",\"airline\":\""
        << jsonEscape(flight.getAirline())
        << "\",\"source\":\""
        << jsonEscape(flight.getSource())
        << "\",\"destination\":\""
        << jsonEscape(flight.getDestination())
        << "\",\"departureTime\":\""
        << jsonEscape(flight.getDepartureTime())
        << "\",\"arrivalTime\":\""
        << jsonEscape(flight.getArrivalTime())
        << "\",\"gate\":\""
        << jsonEscape(flight.getGate())
        << "\",\"status\":\""
        << jsonEscape(flight.getStatus())
        << "\",\"fare\":"
        << std::setprecision(15)
        << flight.getFare()
        << "}";

    return out.str();
}

// Convert Passenger object into JSON.
std::string passengerToJson(const Passenger& passenger)
{
    std::ostringstream out;

    out << "{\"passengerId\":\""
        << jsonEscape(passenger.getPassengerId())
        << "\",\"name\":\""
        << jsonEscape(passenger.getName())
        << "\",\"age\":"
        << passenger.getAge()
        << ",\"gender\":\""
        << jsonEscape(passenger.getGender())
        << "\",\"phoneNumber\":\""
        << jsonEscape(passenger.getPhoneNumber())
        << "\",\"flightNumber\":\""
        << jsonEscape(passenger.getFlightNumber())
        << "\",\"seatNumber\":\""
        << jsonEscape(passenger.getSeatNumber())
        << "\",\"assistanceType\":\""
        << jsonEscape(passenger.getAssistanceType())
        << "\",\"status\":\""
        << jsonEscape(passenger.getStatus())
        << "\"}";

    return out.str();
}

bool databaseCredentialsAvailable()
{
    return getDbUser() != nullptr &&
           getDbPassword() != nullptr &&
           *getDbUser() != '\0' &&
           *getDbPassword() != '\0';
}


// Remove leading and trailing whitespace.
std::string trim(const std::string& value)
{
    const auto first = std::find_if(
        value.begin(), value.end(),
        [](unsigned char ch) {
            return !std::isspace(ch);
        }
    );

    const auto last = std::find_if(
        value.rbegin(), value.rend(),
        [](unsigned char ch) {
            return !std::isspace(ch);
        }
    ).base();

    if (first >= last)
        return "";

    return std::string(first, last);
}

// Convert a string to uppercase.
std::string toUpper(std::string value)
{
    for (char& ch : value)
    {
        ch = static_cast<char>(
            std::toupper(static_cast<unsigned char>(ch))
        );
    }

    return value;
}

// Validate 24-hour time in HH:MM format.
bool isValidFlightTime(const std::string& value)
{
    if (value.size() != 5 || value[2] != ':')
        return false;

    if (!std::isdigit(static_cast<unsigned char>(value[0])) ||
        !std::isdigit(static_cast<unsigned char>(value[1])) ||
        !std::isdigit(static_cast<unsigned char>(value[3])) ||
        !std::isdigit(static_cast<unsigned char>(value[4])))
        return false;

    int hours = std::stoi(value.substr(0, 2));
    int minutes = std::stoi(value.substr(3, 2));

    return hours >= 0 && hours <= 23 &&
           minutes >= 0 && minutes <= 59;
}

// Load existing database records into FlightManager.
bool loadFlightsIntoManager(
    mysqlx::Session& session,
    FlightManager& manager)
{
    auto result = session.sql(
        "SELECT flight_number, airline, source, destination, "
        "departure_time, arrival_time, gate, status, fare "
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

        if (!manager.addFlightRecord(flight))
            return false;

        row = result.fetchOne();
    }

    return true;
}

// Load passenger records from MySQL into PassengerManager.
bool loadPassengersIntoManager(
    mysqlx::Session& session,
    PassengerManager& manager)
{
    auto result = session.sql(
        "SELECT passenger_id, name, age, gender, "
        "phone_number, flight_number, seat_number, "
        "assistance_type, status "
        "FROM passengers ORDER BY passenger_id"
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

        if (!manager.addPassengerRecord(passenger))
            return false;

        row = result.fetchOne();
    }

    return true;
}

// Convert a vector of strings into a JSON array.
std::string stringVectorToJson(
    const std::vector<std::string>& values)
{
    std::ostringstream out;
    out << "[";

    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
            out << ",";

        out << "\"" << jsonEscape(values[i]) << "\"";
    }

    out << "]";
    return out.str();
}


// Parse a JSON array containing strings.
std::vector<std::string> parseJsonStringArray(
    const std::string& json)
{
    std::vector<std::string> values;
    std::size_t i = 0;

    auto skipWhitespace = [&]()
    {
        while (i < json.size() &&
               std::isspace(static_cast<unsigned char>(json[i])))
        {
            ++i;
        }
    };

    skipWhitespace();

    if (i >= json.size() || json[i] != '[')
        return values;

    ++i;

    while (i < json.size())
    {
        skipWhitespace();

        if (i < json.size() && json[i] == ']')
            break;

        if (i >= json.size() || json[i] != '"')
            return {};

        ++i;

        std::string value;
        bool closed = false;

        while (i < json.size())
        {
            char ch = json[i++];

            if (ch == '"')
            {
                closed = true;
                break;
            }

            if (ch == '\\')
            {
                if (i >= json.size())
                    return {};

                char escaped = json[i++];

                switch (escaped)
                {
                    case '"':  value.push_back('"');  break;
                    case '\\': value.push_back('\\'); break;
                    case '/':  value.push_back('/');  break;
                    case 'b':  value.push_back('\b'); break;
                    case 'f':  value.push_back('\f'); break;
                    case 'n':  value.push_back('\n'); break;
                    case 'r':  value.push_back('\r'); break;
                    case 't':  value.push_back('\t'); break;

                    case 'u':
                    {
                        if (i + 4 > json.size())
                            return {};

                        unsigned int code = 0;

                        for (int j = 0; j < 4; ++j)
                        {
                            char hex = json[i++];

                            code *= 16;

                            if (hex >= '0' && hex <= '9')
                                code += hex - '0';
                            else if (hex >= 'a' && hex <= 'f')
                                code += hex - 'a' + 10;
                            else if (hex >= 'A' && hex <= 'F')
                                code += hex - 'A' + 10;
                            else
                                return {};
                        }

                        value.push_back(
                            code <= 0x7F
                                ? static_cast<char>(code)
                                : '?'
                        );

                        break;
                    }

                    default:
                        return {};
                }
            }
            else
            {
                value.push_back(ch);
            }
        }

        if (!closed)
            return {};

        values.push_back(value);

        skipWhitespace();

        if (i < json.size() && json[i] == ',')
        {
            ++i;
            continue;
        }

        if (i < json.size() && json[i] == ']')
            break;

        return {};
    }

    return values;
}


// Convert an emergency incident record into JSON.
std::string emergencyIncidentToJson(
    const EmergencyIncidentRecord& incident)
{
    std::ostringstream out;

    out << "{"
        << "\"caseId\":\"" << jsonEscape(incident.caseId) << "\","
        << "\"primaryPassengerId\":\""
        << jsonEscape(incident.primaryPassengerId) << "\","
        << "\"primaryPassengerName\":\""
        << jsonEscape(incident.primaryPassengerName) << "\","
        << "\"flightNumber\":\""
        << jsonEscape(incident.flightNumber) << "\","
        << "\"emergencyType\":\""
        << jsonEscape(incident.emergencyType) << "\","
        << "\"severity\":" << incident.severity << ","
        << "\"incidentLocation\":\""
        << jsonEscape(incident.incidentLocation) << "\","
        << "\"description\":\""
        << jsonEscape(incident.description) << "\","
        << "\"reportedAt\":\""
        << jsonEscape(incident.reportedAt) << "\","
        << "\"status\":\""
        << jsonEscape(incident.status) << "\","
        << "\"recommendedFacility\":\""
        << jsonEscape(incident.recommendedFacility) << "\","
        << "\"route\":" << stringVectorToJson(incident.route) << ","
        << "\"routeMinutes\":";

    if (incident.routeMinutes >= 0)
        out << incident.routeMinutes;
    else
        out << "null";

    out << ",\"routeMeters\":";

    if (incident.routeMeters >= 0)
        out << incident.routeMeters;
    else
        out << "null";

    out << ",\"routeMessage\":\""
        << jsonEscape(incident.routeMessage) << "\","
        << "\"affectedPassengers\":[";

    for (std::size_t i = 0;
         i < incident.affectedPassengers.size(); ++i)
    {
        if (i > 0)
            out << ",";

        const auto& passenger = incident.affectedPassengers[i];

        out << "{"
            << "\"passengerId\":\""
            << jsonEscape(passenger.passengerId) << "\","
            << "\"passengerName\":\""
            << jsonEscape(passenger.passengerName) << "\""
            << "}";
    }

    out << "]}";

    return out.str();
}


// Convert a notification record into JSON.
std::string notificationRecordToJson(
    const NotificationRecord& notification)
{
    std::ostringstream out;

    out << "{"
        << "\"notificationId\":" << notification.notificationId << ","
        << "\"passengerId\":\""
        << jsonEscape(notification.passengerId) << "\","
        << "\"passengerName\":\""
        << jsonEscape(notification.passengerName) << "\","
        << "\"flightNumber\":\""
        << jsonEscape(notification.flightNumber) << "\","
        << "\"type\":\""
        << jsonEscape(notification.type) << "\","
        << "\"message\":\""
        << jsonEscape(notification.message) << "\","
        << "\"isRead\":"
        << (notification.isRead ? "true" : "false") << ","
        << "\"createdAt\":\""
        << jsonEscape(notification.createdAt) << "\""
        << "}";

    return out.str();
}


// Load emergency incidents and affected passengers into the C++ manager.
bool loadEmergencyCasesIntoManager(
    mysqlx::Session& session,
    EmergencyAssistanceManager& manager)
{
    auto result = session.sql(
        "SELECT case_id, primary_passenger_id, "
        "primary_passenger_name, flight_number, emergency_type, "
        "severity, incident_location, description, "
        "DATE_FORMAT(reported_at, '%Y-%m-%dT%H:%i:%s.%f'), "
        "status, recommended_facility, "
        "COALESCE(CAST(route_json AS CHAR), '[]'), "
        "COALESCE(route_minutes, -1), "
        "COALESCE(route_meters, -1), route_message "
        "FROM emergency_cases "
        "ORDER BY severity ASC, reported_at ASC, case_id ASC"
    ).execute();

    // First collect the main incident records.
    // This avoids running another query while the first result is open.
    std::vector<EmergencyIncidentRecord> incidents;

    auto row = result.fetchOne();

    while (row)
    {
        EmergencyIncidentRecord incident;

        incident.caseId = row[0].get<std::string>();
        incident.primaryPassengerId = row[1].get<std::string>();
        incident.primaryPassengerName = row[2].get<std::string>();
        incident.flightNumber = row[3].get<std::string>();
        incident.emergencyType = row[4].get<std::string>();
        incident.severity = row[5].get<int>();
        incident.incidentLocation = row[6].get<std::string>();
        incident.description = row[7].get<std::string>();
        incident.reportedAt = row[8].get<std::string>();
        incident.status = row[9].get<std::string>();
        incident.recommendedFacility = row[10].get<std::string>();

        incident.route = parseJsonStringArray(
            row[11].get<std::string>()
        );

        incident.routeMinutes = row[12].get<long long>();
        incident.routeMeters = row[13].get<double>();
        incident.routeMessage = row[14].get<std::string>();

        incidents.push_back(incident);

        row = result.fetchOne();
    }

    // Load the affected passengers for every incident.
    for (auto& incident : incidents)
    {
        auto affectedResult = session.sql(
            "SELECT passenger_id, passenger_name "
            "FROM emergency_case_passengers "
            "WHERE case_id = ? "
            "ORDER BY is_primary DESC, created_at ASC, passenger_id ASC"
        ).bind(incident.caseId).execute();

        auto affectedRow = affectedResult.fetchOne();

        while (affectedRow)
        {
            EmergencyAffectedPassengerRecord passenger;

            passenger.passengerId =
                affectedRow[0].get<std::string>();

            passenger.passengerName =
                affectedRow[1].get<std::string>();

            incident.affectedPassengers.push_back(passenger);

            affectedRow = affectedResult.fetchOne();
        }

        std::string message;

        if (!manager.addIncidentRecord(incident, message))
            return false;
    }

    return true;
}


// Load notification records into the C++ linked-list manager.
bool loadNotificationsIntoManager(mysqlx::Session& session,
    NotificationManager& manager)
{
    auto result = session.sql(
        "SELECT notification_id, passenger_id, passenger_name, "
        "flight_number, notification_type, message, is_read, "
        "DATE_FORMAT(created_at, '%Y-%m-%dT%H:%i:%s.%f') "
        "FROM notifications "
        "ORDER BY created_at DESC, notification_id DESC"
    ).execute();

    auto row = result.fetchOne();

    while (row)
    {
        NotificationRecord notification;

        notification.notificationId =
            row[0].get<long long>();

        notification.passengerId =
            row[1].get<std::string>();

        notification.passengerName =
            row[2].get<std::string>();

        notification.flightNumber =
            row[3].get<std::string>();

        notification.type =
            row[4].get<std::string>();

        notification.message =
            row[5].get<std::string>();

        notification.isRead =
            row[6].get<int>() != 0;

        notification.createdAt =
            row[7].get<std::string>();

        std::string message;

        if (!manager.appendNotificationRecord(notification, message))
            return false;

        row = result.fetchOne();
    }

    return true;
}

// Send a JSON error response for Airport Navigator endpoints.
void setNavigatorErrorResponse(
    httplib::Response& response,
    int status,
    const std::string& message)
{
    response.status = status;

    response.set_content(
        "{\"success\":false,\"message\":\"" +
        jsonEscape(message) + "\"}",
        "application/json"
    );
}


// Load one airport's graph from MySQL into AirportNavigator.
// The constructor creates a default map, so clear it before loading.
bool loadAirportGraphIntoManager(
    mysqlx::Session& session,
    const std::string& airportCode,
    AirportNavigator& navigator,
    int& airportId,
    std::string& message)
{
    auto airportResult = session.sql(
        "SELECT airport_id FROM airports WHERE airport_code = ?"
    ).bind(airportCode).execute();

    auto airportRow = airportResult.fetchOne();

    if (!airportRow)
    {
        message = "Airport code was not found.";
        return false;
    }

    airportId = airportRow[0].get<int>();

    navigator.clearMap();

    // Load all vertices of the selected airport.
    auto locationResult = session.sql(
        "SELECT location_name "
        "FROM airport_locations "
        "WHERE airport_id = ? "
        "ORDER BY location_id"
    ).bind(airportId).execute();

    auto locationRow = locationResult.fetchOne();
    std::size_t locationCount = 0;

    while (locationRow)
    {
        const std::string locationName =
            locationRow[0].get<std::string>();

        if (!navigator.addLocation(locationName, message))
            return false;

        ++locationCount;
        locationRow = locationResult.fetchOne();
    }

    if (locationCount == 0)
    {
        message = "No locations are configured for this airport.";
        return false;
    }

    // Load the undirected edges and their walking-time weights.
    auto pathResult = session.sql(
        "SELECT f.location_name, t.location_name, p.walking_minutes "
        "FROM airport_paths p "
        "JOIN airport_locations f "
        "ON f.location_id = p.from_location_id "
        "JOIN airport_locations t "
        "ON t.location_id = p.to_location_id "
        "WHERE p.airport_id = ? "
        "ORDER BY p.path_id"
    ).bind(airportId).execute();

    auto pathRow = pathResult.fetchOne();

    while (pathRow)
    {
        const std::string from =
            pathRow[0].get<std::string>();

        const std::string to =
            pathRow[1].get<std::string>();

        const int walkingMinutes =
            pathRow[2].get<int>();

        if (!navigator.addPath(
                from, to, walkingMinutes, message))
        {
            return false;
        }

        pathRow = pathResult.fetchOne();
    }

    message = "Airport graph loaded successfully.";
    return true;
}


// Convert a string vector into a JSON array.
std::string navigatorStringArrayToJson(
    const std::vector<std::string>& values)
{
    std::ostringstream out;
    out << "[";

    for (std::size_t i = 0; i < values.size(); ++i)
    {
        if (i > 0)
            out << ",";

        out << "\"" << jsonEscape(values[i]) << "\"";
    }

    out << "]";
    return out.str();
}


// Obtain the stored distance in metres for one graph edge.
// The path can be stored in either endpoint order.
int getAirportSegmentDistanceMeters(
    mysqlx::Session& session,
    int airportId,
    const std::string& from,
    const std::string& to)
{
    auto result = session.sql(
        "SELECT p.distance_meters "
        "FROM airport_paths p "
        "JOIN airport_locations f "
        "ON f.location_id = p.from_location_id "
        "JOIN airport_locations t "
        "ON t.location_id = p.to_location_id "
        "WHERE p.airport_id = ? "
        "AND ("
        " (f.location_name = ? AND t.location_name = ?) "
        " OR "
        " (f.location_name = ? AND t.location_name = ?) "
        ") "
        "LIMIT 1"
    ).bind(
        airportId,
        from,
        to,
        to,
        from
    ).execute();

    auto row = result.fetchOne();

    if (!row)
    {
        throw std::runtime_error(
            "Could not find the distance for a calculated route segment."
        );
    }

    return row[0].get<int>();
}


// Build the selected airport's complete graph response.
// Coordinates and distances are read from MySQL, not hardcoded.
std::string buildAirportGraphJson(
    mysqlx::Session& session,
    int airportId)
{
    auto airportResult = session.sql(
        "SELECT airport_code, airport_name, city "
        "FROM airports WHERE airport_id = ?"
    ).bind(airportId).execute();

    auto airportRow = airportResult.fetchOne();

    if (!airportRow)
        throw std::runtime_error("Airport details could not be loaded.");

    const std::string airportCode =
        airportRow[0].get<std::string>();

    const std::string airportName =
        airportRow[1].get<std::string>();

    const std::string city =
        airportRow[2].get<std::string>();

    std::ostringstream body;

    body << "{\"airport\":{"
         << "\"airportCode\":\"" << jsonEscape(airportCode) << "\","
         << "\"airportName\":\"" << jsonEscape(airportName) << "\","
         << "\"city\":\"" << jsonEscape(city) << "\""
         << "},\"locations\":[";

    // Locations and map coordinates.
    auto locationResult = session.sql(
        "SELECT location_name, location_type, map_x, map_y "
        "FROM airport_locations "
        "WHERE airport_id = ? "
        "ORDER BY location_id"
    ).bind(airportId).execute();

    auto locationRow = locationResult.fetchOne();
    bool first = true;

    while (locationRow)
    {
        if (!first)
            body << ",";

        first = false;

        body << "{"
             << "\"name\":\""
             << jsonEscape(locationRow[0].get<std::string>())
             << "\","
             << "\"type\":\""
             << jsonEscape(locationRow[1].get<std::string>())
             << "\","
             << "\"x\":" << locationRow[2].get<int>() << ","
             << "\"y\":" << locationRow[3].get<int>()
             << "}";

        locationRow = locationResult.fetchOne();
    }

    body << "],\"edges\":[";

    // Edges, walking times and illustrative distances.
    auto pathResult = session.sql(
        "SELECT f.location_name, t.location_name, "
        "p.walking_minutes, p.distance_meters "
        "FROM airport_paths p "
        "JOIN airport_locations f "
        "ON f.location_id = p.from_location_id "
        "JOIN airport_locations t "
        "ON t.location_id = p.to_location_id "
        "WHERE p.airport_id = ? "
        "ORDER BY p.path_id"
    ).bind(airportId).execute();

    auto pathRow = pathResult.fetchOne();
    first = true;

    while (pathRow)
    {
        if (!first)
            body << ",";

        first = false;

        body << "{"
             << "\"from\":\""
             << jsonEscape(pathRow[0].get<std::string>())
             << "\","
             << "\"to\":\""
             << jsonEscape(pathRow[1].get<std::string>())
             << "\","
             << "\"walkingMinutes\":"
             << pathRow[2].get<int>() << ","
             << "\"distanceMeters\":"
             << pathRow[3].get<int>()
             << "}";

        pathRow = pathResult.fetchOne();
    }

    body << "]}";

    return body.str();
}
// Convert a GroupTravelManager record into JSON.
std::string travelGroupRecordToJson(
    const TravelGroupRecord& record)
{
    std::ostringstream body;

    body << "{"
         << "\"groupId\":\""
         << jsonEscape(record.groupId) << "\","
         << "\"groupName\":\""
         << jsonEscape(record.groupName) << "\","
         << "\"flightNumber\":\""
         << jsonEscape(record.flightNumber) << "\","
         << "\"members\":[";

    for (std::size_t i = 0; i < record.passengerIds.size(); ++i)
    {
        if (i > 0)
            body << ",";

        body << "\""
             << jsonEscape(record.passengerIds[i])
             << "\"";
    }

    body << "],"
         << "\"memberCount\":"
         << record.passengerIds.size()
         << "}";

    return body.str();
}






int main()
{
    httplib::Server server;

        // ================================================
    // GLOBAL CORS PREFLIGHT HANDLER
    // ================================================

    std::function<void(
        const httplib::Request&,
        httplib::Response&
    )> corsOptionsHandler =
        [](
            const httplib::Request& request,
            httplib::Response& response)
    {
        setCorsHeaders(response);
        response.status = 204;
    };

    server.Options(R"(/api/.*)", corsOptionsHandler);
    registerAuthRoutes(server);
    registerHealthRoutes(server);
    registerFlightRoutes(server);
    registerPassengerRoutes(server);
    registerCheckInRoutes(server);
    registerBoardingRoutes(server);
    registerPriorityRoutes(server);
    registerEmergencyRoutes(server);
    registerSmartFlightRoutes(server);
    registerNavigationRoutes(server);
    registerGroupTravelRoutes(server);
    registerBaggageRoutes(server);
    registerNotificationRoutes(server);
    registerAeroAssistRoutes(server);

// 1. API health check.
server.Get(
        "/api/health",
        [](const httplib::Request&,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            response.set_content(
                R"({"status":"ok","service":"AeroStruct API"})",
                "application/json"
            );
        }
    );

    // 2. MySQL connection health check.
    server.Get(
        "/api/db-health",
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

                auto result =
                    session.sql("SELECT VERSION()").execute();

                auto row = result.fetchOne();

                if (!row)
                {
                    response.status = 503;
                    response.set_content(
                        R"({"status":"error","message":"Database query returned no result"})",
                        "application/json"
                    );
                    return;
                }

                std::ostringstream version;
                version << row[0];

                std::string body =
                    "{\"status\":\"ok\","
                    "\"database\":\"connected\","
                    "\"mysqlVersion\":\"" +
                    version.str() + "\"}";

                response.set_content(body, "application/json");
            }
            catch (const std::exception& error)
            {
                std::cerr
                    << "MySQL health check failed: "
                    << error.what() << '\n';

                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Database connection failed; check the server console"})",
                    "application/json"
                );
            }
        }
    );


    std::cout << "AeroStruct API is starting...\n";
    std::cout << "Health: http://127.0.0.1:8080/api/health\n";
    std::cout << "Database: http://127.0.0.1:8080/api/db-health\n";
    std::cout << "Flights: http://127.0.0.1:8080/api/flights\n";

    if (!server.listen("127.0.0.1", 8080))
    {
        std::cerr << "Could not start the API server.\n";
        return 1;
    }

    return 0;
}
