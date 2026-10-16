#include "BoardingQueue.h"
#include "PassengerManager.h"
#include "NotificationManager.h"
#include "FlightManager.h"
#include <iostream>
#include <cctype>

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
// Normal method: ask for a passenger ID.
void BoardingQueue::enqueue(
    PassengerManager& passengerManager,
    FlightManager& flightManager,
    NotificationManager& notificationManager)
{
    string id;

    cout << "\n========== Boarding Queue ==========\n";
    cout << "Enter Passenger ID: ";
    getline(cin >> ws, id);

    enqueuePassengerById(
        id, passengerManager, flightManager, notificationManager);
}


// Add a specific passenger without asking for their ID again.
void BoardingQueue::enqueuePassengerById(const string& id, PassengerManager& passengerManager,
    FlightManager& flightManager,
    NotificationManager& notificationManager)
{
    string name, gender, phone;
    string flight, seat, assistance, status, gate;
    int age;

    // 1. Prevent duplicate entries.
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

    // 2. Retrieve the passenger's latest record.
    if (!passengerManager.getPassengerQueueDetails(
            id, name, age, gender, phone,
            flight, seat, assistance, status))
    {
        cout << "Passenger not found.\n";
        return;
    }

    // 3. Verify that the flight exists.
    string flightStatus;

    if (!flightManager.getFlightStatus(flight, flightStatus))
    {
        cout << "Boarding denied: flight not found.\n";
        return;
    }

    // Normalize flight status.
    for (char& ch : flightStatus)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    if (flightStatus == "cancelled" ||flightStatus == "canceled")
    {
        cout << "Boarding denied: flight is cancelled.\n";
        return;
    }

    // 4. Only security-cleared passengers may enter.
    string normalizedStatus = status;

    for (char& ch : normalizedStatus)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    if (normalizedStatus != "security-cleared" &&
        normalizedStatus != "security cleared")
    {
        cout << "Boarding denied: security clearance is required.\n";
        cout << "Current status: " << status << "\n";
        return;
    }

    // 5. Obtain the boarding gate.
    cout << "Enter Boarding Gate: ";
    getline(cin >> ws, gate);

    // 6. Add passenger to the linked-list queue.
    Passenger passenger(
        id, name, age, gender, phone,
        flight, seat, assistance, status);

    Node* newNode = new Node(passenger);

    if (front == nullptr)
    {
        front = rear = newNode;
    }
    else
    {
        rear->next = newNode;
        rear = newNode;
    }

    size++;

    cout << "\nPassenger added to boarding queue successfully.\n";

    // 7. Send the boarding reminder.
    notificationManager.sendBoardingReminder(
        id, name, flight, gate);
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
int BoardingQueue::removePassengersByFlightNumber(const std::string& flightNumber)
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
                front = current;    //case for first node
            else
                previous->next = current; //case for middle or last node

            if (temp == rear)
                rear = previous; //updating rear

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
