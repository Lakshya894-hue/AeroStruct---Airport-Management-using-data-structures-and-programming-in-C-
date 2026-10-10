#ifndef PASSENGERMANAGER_H
#define PASSENGERMANAGER_H

#include "Passenger.h"
#include <cstddef>
class UndoStack;
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

    void addPassenger();
    void displayPassengers();
    void searchPassenger();
    void updatePassenger();
    void deletePassenger(UndoStack& undoStack);

    bool restorePassenger(const Passenger& passenger,
                      std::size_t position);
};

#endif