#include "FlightManager.h"
#include "SmartFlightSuggestion.h"
#include "PassengerManager.h"
#include "NotificationManager.h"
#include <iostream>
#include <string>
#include <vector>

using namespace std;

FlightManager::FlightManager(int maxFlights)
{
    capacity = maxFlights;
    size = 0;

    flights = new Flight[capacity];
}



void FlightManager::addFlight()
{
    if (size == capacity)
    {
        cout << "Flight storage is full.\n";
        return;
    }

    string flightNumber;
    string airline;
    string source;
    string destination;
    string departureTime;
    string arrivalTime;
    string status;

    cin.ignore();
    cout << "Enter Flight Number: ";
    getline(cin,flightNumber);

    // Check for duplicate flight number
    for (int i = 0; i < size; i++)
    {
        if (flights[i].getFlightNumber() == flightNumber)
        {
            cout << "Flight with this number already exists.\n";
            return;
        }
    }

    cin.ignore();
    cout << "Enter Airline: ";
    getline(cin,airline);

    cout << "Enter Source: ";
    getline(cin,source);

    cout << "Enter Destination: ";
    getline(cin,destination);

    cout << "Enter Departure Time: ";
    getline(cin,departureTime);

    cout << "Enter Arrival Time: ";
    getline(cin,arrivalTime);

    cout << "Enter Status: ";
    getline(cin,status);

    flights[size].setFlightNumber(flightNumber);
    flights[size].setAirline(airline);
    flights[size].setSource(source);
    flights[size].setDestination(destination);
    flights[size].setDepartureTime(departureTime);
    flights[size].setArrivalTime(arrivalTime);
    flights[size].setStatus(status);

    size++;

    cout << "Flight added successfully.\n";
}

void FlightManager::displayFlights()
{
    if (size == 0)
    {
        cout << "No flights available.\n";
        return;
    }

    cout << "\n========== ALL FLIGHTS ==========\n";

    for (int i = 0; i < size; i++)
    {
        cout << "\nFlight " << i + 1 << endl;
        flights[i].display();
    }
}

void FlightManager::searchFlight()
{
    if (size == 0)
    {
        cout << "No flights available.\n";
        return;
    }

    string flightNumber;

    cin.ignore();
    cout << "Enter Flight Number to search: ";
    getline(cin,flightNumber);

    for (int i = 0; i < size; i++)
    {
        if (flights[i].getFlightNumber() == flightNumber)
        {
            cout << "Flight found:\n";
            flights[i].display();
            return;
        }
    }

    cout << "Flight not found.\n";
}

bool FlightManager::flightExists(const std::string& flightNumber)
{
    for (int i = 0; i < size; i++)
    {
        if (flights[i].getFlightNumber() == flightNumber)
        {
            return true;
        }
    }

    return false;
}

void FlightManager::updateFlight(
    PassengerManager& passengerManager,
    NotificationManager& notificationManager)
{
    if (size == 0)
    {
        cout << "No flights available.\n";
        return;
    }

    string flightNumber;

    cout << "Enter Flight Number to update: ";
    getline(cin >> ws, flightNumber);

    for (int i = 0; i < size; i++)
    {
        if (flights[i].getFlightNumber() == flightNumber)
        {
            string airline;
            string source;
            string destination;
            string departureTime;
            string arrivalTime;
            string status;

            // Save the old details before updating the flight
            string oldDeparture = flights[i].getDepartureTime();
            string oldStatus = flights[i].getStatus();

            cout << "\nFlight found. Enter updated details:\n";

            cout << "Enter Airline: ";
            getline(cin, airline);

            cout << "Enter Source: ";
            getline(cin, source);

            cout << "Enter Destination: ";
            getline(cin, destination);

            cout << "Enter Departure Time: ";
            getline(cin, departureTime);

            cout << "Enter Arrival Time: ";
            getline(cin, arrivalTime);

            cout << "Enter Status: ";
            getline(cin, status);

            // Normalize status text for comparisons
            string normalizedStatus = status;

            for (char& ch : normalizedStatus)
            {
                ch = static_cast<char>(
                    tolower(static_cast<unsigned char>(ch))
                );
            }

            string normalizedOldStatus = oldStatus;

            for (char& ch : normalizedOldStatus)
            {
                ch = static_cast<char>(
                    tolower(static_cast<unsigned char>(ch))
                );
            }

            bool departureChanged =
                oldDeparture != departureTime;

            bool statusChanged =
                oldStatus != status;

            bool isCancelled =
                normalizedStatus == "cancelled" ||
                normalizedStatus == "canceled";

            bool wasCancelled =
                normalizedOldStatus == "cancelled" ||
                normalizedOldStatus == "canceled";

            bool isDelayed =
                normalizedStatus.find("delay") != string::npos;

            // Update the flight details
            flights[i].setAirline(airline);
            flights[i].setSource(source);
            flights[i].setDestination(destination);
            flights[i].setDepartureTime(departureTime);
            flights[i].setArrivalTime(arrivalTime);
            flights[i].setStatus(status);

            cout << "Flight updated successfully.\n";

            // Generate the appropriate notification
            if (isCancelled && !wasCancelled)
            {
                passengerManager.notifyFlightCancellation(
                    flightNumber,
                    notificationManager,
                    "The flight has been cancelled."
                );
            }
            else if (!isCancelled &&
                     (statusChanged || departureChanged) &&
                     isDelayed)
            {
                passengerManager.notifyFlightDelay(
                    flightNumber,
                    notificationManager,
                    departureTime
                );
            }
            else if (!isCancelled && statusChanged)
            {
                passengerManager.notifyFlightStatusUpdate(
                    flightNumber,
                    notificationManager,
                    status
                );
            }

            return;
        }
    }

    cout << "Flight not found.\n";
}

void FlightManager::deleteFlight()
{
    if (size == 0)
    {
        cout << "No flights available.\n";
        return;
    }

    string flightNumber;

    cin.ignore();
    cout << "Enter Flight Number to delete: ";
    getline(cin, flightNumber);

    for (int i = 0; i < size; i++)
    {
        if (flights[i].getFlightNumber() == flightNumber)
        {
            for (int j = i; j < size - 1; j++)
            {
                flights[j].swapFlight(flights[j + 1]);
            }

            size--;

            cout << "\nFlight deleted successfully.\n";
            return;
        }
    }

    cout << "Flight not found.\n";
}

void FlightManager::cancelFlight(PassengerManager& passengerManager, NotificationManager& notificationManager)
{
    if (size == 0)
    {
        cout << "No flights available.\n";
        return;
    }

    string flightNumber;

    cout << "Enter Flight Number to cancel: ";
    getline(cin >> ws, flightNumber);

    for (int i = 0; i < size; i++)
    {
        if (flights[i].getFlightNumber() == flightNumber)
        {
            // Avoid generating duplicate cancellation notifications
            if (flights[i].getStatus() == "Cancelled")
            {
                cout << "This flight is already cancelled.\n";
                return;
            }

            flights[i].setStatus("Cancelled");

            cout << "Flight cancelled successfully.\n";

            // Notify every registered passenger booked on this flight
            passengerManager.notifyFlightCancellation(
                flightNumber,
                notificationManager
            );

            return;
        }
    }

    cout << "Flight not found.\n";
}

void FlightManager::sortFlights()
{
    if (size <= 1)
    {
        cout << "Not enough flights to sort.\n";
        return;
    }

    for (int i = 0; i < size - 1; i++)
    {
        bool swapped = false;

        for (int j = 0; j < size - i - 1; j++)
        {
            if (flights[j].getFlightNumber() >
                flights[j + 1].getFlightNumber())
            {
                flights[j].swapFlight(flights[j + 1]);
                swapped = true;
            }
        }

        if (!swapped)
        {
            break;
        }
    }

    cout << "\nFlights sorted successfully by Flight Number.\n";
}

void FlightManager::suggestAlternativeFlights()
{
    if (size == 0)
    {
        cout << "\nNo flights available.\n";
        return;
    }

    string flightNumber;

    cout << "\n========== Smart Flight Suggestions ==========\n";
    cout << "Enter Flight Number: ";
    getline(cin >> ws, flightNumber);

    int index = -1;

    // Find the selected flight
    for (int i = 0; i < size; i++)
    {
        if (flights[i].getFlightNumber() == flightNumber)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        cout << "Flight not found.\n";
        return;
    }

    cout << "\nSelected Flight:\n";
    flights[index].display();

    // Find alternative flights on the same route
    vector<SmartFlightOption> alternatives =
        SmartFlightSuggestion::findAlternatives(
            flights[index], flights, size
        );

    if (alternatives.empty())
    {
        cout << "\nNo alternative flights found on this route.\n";
        return;
    }

    cout << "\n========== Available Alternatives ==========\n";

    for (size_t i = 0; i < alternatives.size(); i++)
    {
        cout << "\nOption " << i + 1 << endl;
        cout << "Flight Number : "
             << alternatives[i].flightNumber << endl;
        cout << "Airline       : "
             << alternatives[i].airline << endl;
        cout << "Route         : "
             << alternatives[i].source << " -> "
             << alternatives[i].destination << endl;
        cout << "Departure     : "
             << alternatives[i].departureTime << endl;
        cout << "Arrival       : "
             << alternatives[i].arrivalTime << endl;
        cout << "Status        : "
             << alternatives[i].status << endl;
        cout << "--------------------------------------\n";
    }
}

FlightManager::~FlightManager()
{
    delete[] flights;
}