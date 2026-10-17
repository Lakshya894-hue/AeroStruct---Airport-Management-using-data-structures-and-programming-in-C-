#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include "Passenger.h"
#include <string>
#include <cstddef>

class PassengerManager;
class FlightManager;

class PriorityQueue
{
private:
    struct Node
    {
        Passenger passenger;
        int priority;
        Node* next;

        Node(const Passenger& p, int pr)
            : passenger(p), priority(pr), next(nullptr) {}
    };

    Node* front;
    std::size_t size;

public:
    PriorityQueue();
    ~PriorityQueue();

    void enqueue(
        PassengerManager& passengerManager,
        FlightManager& flightManager);

    void dequeue();
    void displayQueue();

    bool isEmpty() const;
    int removePassengersByFlightNumber(const std::string& flightNumber);
};

#endif
