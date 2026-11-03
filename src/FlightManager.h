#ifndef FLIGHTMANAGER_H
#define FLIGHTMANAGER_H

#include "Flight.h"
#include <string>

class PriorityQueue;
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
    void updateFlight(PassengerManager& passengerManager,NotificationManager& notificationManager,
    CheckInQueue& checkInQueue,
    BoardingQueue& boardingQueue,
    PriorityQueue& priorityQueue);
    void deleteFlight();
    void cancelFlight(PassengerManager& passengerManager,
    NotificationManager& notificationManager,
    CheckInQueue& checkInQueue,
    BoardingQueue& boardingQueue,
    PriorityQueue& priorityQueue);
    void sortFlights();
    void suggestAlternativeFlights();
    bool flightExists(const std::string& flightNumber);
    bool getFlightStatus(const std::string& flightNumber, std::string& status);

    // Non-interactive operations for API integration
    int getFlightCount() const;

    bool getFlightAt(int index, Flight& result) const;

    bool findFlightRecord(const std::string& flightNumber, Flight& result);

    bool addFlightRecord(Flight flight);

    bool updateFlightRecord(const std::string& flightNumber, Flight updatedFlight);

    bool removeFlightRecord(const std::string& flightNumber);
};

#endif
