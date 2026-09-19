#include "FlightManager.h"
#include <iostream>
#include <string>

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

void FlightManager::updateFlight()
{
    if (size == 0)
    {
        cout << "No flights available.\n";
        return;
    }

    string flightNumber;

    cin.ignore();
    cout << "Enter Flight Number to update: ";
    getline(cin,flightNumber);

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

            flights[i].setAirline(airline);
            flights[i].setSource(source);
            flights[i].setDestination(destination);
            flights[i].setDepartureTime(departureTime);
            flights[i].setArrivalTime(arrivalTime);
            flights[i].setStatus(status);

            cout << "Flight updated successfully.\n";
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

void FlightManager::cancelFlight()
{
    if (size == 0)
    {
        cout << "No flights available.\n"<<endl;
        return;
    }

    string flightNumber;

    cin.ignore();
    cout << "Enter Flight Number to cancel: ";
    getline(cin,flightNumber);

    for (int i = 0; i < size; i++)
    {
        if (flights[i].getFlightNumber() == flightNumber)
        {
            flights[i].setStatus("Cancelled");

            cout << "Flight cancelled successfully.\n";
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

FlightManager::~FlightManager()
{
    delete[] flights;
}