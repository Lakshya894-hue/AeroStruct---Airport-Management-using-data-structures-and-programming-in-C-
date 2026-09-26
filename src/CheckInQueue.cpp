#include "CheckInQueue.h"
#include <iostream>

using namespace std;

// Constructor
CheckInQueue::CheckInQueue()
{
    front = nullptr;
    rear = nullptr;
    size = 0;
}

// Destructor
CheckInQueue::~CheckInQueue()
{
    Node* current = front;

    while (current != nullptr)
    {
        Node* temp = current;
        current = current->next;
        delete temp;
    }

    front = nullptr;
    rear = nullptr;
    size = 0;
}

// Check if queue is empty
bool CheckInQueue::isEmpty() const
{
    return front == nullptr;
}

// Enqueue Passenger
void CheckInQueue::enqueue()
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

    cout << "\n========== Check-In ==========\n";

    cout << "Enter Passenger ID: ";
    getline(cin >> ws, id);

    // Check whether passenger is already in the queue
    Node* current = front;

    while (current != nullptr)
    {
        if (current->data.getPassengerId() == id)
        {
            cout << "Passenger is already in the check-in queue.\n";
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

    Passenger passenger(id,name,age,gender,
        phone,flight,seat,assistance,status
    );

    Node* newNode = new Node(passenger);

    // If queue is empty
    if (front == nullptr)
    {
        front = newNode;
        rear = newNode;
    }
    else
    {
        // Insert at rear
        rear->next = newNode;
        rear = newNode;
    }

    size++;

    cout << "\nPassenger added to check-in queue successfully.\n";
}

// Dequeue Passenger
void CheckInQueue::dequeue()
{
    if (isEmpty())
    {
        cout << "\nCheck-in queue is empty.\n";
        return;
    }

    Node* temp = front;

    cout << "\n========== Processing Passenger ==========\n";

    cout << "Passenger being processed:\n";
    temp->data.display();

    // Move front to next passenger
    front = front->next;

    // If queue becomes empty
    if (front == nullptr)
    {
        rear = nullptr;
    }

    delete temp;
    size--;

    cout << "\nPassenger processed successfully.\n";
}

// Display Queue
void CheckInQueue::displayQueue()
{
    if (isEmpty())
    {
        cout << "\nCheck-in queue is empty.\n";
        return;
    }

    cout << "\n========== Check-In Queue ==========\n";
    cout << "Total passengers waiting: " << size << "\n\n";

    Node* current = front;
    int position = 1;

    while (current != nullptr)
    {
        cout << "Position: " << position << "\n";

        current->data.display();

        cout << "--------------------------------------\n";

        current = current->next;
        position++;
    }
}