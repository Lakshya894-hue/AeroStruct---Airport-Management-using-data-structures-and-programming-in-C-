#ifndef AIRPORTNAVIGATOR_H
#define AIRPORTNAVIGATOR_H

#include <cstddef>
#include <string>
#include <vector>

class AirportNavigator
{
public:
    struct RouteResult
    {
        bool success = false;
        std::vector<std::string> path;
        long long totalMinutes = 0;
        std::string message;
    };

    struct PathRecord
    {
        std::string from;
        std::string to;
        int walkingMinutes = 0;
    };

    AirportNavigator();

    // Add a location or an undirected walking path. The message explains failure.
    bool addLocation(const std::string& name, std::string& message);
    bool addPath(const std::string& from,
                 const std::string& to,
                 int walkingMinutes,
                 std::string& message);

    std::vector<std::string> getLocations() const;
    std::vector<PathRecord> getPaths() const;

    void clearMap();

    void displayLocations() const;

    // Dijkstra's algorithm: finds the shortest route by total walking minutes.
    RouteResult findShortestRoute(const std::string& start,
                                  const std::string& destination) const;

    // Traversal order for the connected component reachable from the start.
    bool breadthFirstTraversal(const std::string& start,
                               std::vector<std::string>& order,
                               std::string& message) const;
    bool depthFirstTraversal(const std::string& start,
                             std::vector<std::string>& order,
                             std::string& message) const;

    // Console menu for the C++ application.
    void runMenu();

private:
    struct Edge
    {
        std::size_t destination;
        int walkingMinutes;
    };

    struct Location
    {
        std::string name;
        std::vector<Edge> edges;
    };

    std::vector<Location> locations;

    static std::string trim(const std::string& value);
    static std::string normalize(const std::string& value);
    static bool isValidLocationName(const std::string& name,
                                    std::string& message);
    int findLocationIndex(const std::string& name) const;
    void addDefaultMap();
    void depthFirstVisit(std::size_t index,
                         std::vector<bool>& visited,
                         std::vector<std::string>& order) const;
};

#endif
