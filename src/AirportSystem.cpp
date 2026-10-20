#include "AirportSystem.h"

AirportSystem::AirportSystem()
{
    notificationManager.setFlightManager(flightManager);
    notificationManager.setPassengerManager(passengerManager);
}

FlightManager& AirportSystem::getFlightManager()
{
    return flightManager;
}

PassengerManager& AirportSystem::getPassengerManager()
{
    return passengerManager;
}

CheckInQueue& AirportSystem::getCheckInQueue()
{
    return checkInQueue;
}

BoardingQueue& AirportSystem::getBoardingQueue()
{
    return boardingQueue;
}

PriorityQueue& AirportSystem::getPriorityQueue()
{
    return priorityQueue;
}

UndoStack& AirportSystem::getUndoStack()
{
    return undoStack;
}

NotificationManager& AirportSystem::getNotificationManager()
{
    return notificationManager;
}

GroupTravelManager& AirportSystem::getGroupTravelManager()
{
    return groupTravelManager;
}

BaggageManager& AirportSystem::getBaggageManager()
{
    return baggageManager;
}
