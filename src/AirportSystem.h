#ifndef AIRPORTSYSTEM_H
#define AIRPORTSYSTEM_H

#include "FlightManager.h"
#include "PassengerManager.h"
#include "CheckInQueue.h"
#include "BoardingQueue.h"
#include "PriorityQueue.h"
#include "UndoStack.h"
#include "NotificationManager.h"
#include "GroupTravelManager.h"

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
    GroupTravelManager groupTravelManager;

public:
    FlightManager& getFlightManager();
    PassengerManager& getPassengerManager();
    CheckInQueue& getCheckInQueue();
    BoardingQueue& getBoardingQueue();
    PriorityQueue& getPriorityQueue();
    UndoStack& getUndoStack();
    NotificationManager& getNotificationManager();
    GroupTravelManager& getGroupTravelManager();
    AirportSystem();
};

#endif
