#include "SmartFlightRoutes.h"

#include "Flight.h"
#include "FlightManager.h"
#include "SmartFlightSuggestion.h"
#include "httplib.h"

#include <mysqlx/xdevapi.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// Existing helper functions implemented in api_main.cpp.
const char* getDbUser();
const char* getDbPassword();

bool databaseCredentialsAvailable();

void setCorsHeaders(httplib::Response& response);

std::string trim(const std::string& value);
std::string toUpper(std::string value);
std::string jsonEscape(const std::string& value);

bool loadFlightsIntoManager(
    mysqlx::Session& session,
    FlightManager& manager
);

void registerSmartFlightRoutes(httplib::Server& server)
{
    server.Get(
        "/api/flights/suggestions",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            if (!request.has_param("flightNumber"))
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Flight number is required"})",
                    "application/json"
                );
                return;
            }

            std::string flightNumber =
                toUpper(trim(request.get_param_value("flightNumber")));

            if (flightNumber.empty() || flightNumber.size() > 20)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid flight number"})",
                    "application/json"
                );
                return;
            }

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

                FlightManager flightManager(10000);

                if (!loadFlightsIntoManager(session, flightManager))
                {
                    throw std::runtime_error(
                        "Could not load flights into FlightManager"
                    );
                }

                // Find the selected flight.
                Flight selectedFlight;

                if (!flightManager.findFlightRecord(
                        flightNumber, selectedFlight))
                {
                    response.status = 404;
                    response.set_content(
                        R"({"status":"error","message":"Selected flight not found"})",
                        "application/json"
                    );
                    return;
                }

                // Retrieve every flight from FlightManager.
                const int flightCount =
                    flightManager.getFlightCount();

                std::vector<Flight> flights(flightCount);

                for (int i = 0; i < flightCount; ++i)
                {
                    if (!flightManager.getFlightAt(i, flights[i]))
                    {
                        throw std::runtime_error(
                            "Could not retrieve flight record"
                        );
                    }
                }

                // Run the existing C++ recommendation algorithm.
                const std::vector<SmartFlightOption> alternatives =
                    SmartFlightSuggestion::findAlternatives(
                        selectedFlight,
                        flights.data(),
                        flightCount
                    );

                // Prepare JSON response.
                std::ostringstream body;

                body << "{"
                     << "\"selectedFlight\":\""
                     << jsonEscape(selectedFlight.getFlightNumber())
                     << "\","
                     << "\"alternatives\":[";

                for (std::size_t i = 0; i < alternatives.size(); ++i)
                {
                    const SmartFlightOption& option = alternatives[i];

                    if (i > 0)
                        body << ",";

                    body << "{"
                         << "\"flightNumber\":\""
                         << jsonEscape(option.flightNumber) << "\","
                         << "\"airline\":\""
                         << jsonEscape(option.airline) << "\","
                         << "\"source\":\""
                         << jsonEscape(option.source) << "\","
                         << "\"destination\":\""
                         << jsonEscape(option.destination) << "\","
                         << "\"departureTime\":\""
                         << jsonEscape(option.departureTime) << "\","
                         << "\"arrivalTime\":\""
                         << jsonEscape(option.arrivalTime) << "\","
                         << "\"status\":\""
                         << jsonEscape(option.status) << "\","
                         << "\"fare\":"
                         << std::setprecision(15)
                         << option.fare << ","
                         << "\"durationMinutes\":"
                         << option.durationMinutes << ","
                         << "\"recommendationScore\":"
                         << option.recommendationScore
                         << "}";
                }

                body << "]}";

                response.status = 200;
                response.set_content(
                    body.str(),
                    "application/json"
                );
            }
            catch (const std::exception& error)
            {
                std::cerr
                    << "SmartFlight API error: "
                    << error.what() << '\n';

                response.status = 503;
                response.set_content(
                    R"({"status":"error","message":"Could not generate flight suggestions; check server console"})",
                    "application/json"
                );
            }
        }
    );
}
