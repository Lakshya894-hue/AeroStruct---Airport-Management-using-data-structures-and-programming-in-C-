#ifndef PASSENGER_H
#define PASSENGER_H

#include "Person.h"
#include <string>

using namespace std;

class Passenger : public Person
{
private:
    string passengerId;
    string gender;
    string phoneNumber;
    string flightNumber;
    string seatNumber;
    string assistanceType;
    string status;

public:
    Passenger();

    Passenger(string id,
              string n,
              int a,
              string g,
              string phone,
              string flight,
              string seat,
              string assistance,
              string stat);

    void display() override;

    string getPassengerId() const;
    string getGender() const;
    string getPhoneNumber() const;
    string getFlightNumber() const;
    string getSeatNumber() const;
    string getAssistanceType() const;
    string getStatus() const;
    bool operator==(const Passenger& other) const;

    void setGender(string g);
    void setPhoneNumber(string phone);
    void setFlightNumber(string flight);
    void setSeatNumber(string seat);
    void setAssistanceType(string assistance);
    void setStatus(string stat);
};

#endif