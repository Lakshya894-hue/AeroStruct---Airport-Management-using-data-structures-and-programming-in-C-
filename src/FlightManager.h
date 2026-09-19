#ifndef FLIGHTMANAGER_H
#define FLIGHTMANAGER_H

#include "Flight.h"

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
    void updateFlight();
    void deleteFlight();
    void cancelFlight();
    void sortFlights();
};

#endif