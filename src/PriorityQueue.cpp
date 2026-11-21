#include "PriorityQueue.h"
#include "AeroAssist.h"
#include "PassengerManager.h"
#include "FlightManager.h"
#include <cctype>
#include <limits>
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


// Check whether a passenger already has an active request.
bool PriorityQueue::containsPassenger(
    const std::string& passengerId
) const
{
    Node* current = front;

    while (current != nullptr)
    {
        if (current->passenger.getPassengerId() == passengerId)
            return true;

        current = current->next;
    }

    return false;
}


// Insert according to priority.
// Lower priority numbers represent higher priority.
// Equal priorities retain FIFO ordering.
bool PriorityQueue::enqueueRecord(
    const Passenger& passenger,
    int priority,
    const std::string& reason
)
{
    if (passenger.getPassengerId().empty() ||
        priority < 1 || priority > 5 ||
        containsPassenger(passenger.getPassengerId()))
    {
        return false;
    }

    Node* newNode = new Node(passenger, priority, reason);

    if (front == nullptr || priority < front->priority)
    {
        newNode->next = front;
        front = newNode;
    }
    else
    {
        Node* current = front;

        while (current->next != nullptr &&
               current->next->priority <= priority)
        {
            current = current->next;
        }

        newNode->next = current->next;
        current->next = newNode;
    }

    ++size;
    return true;
}


// Remove the highest-priority passenger.
bool PriorityQueue::dequeueRecord(
    Passenger& passenger,
    int& priority,
    std::string& reason
)
{
    if (isEmpty())
        return false;

    Node* temp = front;

    passenger = temp->passenger;
    priority = temp->priority;
    reason = temp->reason;

    front = front->next;

    delete temp;
    --size;

    return true;
}


// Return the number of active assistance requests.
std::size_t PriorityQueue::getQueueSize() const
{
    return size;
}


// Retrieve a request at a specified position.
bool PriorityQueue::getPassengerAt(
    std::size_t index,
    Passenger& passenger,
    int& priority,
    std::string& reason
) const
{
    if (index >= size)
        return false;

    Node* current = front;

    for (std::size_t i = 0; i < index; ++i)
        current = current->next;

    passenger = current->passenger;
    priority = current->priority;
    reason = current->reason;

    return true;
}

// Add passenger according to priority
// Add a registered passenger according to priority.
void PriorityQueue::enqueue(
    PassengerManager& passengerManager,
    FlightManager& flightManager)
{
    string id;
    string name, gender, phone;
    string flight, seat, assistance, status;
    string flightStatus;
    int age;
    int priority;

    cout << "\n========== Priority Assistance ==========\n";
    cout << "Enter Registered Passenger ID: ";
    getline(cin >> ws, id);

    // Step 1: Prevent duplicate entries.
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

    // Step 2: Retrieve the passenger's actual registered details.
    if (!passengerManager.getPassengerQueueDetails(
            id, name, age, gender, phone,
            flight, seat, assistance, status))
    {
        cout << "Passenger not found in Passenger Management.\n";
        cout << "Please register the passenger first.\n";
        return;
    }

    // Step 3: Verify that the passenger's flight exists.
    if (!flightManager.getFlightStatus(flight, flightStatus))
    {
        cout << "Request denied: the passenger's flight does not exist.\n";
        return;
    }

    // Step 4: Normalize flight status for comparison.
    for (char& ch : flightStatus)
    {
        ch = static_cast<char>(
            tolower(static_cast<unsigned char>(ch)));
    }

    // Step 5: Reject cancelled flights.
    if (flightStatus == "cancelled" ||
        flightStatus == "canceled")
    {
        cout << "Request denied: the passenger's flight has been cancelled.\n";
        return;
    }

    // Step 6: Get a priority recommendation from AeroAssist.
    AeroAssistAdvice advice = AeroAssist::getAdvice(assistance);

    cout << "\n========== AeroAssist Recommendation ==========\n";
    cout << "Passenger Name     : " << name << '\n';
    cout << "Flight Number      : " << flight << '\n';
    cout << "Suggested Category : " << advice.category << '\n';
    cout << "Suggested Priority : " << advice.priority
         << " (1 = Highest)\n";
    cout << "Support Team       : " << advice.supportTeam << '\n';
    cout << "Recommended Action : "
         << advice.recommendedAction << '\n';

    // Step 7: Accept or override the recommendation.
    char choice;

    cout << "\nUse AeroAssist's recommended priority? (Y/N): ";

    while (!(cin >> choice) ||
           (choice != 'Y' && choice != 'y' &&
            choice != 'N' && choice != 'n'))
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        cout << "Enter Y or N: ";
    }

    if (choice == 'Y' || choice == 'y')
    {
        priority = advice.priority;
        cout << "Recommended priority selected.\n";
    }
    else
    {
        cout << "\nSelect Priority:\n";
        cout << "1. Emergency\n";
        cout << "2. Medical Assistance\n";
        cout << "3. Wheelchair / Accessibility\n";
        cout << "4. VIP\n";
        cout << "5. Normal\n";
        cout << "Enter priority: ";

        while (!(cin >> priority) ||
               priority < 1 || priority > 5)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            cout << "Invalid priority. Enter a value from 1 to 5: ";
        }
    }

    // Step 8: Create the queue entry using registered details.
    Passenger p(
        id, name, age, gender, phone,
        flight, seat, assistance, status);

    Node* newNode = new Node(p, priority);

    // Insert before the first passenger with lower priority.
    // Equal-priority requests retain FIFO order.
    if (front == nullptr || priority < front->priority)
    {
        newNode->next = front;
        front = newNode;
    }
    else
    {
        current = front;

        while (current->next != nullptr &&
               current->next->priority <= priority)
        {
            current = current->next;
        }

        newNode->next = current->next;
        current->next = newNode;
    }

    size++;

    cout << "\nPassenger added to priority queue successfully.\n";
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
int PriorityQueue::removePassengersByFlightNumber(const std::string& flightNumber)
{
    if (flightNumber.empty())
        return 0;

    // Compare flight numbers without case sensitivity.
    auto normalize = [](std::string value)//lambda function
    {
        for (char& ch : value)
        {
            ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
        }
        return value;
    };

    const std::string target = normalize(flightNumber);

    Node* current = front;
    Node* previous = nullptr;
    int removedCount = 0;

    while (current != nullptr)
    {
        if (normalize(current->passenger.getFlightNumber())== target)
        {
            Node* temp = current;
            current = current->next;

            if (previous == nullptr)
                front = current;
            else
                previous->next = current;

            delete temp;
            --size;
            ++removedCount;
        }
        else
        {
            previous = current;
            current = current->next;
        }
    }

    return removedCount;
}

 
   
