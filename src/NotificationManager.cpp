#include "NotificationManager.h"
#include <iostream>

using namespace std;

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
{
}

NotificationManager::NotificationManager()
{
    head = nullptr;
    tail = nullptr;
    size = 0;
    nextNotificationId = 1;
}

NotificationManager::~NotificationManager()
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

    Node* newNode = new Node(nextNotificationId++, passengerId,
                             passengerName, flightNumber, type, message);

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
    cout << "Type    : " << getTypeName(type) << "\n";
    cout << "Message : " << message << "\n";
}

string NotificationManager::getTypeName(NotificationType type)
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
        default:
            return "General Notification";
    }
}

void NotificationManager::sendCheckInReminder(
    const string& passengerId,
    const string& passengerName,
    const string& flightNumber)
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
                     " to " + newGate + ". Please proceed to the updated gate.";

    addNotification(passengerId, passengerName, flightNumber,
                    NotificationType::GateChange, message);
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

    addNotification(passengerId, passengerName, flightNumber,
                    NotificationType::FlightStatus, message);
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

    addNotification(passengerId, passengerName, flightNumber,
                    NotificationType::Delay, message);
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

void NotificationManager::displayPassengerNotifications(
    const string& passengerId) const
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

void NotificationManager::displayUnreadNotifications(
    const string& passengerId) const
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

void NotificationManager::markPassengerNotificationsRead(
    const string& passengerId)
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
        cout << "No unread notifications to mark for passenger ID "
             << passengerId << ".\n";
    }
    else
    {
        cout << markedCount << " notification(s) marked as read for passenger ID "
             << passengerId << ".\n";
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
