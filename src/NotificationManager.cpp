#include "NotificationManager.h"
#include <iostream>
#include <cctype>

using namespace std;
namespace
{
    // Convert a valid HH:MM time into minutes after midnight.
    bool parseTimeToMinutes(const string& time,int& totalMinutes)
    {
        if (time.size() != 5 || time[2] != ':') // Accepts time in five characters in HH:MM format.
        {
            return false;
        }

        for (int i = 0; i < 5; i++)
        {
            if (i == 2) // check for ':' 
                continue; 

            if (!isdigit(static_cast<unsigned char>(time[i]))) //check for digits
            {
                return false;
            }
        }

        int hours =
            (time[0] - '0') * 10 + (time[1] - '0');

        int minutes =
            (time[3] - '0') * 10 + (time[4] - '0');

        if (hours > 23 || minutes > 59)
        {
            return false;
        }

        totalMinutes = hours * 60 + minutes; //convert in minutes

        return true;
    }
}
NotificationManager::Node::Node(
    int id, 
    const string& pId,
    const string& pName,
    const string& fNumber,
    NotificationType notificationType,
    const string& notificationMessage)
    : notificationId(id),
      passengerId(pId),
      passengerName(pName),
      flightNumber(fNumber),
      type(notificationType),
      message(notificationMessage),
      isRead(false),
      next(nullptr)
{}

NotificationManager::NotificationManager()
{
    head = nullptr;
    tail = nullptr;
    size = 0;
    nextNotificationId = 1;
}

NotificationManager::~NotificationManager() //destructor
{
    Node* current = head;

    while (current != nullptr)
    {
        Node* temp = current;
        current = current->next;
        delete temp;
    }

    head = nullptr;
    tail = nullptr;
    size = 0;
}

bool NotificationManager::hasRequiredDetails(
    const string& passengerId,
    const string& passengerName,
    const string& flightNumber)
{
    return !passengerId.empty() &&
           !passengerName.empty() &&
           !flightNumber.empty();
}

void NotificationManager::addNotification(
    const string& passengerId,
    const string& passengerName,
    const string& flightNumber,
    NotificationType type,
    const string& message)
{
    if (!hasRequiredDetails(passengerId, passengerName, flightNumber))
    {
        cout << "Notification not created. Passenger ID, name, and flight number are required.\n";
        return;
    }

    Node* newNode = new Node(nextNotificationId++, passengerId,passengerName, flightNumber, type, message);

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

    cout << "\n[Notification Created]\n";
    cout << "To      : " << passengerName << " (ID: " << passengerId << ")\n";
    cout << "Flight  : " << flightNumber << "\n";
    cout << "Type    : " << getTypeName(type) << "\n"; // converted to human readable
    cout << "Message : " << message << "\n";
}

string NotificationManager::getTypeName(NotificationType type) //enum class 
{
    switch (type)
    {
        case NotificationType::CheckInReminder:
            return "Check-in Reminder";
        case NotificationType::BoardingReminder:
            return "Boarding Reminder";
        case NotificationType::GateChange:
            return "Gate Change";
        case NotificationType::FlightStatus:
            return "Flight Status Update";
        case NotificationType::Delay:
            return "Delay Alert";
        case NotificationType::Cancellation:
            return "Cancellation Alert";
        case NotificationType::ConnectionRisk:
            return "Connection Risk Alert";
        default:
            return "General Notification";
    }
}

void NotificationManager::sendCheckInReminder(
    const string& passengerId,
    const string& passengerName,
    const string& flightNumber) //receives details
{
    const string message = "Hello " + passengerName +
        ", check-in is open for flight " + flightNumber +
        ". Please proceed to the check-in counter.";

    addNotification(passengerId, passengerName, flightNumber,
                    NotificationType::CheckInReminder, message);
}

void NotificationManager::sendBoardingReminder(
    const string& passengerId,
    const string& passengerName,
    const string& flightNumber,
    const string& gate)
{
    string message = "Hello " + passengerName +
        ", boarding for flight " + flightNumber + " is starting.";

    if (!gate.empty())
    {
        message += " Please proceed to gate " + gate + ".";
    }
    else
    {
        message += " Please check the airport displays for your gate.";
    }

    addNotification(passengerId, passengerName, flightNumber,
                    NotificationType::BoardingReminder, message);
}

void NotificationManager::notifyGateChange(const string& passengerId,const string& passengerName,
    const string& flightNumber,
    const string& oldGate,
    const string& newGate)
{
    if (newGate.empty())
    {
        cout << "Gate-change notification not created: new gate is required.\n";
        return;
    }

    string message = "Hello " + passengerName + ", the gate for flight " +
                     flightNumber + " has changed from " +
                     (oldGate.empty() ? string("unassigned") : oldGate) +
                     " to " + newGate + ". Please proceed to the updated gate.";//conditional operator

    addNotification(passengerId, passengerName, flightNumber,NotificationType::GateChange, message);
}

void NotificationManager::notifyFlightStatus(const string& passengerId,const string& passengerName,
    const string& flightNumber,
    const string& status)
{
    if (status.empty())
    {
        cout << "Flight-status notification not created: status is required.\n";
        return;
    }

    string message = "Hello " + passengerName + ", the current status of flight " +
                     flightNumber + " is: " + status + ".";

    addNotification(passengerId, passengerName, flightNumber,NotificationType::FlightStatus, message);
}

void NotificationManager::notifyDelay(
    const string& passengerId,
    const string& passengerName,
    const string& flightNumber,
    const string& revisedDeparture,
    const string& reason)
{
    string message = "Hello " + passengerName + ", flight " + flightNumber +
                     " has been delayed.";

    if (!revisedDeparture.empty())
    {
        message += " Revised departure time: " + revisedDeparture + ".";
    }

    if (!reason.empty())
    {
        message += " Reason: " + reason + ".";
    }

    message += " Please follow airport announcements for further updates.";

    addNotification(passengerId, passengerName, flightNumber,NotificationType::Delay, message);
}

void NotificationManager::notifyCancellation(
    const string& passengerId,
    const string& passengerName,
    const string& flightNumber,
    const string& reason)
{
    string message = "Hello " + passengerName + ", flight " + flightNumber +
                     " has been cancelled.";

    if (!reason.empty())
    {
        message += " Details: " + reason + ".";
    }

    message += " Please contact the airline or service desk for rebooking assistance.";

    addNotification(passengerId, passengerName, flightNumber,
                    NotificationType::Cancellation, message);
}


bool NotificationManager::notifyConnectionRisk(
    const string& passengerId,
    const string& passengerName,
    const string& arrivingFlightNumber,
    const string& arrivalTime,
    int transferMinutes,
    const string& connectingFlightNumber,
    const string& nextDepartureTime,
    int minimumBufferMinutes)
{
    // Step 1: Validate required passenger and flight details.
    if (!hasRequiredDetails(passengerId, passengerName, arrivingFlightNumber) ||
        connectingFlightNumber.empty())
    {
        cout << "Connection-risk check failed: "
             << "passenger details and both flight numbers are required.\n";
        return false;
    }

    // Step 2: Validate transfer duration and configured threshold.
    if (transferMinutes < 0 || minimumBufferMinutes < 0)//estimated time needed to transfer between flights.
    {
        cout << "Connection-risk check failed: "
             << "transfer time and minimum buffer cannot be negative.\n";
        return false;
    }

    // Step 3: Convert both clock times into minutes.
    int arrivalMinutes;
    int departureMinutes;

    if (!parseTimeToMinutes(arrivalTime, arrivalMinutes) ||
        !parseTimeToMinutes(nextDepartureTime, departureMinutes))
    {
        cout << "Connection-risk check failed: "
             << "times must use valid HH:MM format.\n";
        return false;
    }

    // Step 4: Treat an earlier next-departure clock time
    // as a departure on the following day.
    if (departureMinutes < arrivalMinutes)
    {
        departureMinutes += 24 * 60; //midnight departure
    }

    // Step 5: Calculate when the passenger finishes transferring.
    const long long transferCompleteMinutes =
        static_cast<long long>(arrivalMinutes) + transferMinutes;

    // Step 6: Calculate the remaining buffer before departure.
    const long long availableBufferMinutes =
        static_cast<long long>(departureMinutes) -
        transferCompleteMinutes;

    // Step 7: No alert is needed if the buffer meets the threshold.
    if (availableBufferMinutes >= minimumBufferMinutes)
    {
        cout << "\n[Connection Check]\n";
        cout << "No connection-risk alert is required.\n";
        cout << "Available buffer after transfer: "
             << availableBufferMinutes << " minute(s).\n";

        return false;
    }

    // Step 8: Construct the personalized risk message.
    string message =
        "Hello " + passengerName +
        ", a tight connection has been detected between arriving flight " +
        arrivingFlightNumber +
        " and connecting flight " + connectingFlightNumber + ".";

    message += " Arrival time: " + arrivalTime + ".";
    message += " Estimated transfer time: " +
               to_string(transferMinutes) + " minute(s).";
    message += " Next departure: " + nextDepartureTime + ".";

    if (availableBufferMinutes < 0)
    {
        message += " The estimated transfer would finish " + to_string(-availableBufferMinutes) +
                   " minute(s) after the scheduled departure time.";
    }
    else
    {
        message += " Only " + to_string(availableBufferMinutes) +
                   " minute(s) remain after the estimated transfer.";
    }

    message += " Minimum required buffer: " +
               to_string(minimumBufferMinutes) + " minute(s).";

    message +=
        " Please contact the airline or service desk for connection assistance.";

    // Step 9: Store the notification against the connecting flight.
    addNotification(passengerId,passengerName,connectingFlightNumber,NotificationType::ConnectionRisk,message);

    return true;
}

void NotificationManager::displayPassengerNotifications(const string& passengerId) const
{
    if (passengerId.empty())
    {
        cout << "Passenger ID is required.\n";
        return;
    }

    bool found = false;
    Node* current = head;

    cout << "\n========== Passenger Notification History ==========" << endl;

    while (current != nullptr)
    {
        if (current->passengerId == passengerId)
        {
            found = true;
            cout << "\nNotification ID : " << current->notificationId << endl;
            cout << "Passenger       : " << current->passengerName << endl;
            cout << "Flight          : " << current->flightNumber << endl;
            cout << "Type            : " << getTypeName(current->type) << endl;
            cout << "Message         : " << current->message << endl;
            cout << "Read Status     : " << (current->isRead ? "Read" : "Unread") << endl;
        }

        current = current->next;
    }

    if (!found)
    {
        cout << "No notifications found for passenger ID " << passengerId << ".\n";
    }
}

void NotificationManager::displayAllNotifications() const
{
    if (isEmpty())
    {
        cout << "\nNo notifications have been created yet.\n";
        return;
    }

    cout << "\n========== All Passenger Notifications ==========" << endl;

    Node* current = head;

    while (current != nullptr)
    {
        cout << "\nNotification ID : " << current->notificationId << endl;
        cout << "Passenger       : " << current->passengerName
             << " (ID: " << current->passengerId << ")" << endl;
        cout << "Flight          : " << current->flightNumber << endl;
        cout << "Type            : " << getTypeName(current->type) << endl;
        cout << "Message         : " << current->message << endl;
        cout << "Read Status     : " << (current->isRead ? "Read" : "Unread") << endl;
        cout << "-----------------------------------------------\n";
        current = current->next;
    }
}

void NotificationManager::displayUnreadNotifications(const string& passengerId) const
{
    if (passengerId.empty())
    {
        cout << "Passenger ID is required.\n";
        return;
    }

    bool found = false;
    Node* current = head;

    cout << "\n========== Unread Notifications ==========" << endl;

    while (current != nullptr)
    {
        if (current->passengerId == passengerId && !current->isRead)
        {
            found = true;
            cout << "\nNotification ID : " << current->notificationId << endl;
            cout << "Flight          : " << current->flightNumber << endl;
            cout << "Type            : " << getTypeName(current->type) << endl;
            cout << "Message         : " << current->message << endl;
        }

        current = current->next;
    }

    if (!found)
    {
        cout << "No unread notifications for passenger ID " << passengerId << ".\n";
    }
}

void NotificationManager::markPassengerNotificationsRead(const string& passengerId)
{
    if (passengerId.empty())
    {
        cout << "Passenger ID is required.\n";
        return;
    }

    int markedCount = 0;
    Node* current = head;

    while (current != nullptr)
    {
        if (current->passengerId == passengerId && !current->isRead)
        {
            current->isRead = true;
            markedCount++;
        }

        current = current->next;
    }

    if (markedCount == 0)
    {
        cout << "No unread notifications to mark for passenger ID "<< passengerId << ".\n";
    }
    else
    {
        cout << markedCount << " notification(s) marked as read for passenger ID "<< passengerId << ".\n";
    }
}

bool NotificationManager::isEmpty() const
{
    return head == nullptr;
}

int NotificationManager::getSize() const
{
    return size;
}
