#include "AirportNavigator.h"
#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;

AirportNavigator::AirportNavigator()
{
    addDefaultMap();
}

string AirportNavigator::trim(const string& value) //remove trailing spaces from the string
{
    size_t first = 0;
    while (first < value.size() && isspace(static_cast<unsigned char>(value[first])))
    {
        ++first; //first non_whitespace character
    }

    size_t last = value.size();
    while (last > first && isspace(static_cast<unsigned char>(value[last - 1])))
    {
        --last; //last non -whitespace character
    }

    return value.substr(first, last - first);
}

string AirportNavigator::normalize(const string& value)
{
    string result = trim(value);
    for (char& ch : result)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch))); //converts to lower case
    }
    return result;
}

bool AirportNavigator::isValidLocationName(const string& name, string& message)
{
    if (name.empty())
    {
        message = "Location name cannot be empty.";
        return false;
    }

    if (name.size() < 2 || name.size() > 50)
    {
        message = "Location name must contain between 2 and 50 characters.";//checks for location size
        return false;
    }

    bool hasLetterOrDigit = false;
    for (unsigned char ch : name)
    {
        if (isalnum(ch))
        {
            hasLetterOrDigit = true;
        }
        else if (ch != ' ' && ch != '-' && ch != '_' && ch != '/' &&
         ch != '\'' && ch != '(' && ch != ')' && ch != '&') //accepts these characters
        {
            message = "Location name contains an unsupported character.";
            return false;
        }
    }

    if (!hasLetterOrDigit)
    {
        message = "Location name must contain a letter or number.";
        return false;
    }

    return true;
}

int AirportNavigator::findLocationIndex(const string& name) const
{
    const string key = normalize(name);
    if (key.empty())
    {
        return -1;
    }

    for (size_t i = 0; i < locations.size(); ++i)
    {
        if (normalize(locations[i].name) == key)
        {
            return static_cast<int>(i);
        }
    }

    return -1;
}

bool AirportNavigator::addLocation(const string& rawName, string& message)
{
    const string name = trim(rawName);
    if (!isValidLocationName(name, message))
    {
        return false;
    }

    if (findLocationIndex(name) != -1)
    {
        message = "That location already exists (comparison ignores letter case).";
        return false;
    }

    if (locations.size() >= 100) // max 100 locations can be added
    {
        message = "The map already contains the maximum of 100 locations.";
        return false;
    }

    locations.push_back(Location{name, {}}); //creates a new location of with empty edges
    message = "Location added successfully.";
    return true;
}

bool AirportNavigator::addPath(const string& rawFrom, const string& rawTo, int walkingMinutes,
                               string& message)
{
    const string from = trim(rawFrom);
    const string to = trim(rawTo);

    if (from.empty() || to.empty())
    {
        message = "Both starting and destination locations are required.";
        return false;
    }

    const int fromIndex = findLocationIndex(from);
    const int toIndex = findLocationIndex(to);

    if (fromIndex == -1)
    {
        message = "Starting location does not exist. Add or select a valid location.";
        return false;
    }

    if (toIndex == -1)
    {
        message = "Destination location does not exist. Add or select a valid location.";
        return false;
    }

    if (fromIndex == toIndex)
    {
        message = "A path must connect two different locations.";
        return false;
    }

    if (walkingMinutes < 1 || walkingMinutes > 240)
    {
        message = "Walking time must be a whole number between 1 and 240 minutes.";
        return false;
    }

    for (const Edge& edge : locations[static_cast<size_t>(fromIndex)].edges)
    {
        if (edge.destination == static_cast<size_t>(toIndex))
        {
            message = "A path between these locations already exists.";
            return false;
        }
    }

    // The airport map is undirected: both directions get the same walking time.
    locations[static_cast<size_t>(fromIndex)].edges.push_back(
        Edge{static_cast<size_t>(toIndex), walkingMinutes});
    locations[static_cast<size_t>(toIndex)].edges.push_back(
        Edge{static_cast<size_t>(fromIndex), walkingMinutes});

    message = "Walking path added successfully in both directions.";
    return true;
}

vector<string> AirportNavigator::getLocations() const
{
    vector<string> names;
    names.reserve(locations.size());//reserves sufficient capacity for the expected number of names.
    for (const Location& location : locations)
    {
        names.push_back(location.name);//copies its name into the result vector.
    }
    return names;
}

// Retrieve every undirected path exactly once.
vector<AirportNavigator::PathRecord>
AirportNavigator::getPaths() const
{
    vector<PathRecord> paths;

    for (size_t from = 0; from < locations.size(); ++from)
    {
        for (const Edge& edge : locations[from].edges)
        {
            // Each undirected path is stored in both directions.
            // Include it only once.
            if (edge.destination < locations.size() &&
                from < edge.destination)
            {
                paths.push_back({
                    locations[from].name,
                    locations[edge.destination].name,
                    edge.walkingMinutes
                });
            }
        }
    }

    return paths;
}


// Clear the existing graph before reconstructing it from MySQL.
void AirportNavigator::clearMap()
{
    locations.clear();
}

void AirportNavigator::displayLocations() const
{
    if (locations.empty())
    {
        cout << "No airport locations are available.\n";
        return;
    }

    cout << "\n========== Airport Locations and Walking Paths ==========\n";
    for (const Location& location : locations)
    {
        cout << "\n" << location.name << ":\n";

        if (location.edges.empty())
        {
            cout << "  No connected paths.\n";
            continue;
        }

        // The adjacency list stores every undirected path in both directions,
        // so each location displays all of its directly reachable neighbours.
        for (const Edge& edge : location.edges)
        {
            cout << "  -> " << locations[edge.destination].name
                 << " (" << edge.walkingMinutes << " min)\n"; //displays existing connections
        }
    }
    //cout << "\nTimes are illustrative walking minutes, not live airport data.\n";
}

AirportNavigator::RouteResult AirportNavigator::findShortestRoute(const string& start,
    const string& destination) const //dijikstra's algorithm
{
    RouteResult result;
    const int startIndex = findLocationIndex(start);
    const int destinationIndex = findLocationIndex(destination);

    if (trim(start).empty() || trim(destination).empty())
    {
        result.message = "Starting location and destination are required.";
        return result;
    }

    if (startIndex == -1)
    {
        result.message = "Starting location was not found.";
        return result;
    }

    if (destinationIndex == -1)
    {
        result.message = "Destination was not found.";
        return result;
    }

    if (startIndex == destinationIndex)
    {
        result.message = "Starting location and destination must be different.";
        return result;
    }

    const size_t count = locations.size();
    const long long INF = numeric_limits<long long>::max() / 4;
    vector<long long> distance(count, INF);//stores the best walking time
    vector<int> previous(count, -1);//remembers the previous location on the best route found so far.
    using QueueEntry = pair<long long, size_t>;
    priority_queue<QueueEntry, vector<QueueEntry>, greater<QueueEntry>> frontier;//minimum-priority queue

    distance[static_cast<size_t>(startIndex)] = 0;
    frontier.push({0, static_cast<size_t>(startIndex)});

    while (!frontier.empty())
    {
        const auto current = frontier.top();
        frontier.pop();

        const long long currentDistance = current.first;//walking time
        const size_t currentIndex = current.second;

        if (currentDistance != distance[currentIndex])
        {
            continue; // Ignore an outdated priority-queue entry.
        }

        if (currentIndex == static_cast<size_t>(destinationIndex))
        {
            break;
        }

        for (const Edge& edge : locations[currentIndex].edges)
        {
            const long long candidate = currentDistance + edge.walkingMinutes; //time required to reach the neighbour through the current location.
            if (candidate < distance[edge.destination]) //updates time, remebers previous locations
            {
                distance[edge.destination] = candidate;
                previous[edge.destination] = static_cast<int>(currentIndex);
                frontier.push({candidate, edge.destination});
            }
        }
    }

    if (distance[static_cast<size_t>(destinationIndex)] == INF)
    {
        result.message = "No route exists between those locations."; //if route dosen't exists
        return result;
    }

    vector<size_t> reversedPath;
    int cursor = destinationIndex;
    while (cursor != -1)
    {
        reversedPath.push_back(static_cast<size_t>(cursor)); //reverse the order of the path
        if (cursor == startIndex)
        {
            break;
        }
        cursor = previous[static_cast<size_t>(cursor)];
    }

    if (reversedPath.empty() || reversedPath.back() != static_cast<size_t>(startIndex))
    {
        result.message = "Could not reconstruct the route.";
        return result;
    }

    reverse(reversedPath.begin(), reversedPath.end());
    for (size_t index : reversedPath)
    {
        result.path.push_back(locations[index].name); //converts index to location names
    }

    result.success = true;
    result.totalMinutes = distance[static_cast<size_t>(destinationIndex)];
    result.message = "Shortest walking-time route found.";
    return result;
}

bool AirportNavigator::breadthFirstTraversal(const string& start, vector<string>& order,
                                             string& message) const
{
    order.clear();//clears all previous
    const int startIndex = findLocationIndex(start);
    if (trim(start).empty())
    {
        message = "Starting location is required.";
        return false;
    }
    if (startIndex == -1)
    {
        message = "Starting location was not found.";
        return false;
    }

    vector<bool> visited(locations.size(), false);//flag The visited vector prevents the algorithm from processing the same location repeatedly
    queue<size_t> pending;
    pending.push(static_cast<size_t>(startIndex));
    visited[static_cast<size_t>(startIndex)] = true;

    while (!pending.empty())      //It removes the next location, adds the name to the visit order, and checks every connected edge.
    {
        const size_t current = pending.front();
        pending.pop();
        order.push_back(locations[current].name);

        for (const Edge& edge : locations[current].edges)
        {
            if (!visited[edge.destination])
            {
                visited[edge.destination] = true;
                pending.push(edge.destination);
            }
        }
    }

    message = "BFS traversal completed for locations reachable from the start.";
    return true;
}

void AirportNavigator::depthFirstVisit(size_t index,
                                       vector<bool>& visited,
                                       vector<string>& order) const
{
    visited[index] = true;//falg marks the current location visited
    order.push_back(locations[index].name);//pushes location name

    for (const Edge& edge : locations[index].edges)
    {
        if (!visited[edge.destination])
        {
            depthFirstVisit(edge.destination, visited, order); 
        }
    }
}

bool AirportNavigator::depthFirstTraversal(const string& start,vector<string>& order,
                                           string& message) const
{
    order.clear();
    const int startIndex = findLocationIndex(start);
    if (trim(start).empty())
    {
        message = "Starting location is required.";
        return false;
    }
    if (startIndex == -1)
    {
        message = "Starting location was not found.";
        return false;
    }

    vector<bool> visited(locations.size(), false);
    depthFirstVisit(static_cast<size_t>(startIndex), visited, order); //calls depthFirstVisit
    message = "DFS traversal completed for locations reachable from the start.";
    return true;
}

void AirportNavigator::addDefaultMap()
{
    const vector<string> defaultLocations = {
        "Main Entrance", "Check-In Counter", "Security Check",
        "Departure Lounge", "Gate A1", "Gate A2", "Medical Centre",
        "Baggage Claim", "Restrooms"
    };

    string message;
    for (const string& name : defaultLocations)
    {
        addLocation(name, message);
    }

    struct DefaultPath
    {
        const char* from;
        const char* to;
        int minutes;
    };

    const DefaultPath defaultPaths[] = {
        {"Main Entrance", "Check-In Counter", 2},
        {"Check-In Counter", "Security Check", 4},
        {"Security Check", "Departure Lounge", 3},
        {"Departure Lounge", "Gate A1", 2},
        {"Departure Lounge", "Gate A2", 4},
        {"Main Entrance", "Medical Centre", 3},
        {"Medical Centre", "Departure Lounge", 5},
        {"Main Entrance", "Baggage Claim", 5},
        {"Baggage Claim", "Restrooms", 2},
        {"Departure Lounge", "Restrooms", 2},
        {"Medical Centre", "Baggage Claim", 4}
    };

    for (const DefaultPath& path : defaultPaths)
    {
        addPath(path.from, path.to, path.minutes, message);
    }
}

namespace
{
bool readLinePrompt(const string& prompt, string& output)
{
    cout << prompt;
    if (!getline(cin, output))
    {
        return false;
    }
    return true;
}

bool parseIntegerLine(const string& line, int& value)
{
    // checks whether an input line contains exactly one valid integer within the range that an int can store.
    istringstream input(line);
    long long parsed = 0;
    char extra = '\0';

    if (!(input >> parsed) || (input >> extra))
    {
        return false;
    }
    if (parsed < numeric_limits<int>::min() ||
        parsed > numeric_limits<int>::max())
    {
        return false;
    }

    value = static_cast<int>(parsed);
    return true;
}

bool readIntegerPrompt(const string& prompt,int minimum, int maximum, int& value)
{
    while (true)
    {
        string line;
        if (!readLinePrompt(prompt, line))
        {
            return false;
        }

        int parsed = 0;
        if (!parseIntegerLine(line, parsed))
        {
            cout << "Invalid input. Please enter a whole number.\n";
            continue;
        }
        if (parsed < minimum || parsed > maximum)
        {
            cout << "Enter a value from " << minimum << " to " << maximum << ".\n";
            continue;
        }

        value = parsed;
        return true;
    }
}

void displayTraversal(const vector<string>& order)
{
    for (size_t i = 0; i < order.size(); ++i)
    {
        if (i > 0)
        {
            cout << " -> "; //displaying
        }
        cout << order[i];
    }
    cout << '\n';
}
}

void AirportNavigator::runMenu()
{
    while (true)
    {
        cout << "\n========== Airport Navigator ==========" << '\n'
             << "1. Display locations and paths\n"
             << "2. Find shortest walking-time route (Dijkstra)\n"
             << "3. BFS traversal\n"
             << "4. DFS traversal\n"
             << "5. Add location\n"
             << "6. Add walking path\n"
             << "0. Return to main menu\n";

        int choice = 0;
        if (!readIntegerPrompt("Enter your choice: ", 0, 6, choice))
        {
            cout << "Input stream closed. Returning to the main menu.\n";
            return;
        }

        if (choice == 0)
        {
            return;
        }

        if (choice == 1)
        {
            displayLocations();
        }
        else if (choice == 2)
        {
            string start, destination;
            if (!readLinePrompt("Enter starting location: ", start) ||
                !readLinePrompt("Enter destination: ", destination))
            {
                cout << "Input stream closed. Returning to the main menu.\n";
                return;
            }

            const RouteResult result = findShortestRoute(start, destination);
            if (!result.success)
            {
                cout << "Route unavailable: " << result.message << '\n';
            }
            else
            {
                cout << "Shortest route: ";
                displayTraversal(result.path);
                cout << "Total walking time: " << result.totalMinutes
                     << " minute(s).\n";
            }
        }
        else if (choice == 3 || choice == 4)
        {
            string start;
            if (!readLinePrompt("Enter starting location: ", start))
            {
                cout << "Input stream closed. Returning to the main menu.\n";
                return;
            }

            vector<string> order;
            string message;
            const bool success = choice == 3
                ? breadthFirstTraversal(start, order, message)
                : depthFirstTraversal(start, order, message);

            if (!success)
            {
                cout << "Traversal unavailable: " << message << '\n';
            }
            else
            {
                cout << message << '\n';
                displayTraversal(order);
            }
        }
        else if (choice == 5)
        {
            string name;
            if (!readLinePrompt("Enter new location name: ", name))
            {
                cout << "Input stream closed. Returning to the main menu.\n";
                return;
            }

            string message;
            if (addLocation(name, message))
            {
                cout << message << '\n';
            }
            else
            {
                cout << "Location not added: " << message << '\n';
            }
        }
        else if (choice == 6)
        {
            string from, to;
            int minutes = 0;
            if (!readLinePrompt("Enter starting location: ", from) ||
                !readLinePrompt("Enter destination location: ", to) ||
                !readIntegerPrompt("Enter walking time in minutes (1-240): ",
                                   1, 240, minutes))
            {
                cout << "Input stream closed. Returning to the main menu.\n";
                return;
            }

            string message;
            if (addPath(from, to, minutes, message))
            {
                cout << message << '\n';
            }
            else
            {
                cout << "Path not added: " << message << '\n';
            }
        }
    }
}
