#ifndef SMART_FLIGHT_SUGGESTION_H
#define SMART_FLIGHT_SUGGESTION_H

#include "Flight.h"
#include <string>
#include <vector>

struct SmartFlightOption
{
    std::string flightNumber;
    std::string airline;
    std::string source;
    std::string destination;
    std::string departureTime;
    std::string arrivalTime;
    std::string status;
};

class SmartFlightSuggestion
{
public:
    // Finds other non-cancelled flights operating on the same route.
    // Flight times should use 24-hour HH:MM format for correct sorting.
    static std::vector<SmartFlightOption> findAlternatives(
        Flight& selectedFlight, Flight* flights, int flightCount);
};

#endif