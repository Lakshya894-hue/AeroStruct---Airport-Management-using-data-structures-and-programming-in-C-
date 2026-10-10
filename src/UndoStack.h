#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include "Passenger.h"
#include <cstddef>

class PassengerManager;

class UndoStack
{
private:
    struct Node
    {
        Passenger passenger;
        std::size_t position;
        Node* next;

        Node(const Passenger& p, std::size_t pos)
            : passenger(p), position(pos), next(nullptr)
        {
        }
    };

    Node* top;
    std::size_t size;

public:
    UndoStack();
    ~UndoStack();

    void push(const Passenger& passenger, std::size_t position);
    bool undoDelete(PassengerManager& passengerManager);
    void display();

    bool isEmpty() const;
};

#endif