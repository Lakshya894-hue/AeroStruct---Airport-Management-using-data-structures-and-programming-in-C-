#ifndef EMERGENCYASSISTANCEMANAGER_H
#define EMERGENCYASSISTANCEMANAGER_H

#include <cstddef>
#include <string>
#include <vector>

class PassengerManager;
class FlightManager;
class AirportNavigator;

// API-friendly representation of an affected passenger.
struct EmergencyAffectedPassengerRecord
{
    std::string passengerId;
    std::string passengerName;
};

// API-friendly representation of an emergency incident.
// This does not replace the internal linked-list node.
struct EmergencyIncidentRecord
{
    std::string caseId;
    std::string primaryPassengerId;
    std::string primaryPassengerName;
    std::string flightNumber;

    std::string emergencyType;
    int severity = 2;

    std::string incidentLocation;
    std::string description;
    std::string reportedAt;
    std::string status;

    std::string recommendedFacility;
    std::vector<std::string> route;

    long long routeMinutes = -1;
    double routeMeters = -1.0;

    std::vector<EmergencyAffectedPassengerRecord>
        affectedPassengers;

    std::string routeMessage;
};

// Console-based emergency incident register for the AeroStruct demonstration.
// Lower severity numbers represent more urgent cases (1 = Critical).
// This is a simulation, not a replacement for real emergency response services.
class EmergencyAssistanceManager
{
private:
    struct AffectedPassenger
    {
        std::string passengerId;
        std::string passengerName;
    };

    struct IncidentNode
    {
        std::string caseId;
        std::string primaryPassengerId;
        std::string primaryPassengerName;
        std::string flightNumber;
        std::string emergencyType;
        std::string incidentLocation;
        std::string description;
        std::string reportedAt;
        std::string status;
        std::string recommendedFacility;
        std::vector<std::string> route;
        long long routeMinutes = -1;
        double routeMeters = -1.0;

        std::string routeMessage = "Find a suitable facility route when needed.";

        int severity = 2;
        std::vector<AffectedPassenger> affectedPassengers;
        IncidentNode* next = nullptr;
    };

    IncidentNode* head;
    std::size_t incidentCount;
    unsigned long nextCaseNumber;

    static std::string trim(const std::string& value);
    static std::string normalize(const std::string& value);
    static std::string currentTimestamp();
    static std::string severityName(int severity);
    static std::string statusName(const std::string& status);
    static bool isUnavailableFlightStatus(const std::string& status);
    static bool containsIgnoreCase(const std::string& text,
                                   const std::string& part);
    static bool isSuitableFacility(const std::string& facility,
                                   const std::string& emergencyType);

    IncidentNode* findIncident(const std::string& caseId) const;
    bool findNearestFacility(const std::string& incidentLocation,
                             const std::string& emergencyType,
                             const AirportNavigator& navigator,
                             std::string& facility,
                             std::vector<std::string>& route,
                             long long& routeMinutes,
                             std::string& message) const;
    void displayIncident(const IncidentNode* incident,
                         PassengerManager& passengerManager,
                         FlightManager& flightManager,
                         bool includeRoute) const;

public:
    EmergencyAssistanceManager();
    ~EmergencyAssistanceManager();

    EmergencyAssistanceManager(const EmergencyAssistanceManager&) = delete;
    EmergencyAssistanceManager& operator=(const EmergencyAssistanceManager&) = delete;

    bool isEmpty() const;
    std::size_t getIncidentCount() const;

    // API-friendly emergency incident operations.
    bool addIncidentRecord(
    const EmergencyIncidentRecord& record,
    std::string& message
   );

    bool getIncidentAt(
    std::size_t index,
    EmergencyIncidentRecord& record
    ) const;

    bool updateIncidentStatusRecord(
    const std::string& caseId,
    const std::string& nextStatus,
    std::string& message
    );

    bool updateIncidentRouteRecord(
    const std::string& caseId,
    const std::string& facility,
    const std::vector<std::string>& route,
    long long routeMinutes,
    double routeMeters,
    const std::string& routeMessage,
    std::string& message
    );

    void registerEmergencyCase(PassengerManager& passengerManager,
                               FlightManager& flightManager,
                               AirportNavigator& navigator);
    void displayIncidentRecords(PassengerManager& passengerManager,
                                FlightManager& flightManager) const;
    void displayPriorityCases(PassengerManager& passengerManager,
                              FlightManager& flightManager) const;
    void searchIncident(PassengerManager& passengerManager,
                        FlightManager& flightManager) const;
    void updateIncidentStatus();
    void showFacilityRoute(AirportNavigator& navigator) const;
    void runMenu(PassengerManager& passengerManager,
                 FlightManager& flightManager,
                 AirportNavigator& navigator);
};

#endif
