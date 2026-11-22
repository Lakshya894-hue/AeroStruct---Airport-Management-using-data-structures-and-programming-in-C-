#include "NavigationRoutes.h"

#include "AirportNavigator.h"
#include "httplib.h"

#include <mysqlx/xdevapi.h>

#include <cstddef>
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

void setNavigatorErrorResponse(
    httplib::Response& response,
    int status,
    const std::string& message);

bool loadAirportGraphIntoManager(
    mysqlx::Session& session,
    const std::string& airportCode,
    AirportNavigator& navigator,
    int& airportId,
    std::string& message);

std::string navigatorStringArrayToJson(
    const std::vector<std::string>& values);

int getAirportSegmentDistanceMeters(
    mysqlx::Session& session,
    int airportId,
    const std::string& from,
    const std::string& to);

std::string buildAirportGraphJson(
    mysqlx::Session& session,
    int airportId);

void registerNavigationRoutes(httplib::Server& server)
{
    // =========================================================
    // GET /api/airports
    // Retrieve all airports.
    // =========================================================

    server.Get("/api/airports",
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
                "localhost",
                33060,
                getDbUser(),
                getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            auto result = session.sql(
                "SELECT airport_id, airport_code, airport_name, city "
                "FROM airports "
                "ORDER BY airport_code"
            ).execute();

            auto row = result.fetchOne();

            std::ostringstream out;
            out << "[";

            bool first = true;

            while (row)
            {
                if (!first)
                    out << ",";

                first = false;

                out << "{"
                    << "\"airportId\":" << row[0].get<int>() << ","
                    << "\"airportCode\":\""
                    << jsonEscape(row[1].get<std::string>()) << "\","
                    << "\"airportName\":\""
                    << jsonEscape(row[2].get<std::string>()) << "\","
                    << "\"city\":\""
                    << jsonEscape(row[3].get<std::string>()) << "\""
                    << "}";

                row = result.fetchOne();
            }

            out << "]";

            response.set_content(
                out.str(),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Airport listing failed: "
                << error.what() << '\n';

            setNavigatorErrorResponse(
                response, 503,
                "Could not retrieve airport records.");
        }
    });

    // =========================================================
    // GET /api/navigator/graph
    // Retrieve the complete graph of a selected airport.
    // =========================================================

    server.Get("/api/navigator/graph",
        [](const httplib::Request& request,
           httplib::Response& response)
    {
        setCorsHeaders(response);

        if (!request.has_param("airportCode"))
        {
            setNavigatorErrorResponse(
                response, 400,
                "Airport code is required.");
            return;
        }

        if (!databaseCredentialsAvailable())
        {
            setNavigatorErrorResponse(
                response, 500,
                "Database credentials are not configured.");
            return;
        }

        const std::string airportCode =
            toUpper(trim(request.get_param_value("airportCode")));

        try
        {
            mysqlx::Session session(
                "localhost",
                33060,
                getDbUser(),
                getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AirportNavigator navigator;
            int airportId = 0;
            std::string message;

            if (!loadAirportGraphIntoManager(
                    session,
                    airportCode,
                    navigator,
                    airportId,
                    message))
            {
                setNavigatorErrorResponse(
                    response, 404, message);
                return;
            }

            response.set_content(
                buildAirportGraphJson(session, airportId),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Airport graph loading failed: "
                << error.what() << '\n';

            setNavigatorErrorResponse(
                response, 503,
                "Could not load the selected airport graph.");
        }
    });

    // =========================================================
    // POST /api/navigator/route
    // Find the shortest route using Dijkstra's algorithm.
    // =========================================================

    server.Post("/api/navigator/route",
        [](const httplib::Request& request,
           httplib::Response& response)
    {
        setCorsHeaders(response);

        auto readField = [&](const std::string& key)
        {
            if (request.has_param(key))
                return trim(request.get_param_value(key));

            return std::string();
        };

        const std::string airportCode =
            toUpper(readField("airportCode"));

        const std::string from = readField("from");
        const std::string to = readField("to");

        if (airportCode.empty() || from.empty() || to.empty())
        {
            setNavigatorErrorResponse(
                response, 400,
                "Airport code, starting location and destination are required.");
            return;
        }

        if (from == to)
        {
            setNavigatorErrorResponse(
                response, 400,
                "Starting location and destination must be different.");
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
                "localhost",
                33060,
                getDbUser(),
                getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AirportNavigator navigator;
            int airportId = 0;
            std::string message;

            if (!loadAirportGraphIntoManager(
                    session,
                    airportCode,
                    navigator,
                    airportId,
                    message))
            {
                setNavigatorErrorResponse(
                    response, 404, message);
                return;
            }

            const auto route =
                navigator.findShortestRoute(from, to);

            if (!route.success)
            {
                setNavigatorErrorResponse(
                    response, 404, route.message);
                return;
            }

            long long totalMeters = 0;

            for (std::size_t i = 1; i < route.path.size(); ++i)
            {
                totalMeters += getAirportSegmentDistanceMeters(
                    session,
                    airportId,
                    route.path[i - 1],
                    route.path[i]
                );
            }

            std::ostringstream out;

            out << "{"
                << "\"success\":true,"
                << "\"airportCode\":\""
                << jsonEscape(airportCode) << "\","
                << "\"from\":\""
                << jsonEscape(from) << "\","
                << "\"to\":\""
                << jsonEscape(to) << "\","
                << "\"path\":"
                << navigatorStringArrayToJson(route.path) << ","
                << "\"totalMinutes\":"
                << route.totalMinutes << ","
                << "\"distanceMeters\":"
                << totalMeters << ","
                << "\"message\":\""
                << jsonEscape(route.message) << "\""
                << "}";

            response.set_content(
                out.str(),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Airport route calculation failed: "
                << error.what() << '\n';

            setNavigatorErrorResponse(
                response, 500,
                "Could not calculate the airport route.");
        }
    });

    // =========================================================
    // POST /api/navigator/traverse
    // Perform BFS or DFS traversal.
    // =========================================================

    server.Post("/api/navigator/traverse",
        [](const httplib::Request& request,
           httplib::Response& response)
    {
        setCorsHeaders(response);

        auto readField = [&](const std::string& key)
        {
            if (request.has_param(key))
                return trim(request.get_param_value(key));

            return std::string();
        };

        const std::string airportCode =
            toUpper(readField("airportCode"));

        const std::string start = readField("start");

        const std::string algorithm =
            toUpper(readField("algorithm"));

        if (airportCode.empty() ||
            start.empty() ||
            (algorithm != "BFS" && algorithm != "DFS"))
        {
            setNavigatorErrorResponse(
                response, 400,
                "Provide a valid airport code, starting location and BFS or DFS algorithm.");
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
                "localhost",
                33060,
                getDbUser(),
                getDbPassword()
            );

            session.sql("USE aerostruct_db").execute();

            AirportNavigator navigator;
            int airportId = 0;
            std::string message;

            if (!loadAirportGraphIntoManager(
                    session,
                    airportCode,
                    navigator,
                    airportId,
                    message))
            {
                setNavigatorErrorResponse(
                    response, 404, message);
                return;
            }

            std::vector<std::string> order;
            bool success = false;

            if (algorithm == "BFS")
            {
                success = navigator.breadthFirstTraversal(
                    start, order, message);
            }
            else
            {
                success = navigator.depthFirstTraversal(
                    start, order, message);
            }

            if (!success)
            {
                setNavigatorErrorResponse(
                    response, 400, message);
                return;
            }

            std::ostringstream out;

            out << "{"
                << "\"success\":true,"
                << "\"airportCode\":\""
                << jsonEscape(airportCode) << "\","
                << "\"algorithm\":\""
                << algorithm << "\","
                << "\"start\":\""
                << jsonEscape(start) << "\","
                << "\"order\":"
                << navigatorStringArrayToJson(order) << ","
                << "\"message\":\""
                << jsonEscape(message) << "\""
                << "}";

            response.set_content(
                out.str(),
                "application/json"
            );
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Airport graph traversal failed: "
                << error.what() << '\n';

            setNavigatorErrorResponse(
                response, 500,
                "Could not perform graph traversal.");
        }
    });
}
