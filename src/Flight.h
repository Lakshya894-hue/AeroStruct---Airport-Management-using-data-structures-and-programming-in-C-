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
    double fare;
    string gate;

public:
    Flight();

    Flight(string fno, string air, string src, string dest,
           string dep, string arr, string stat,
           double amount = 0.0);

    void display();

    string getFlightNumber() const;
    string getAirline() const;
    string getSource() const;
    string getDestination() const;
    string getDepartureTime() const;
    string getArrivalTime() const;
    string getStatus() const;
    double getFare() const;
    string getGate() const;

    void setFlightNumber(string fno);
    void setAirline(string air);
    void setSource(string src);
    void setDestination(string dest);
    void setDepartureTime(string dep);
    void setArrivalTime(string arr);
    void setStatus(string stat);
    void setFare(double amount);
    void setGate(string g);

    void swapFlight(Flight &other);
};

#endif
