#include "BoardingQueue.h"
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

void BoardingQueue::enqueue()
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

    cout << "\n========== Boarding Queue ==========\n";

    cout << "Enter Passenger ID: ";
    getline(cin >> ws, id);

    // Check if passenger is already waiting for boarding
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

    Passenger p(id, name, age, gender, phone,
                flight, seat, assistance, status);

    Node* newNode = new Node(p);

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

    cout << "Passenger added to boarding queue.\n";
}

void BoardingQueue::dequeue()
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

    front = front->next;

    if (front == nullptr)
    {
        rear = nullptr;
    }

    delete temp;
    size--;

    cout << "\nPassenger has boarded successfully.\n";
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