#ifndef FLIGHT_H
#define FLIGHT_H

#include <string>
using namespace std;

class Flight
{
private:
    string flightNumber;
    string airline;
    string source;
    string destination;
    string departureTime;
    string arrivalTime;
    string status;

public:
    Flight();

    Flight(string fno, string air, string src, string dest,
           string dep, string arr, string stat);

    void display();

    string getFlightNumber();
    string getAirline();
    string getSource();
    string getDestination();
    string getDepartureTime();
    string getArrivalTime();
    string getStatus();

    void setFlightNumber(string fno);
    void setAirline(string air);
    void setSource(string src);
    void setDestination(string dest);
    void setDepartureTime(string dep);
    void setArrivalTime(string arr);
    void setStatus(string stat);

    void swapFlight(Flight &other);
};

#endif