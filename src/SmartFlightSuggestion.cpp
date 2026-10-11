#include "SmartFlightSuggestion.h"
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace
{
    std::string normalize(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
        return value;
    }

    bool isUnavailable(const std::string& status)
    {
        const std::string value = normalize(status);
        return value == "cancelled" || value == "canceled" ||
               value == "completed";
    }
}

std::vector<SmartFlightOption> SmartFlightSuggestion::findAlternatives(
    Flight& selectedFlight, Flight* flights, int flightCount)
{
    std::vector<SmartFlightOption> options;

    if (flights == nullptr || flightCount <= 0)
    {
        return options;
    }

    const std::string selectedNumber = normalize(selectedFlight.getFlightNumber());
    const std::string selectedSource = normalize(selectedFlight.getSource());
    const std::string selectedDestination = normalize(selectedFlight.getDestination());

    for (int i = 0; i < flightCount; ++i)
    {
        const std::string number = flights[i].getFlightNumber();
        const std::string status = flights[i].getStatus();

        // Do not suggest the same flight or a flight that is no longer usable.
        if (normalize(number) == selectedNumber || isUnavailable(status))
        {
            continue;
        }

        const bool sameSource = normalize(flights[i].getSource()) == selectedSource;
        const bool sameDestination = normalize(flights[i].getDestination()) == selectedDestination;

        if (sameSource && sameDestination)
        {
            options.push_back({
                number,
                flights[i].getAirline(),
                flights[i].getSource(),
                flights[i].getDestination(),
                flights[i].getDepartureTime(),
                flights[i].getArrivalTime(),
                status
            });
        }
    }

    // Sort the alternatives by departure time (HH:MM format).
    std::sort(options.begin(), options.end(),
        [](const SmartFlightOption& a, const SmartFlightOption& b)
        {
            return a.departureTime < b.departureTime;
        });

    return options;
}