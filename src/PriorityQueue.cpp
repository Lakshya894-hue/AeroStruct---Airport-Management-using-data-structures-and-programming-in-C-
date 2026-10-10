#include "PriorityQueue.h"
#include <iostream>

using namespace std;

// Constructor
PriorityQueue::PriorityQueue()
{
    front = nullptr;
    size = 0;
}

// Destructor
PriorityQueue::~PriorityQueue()
{
    while (front != nullptr)
    {
        Node* temp = front;
        front = front->next;
        delete temp;
    }

    size = 0;
}

// Check if queue is empty
bool PriorityQueue::isEmpty() const
{
    return front == nullptr;
}

// Add passenger according to priority
void PriorityQueue::enqueue()
{
    string id;
    string name;
    int age;
    string gender;
    string phone;
    string flight;
    string seat;
    string assistance;
    string status;
    int priority;

    cout << "\n========== Priority Assistance ==========\n";

    cout << "Enter Passenger ID: ";
    getline(cin >> ws, id);

    // Check if passenger is already in the queue
    Node* current = front;

    while (current != nullptr)
    {
        if (current->passenger.getPassengerId() == id)
        {
            cout << "Passenger is already in the priority queue.\n";
            return;
        }

        current = current->next;
    }

    cout << "Enter Name: ";
    getline(cin >> ws, name);

    cout << "Enter Age: ";
    cin >> age;

    while (cin.fail() || age < 0)
    {
        cin.clear();
        cin.ignore(1000, '\n');

        cout << "Invalid age. Enter again: ";
        cin >> age;
    }

    cout << "Enter Gender: ";
    getline(cin >> ws, gender);

    cout << "Enter Phone Number: ";
    getline(cin >> ws, phone);

    cout << "Enter Flight Number: ";
    getline(cin >> ws, flight);

    cout << "Enter Seat Number: ";
    getline(cin >> ws, seat);

    cout << "Enter Assistance Type: ";
    getline(cin >> ws, assistance);

    cout << "Enter Status: ";
    getline(cin >> ws, status);

    cout << "\nSelect Priority:\n";
    cout << "1. Emergency\n";
    cout << "2. Medical Assistance\n";
    cout << "3. Wheelchair / Accessibility\n";
    cout << "4. VIP\n";
    cout << "5. Normal\n";
    cout << "Enter priority: ";
    cin >> priority;

    while (cin.fail() || priority < 1 || priority > 5)
    {
        cin.clear();
        cin.ignore(1000, '\n');

        cout << "Invalid priority. Enter a value from 1 to 5: ";
        cin >> priority;
    }

    Passenger p(id, name, age, gender, phone,
                flight, seat, assistance, status);

    Node* newNode = new Node(p, priority);

    // Empty queue
    if (front == nullptr)
    {
        front = newNode;
        size++;

        cout << "\nPassenger added to priority queue.\n";
        return;
    }

    // Insert at front if this passenger has higher priority
    if (priority < front->priority)
    {
        newNode->next = front;
        front = newNode;
        size++;

        cout << "\nPassenger added to priority queue.\n";
        return;
    }

    // Find the correct position
    current = front;

    while (current->next != nullptr &&
           current->next->priority <= priority)
    {
        current = current->next;
    }

    newNode->next = current->next;
    current->next = newNode;

    size++;

    cout << "\nPassenger added to priority queue.\n";
}

// Process highest-priority passenger
void PriorityQueue::dequeue()
{
    if (isEmpty())
    {
        cout << "\nPriority queue is empty.\n";
        return;
    }

    Node* temp = front;

    cout << "\n========== Processing Priority Passenger ==========\n";

    cout << "Passenger being processed:\n";
    temp->passenger.display();

    cout << "Priority: " << temp->priority << endl;

    front = front->next;

    delete temp;
    size--;

    cout << "\nPassenger processed successfully.\n";
}

// Display priority queue
void PriorityQueue::displayQueue()
{
    if (isEmpty())
    {
        cout << "\nPriority queue is empty.\n";
        return;
    }

    cout << "\n========== Priority Queue ==========\n";
    cout << "Passengers waiting: " << size << "\n\n";

    Node* current = front;
    int position = 1;

    while (current != nullptr)
    {
        cout << "Position: " << position << endl;

        cout << "Priority: " << current->priority << endl;

        current->passenger.display();

        cout << "--------------------------------------\n";

        current = current->next;
        position++;
    }
}