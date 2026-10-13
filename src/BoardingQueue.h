#ifndef BOARDINGQUEUE_H
#define BOARDINGQUEUE_H

#include "Passenger.h"
#include <cstddef>
class PassengerManager;
class NotificationManager;
class PassengerManager;

class BoardingQueue
{
private:
    struct Node
    {
        Passenger passenger;
        Node* next;

        Node(const Passenger& p): passenger(p), next(nullptr){}
    };

    Node* front;
    Node* rear;
    std::size_t size;

public:
    BoardingQueue();
    ~BoardingQueue();

    void enqueue(PassengerManager& passengerManager, NotificationManager& notificationManager);
    void dequeue(PassengerManager& passengerManager);
    void displayQueue();

    bool isEmpty() const;
};

#endif