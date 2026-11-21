
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
        std::string reason;
        Node* next;

        Node(
            const Passenger& p,
            int pr,
            const std::string& requestReason = ""
        )
            : passenger(p),
              priority(pr),
              reason(requestReason),
              next(nullptr) {}
    };

    Node* front;
    std::size_t size;

public:
    PriorityQueue();
    ~PriorityQueue();

    void enqueue(
        PassengerManager& passengerManager,
        FlightManager& flightManager
    );

    void dequeue();
    void displayQueue();

    bool isEmpty() const;

    int removePassengersByFlightNumber(
        const std::string& flightNumber
    );

    // API-friendly queue operations.
    bool enqueueRecord(
        const Passenger& passenger,
        int priority,
        const std::string& reason
    );

    bool dequeueRecord(
        Passenger& passenger,
        int& priority,
        std::string& reason
    );

    bool containsPassenger(
        const std::string& passengerId
    ) const;

    std::size_t getQueueSize() const;

    bool getPassengerAt(
        std::size_t index,
        Passenger& passenger,
        int& priority,
        std::string& reason
    ) const;
};

#endif
        
          
