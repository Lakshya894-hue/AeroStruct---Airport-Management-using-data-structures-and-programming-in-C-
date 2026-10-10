#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include "Passenger.h"
#include <cstddef>

class PriorityQueue
{
private:
    struct Node
    {
        Passenger passenger;
        int priority;
        Node* next;

        Node(const Passenger& p, int pr)
            : passenger(p), priority(pr), next(nullptr){}
    };

    Node* front;
    std::size_t size;

public:
    PriorityQueue();
    ~PriorityQueue();

    void enqueue();
    void dequeue();
    void displayQueue();

    bool isEmpty() const;
};

#endif