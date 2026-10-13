#ifndef AIRPORTSYSTEM_H
#define AIRPORTSYSTEM_H

#include "FlightManager.h"
#include "PassengerManager.h"
#include "CheckInQueue.h"
#include "BoardingQueue.h"
#include "PriorityQueue.h"
#include "UndoStack.h"
#include "NotificationManager.h"

class AirportSystem
{
private:
    FlightManager flightManager;
    PassengerManager passengerManager;
    CheckInQueue checkInQueue;
    BoardingQueue boardingQueue;
    PriorityQueue priorityQueue;
    UndoStack undoStack;
    NotificationManager notificationManager;

public:
    FlightManager& getFlightManager();
    PassengerManager& getPassengerManager();
    CheckInQueue& getCheckInQueue();
    BoardingQueue& getBoardingQueue();
    PriorityQueue& getPriorityQueue();
    UndoStack& getUndoStack();
    NotificationManager& getNotificationManager();
};

#endif