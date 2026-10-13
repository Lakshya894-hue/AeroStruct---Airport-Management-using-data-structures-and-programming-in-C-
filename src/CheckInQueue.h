#ifndef CHECKINQUEUE_H
#define CHECKINQUEUE_H

#include "Passenger.h"
#include <cstddef>

class PassengerManager;
class NotificationManager;
class PassengerManager;

class CheckInQueue
{
private:

    struct Node
    {
        Passenger data;
        Node* next;

        Node(const Passenger& passenger)
            : data(passenger), next(nullptr)
        {
        }
    };

    Node* front;
    Node* rear;
    std::size_t size;

public:

    CheckInQueue();
    ~CheckInQueue();

    void enqueue(PassengerManager& passengerManager, NotificationManager& notificationManager);
    void dequeue(PassengerManager& passengerManager);
    void displayQueue();

    bool isEmpty() const;
};

#endif