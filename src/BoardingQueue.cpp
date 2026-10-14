#include "BoardingQueue.h"
#include "PassengerManager.h"
#include "NotificationManager.h"
#include "FlightManager.h"
#include <iostream>

using namespace std;

BoardingQueue::BoardingQueue()
{
    front = nullptr;
    rear = nullptr;
    size = 0;
}

BoardingQueue::~BoardingQueue()
{
    while (front != nullptr)
    {
        Node* temp = front;
        front = front->next;
        delete temp;
    }

    rear = nullptr;
    size = 0;
}

bool BoardingQueue::isEmpty() const
{
    return front == nullptr;
}

// Enqueue a registered passenger and generate a boarding reminder
void BoardingQueue::enqueue(PassengerManager& passengerManager, FlightManager& flightManager,
    NotificationManager& notificationManager)
{
    string id, name, gender, phone;
    string flight, seat, assistance, status, gate;
    int age;

    cout << "\n========== Boarding Queue ==========\n";
    cout << "Enter Registered Passenger ID: ";
    getline(cin >> ws, id);

    // Prevent duplicate entries in the boarding queue
    Node* current = front;

    while (current != nullptr)
    {
        if (current->passenger.getPassengerId() == id)
        {
            cout << "Passenger is already in the boarding queue.\n";
            return;
        }

        current = current->next;
    }

    // Retrieve details from Passenger Management
    if (!passengerManager.getPassengerQueueDetails(
            id, name, age, gender, phone, flight,
            seat, assistance, status))
    {
        cout << "Passenger not found in Passenger Management.\n";
        cout << "Please register the passenger first.\n";
        return;
    }
    // Verify that the passenger's flight exists
    string flightStatus;

    if (!flightManager.getFlightStatus(flight, flightStatus))
    {
        cout << "Flight not found. Passenger cannot enter the queue.\n";
        return;
    }

    // Normalize status before checking cancellation
    for (char& ch : flightStatus)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

     if (flightStatus == "cancelled" ||
    flightStatus == "canceled")
    {
        cout << "Boarding denied: this flight has been cancelled.\n";
        return;
    }
    // Only checked-in passengers can enter the boarding queue
    string normalizedStatus = status;

    for (char& ch : normalizedStatus)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    if (normalizedStatus != "checked-in" &&normalizedStatus != "checked in")
    {
        cout << "Boarding denied: passenger must complete check-in first.\n";
        cout << "Current passenger status: " << status << "\n";
        return;
    }

    cout << "Enter Boarding Gate: ";
    getline(cin >> ws, gate);

    Passenger passenger(
        id, name, age, gender, phone,
        flight, seat, assistance, status);

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

    cout << "Passenger added to boarding queue successfully.\n";

    // Create the personalized boarding notification
    notificationManager.sendBoardingReminder(id, name, flight, gate);
}

void BoardingQueue::dequeue(PassengerManager& passengerManager)
{
    if (isEmpty())
    {
        cout << "\nBoarding queue is empty.\n";
        return;
    }

    Node* temp = front;

    cout << "\n========== Boarding Passenger ==========\n";
    cout << "Passenger boarding now:\n";
    temp->passenger.display();

    string passengerId = temp->passenger.getPassengerId();

    // Update the original passenger record
    bool updated = passengerManager.updatePassengerStatus(
        passengerId, "Boarded");

    // Remove passenger from the boarding queue
    front = front->next;

    if (front == nullptr)
    {
        rear = nullptr;
    }

    delete temp;
    size--;

    if (updated)
    {
        cout << "\nPassenger has boarded successfully.\n";
        cout << "Passenger status updated to Boarded.\n";
    }
    else
    {
        cout << "\nQueue entry processed, but the passenger record "
             << "was not found.\n";
    }
}

void BoardingQueue::displayQueue()
{
    if (isEmpty())
    {
        cout << "\nBoarding queue is empty.\n";
        return;
    }

    cout << "\n========== Current Boarding Queue ==========\n";
    cout << "Passengers waiting: " << size << "\n\n";

    Node* current = front;
    int position = 1;

    while (current != nullptr)
    {
        cout << "Position " << position << ":\n";

        current->passenger.display();

        cout << "--------------------------------------\n";

        current = current->next;
        position++;
    }
}
int BoardingQueue::removePassengersByFlightNumber(
    const std::string& flightNumber)
{
    Node* current = front;
    Node* previous = nullptr;
    int removedCount = 0;

    while (current != nullptr)
    {
        if (current->passenger.getFlightNumber() == flightNumber)
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