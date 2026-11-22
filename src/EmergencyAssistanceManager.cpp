#include "EmergencyAssistanceManager.h"
#include "PassengerManager.h"
#include "FlightManager.h"
#include "AirportNavigator.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace
{
    bool readIntegerInRange(const string& prompt, int minimum, int maximum, int& value) 
    {
    //accepts a valid number
        while (true)
        {
            cout << prompt;
            if (cin >> value)
            {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                if (value >= minimum && value <= maximum)
                    return true;

                cout << "Enter a number from " << minimum << " to "
                     << maximum << ".\n";
            }
            else
            {
                if (cin.eof())
                    return false;

                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); //within the range
                cout << "Invalid input. Enter a whole number.\n";
            }
        }
    }

    bool readRequiredLine(const string& prompt, string& value, size_t maximumLength)
    {
        //reads passenger ID or incident description
        cout << prompt;
        if (!getline(cin >> ws, value))
            return false;

        if (value.size() > maximumLength)//if input exceeds max length
        {
            cout << "Input must not exceed " << maximumLength 
                 << " characters.\n";
            return false;
        }

        // Reject a value containing only whitespace.
        for (unsigned char ch : value)
        {
            if (!isspace(ch))
                return true;
        }

        cout << "This field is required.\n";
        return false;
    }

    string selectEmergencyType(int choice) //converts user numeric value into string
    {
        switch (choice)
        {
            case 1: return "Medical Emergency";
            case 2: return "Fire / Smoke";
            case 3: return "Security Threat";
            case 4: return "Missing / Separated Passenger";
            default: return "Other Emergency";
        }
    }

    string selectSeverityName(int severity)
    {
        switch (severity)
        {
            case 1: return "Critical";
            case 2: return "High";
            default: return "Moderate";
        }
    }
}

EmergencyAssistanceManager::EmergencyAssistanceManager()
    : head(nullptr), incidentCount(0), nextCaseNumber(1){}

EmergencyAssistanceManager::~EmergencyAssistanceManager()
{
    //free dynamically allocated node when object is destroyed
    while (head != nullptr)
    {
        IncidentNode* incident = head;
        head = head->next;
        delete incident;
    }

    incidentCount = 0;
}

bool EmergencyAssistanceManager::isEmpty() const
{
    return head == nullptr;
}

size_t EmergencyAssistanceManager::getIncidentCount() const
{
    return incidentCount;
}

bool EmergencyAssistanceManager::addIncidentRecord(
    const EmergencyIncidentRecord& record,
    std::string& message)
{
    const std::string caseId = trim(record.caseId);
    const std::string primaryId = trim(record.primaryPassengerId);
    const std::string primaryName = trim(record.primaryPassengerName);
    const std::string flightNumber = trim(record.flightNumber);
    const std::string emergencyType = trim(record.emergencyType);
    const std::string incidentLocation = trim(record.incidentLocation);
    const std::string description = trim(record.description);
    const std::string status = trim(record.status);

    // Validate mandatory fields.
    if (caseId.empty() || caseId.size() > 20 ||
        primaryId.empty() || primaryId.size() > 30 ||
        primaryName.empty() || primaryName.size() > 100 ||
        flightNumber.empty() || flightNumber.size() > 20 ||
        emergencyType.empty() || emergencyType.size() > 80 ||
        incidentLocation.empty() || incidentLocation.size() > 100 ||
        description.size() < 3 || description.size() > 300)
    {
        message = "One or more emergency fields are invalid.";
        return false;
    }

    if (record.severity < 1 || record.severity > 3)
    {
        message = "Severity must be between 1 and 3.";
        return false;
    }

    if (status != "Open" &&
        status != "In Progress" &&
        status != "Resolved")
    {
        message = "Invalid emergency status.";
        return false;
    }

    if (record.affectedPassengers.empty() ||
        record.affectedPassengers.size() > 21)
    {
        message = "An incident must contain 1 to 21 affected passengers.";
        return false;
    }

    if (record.routeMinutes < -1 || record.routeMeters < -1.0 ||
        record.recommendedFacility.size() > 150 ||
        record.routeMessage.size() > 255)
    {
        message = "Invalid facility route information.";
        return false;
    }

    if (record.route.size() > 100)
    {
        message = "Route contains too many locations.";
        return false;
    }

    for (const std::string& location : record.route)
    {
        if (trim(location).empty() || location.size() > 100)
        {
            message = "Invalid location in facility route.";
            return false;
        }
    }

    // Prevent duplicate case IDs.
    if (findIncident(caseId) != nullptr)
    {
        message = "Emergency case ID already exists.";
        return false;
    }

    // Validate affected passengers and preserve the primary passenger.
    std::vector<AffectedPassenger> affected;
    std::vector<std::string> seenIds;

    bool primaryFound = false;

    for (const auto& person : record.affectedPassengers)
    {
        const std::string passengerId = trim(person.passengerId);
        const std::string passengerName = trim(person.passengerName);

        if (passengerId.empty() || passengerId.size() > 30 ||
            passengerName.empty() || passengerName.size() > 100)
        {
            message = "Invalid affected passenger details.";
            return false;
        }

        const std::string key = normalize(passengerId);

        if (key == normalize(primaryId))
        {
            if (primaryFound)
            {
                message = "Primary passenger is duplicated.";
                return false;
            }

            primaryFound = true;
            continue;
        }

        if (std::find(seenIds.begin(), seenIds.end(), key)
            != seenIds.end())
        {
            message = "Duplicate affected passenger.";
            return false;
        }

        seenIds.push_back(key);
        affected.push_back({passengerId, passengerName});
    }

    if (!primaryFound)
    {
        message = "Primary passenger must be included in affected passengers.";
        return false;
    }

    // Always keep the primary passenger first.
    affected.insert(
        affected.begin(),
        {primaryId, primaryName}
    );

    // Construct the linked-list node.
    IncidentNode* incident = new IncidentNode{};

    incident->caseId = caseId;
    incident->primaryPassengerId = primaryId;
    incident->primaryPassengerName = primaryName;
    incident->flightNumber = flightNumber;
    incident->emergencyType = emergencyType;
    incident->incidentLocation = incidentLocation;
    incident->description = description;

    incident->reportedAt = record.reportedAt.empty()
        ? currentTimestamp()
        : trim(record.reportedAt);

    incident->status = status;
    incident->recommendedFacility =
        trim(record.recommendedFacility);

    incident->route = record.route;
    incident->routeMinutes = record.routeMinutes;
    incident->routeMeters = record.routeMeters;
    incident->severity = record.severity;

    incident->affectedPassengers = std::move(affected);

    incident->routeMessage = record.routeMessage.empty()
        ? "Find a suitable facility route when needed."
        : trim(record.routeMessage);

    // Insert in severity order.
    // Equal severity retains FIFO registration order.
    if (head == nullptr || incident->severity < head->severity)
    {
        incident->next = head;
        head = incident;
    }
    else
    {
        IncidentNode* current = head;

        while (current->next != nullptr &&
               current->next->severity <= incident->severity)
        {
            current = current->next;
        }

        incident->next = current->next;
        current->next = incident;
    }

    ++incidentCount;

    // Keep the console-generated case number synchronized.
    if (caseId.size() > 3 && normalize(caseId.substr(0, 3)) == "emr")
    {
        try
        {
            std::size_t parsedLength = 0;

            unsigned long parsedNumber =
                std::stoul(caseId.substr(3), &parsedLength);

            if (parsedLength == caseId.size() - 3 &&
                parsedNumber < std::numeric_limits<unsigned long>::max())
            {
                nextCaseNumber = std::max(
                    nextCaseNumber,
                    parsedNumber + 1
                );
            }
        }
        catch (const std::exception&)
        {
            // Ignore non-numeric case ID suffixes.
        }
    }

    message = "Emergency case registered successfully.";
    return true;
}
bool EmergencyAssistanceManager::getIncidentAt(
    std::size_t index,
    EmergencyIncidentRecord& record) const
{
    IncidentNode* current = head;

    std::size_t currentIndex = 0;

    while (current != nullptr && currentIndex < index)
    {
        current = current->next;
        ++currentIndex;
    }

    if (current == nullptr)
        return false;

    record = EmergencyIncidentRecord{};

    record.caseId = current->caseId;
    record.primaryPassengerId = current->primaryPassengerId;
    record.primaryPassengerName = current->primaryPassengerName;
    record.flightNumber = current->flightNumber;

    record.emergencyType = current->emergencyType;
    record.severity = current->severity;

    record.incidentLocation = current->incidentLocation;
    record.description = current->description;
    record.reportedAt = current->reportedAt;
    record.status = current->status;

    record.recommendedFacility = current->recommendedFacility;
    record.route = current->route;

    record.routeMinutes = current->routeMinutes;
    record.routeMeters = current->routeMeters;

    record.routeMessage = current->routeMessage;

    for (const auto& person : current->affectedPassengers)
    {
        record.affectedPassengers.push_back({
            person.passengerId,
            person.passengerName
        });
    }

    return true;
}

bool EmergencyAssistanceManager::updateIncidentStatusRecord(
    const std::string& caseId,
    const std::string& nextStatus,
    std::string& message)
{
    IncidentNode* incident = findIncident(trim(caseId));

    if (incident == nullptr)
    {
        message = "Emergency case not found.";
        return false;
    }

    const std::string status = normalize(trim(nextStatus));

    if (status != "open" &&
        status != "in progress" &&
        status != "resolved")
    {
        message = "Invalid emergency status.";
        return false;
    }

    if (normalize(incident->status) == "resolved")
    {
        message = "Resolved cases cannot be modified.";
        return false;
    }

    if (status == "open")
        incident->status = "Open";
    else if (status == "in progress")
        incident->status = "In Progress";
    else
        incident->status = "Resolved";

    message = "Emergency status updated successfully.";
    return true;
}

bool EmergencyAssistanceManager::updateIncidentRouteRecord(
    const std::string& caseId,
    const std::string& facility,
    const std::vector<std::string>& route,
    long long routeMinutes,
    double routeMeters,
    const std::string& routeMessage,
    std::string& message)
{
    IncidentNode* incident = findIncident(trim(caseId));

    if (incident == nullptr)
    {
        message = "Emergency case not found.";
        return false;
    }

    const std::string cleanFacility = trim(facility);
    const std::string cleanMessage = trim(routeMessage);

    if (cleanFacility.size() > 150 ||
        cleanMessage.size() > 255 ||
        routeMinutes < -1 ||
        routeMeters < -1.0 ||
        route.size() > 100)
    {
        message = "Invalid route information.";
        return false;
    }

    for (const std::string& location : route)
    {
        if (trim(location).empty() || location.size() > 100)
        {
            message = "Invalid route location.";
            return false;
        }
    }

    incident->recommendedFacility = cleanFacility;
    incident->route = route;
    incident->routeMinutes = routeMinutes;
    incident->routeMeters = routeMeters;

    incident->routeMessage = cleanMessage.empty()
        ? "Route information updated."
        : cleanMessage;

    message = "Emergency facility route updated successfully.";
    return true;
}

string EmergencyAssistanceManager::trim(const string& value) //removes trailing whitespaces
{
    size_t first = 0;
    while (first < value.size() && isspace(static_cast<unsigned char>(value[first])))
        ++first;

    size_t last = value.size();
    while (last > first && isspace(static_cast<unsigned char>(value[last - 1])))
        --last;

    return value.substr(first, last - first);
}

string EmergencyAssistanceManager::normalize(const string& value) //converts into lowercase
{
    string result = trim(value);
    for (char& ch : result)
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    return result;
}

string EmergencyAssistanceManager::currentTimestamp() //generate local time
{
    const time_t now = time(nullptr);
    const tm* local = localtime(&now); //converts into local time
    if (local == nullptr)
        return "Timestamp unavailable";

    char buffer[32] = {};
    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local) == 0)
        return "Timestamp unavailable";
    return buffer;
}

string EmergencyAssistanceManager::severityName(int severity)
{
    return selectSeverityName(severity);
}

string EmergencyAssistanceManager::statusName(const string& status)
{
    return status;
}

bool EmergencyAssistanceManager::isUnavailableFlightStatus(const string& status)
{
    const string normalized = normalize(status);
    return normalized == "cancelled" || normalized == "canceled" || normalized == "completed";
}

bool EmergencyAssistanceManager::containsIgnoreCase(const string& text, const string& part)
{
    return normalize(text).find(normalize(part)) != string::npos; //part of the string contains
}

bool EmergencyAssistanceManager::isSuitableFacility(const string& facility, const string& emergencyType)
{
    //check if location name matches the type of emergency
    const string type = normalize(emergencyType);
    const string name = normalize(facility);

    const bool medical = type.find("medical") != string::npos;
    const bool fire = type.find("fire") != string::npos ||
                      type.find("smoke") != string::npos;
    const bool security = type.find("security") != string::npos;
    const bool missing = type.find("missing") != string::npos ||
                         type.find("separated") != string::npos;

    if (medical)
        return name.find("medical") != string::npos ||
               name.find("clinic") != string::npos ||
               name.find("first aid") != string::npos ||
               name.find("health centre") != string::npos ||
               name.find("health center") != string::npos;

    if (fire)
        return name.find("fire station") != string::npos ||
               name.find("fire response") != string::npos ||
               name.find("emergency response") != string::npos ||
               name.find("emergency services") != string::npos;

    if (security)
        return name.find("security office") != string::npos ||
               name.find("security centre") != string::npos ||
               name.find("security center") != string::npos ||
               name.find("police") != string::npos ||
               name.find("emergency response") != string::npos;

    if (missing)
        return name.find("information desk") != string::npos ||
               name.find("security") != string::npos ||
               name.find("help desk") != string::npos;

    return name.find("emergency") != string::npos ||
           name.find("medical") != string::npos ||
           name.find("clinic") != string::npos ||
           name.find("first aid") != string::npos ||
           name.find("fire station") != string::npos ||
           name.find("security office") != string::npos ||
           name.find("police") != string::npos;
}

EmergencyAssistanceManager::IncidentNode* 
EmergencyAssistanceManager::findIncident(const string& caseId) const
{
    //find incident using case ID and return pointer to node
    IncidentNode* current = head;
    const string key = normalize(caseId);
    while (current != nullptr)
    {
        if (normalize(current->caseId) == key)
            return current;
        current = current->next;
    }
    return nullptr;
}

bool EmergencyAssistanceManager::findNearestFacility( const string& incidentLocation,
    const string& emergencyType,
    const AirportNavigator& navigator,
    string& facility,
    vector<string>& route,
    long long& routeMinutes,
    string& message) const        //find the reachable facility and lowest walikng time
{
    facility.clear(); //clers all previous input
    route.clear();
    routeMinutes = -1;

    const vector<string> locations = navigator.getLocations(); //get locations
    bool foundRoute = false;

    for (const string& candidate : locations)
    {
        if (!isSuitableFacility(candidate, emergencyType))
            continue;

        // If the incident is already at the facility, the route is zero minutes.
        if (normalize(incidentLocation) == normalize(candidate))
        {
            foundRoute = true;
            facility = candidate;
            route = {candidate};
            routeMinutes = 0;
            break;
        }

        const AirportNavigator::RouteResult result = navigator.findShortestRoute(incidentLocation, candidate);
        if (!result.success)
            continue;

        //calculate a shortest route to each suitable facility. It then compares those route times and selects the smallest reachable one.
        if (!foundRoute || result.totalMinutes < routeMinutes)
        {
            foundRoute = true;
            facility = candidate;
            route = result.path;
            routeMinutes = result.totalMinutes;
        }
    }

    if (foundRoute)
    {
        message = "Nearest reachable matching facility found.";
        return true;
    }

    message = "No reachable facility matching this emergency type is mapped. "
              "Add a suitable facility and paths in Airport Navigator.";
    return false;
}

void EmergencyAssistanceManager::registerEmergencyCase(PassengerManager& passengerManager,
    FlightManager& flightManager,
    AirportNavigator& navigator)
{
    cout << "\n========== Register Emergency Case ==========\n";

    string passengerId;
    if (!readRequiredLine("Primary passenger ID: ", passengerId, 30)) 
    {
        cout << "Emergency case not registered.\n";
        return;
    }

    string passengerName, gender, phone, passengerFlight, seat, assistance, passengerStatus;
    int age = 0;
    if (!passengerManager.getPassengerQueueDetails( passengerId, passengerName, age, gender, phone,
            passengerFlight, seat, assistance, passengerStatus))  //validates passenger
    {
        cout << "Passenger not found. Register the passenger first.\n";
        return;
    }

    string flightStatus;
    //validates if passenger has registered for that flight
    if (trim(passengerFlight).empty() || !flightManager.getFlightStatus(passengerFlight, flightStatus))
    {
        cout << "Emergency case not registered: passenger's flight was not found.\n";
        return;
    }
    //check for avalability of flights
    if (isUnavailableFlightStatus(flightStatus))
    {
        cout << "Emergency case not registered: passenger's flight is "
             << flightStatus << ".\n";
        return;
    }

    cout << "\nEmergency type:\n"
         << "1. Medical Emergency\n"
         << "2. Fire / Smoke\n"
         << "3. Security Threat\n"
         << "4. Missing / Separated Passenger\n"
         << "5. Other Emergency\n";
    int typeChoice = 0;
    if (!readIntegerInRange("Choose emergency type (1-5): ", 1, 5, typeChoice))
        return;

    cout << "\nSeverity level (selected by operator; demonstration only):\n"
         << "1. Critical - highest priority\n"
         << "2. High\n"
         << "3. Moderate\n";
    int severity = 0;
    if (!readIntegerInRange("Choose severity (1-3): ", 1, 3, severity))
        return;

    const vector<string> locations = navigator.getLocations();//get location of the facility
    if (locations.empty())
    {
        cout << "No airport locations are configured. Configure Airport Navigator first.\n";
        return;
    }

    cout << "\nIncident location:\n";
    for (size_t i = 0; i < locations.size(); ++i)
        cout << (i + 1) << ". " << locations[i] << '\n';

    int locationChoice = 0;
    if (!readIntegerInRange("Select incident location: ", 1,
                            static_cast<int>(locations.size()), locationChoice))
        return;
    const string incidentLocation = locations[static_cast<size_t>(locationChoice - 1)];

    string description;
    if (!readRequiredLine("Incident description (max 300 characters): ",description, 300))
    {
        cout << "Emergency case not registered.\n";
        return;
    }
    description = trim(description);
    if (description.empty())
    {
        cout << "Description cannot be blank. Emergency case not registered.\n";
        return;
    }

    int additionalCount = 0;
    if (!readIntegerInRange("Number of additional affected passengers (0-20): ",
                            0, 20, additionalCount))
        return;

    vector<AffectedPassenger> affected;
    affected.push_back({passengerId, passengerName});//if any affected passenger push back his name

    for (int i = 0; i < additionalCount; ++i)
    {
        bool accepted = false;
        while (!accepted)
        {
            string affectedId;
            if (!readRequiredLine("Affected passenger ID: ", affectedId, 30))
            {
                cout << "Emergency case not registered.\n";
                return;
            }
            //check for duplicate 
            bool duplicate = false;
            for (const AffectedPassenger& previous : affected)
            {
                if (normalize(previous.passengerId) == normalize(affectedId))
                {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate)
            {
                cout << "That passenger is already listed for this case. Enter another ID.\n";
                continue;
            }

            string name, affectedFlight, affectedGender, affectedPhone;
            string affectedSeat, affectedAssistance, affectedStatus;
            int affectedAge = 0;
            if (!passengerManager.getPassengerQueueDetails(
                    affectedId, name, affectedAge, affectedGender, affectedPhone,
                    affectedFlight, affectedSeat, affectedAssistance, affectedStatus))
            {
                cout << "Passenger not found. Register them first, then try again.\n";
                continue;
            }

            affected.push_back({affectedId, name});//checks for other affected
            accepted = true;
        }
    }

    string facility, routeMessage;
    vector<string> route;
    long long routeMinutes = -1;
    const string emergencyType = selectEmergencyType(typeChoice);
    const bool hasRoute = findNearestFacility(
        incidentLocation, emergencyType, navigator, facility, route,
        routeMinutes, routeMessage);

    ostringstream idBuilder;
    idBuilder << "EMR" << setfill('0') << setw(6) << nextCaseNumber; // empty pos with 0,  set width of 6 e.g. EMR000001

    IncidentNode* incident = new IncidentNode{}; //Cretes DMA Incident Node
    incident->caseId = idBuilder.str();
    incident->primaryPassengerId = passengerId;
    incident->primaryPassengerName = passengerName;
    incident->flightNumber = passengerFlight;
    incident->emergencyType = emergencyType;
    incident->incidentLocation = incidentLocation;
    incident->description = description;
    incident->reportedAt = currentTimestamp();
    incident->status = "Open";
    incident->recommendedFacility = hasRoute ? facility : "No suitable mapped facility";
    incident->route = route;
    incident->routeMinutes = routeMinutes;
    incident->severity = severity;
    incident->affectedPassengers = std::move(affected);

    // Insert by severity (1 is most urgent); new cases of equal severity stay FIFO.
    if (head == nullptr || severity < head->severity)
    {
        incident->next = head;
        head = incident;
    }
    else
    {
        IncidentNode* current = head;
        while (current->next != nullptr && current->next->severity <= severity)
            current = current->next;
        incident->next = current->next;
        current->next = incident;
    }

    ++incidentCount;
    ++nextCaseNumber;

    cout << "\nEmergency incident registered successfully.\n";
    cout << "Case ID       : " << incident->caseId << '\n';
    cout << "Primary person: " << passengerName << " (" << passengerId << ")\n";
    cout << "Flight        : " << passengerFlight << '\n';
    cout << "Emergency     : " << emergencyType << '\n';
    cout << "Severity      : " << severityName(severity) << " (priority " << severity << ")\n";
    cout << "Status        : " << incident->status << '\n';
    cout << "Reported at   : " << incident->reportedAt << '\n';
    cout << "Affected people: " << incident->affectedPassengers.size() << '\n';

    if (hasRoute)
    {
        cout << "Nearest facility: " << facility << '\n';
        cout << "Estimated route: ";
        for (size_t i = 0; i < route.size(); ++i)
        {
            if (i > 0) cout << " -> ";
            cout << route[i];
        }
        cout << "\nIllustrative walking time: " << routeMinutes << " minute(s)\n";
    }
    else
    {
        cout << "Route           : " << routeMessage << '\n';
    }

    //cout << "Note: This is a local simulation. Contact real emergency staff for actual incidents.\n";
}

void EmergencyAssistanceManager::displayIncident(const IncidentNode* incident,
    PassengerManager& passengerManager,
    FlightManager& flightManager,
    bool includeRoute) const
{
    if (incident == nullptr)
        return;

    string currentFlightStatus;
    const bool flightFound = flightManager.getFlightStatus(incident->flightNumber, currentFlightStatus);

    cout << "\n==================================================\n";
    cout << "Case ID          : " << incident->caseId << '\n';
    cout << "Primary Passenger: " << incident->primaryPassengerName << " ("
         << incident->primaryPassengerId << ")\n";
    cout << "Flight Number    : " << incident->flightNumber << '\n';
    cout << "Current Flight   : "
         << (flightFound ? currentFlightStatus : "Flight Not Found") << '\n';
    cout << "Emergency Type   : " << incident->emergencyType << '\n';
    cout << "Severity         : " << severityName(incident->severity)
         << " (priority " << incident->severity << ")\n";
    cout << "Location         : " << incident->incidentLocation << '\n';
    cout << "Description      : " << incident->description << '\n';
    cout << "Reported At      : " << incident->reportedAt << '\n';
    cout << "Case Status      : " << statusName(incident->status) << '\n';
    cout << "Affected Passengers:\n";

    for (const AffectedPassenger& affected : incident->affectedPassengers)
    {
        cout << "  - " << affected.passengerName << " ("
             << affected.passengerId << ")";

        // Confirm whether the referenced passenger record still exists.
        string name, gender, phone, flight, seat, assistance, status;
        int age = 0;
        if (!passengerManager.getPassengerQueueDetails(affected.passengerId, name, age, gender, phone,
                flight, seat, assistance, status))
            cout << " [record no longer exists]";
        cout << '\n';
    }

    cout << "Recommended Facility: " << incident->recommendedFacility << '\n';
    if (includeRoute && !incident->route.empty())
    {
        cout << "Route               : ";
        for (size_t i = 0; i < incident->route.size(); ++i)
        {
            if (i > 0) cout << " -> ";
            cout << incident->route[i];
        }
        cout << "\nWalking Time        : " << incident->routeMinutes
             << " illustrative minute(s)\n";
    }
    else if (includeRoute)
    {
        cout << "Route               : No stored route is available.\n";
    }
}

void EmergencyAssistanceManager::displayIncidentRecords(PassengerManager& passengerManager,
    FlightManager& flightManager) const
{
    if (isEmpty())
    {
        cout << "\nNo emergency incident records are available.\n";
        return;
    }

    cout << "\n========== Emergency Incident Records (priority order) ==========\n";
    IncidentNode* current = head;
    while (current != nullptr)
    {
        displayIncident(current, passengerManager, flightManager, true); //priority order
        current = current->next;
    }
    cout << "\nTotal incident records: " << incidentCount << '\n';
}

void EmergencyAssistanceManager::displayPriorityCases(
    PassengerManager& passengerManager,
    FlightManager& flightManager) const
{
    if (isEmpty())
    {
        cout << "\nEmergency priority list is empty.\n";
        return;
    }

    bool foundActive = false;
    cout << "\n========== Active Emergency Cases (highest priority first) ==========\n";
    IncidentNode* current = head;
    while (current != nullptr)
    {
        if (normalize(current->status) != "resolved")
        {
            foundActive = true;
            displayIncident(current, passengerManager, flightManager, false);
        }
        current = current->next;
    }

    if (!foundActive)
        cout << "No unresolved emergency cases.\n";
}

void EmergencyAssistanceManager::searchIncident(PassengerManager& passengerManager,
    FlightManager& flightManager) const
{
    string caseId;
    if (!readRequiredLine("Enter emergency case ID (e.g. EMR000001): ", caseId, 20))
        return;

    const IncidentNode* incident = findIncident(caseId);
    if (incident == nullptr)
    {
        cout << "Emergency case was not found.\n";
        return;
    }

    displayIncident(incident, passengerManager, flightManager, true);
}

void EmergencyAssistanceManager::updateIncidentStatus()
{
    string caseId;
    if (!readRequiredLine("Enter case ID: ", caseId, 20))
        return;

    IncidentNode* incident = findIncident(caseId);
    if (incident == nullptr)
    {
        cout << "Emergency case was not found.\n";
        return;
    }

    if (normalize(incident->status) == "resolved")
    {
        cout << "This case is already resolved; its status cannot be changed.\n";
        return;
    }

    cout << "Current status: " << incident->status << '\n'
         << "1. In Progress\n"
         << "2. Resolved\n";
    int choice = 0;
    if (!readIntegerInRange("Choose new status (1-2): ", 1, 2, choice))
        return;

    incident->status = (choice == 1) ? "In Progress" : "Resolved";
    cout << "Case " << incident->caseId << " updated to: "<< incident->status << '\n';
}

void EmergencyAssistanceManager::showFacilityRoute(AirportNavigator& navigator) const
{
    string caseId;
    if (!readRequiredLine("Enter case ID: ", caseId, 20))
        return;

    const IncidentNode* incident = findIncident(caseId);
    if (incident == nullptr)
    {
        cout << "Emergency case was not found.\n";
        return;
    }

    string facility, routeMessage;
    vector<string> route;
    long long minutes = -1;
    if (!findNearestFacility(incident->incidentLocation, incident->emergencyType,
                             navigator, facility, route, minutes, routeMessage))
    {
        cout << routeMessage << '\n';
        return;
    }

    cout << "\nNearest matching facility: " << facility << '\n';
    cout << "Route: ";
    for (size_t i = 0; i < route.size(); ++i)
    {
        if (i > 0) cout << " -> ";
        cout << route[i];
    }
    cout << "\nIllustrative walking time: " << minutes << " minute(s)\n";
}

void EmergencyAssistanceManager::runMenu(
    PassengerManager& passengerManager,
    FlightManager& flightManager,
    AirportNavigator& navigator)
{
    int choice = -1;
    do
    {
        cout << "\n========== Emergency Assistance System ==========\n"
             << "1. Register Emergency Case\n"
             << "2. Display Incident Records\n"
             << "3. Display Active Cases by Priority\n"
             << "4. Search Incident by Case ID\n"
             << "5. Update Incident Status\n"
             << "6. Find Nearest Suitable Facility / Route\n"
             << "0. Back to Main Menu\n";

        if (!readIntegerInRange("Enter choice (0-6): ", 0, 6, choice))
            return;

        switch (choice)
        {
            case 1:
                registerEmergencyCase(passengerManager, flightManager, navigator);
                break;
            case 2:
                displayIncidentRecords(passengerManager, flightManager);
                break;
            case 3:
                displayPriorityCases(passengerManager, flightManager);
                break;
            case 4:
                searchIncident(passengerManager, flightManager);
                break;
            case 5:
                updateIncidentStatus();
                break;
            case 6:
                showFacilityRoute(navigator);
                break;
            case 0:
                cout << "Returning to the main menu...\n";
                break;
            default:
                // readIntegerInRange prevents this branch for numeric input.
                cout << "Invalid menu choice.\n";
        }
    } while (choice != 0);
}
