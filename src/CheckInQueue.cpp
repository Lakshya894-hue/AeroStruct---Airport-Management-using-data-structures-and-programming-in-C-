#include "CheckInQueue.h"
#include "PassengerManager.h"
#include "NotificationManager.h"
#include "PassengerManager.h"
#include "FlightManager.h"
#include <cctype>
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
// Enqueue a registered passenger and generate a check-in notification
void CheckInQueue::enqueue(PassengerManager& passengerManager, FlightManager& flightManager,
    NotificationManager& notificationManager)
{
    string id, name, gender, phone;
    string flight, seat, assistance, status;
    int age;

    cout << "\n========== Check-In ==========\n";
    cout << "Enter Registered Passenger ID: ";
    getline(cin >> ws, id);

    // Prevent duplicate entries in the queue
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

    // Retrieve details from Passenger Management
    if (!passengerManager.getPassengerQueueDetails(id, name, age, gender, phone, flight,
            seat, assistance, status))
    {
        cout << "Passenger not found in Passenger Management.\n";
        cout << "Please register the passenger first.\n";
        return;
    }

    // Check whether the passenger's flight is still available
    string flightStatus;

    if (!flightManager.getFlightStatus(flight, flightStatus))
    {
        cout << "Flight not found. Passenger cannot enter the queue.\n";
        return;
    }

    // Normalize flight status for case-insensitive comparison
    for (char& ch : flightStatus)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    if (flightStatus == "cancelled" || flightStatus == "canceled")
    {
        cout << "Check-in denied: this flight has been cancelled.\n";
        return;
    }

    // Prevent passengers from checking in twice
    string normalizedStatus = status;

    for (char& ch : normalizedStatus)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    if (normalizedStatus == "checked-in" ||normalizedStatus == "checked in" ||normalizedStatus == "boarded")
    {
        cout << "Passenger has already checked in or boarded.\n";
        return;
    }

    Passenger passenger(id, name, age, gender, phone,flight, seat, assistance, status);

    Node* newNode = new Node(passenger);

    if (front == nullptr)
    {
        front = newNode;
        rear = newNode;
    }
    else
    {
        rear->next = newNode;
        rear = newNode;
    }

    size++;

    cout << "\nPassenger added to check-in queue successfully.\n";

    // Create the personalized check-in notification
    notificationManager.sendCheckInReminder(
        id, name, flight);
}
// Dequeue Passenger
void CheckInQueue::dequeue(PassengerManager& passengerManager)
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

    string passengerId = temp->data.getPassengerId();

    // Update the original passenger record
    bool updated = passengerManager.updatePassengerStatus(
        passengerId, "Checked-In");

    // Remove passenger from the queue
    front = front->next;

    if (front == nullptr)
    {
        rear = nullptr;
    }

    delete temp;
    size--;

    if (updated)
    {
        cout << "\nCheck-in completed successfully.\n";
        cout << "Passenger status updated to Checked-In.\n";
    }
    else
    {
        cout << "\nQueue entry processed, but the passenger record "
             << "was not found.\n";
    }
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
int CheckInQueue::removePassengersByFlightNumber(const std::string& flightNumber)
{
    Node* current = front;
    Node* previous = nullptr;
    int removedCount = 0;

    while (current != nullptr)
    {
        if (current->data.getFlightNumber() == flightNumber)
        {
            Node* temp = current;
            current = current->next;

            if (previous == nullptr)
                front = current;
            else
                previous->next = current;

            if (temp == rear)
                rear = previous;

            delete temp;
            size--;
            removedCount++;
        }
        else
        {
            previous = current;
            current = current->next;
        }
    }

    if (front == nullptr)
        rear = nullptr;

    return removedCount;
}