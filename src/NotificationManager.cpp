#include "NotificationManager.h"
#include <limits>
#include <iostream>
#include <cctype>
#include "FlightManager.h"
#include "PassengerManager.h"
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

        int hours = (time[0] - '0') * 10 + (time[1] - '0');

        int minutes = (time[3] - '0') * 10 + (time[4] - '0');

        if (hours > 23 || minutes > 59)
        {
            return false;
        }

        totalMinutes = hours * 60 + minutes; //convert in minutes

        return true;
    }
}
NotificationManager::Node::Node(long long id, const string& pId, const string& pName,const string& fNumber,
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

    flightManager = nullptr;
    passengerManager = nullptr;
}
void NotificationManager::setFlightManager(FlightManager& manager)
{
    flightManager = &manager;
}
void NotificationManager::setPassengerManager(PassengerManager& manager)
{
    passengerManager = &manager;
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

bool NotificationManager::hasRequiredDetails(const string& passengerId, const string& passengerName,
    const string& flightNumber)
{
    return (!passengerId.empty() && !passengerName.empty() && !flightNumber.empty());
}

void NotificationManager::addNotification(const string& passengerId, const string& passengerName,
    const string& flightNumber, NotificationType type,
    const string& message)
{
    if (!hasRequiredDetails(passengerId, passengerName, flightNumber))
    {
        cout << "Notification not created. Passenger ID, name, and flight number are required.\n";
        return;
    }
    // A FlightManager must be connected before notifications are created.
    if (flightManager == nullptr)
    {
        cout << "Notification rejected: Flight Manager is not connected.\n";
        return;
    }

    // Verify that the flight number exists in Flight Management.
    string registeredFlightStatus;

    if (!flightManager->getFlightStatus(flightNumber, registeredFlightStatus))
    {
        cout << "\n[Notification Rejected]\n";
        cout << "Flight " << flightNumber << " is not registered in Flight Management.\n";
        cout << "No notification has been created.\n";
        return;
    }

    // Verify that the managers are connected.
    if (passengerManager == nullptr || flightManager == nullptr)
    {
        cout << "Notification rejected: managers are not connected.\n";
        return;
    }

    // Retrieve the actual registered passenger details.
    string registeredName;
    string registeredFlight;
    string gender, phone, seat, assistance, status;
    int age;

    if (!passengerManager->getPassengerQueueDetails(passengerId,registeredName,age,gender,
        phone, registeredFlight, seat, assistance, status))
    {
        cout << "\n[Notification Rejected]\n";
        cout << "Passenger ID " << passengerId<< " is not registered.\n";
        cout << "No notification has been created.\n";
        return;
    }

    // Normalize flight numbers for comparison.
    auto normalize = [](string value)
    {
        for (char& ch : value)
        {
            ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
        }

        return value;
    };

    // Ensure the passenger belongs to the specified flight.
    if (normalize(registeredFlight) != normalize(flightNumber))
    {
        cout << "\n[Notification Rejected]\n";
        cout << "Passenger " << passengerId << " is not registered on flight " << flightNumber << ".\n";
        cout << "No notification has been created.\n";
        return;
    }

    // Confirm that the flight exists.

    if (!flightManager->getFlightStatus(flightNumber, registeredFlightStatus))
    {
        cout << "\n[Notification Rejected]\n";
        cout << "Flight " << flightNumber << " does not exist.\n";
        cout << "No notification has been created.\n";
        return;
    }
    
    //dynamiccally allocates array for one node
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

void NotificationManager::sendCheckInReminder(const string& passengerId, const string& passengerName,
    const string& flightNumber) //receives details
{
    const string message = "Hello " + passengerName +
        ", check-in is open for flight " + flightNumber +
        ". Please proceed to the check-in counter.";

    addNotification(passengerId, passengerName, flightNumber, NotificationType::CheckInReminder, message);
}

void NotificationManager::sendBoardingReminder(const string& passengerId, const string& passengerName,
    const string& flightNumber, const string& gate)
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

    addNotification(passengerId, passengerName, flightNumber, NotificationType::BoardingReminder, message);
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

void NotificationManager::notifyCancellation(const string& passengerId, const string& passengerName,
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


bool NotificationManager::notifyConnectionRisk(const string& passengerId, const string& passengerName, 
    const string& arrivingFlightNumber, const string& arrivalTime, int transferMinutes,
    const string& connectingFlightNumber, const string& nextDepartureTime,
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
    // transferMinutes: estimated time needed to transfer between flights.
    const long long transferCompleteMinutes = static_cast<long long>(arrivalMinutes) + transferMinutes;

    // Step 6: Calculate the remaining buffer before departure.
    // minimumBufferMinutes: minimum spare time that should remain before departure.
    const long long availableBufferMinutes = static_cast<long long>(departureMinutes) -
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
    string message = "Hello " + passengerName +
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

bool NotificationManager::appendNotificationRecord(
    const NotificationRecord& record,
    std::string& message)
{
    if (record.notificationId <= 0 ||
        record.passengerId.empty() ||
        record.passengerId.size() > 30 ||
        record.passengerName.empty() ||
        record.passengerName.size() > 100 ||
        record.flightNumber.empty() ||
        record.flightNumber.size() > 20 ||
        record.type.empty() ||
        record.type.size() > 50 ||
        record.message.empty() ||
        record.message.size() > 1000)
    {
        message = "Invalid notification record.";
        return false;
    }

    // Prevent duplicate notification IDs.
    Node* current = head;

    while (current != nullptr)
    {
        if (current->notificationId == record.notificationId)
        {
            message = "Notification ID already exists.";
            return false;
        }

        current = current->next;
    }

    if (record.notificationId ==
        std::numeric_limits<long long>::max())
    {
        message = "Notification ID exceeds the supported range.";
        return false;
    }

    // Map the API label to the existing C++ notification enum.
    // Custom labels are preserved separately in typeLabel.
    std::string typeKey = record.type;

    for (char& ch : typeKey)
    {
        ch = static_cast<char>(
            std::tolower(static_cast<unsigned char>(ch))
        );
    }

    NotificationType mappedType = NotificationType::FlightStatus;

    if (typeKey.find("check") != std::string::npos)
        mappedType = NotificationType::CheckInReminder;
    else if (typeKey.find("boarding") != std::string::npos)
        mappedType = NotificationType::BoardingReminder;
    else if (typeKey.find("gate") != std::string::npos)
        mappedType = NotificationType::GateChange;
    else if (typeKey.find("cancellation") != std::string::npos)
        mappedType = NotificationType::Cancellation;
    else if (typeKey.find("delay") != std::string::npos)
        mappedType = NotificationType::Delay;
    else if (typeKey.find("connection") != std::string::npos)
        mappedType = NotificationType::ConnectionRisk;

    // Create the linked-list node.
    Node* newNode = new Node(
        record.notificationId,
        record.passengerId,
        record.passengerName,
        record.flightNumber,
        mappedType,
        record.message
    );

    // Preserve the exact API notification label and timestamp.
    newNode->typeLabel = record.type;
    newNode->createdAt = record.createdAt;
    newNode->isRead = record.isRead;

    // Append at the tail to preserve the supplied order.
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

    ++size;

    if (record.notificationId >= nextNotificationId)
    {
        nextNotificationId = record.notificationId + 1;
    }

    message = "Notification added successfully.";
    return true;
}

bool NotificationManager::getNotificationAt(
    std::size_t index,
    NotificationRecord& record) const
{
    Node* current = head;

    std::size_t currentIndex = 0;

    while (current != nullptr && currentIndex < index)
    {
        current = current->next;
        ++currentIndex;
    }

    if (current == nullptr)
        return false;

    record.notificationId = current->notificationId;
    record.passengerId = current->passengerId;
    record.passengerName = current->passengerName;
    record.flightNumber = current->flightNumber;

    record.type = current->typeLabel.empty()
        ? getTypeName(current->type)
        : current->typeLabel;

    record.message = current->message;
    record.isRead = current->isRead;
    record.createdAt = current->createdAt;

    return true;
}

bool NotificationManager::setNotificationReadRecord(
    long long notificationId,
    bool isRead,
    std::string& message)
{
    Node* current = head;

    while (current != nullptr)
    {
        if (current->notificationId == notificationId)
        {
            current->isRead = isRead;

            message = isRead
                ? "Notification marked as read."
                : "Notification marked as unread.";

            return true;
        }

        current = current->next;
    }

    message = "Notification not found.";
    return false;
}

std::size_t NotificationManager::markAllNotificationRecordsRead(
    const std::string& passengerId)
{
    std::size_t markedCount = 0;

    Node* current = head;

    while (current != nullptr)
    {
        const bool matchesPassenger =
            passengerId.empty() ||
            current->passengerId == passengerId;

        if (matchesPassenger && !current->isRead)
        {
            current->isRead = true;
            ++markedCount;
        }

        current = current->next;
    }

    return markedCount;
}
