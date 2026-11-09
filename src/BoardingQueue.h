
#ifndef BOARDINGQUEUE_H
#define BOARDINGQUEUE_H

#include "Passenger.h"
#include <cstddef>
#include <string>

class PassengerManager;
class NotificationManager;
class FlightManager;

class BoardingQueue
{
private:
    struct Node
    {
        Passenger passenger;
        std::string gate;
        Node* next;

        Node(const Passenger& p, const std::string& g = "")
            : passenger(p), gate(g), next(nullptr) {}
    };

    Node* front;
    Node* rear;
    std::size_t size;

public:
    BoardingQueue();
    ~BoardingQueue();

    void enqueue(
        PassengerManager& passengerManager,
        FlightManager& flightManager,
        NotificationManager& notificationManager
    );

    void dequeue(PassengerManager& passengerManager);
    void displayQueue();

    bool isEmpty() const;

    int removePassengersByFlightNumber(
        const std::string& flightNumber
    );

    void enqueuePassengerById(
        const std::string& id,
        PassengerManager& passengerManager,
        FlightManager& flightManager,
        NotificationManager& notificationManager
    );

    // API-friendly queue operations.
    bool enqueueRecord(
        const Passenger& passenger,
        const std::string& gate
    );

    bool dequeueRecord(
        Passenger& passenger,
        std::string& gate
    );

    bool containsPassenger(
        const std::string& passengerId
    ) const;

    std::size_t getQueueSize() const;

    bool getPassengerAt(
        std::size_t index,
        Passenger& passenger,
        std::string& gate
    ) const;
};

#endif
