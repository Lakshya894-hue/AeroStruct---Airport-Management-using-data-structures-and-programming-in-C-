#include "PassengerManager.h"
#include <iostream>

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
void PassengerManager::addPassenger()
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

    cout << "\n========== Add Passenger ==========\n";

    cout << "Enter Passenger ID: ";
    getline(cin >> ws, id);

    // Check for duplicate Passenger ID
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

    cout << "Enter Flight Number: ";
    getline(cin >> ws, flight);

    cout << "Enter Seat Number: ";
    getline(cin >> ws, seat);

    cout << "Enter Assistance Type (None / Wheelchair / Medical / Other): ";
    getline(cin >> ws, assistance);

    cout << "Enter Status: ";
    getline(cin >> ws, status);

    // Create Passenger object
    Passenger passenger(id, name, age, gender, phone,
                        flight, seat, assistance, status);

    // Create new node
    Node* newNode = new Node(passenger);

    // If list is empty
    if (head == nullptr)
    {
        head = newNode;
        tail = newNode;
    }
    else
    {
        // Add at end
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
void PassengerManager::updatePassenger()
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

            cout << "Enter Flight Number: ";
            getline(cin >> ws, flight);

            cout << "Enter Seat Number: ";
            getline(cin >> ws, seat);

            cout << "Enter Assistance Type: ";
            getline(cin >> ws, assistance);

            cout << "Enter Status: ";
            getline(cin >> ws, status);

            // Update passenger data
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
void PassengerManager::deletePassenger()
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

    Node* itr = head;
    Node* previous = nullptr;

    // Search for passenger
    while (itr != nullptr)
    {
        if (itr->data.getPassengerId() == id)
        {
            break;
        }

        previous = itr;
        itr = itr->next;
    }

    // Passenger not found
    if (itr == nullptr)
    {
        cout << "Passenger not found.\n";
        return;
    }

    // Case 1: Deleting first node
    if (itr == head)
    {
        head = head->next;

        // If it was the only node
        if (head == nullptr)
        {
            tail = nullptr;
        }
    }
    else
    {
        // Remove itr node
        previous->next = itr->next;

        // If deleting last node
        if (itr == tail)
        {
            tail = previous;
        }
    }

    delete itr;
    size--;

    cout << "Passenger deleted successfully.\n";
}