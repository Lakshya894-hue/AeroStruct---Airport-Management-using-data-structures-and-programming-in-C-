#ifndef FLIGHTMANAGER_H
#define FLIGHTMANAGER_H

#include "Flight.h"
#include <string>

class PassengerManager;
class NotificationManager;
class CheckInQueue;
class BoardingQueue;
class FlightManager
{
private:
    Flight* flights;
    int size;
    int capacity;

public:
    FlightManager(int maxFlights = 100);
    ~FlightManager();

    void addFlight();
    void displayFlights();
    void searchFlight();
    void updateFlight(
    PassengerManager& passengerManager,NotificationManager& notificationManager,
    CheckInQueue& checkInQueue, BoardingQueue& boardingQueue);
    void deleteFlight();
    void cancelFlight(
    PassengerManager& passengerManager,
    NotificationManager& notificationManager,
    CheckInQueue& checkInQueue,
    BoardingQueue& boardingQueue);
    void sortFlights();
    void suggestAlternativeFlights();
    bool flightExists(const std::string& flightNumber);
    bool getFlightStatus(const std::string& flightNumber, std::string& status);
};

#endif