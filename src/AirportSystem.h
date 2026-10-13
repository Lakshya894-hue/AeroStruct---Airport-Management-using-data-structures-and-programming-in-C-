#ifndef AIRPORTSYSTEM_H
#define AIRPORTSYSTEM_H

#include "FlightManager.h"
#include "PassengerManager.h"
#include "CheckInQueue.h"
#include "BoardingQueue.h"
#include "PriorityQueue.h"
#include "UndoStack.h"

class AirportSystem
{
private:
    FlightManager flightManager;
    PassengerManager passengerManager;
    CheckInQueue checkInQueue;
    BoardingQueue boardingQueue;
    PriorityQueue priorityQueue;
    UndoStack undoStack;

public:
    FlightManager& getFlightManager();
    PassengerManager& getPassengerManager();
    CheckInQueue& getCheckInQueue();
    BoardingQueue& getBoardingQueue();
    PriorityQueue& getPriorityQueue();
    UndoStack& getUndoStack();
};

#endif