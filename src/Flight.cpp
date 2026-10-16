#include "Flight.h"
#include <iostream>
#include <utility>

using namespace std;

Flight::Flight()
{
    flightNumber = "";
    airline = "";
    source = "";
    destination = "";
    departureTime = "";
    arrivalTime = "";
    status = "Scheduled";
    fare = 0.0;
}

Flight::Flight(string fno, string air, string src, string dest, string dep, 
               string arr, string stat, double amount)
{
    flightNumber = fno;
    airline = air;
    source = src;
    destination = dest;
    departureTime = dep;
    arrivalTime = arr;
    status = stat;
    fare = amount;
}
void Flight::display()
{
    cout << endl;
    cout << "Flight Number : " << flightNumber << endl;
    cout << "Airline       : " << airline << endl;
    cout << "Source        : " << source << endl;
    cout << "Destination   : " << destination << endl;
    cout << "Departure     : " << departureTime << endl;
    cout << "Arrival       : " << arrivalTime << endl;
    cout << "Status        : " << status << endl;
    cout << "Ticket Fare   : Rs. " << fare << endl;
}

string Flight::getFlightNumber()
{
    return flightNumber;
}

string Flight::getAirline()
{
    return airline;
}

string Flight::getSource()
{
    return source;
}

string Flight::getDestination()
{
    return destination;
}

string Flight::getDepartureTime()
{
    return departureTime;
}

string Flight::getArrivalTime()
{
    return arrivalTime;
}

string Flight::getStatus()
{
    return status;
}

double Flight::getFare()
{
    return fare;
}

void Flight::setAirline(string air)
{
    airline = air;
}

void Flight::setFlightNumber(string fno)
{
    flightNumber = fno;
}

void Flight::setSource(string src)
{
    source = src;
}

void Flight::setDestination(string dest)
{
    destination = dest;
}

void Flight::setDepartureTime(string dep)
{
    departureTime = dep;
}

void Flight::setArrivalTime(string arr)
{
    arrivalTime = arr;
}

void Flight::setStatus(string stat)
{
    status = stat;
}

void Flight::setFare(double amount)
{
    fare = amount;
}

void Flight::swapFlight(Flight &other)
{
    swap(flightNumber, other.flightNumber);
    swap(airline, other.airline);
    swap(source, other.source);
    swap(destination, other.destination);
    swap(departureTime, other.departureTime);
    swap(arrivalTime, other.arrivalTime);
    swap(status, other.status);
    swap(fare, other.fare);
}
