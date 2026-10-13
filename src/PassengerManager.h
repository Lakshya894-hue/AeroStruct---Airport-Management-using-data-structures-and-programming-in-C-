#ifndef PASSENGERMANAGER_H
#define PASSENGERMANAGER_H

#include "Passenger.h"
#include <cstddef>
#include <string>
class UndoStack;
class FlightManager;
class NotificationManager;
class PassengerManager
{
private:

    // Node of the Singly Linked List
    struct Node
    {
        Passenger data;
        Node* next;

        Node(const Passenger& passenger): data(passenger), next(nullptr){}
    };

    Node* head;
    Node* tail;
    std::size_t size;

public:

    PassengerManager();
    ~PassengerManager();

    void addPassenger(FlightManager& flightManager);
    void displayPassengers();
    void searchPassenger();
    void updatePassenger(FlightManager& flightManager);
    void deletePassenger(UndoStack& undoStack);

    bool restorePassenger(const Passenger& passenger,std::size_t position);


    bool getPassengerNotificationDetails(const std::string& passengerId, std::string& passengerName,
    std::string& flightNumber) const;
    void notifyFlightCancellation(
    const std::string& flightNumber,
    NotificationManager& notificationManager,
    const std::string& reason = "The flight has been cancelled");

    void notifyFlightDelay(const std::string& flightNumber, NotificationManager& notificationManager,
    const std::string& revisedDeparture,
    const std::string& reason = "Operational reasons");

    void notifyFlightStatusUpdate(const std::string& flightNumber,
    NotificationManager& notificationManager,
    const std::string& status);

    bool getPassengerQueueDetails(const std::string& passengerId, std::string& passengerName, int& age,
    std::string& gender,
    std::string& phoneNumber,
    std::string& flightNumber,
    std::string& seatNumber,
    std::string& assistanceType,
    std::string& status) const;

    bool updatePassengerStatus(
    const std::string& passengerId,
    const std::string& newStatus);
};

#endif