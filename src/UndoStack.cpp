#include "UndoStack.h"
#include "PassengerManager.h"
#include <iostream>

using namespace std;

// Constructor
UndoStack::UndoStack()
{
    top = nullptr;
    size = 0;
}

// Destructor
UndoStack::~UndoStack()
{
    while (top != nullptr)
    {
        Node* temp = top;
        top = top->next;
        delete temp;
    }

    size = 0;
}

// Check whether the stack is empty
bool UndoStack::isEmpty() const
{
    return top == nullptr;
}

// Save deleted passenger on the stack
void UndoStack::push(const Passenger& passenger, std::size_t position)
{
    Node* newNode = new Node(passenger, position);

    newNode->next = top;
    top = newNode;

    size++;
}

// Undo the last passenger deletion
bool UndoStack::undoDelete(PassengerManager& passengerManager)
{
    if (isEmpty())
    {
        cout << "\nThere is no deletion to undo.\n";
        return false;
    }

    // Restore the passenger before removing its saved record
    if (!passengerManager.restorePassenger(
            top->passenger, top->position))
    {
        cout << "\nCould not restore the passenger. "
             << "A passenger with the same ID may already exist.\n";

        return false;
    }

    Node* temp = top;
    top = top->next;

    delete temp;
    size--;

    cout << "\nLast passenger deletion undone successfully.\n";

    return true;
}

// Display passengers saved for undo
void UndoStack::display()
{
    if (isEmpty())
    {
        cout << "\nUndo history is empty.\n";
        return;
    }

    cout << "\n========== Undo History ==========\n";
    cout << "Saved deletions: " << size << "\n\n";

    Node* current = top;
    int count = 1;

    while (current != nullptr)
    {
        cout << count << ". Passenger ID: "
             << current->passenger.getPassengerId() << "\n";

        cout << "   Name: "
             << current->passenger.getName() << "\n";

        cout << "   Original position: "
             << current->position << "\n";

        cout << "----------------------------------\n";

        current = current->next;
        count++;
    }
}