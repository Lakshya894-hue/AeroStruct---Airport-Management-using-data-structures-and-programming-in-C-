#include "Passenger.h"
#include <iostream>

using namespace std;

Passenger::Passenger()
    : Person(), passengerId(""), gender(""),
      phoneNumber(""), flightNumber(""), seatNumber(""),
      assistanceType(""), status("Registered")
{
}
Passenger::Passenger(string id, string n, int a, string g, string phone,
                     string flight, string seat, string assistance, string stat)
    : Person(n, a), passengerId(id), gender(g),
      phoneNumber(phone), flightNumber(flight),
      seatNumber(seat), assistanceType(assistance),
      status(stat)
{
}

void Passenger::display()
{
    cout << "Passenger ID     : " << passengerId << endl;
    cout << "Name             : " << getName() << endl;
    cout << "Age              : " << getAge() << endl;
    cout << "Gender           : " << gender << endl;
    cout << "Phone Number     : " << phoneNumber << endl;
    cout << "Flight Number    : " << flightNumber << endl;
    cout << "Seat Number      : " << seatNumber << endl;
    cout << "Assistance Type  : " << assistanceType << endl;
    cout << "Status           : " << status << endl;
}

// Getters
string Passenger::getPassengerId() const
{
    return passengerId;
}

string Passenger::getGender() const
{
    return gender;
}

string Passenger::getPhoneNumber() const
{
    return phoneNumber;
}

string Passenger::getFlightNumber() const
{
    return flightNumber;
}

string Passenger::getSeatNumber() const
{
    return seatNumber;
}

string Passenger::getAssistanceType() const
{
    return assistanceType;
}

string Passenger::getStatus() const
{
    return status;
}

// Compare two passengers by their ID
bool Passenger::operator==(const Passenger& other) const
{
    return passengerId == other.passengerId;
}

// Setters
void Passenger::setGender(string g)
{
    gender = g;
}

void Passenger::setPhoneNumber(string phone)
{
    phoneNumber = phone;
}

void Passenger::setFlightNumber(string flight)
{
    flightNumber = flight;
}

void Passenger::setSeatNumber(string seat)
{
    seatNumber = seat;
}

void Passenger::setAssistanceType(string assistance)
{
    assistanceType = assistance;
}

void Passenger::setStatus(string stat)
{
    status = stat;
}
