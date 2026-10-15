#include "SmartFlightSuggestion.h"

#include <algorithm>
#include <cctype>

namespace
{
    // Convert text to lowercase for case-insensitive comparison.
    std::string normalize(std::string value)
    {
        std::transform(value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch) 
            {
                return static_cast<char>(std::tolower(ch)); //lambda function(inline function)
            });

        return value;
    }

    // Check whether the flight is unavailable.
    bool isUnavailable(const std::string& status)
    {
        const std::string value = normalize(status);

        return value == "cancelled" ||
               value == "canceled" ||
               value == "completed";
    }

    // Convert HH:MM into minutes after midnight.
    bool parseTime(const std::string& time, int& minutes)
    {
        if (time.size() != 5 || time[2] != ':') //accepts in 24 hours format
        {
            return false;
        }

        if (!std::isdigit(static_cast<unsigned char>(time[0])) ||
            !std::isdigit(static_cast<unsigned char>(time[1])) ||
            !std::isdigit(static_cast<unsigned char>(time[3])) ||
            !std::isdigit(static_cast<unsigned char>(time[4])))
        {
            return false; //check for digits
        }

        int hours =
            (time[0] - '0') * 10 + (time[1] - '0');

        int mins =
            (time[3] - '0') * 10 + (time[4] - '0');

        if (hours > 23 || mins > 59)
        {
            return false;
        }

        minutes = hours * 60 + mins;
        return true;
    }

    // Calculate journey duration, including overnight flights.
    bool calculateDuration(const std::string& departure,const std::string& arrival,
        int& duration)
        {
        int departureMinutes;
        int arrivalMinutes;

        if (!parseTime(departure, departureMinutes) ||
            !parseTime(arrival, arrivalMinutes))
        {
            return false; //invalid time format
        }

        duration = arrivalMinutes - departureMinutes;

        // Arrival is on the following day.
        if (duration < 0)
        {
            duration += 24 * 60;
        }

        return true;
    }
}


// Find and rank alternative flights.
std::vector<SmartFlightOption>SmartFlightSuggestion::findAlternatives(Flight& selectedFlight,Flight* flights,
    int flightCount)
{
    std::vector<SmartFlightOption> options;

    if (flights == nullptr || flightCount <= 0)
    {
        return options;
    }

    // Store details of the selected flight.
    const std::string selectedNumber =
        normalize(selectedFlight.getFlightNumber());

    const std::string selectedSource =
        normalize(selectedFlight.getSource());

    const std::string selectedDestination =
        normalize(selectedFlight.getDestination());


    // Step 1: Find valid alternatives on the same route.
    for (int i = 0; i < flightCount; ++i)
    {
        const std::string number =
            flights[i].getFlightNumber();

        const std::string status =
            flights[i].getStatus();

        // Exclude the selected flight and unavailable flights.
        if (normalize(number) == selectedNumber || isUnavailable(status))
        {
            continue;
        }

        // Both source and destination must match.
        const bool sameSource =
            normalize(flights[i].getSource()) == selectedSource;

        const bool sameDestination =
            normalize(flights[i].getDestination()) ==
            selectedDestination;

        if (!sameSource || !sameDestination)
        {
            continue;
        }

        // Calculate duration using departure and arrival times.
        int duration;

        if (!calculateDuration(flights[i].getDepartureTime(),flights[i].getArrivalTime(),duration))
        {
            // Ignore flights with invalid time formats.
            continue;
        }

        // Store the matching alternative.
        options.push_back({
            number,
            flights[i].getAirline(),
            flights[i].getSource(),
            flights[i].getDestination(),
            flights[i].getDepartureTime(),
            flights[i].getArrivalTime(),
            status,
            flights[i].getFare(),
            duration,
            0.0
        });
    }

    if (options.empty())
    {
        return options;
    }


    // Step 2: Find minimum and maximum fare and duration.
    double minFare = options[0].fare;
    double maxFare = options[0].fare;

    int minDuration = options[0].durationMinutes;
    int maxDuration = options[0].durationMinutes;

    for (const SmartFlightOption& option : options)
    {
        minFare = std::min(minFare, option.fare);
        maxFare = std::max(maxFare, option.fare);

        minDuration = std::min(
            minDuration, option.durationMinutes);

        maxDuration = std::max(
            maxDuration, option.durationMinutes);
    }


    // Step 3: Calculate a combined recommendation score.
    for (SmartFlightOption& option : options)
    {
        double fareScore = 1.0;
        double durationScore = 1.0;

        // Lower fare receives a higher score.
        if (maxFare > minFare)
        {
            fareScore =
                (maxFare - option.fare) /
                (maxFare - minFare);
        }

        // Shorter journey receives a higher score.
        if (maxDuration > minDuration)
        {
            durationScore =
                static_cast<double>(
                    maxDuration - option.durationMinutes) /
                (maxDuration - minDuration);
        }

        // Both factors receive equal weight: 50% each.
        option.recommendationScore =
            (fareScore * 0.5 + durationScore * 0.5) * 100.0;
    }


    // Step 4: Rank the best-scoring alternatives first.
    std::sort(options.begin(),options.end(), 
        [](const SmartFlightOption& a,
           const SmartFlightOption& b)
        {
            if (a.recommendationScore != b.recommendationScore)
            {
                return a.recommendationScore >
                       b.recommendationScore;
            }

            // Tie-breaker 1: lower fare.
            if (a.fare != b.fare)
            {
                return a.fare < b.fare;
            }

            // Tie-breaker 2: shorter journey.
            if (a.durationMinutes != b.durationMinutes)
            {
                return a.durationMinutes < b.durationMinutes;
            }

            // Tie-breaker 3: earlier departure.
            return a.departureTime < b.departureTime;
        });

    return options;
}
