#include "AirportSystem.h"

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