#include "PassengerManager.h"
#include "UndoStack.h"
#include "FlightManager.h"
#include "NotificationManager.h"
#include <iostream>
#include <cctype>

using namespace std;

// Constructor
PassengerManager::PassengerManager()
{
    head = nullptr;
    tail = nullptr;
    size = 0;
}

// Destructor
PassengerManager::~PassengerManager()
{
    Node* itr = head;

    while (itr != nullptr)
    {
        Node* temp = itr;
        itr = itr->next;
        delete temp;
    }

    head = nullptr;
    tail = nullptr;
    size = 0;
}

// Add Passenger

void PassengerManager::addPassenger(FlightManager& flightManager)
{
    string id, name, gender, phone;
    string flight, seat, assistance, status;
    int age;

    cout << "\n========== Add Passenger ==========\n";

    cout << "Enter Passenger ID: ";
    getline(cin >> ws, id);

    // Check for duplicate passenger ID
    Node* itr = head;

    while (itr != nullptr)
    {
        if (itr->data.getPassengerId() == id)
        {
            cout << "Passenger ID already exists.\n";
            return;
        }

        itr = itr->next;
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

    // Validate the passenger's flight
    cout << "Enter Flight Number: ";
    getline(cin >> ws, flight);

    // Check whether the flight exists
    if (!flightManager.flightExists(flight))
    {
        cout << "Flight does not exist. Please add the flight first.\n";
        return;
    }

    // Check whether the flight has been cancelled
    string flightStatus;

    if (!flightManager.getFlightStatus(flight, flightStatus))
    {
        cout << "Unable to retrieve flight status.\n";
        return;
    }

    // Convert status to lowercase for case-insensitive comparison
    for (char& ch : flightStatus)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    if (flightStatus == "cancelled" ||
    flightStatus == "canceled")
    {
        cout << "Passenger registration denied: this flight has been cancelled.\n";
        return;
    }

    cout << "Enter Seat Number: ";
    getline(cin >> ws, seat);

    cout << "Enter Assistance Type (None / Wheelchair / Medical / Other): ";
    getline(cin >> ws, assistance);

    cout << "Enter Status: ";
    getline(cin >> ws, status);

    // Create the Passenger object
    Passenger passenger(id, name, age, gender, phone,
                        flight, seat, assistance, status);

    // Insert the passenger into the linked list
    Node* newNode = new Node(passenger);

    if (head == nullptr)
    {
        head = newNode;
        tail = newNode;
    }
    else
    {
        tail->next = newNode;
        tail = newNode;
    }

    size++;

    cout << "\nPassenger added successfully.\n";
}


// Display All Passengers
void PassengerManager::displayPassengers()
{
    cout << "\n========== Passenger Records ==========\n";

    if (head == nullptr)
    {
        cout << "No passengers available.\n";
        return;
    }

    Node* itr = head;

    cout << "Total Passengers: " << size << "\n\n";

    while (itr != nullptr)
    {
        itr->data.display();
        cout << "--------------------------------------\n";

        itr = itr->next;
    }
}

// Search Passenger
void PassengerManager::searchPassenger()
{
    string id;

    cout << "\n========== Search Passenger ==========\n";

    cout << "Enter Passenger ID: ";
    getline(cin >> ws, id);

    Node* itr = head;

    while (itr != nullptr)
    {
        if (itr->data.getPassengerId() == id)
        {
            cout << "\nPassenger found:\n";
            cout << "--------------------------------------\n";

            itr->data.display();

            return;
        }

        itr = itr->next;
    }

    cout << "Passenger not found.\n";
}

// Update Passenger
void PassengerManager::updatePassenger(FlightManager& flightManager)
{
    string id;

    cout << "\n========== Update Passenger ==========\n";
    cout << "Enter Passenger ID to update: ";
    getline(cin >> ws, id);

    Node* itr = head;

    while (itr != nullptr)
    {
        if (itr->data.getPassengerId() == id)
        {
            string name;
            int age;
            string gender;
            string phone;
            string flight;
            string seat;
            string assistance;
            string status;

            cout << "\nPassenger found.\n";
            cout << "Enter new details:\n\n";

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

            // Validate the flight number
            cout << "Enter Flight Number: ";
            getline(cin >> ws, flight);

            // Validate that the flight exists
            if (!flightManager.flightExists(flight))
            {
                cout << "Flight does not exist. Update cancelled.\n";
                return;
            }

            // Reject cancelled flights      
            string flightStatus;

            if (!flightManager.getFlightStatus(flight, flightStatus))
            {
                cout << "Unable to retrieve flight status. Update cancelled.\n";
                return;
            }

            for (char& ch : flightStatus)
            {
                ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
            }

            if (flightStatus == "cancelled" || flightStatus == "canceled")
            {
                cout << "Update denied: passengers cannot be assigned to a cancelled flight.\n";
                return;
            }

            cout << "Enter Seat Number: ";
            getline(cin >> ws, seat);

            cout << "Enter Assistance Type: ";
            getline(cin >> ws, assistance);

            cout << "Enter Status: ";
            getline(cin >> ws, status);

            // Update passenger data only after validation
            itr->data.setName(name);
            itr->data.setAge(age);
            itr->data.setGender(gender);
            itr->data.setPhoneNumber(phone);
            itr->data.setFlightNumber(flight);
            itr->data.setSeatNumber(seat);
            itr->data.setAssistanceType(assistance);
            itr->data.setStatus(status);

            cout << "Passenger updated successfully.\n";
            return;
        }

        itr = itr->next;
    }

    cout << "Passenger not found.\n";
}

// Delete Passenger
// Delete Passenger and save it for undo
void PassengerManager::deletePassenger(UndoStack& undoStack)
{
    string id;

    cout << "\n========== Delete Passenger ==========\n";
    cout << "Enter Passenger ID to delete: ";
    getline(cin >> ws, id);

    if (head == nullptr)
    {
        cout << "No passengers available.\n";
        return;
    }

    Node* current = head;
    Node* previous = nullptr;
    std::size_t position = 1;

    // Find passenger and remember its position
    while (current != nullptr)
    {
        if (current->data.getPassengerId() == id)
        {
            break;
        }

        previous = current;
        current = current->next;
        position++;
    }

    if (current == nullptr)
    {
        cout << "Passenger not found.\n";
        return;
    }

    // Save the passenger before deleting the node
    undoStack.push(current->data, position);

    // If the first node is being deleted
    if (current == head)
    {
        head = head->next;

        if (head == nullptr)
        {
            tail = nullptr;
        }
    }
    else
    {
        previous->next = current->next;

        // If the last node is being deleted
        if (current == tail)
        {
            tail = previous;
        }
    }

    delete current;
    size--;

    cout << "\nPassenger deleted successfully.\n";
}

// Restore a passenger saved in the Undo Stack
bool PassengerManager::restorePassenger(
    const Passenger& passenger, std::size_t position)
{
    // Avoid duplicate passenger IDs
    Node* current = head;

    while (current != nullptr)
    {
        if (current->data.getPassengerId() == passenger.getPassengerId())
        {
            return false;
        }

        current = current->next;
    }

    Node* newNode = new Node(passenger);

    // Restore at the beginning
    if (head == nullptr || position <= 1)
    {
        newNode->next = head;
        head = newNode;

        if (tail == nullptr)
        {
            tail = newNode;
        }
    }
    else
    {
        Node* current = head;
        std::size_t currentPosition = 1;

        // Stop just before the original position
        while (current->next != nullptr &&
               currentPosition < position - 1)
        {
            current = current->next;
            currentPosition++;
        }

        newNode->next = current->next;
        current->next = newNode;

        if (newNode->next == nullptr)
        {
            tail = newNode;
        }
    }

    size++;

    return true;
}

// Retrieve passenger details for personalized notifications
bool PassengerManager::getPassengerNotificationDetails(
    const std::string& passengerId,
    std::string& passengerName,
    std::string& flightNumber) const
{
    Node* current = head;

    while (current != nullptr)
    {
        if (current->data.getPassengerId() == passengerId)
        {
            passengerName = current->data.getName();
            flightNumber = current->data.getFlightNumber();

            return true;
        }

        current = current->next;
    }

    return false;
}

// Notify all registered passengers affected by a flight cancellation
void PassengerManager::notifyFlightCancellation(
    const std::string& flightNumber,
    NotificationManager& notificationManager,
    const std::string& reason)
{
    Node* current = head;
    int notifiedCount = 0;

    while (current != nullptr)
    {
        Passenger& passenger = current->data;

        // Find passengers booked on the cancelled flight
        if (passenger.getFlightNumber() == flightNumber)
        {
            notificationManager.notifyCancellation(
                passenger.getPassengerId(),
                passenger.getName(),
                flightNumber,
                reason
            );

            notifiedCount++;
        }

        current = current->next;
    }

    if (notifiedCount == 0)
    {
        std::cout << "No registered passengers were found for flight "
                  << flightNumber << ".\n";
    }
    else
    {
        std::cout << "Cancellation notifications created for "
                  << notifiedCount << " passenger(s).\n";
    }
}

// Notify passengers about a flight delay
void PassengerManager::notifyFlightDelay(
    const std::string& flightNumber,
    NotificationManager& notificationManager,
    const std::string& revisedDeparture,
    const std::string& reason)
{
    Node* current = head;
    int notifiedCount = 0;

    while (current != nullptr)
    {
        Passenger& passenger = current->data;

        if (passenger.getFlightNumber() == flightNumber)
        {
            notificationManager.notifyDelay(
                passenger.getPassengerId(),
                passenger.getName(),
                flightNumber,
                revisedDeparture,
                reason
            );

            notifiedCount++;
        }

        current = current->next;
    }

    if (notifiedCount == 0)
    {
        cout << "No registered passengers found for flight "
             << flightNumber << ".\n";
    }
    else
    {
        cout << "Delay notifications created for "
             << notifiedCount << " passenger(s).\n";
    }
}


// Notify passengers about a flight status update
void PassengerManager::notifyFlightStatusUpdate(
    const std::string& flightNumber,
    NotificationManager& notificationManager,
    const std::string& status)
{
    Node* current = head;
    int notifiedCount = 0;

    while (current != nullptr)
    {
        Passenger& passenger = current->data;

        if (passenger.getFlightNumber() == flightNumber)
        {
            notificationManager.notifyFlightStatus(
                passenger.getPassengerId(),
                passenger.getName(),
                flightNumber,
                status
            );

            notifiedCount++;
        }

        current = current->next;
    }

    if (notifiedCount == 0)
    {
        cout << "No registered passengers found for flight "
             << flightNumber << ".\n";
    }
    else
    {
        cout << "Status notifications created for "
             << notifiedCount << " passenger(s).\n";
    }
}

// Retrieve registered passenger details for queue operations
bool PassengerManager::getPassengerQueueDetails(
    const std::string& passengerId,
    std::string& passengerName,
    int& age,
    std::string& gender,
    std::string& phoneNumber,
    std::string& flightNumber,
    std::string& seatNumber,
    std::string& assistanceType,
    std::string& status) const
{
    Node* current = head;

    while (current != nullptr)
    {
        const Passenger& passenger = current->data;

        if (passenger.getPassengerId() == passengerId)
        {
            passengerName = passenger.getName();
            age = passenger.getAge();
            gender = passenger.getGender();
            phoneNumber = passenger.getPhoneNumber();
            flightNumber = passenger.getFlightNumber();
            seatNumber = passenger.getSeatNumber();
            assistanceType = passenger.getAssistanceType();
            status = passenger.getStatus();

            return true;
        }

        current = current->next;
    }

    return false;
}

// Update the status of a registered passenger
bool PassengerManager::updatePassengerStatus(
    const std::string& passengerId,
    const std::string& newStatus)
{
    Node* current = head;

    while (current != nullptr)
    {
        if (current->data.getPassengerId() == passengerId)
        {
            current->data.setStatus(newStatus);
            return true;
        }

        current = current->next;
    }

    return false;
}