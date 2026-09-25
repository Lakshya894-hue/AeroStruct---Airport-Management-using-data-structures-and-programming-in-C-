#ifndef PASSENGER_H
#define PASSENGER_H

#include <string>

using namespace std;

class Passenger
{
private:
    string passengerId;
    string name;
    int age;
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

    void display();

    // using const so that values cannot be modified
    string getPassengerId() const;
    string getName() const;
    int getAge() const;
    string getGender() const;
    string getPhoneNumber() const;
    string getFlightNumber() const;
    string getSeatNumber() const;
    string getAssistanceType() const;
    string getStatus() const;

    void setName(string n);
    void setAge(int a);
    void setGender(string g);
    void setPhoneNumber(string phone);
    void setFlightNumber(string flight);
    void setSeatNumber(string seat);
    void setAssistanceType(string assistance);
    void setStatus(string stat);
};

#endif