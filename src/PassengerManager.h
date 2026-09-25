#ifndef PASSENGERMANAGER_H
#define PASSENGERMANAGER_H

#include "Passenger.h"
#include <cstddef>

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
    size_t size;

public:

    PassengerManager();
    ~PassengerManager();

    void addPassenger();
    void displayPassengers();
    void searchPassenger();
    void updatePassenger();
    void deletePassenger();
};

#endif