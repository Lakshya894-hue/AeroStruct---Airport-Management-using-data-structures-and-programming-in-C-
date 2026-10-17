#include "GroupTravelManager.h"
#include "PassengerManager.h"
#include "FlightManager.h"

#include <iostream>
#include <cctype>

using namespace std;

// Constructor
GroupTravelManager::GroupTravelManager()
    : head(nullptr), groupCount(0)
{}

// Convert IDs and statuses for case-insensitive comparison.
string GroupTravelManager::normalize(string value)
{
    for (char& ch : value)
    {
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    }

    return value;
}

// Find a group by its ID.
GroupTravelManager::GroupNode*
GroupTravelManager::findGroup(const string& groupId) const
{
    GroupNode* current = head;

    while (current != nullptr)
    {
        if (normalize(current->groupId) == normalize(groupId))
            return current;

        current = current->next;
    }

    return nullptr;
}

// Prevent a passenger from belonging to multiple groups.
bool GroupTravelManager::isPassengerAlreadyGrouped(const string& passengerId) const
{
    GroupNode* group = head;

    while (group != nullptr)
    {
        MemberNode* member = group->members;

        while (member != nullptr)
        {
            if (normalize(member->passengerId) ==normalize(passengerId))
            {
                return true;
            }

            member = member->next;
        }

        group = group->next;
    }

    return false;
}

// Destructor: free every member node and group node.
GroupTravelManager::~GroupTravelManager()
{
    while (head != nullptr)
    {
        GroupNode* group = head;
        head = head->next;

        while (group->members != nullptr)
        {
            MemberNode* member = group->members;
            group->members = member->next;
            delete member;
        }

        delete group;
    }

    groupCount = 0;
}

bool GroupTravelManager::isEmpty() const
{
    return head == nullptr;
}

// Create a new group for one flight.
void GroupTravelManager::createGroup(FlightManager& flightManager)
{
    string groupId, groupName, flightNumber;

    cout << "\n========== Create Travel Group ==========\n";

    cout << "Enter Group ID: ";
    getline(cin >> ws, groupId);

    if (findGroup(groupId) != nullptr)
    {
        cout << "Group ID already exists.\n";
        return;
    }

    cout << "Enter Group / Family Name: ";
    getline(cin >> ws, groupName);

    cout << "Enter Flight Number: ";
    getline(cin >> ws, flightNumber);

    if (groupId.empty() ||
        groupName.empty() ||
        flightNumber.empty())
    {
        cout << "Group ID, name and flight number are required.\n";
        return;
    }

    // Validate the selected flight.
    string flightStatus;

    if (!flightManager.getFlightStatus(
            flightNumber, flightStatus))
    {
        cout << "Group not created: flight does not exist.\n";
        return;
    }

    flightStatus = normalize(flightStatus);

    if (flightStatus == "cancelled" ||
        flightStatus == "canceled" ||
        flightStatus == "completed")
    {
        cout << "Group not created: flight is not available.\n";
        return;
    }

    GroupNode* newGroup =
        new GroupNode(groupId, groupName, flightNumber);

    // Insert the new group at the head of the linked list.
    newGroup->next = head;
    head = newGroup;
    ++groupCount;

    cout << "\nTravel group created successfully.\n";
    cout << "Group ID     : " << groupId << '\n';
    cout << "Group Name   : " << groupName << '\n';
    cout << "Flight Number: " << flightNumber << '\n';
    cout << "Next: add registered passengers to this group.\n";
}

// Add an existing passenger to a group.
void GroupTravelManager::addPassengerToGroup(
    PassengerManager& passengerManager,
    FlightManager& flightManager)
{
    string groupId, passengerId;

    cout << "\n========== Add Passenger to Group ==========\n";

    cout << "Enter Group ID: ";
    getline(cin >> ws, groupId);

    GroupNode* group = findGroup(groupId);

    if (group == nullptr)
    {
        cout << "Group not found.\n";
        return;
    }

    // Ensure the group's flight is still available.
    string flightStatus;

    if (!flightManager.getFlightStatus(group->flightNumber, flightStatus))
    {
        cout << "Cannot add member: group flight does not exist.\n";
        return;
    }

    flightStatus = normalize(flightStatus);

    if (flightStatus == "cancelled" ||
        flightStatus == "canceled" ||
        flightStatus == "completed")
    {
        cout << "Cannot add member: group flight is unavailable.\n";
        return;
    }

    cout << "Enter Registered Passenger ID: ";
    getline(cin >> ws, passengerId);

    // Retrieve the passenger's actual record.
    string name, gender, phone;
    string passengerFlight, seat, assistance, status;
    int age;

    if (!passengerManager.getPassengerQueueDetails(
            passengerId, name, age, gender, phone,
            passengerFlight, seat, assistance, status))
    {
        cout << "Passenger not found. Register them first.\n";
        return;
    }

    // All group members must be assigned to the group's flight.
    if (normalize(passengerFlight) !=
        normalize(group->flightNumber))
    {
        cout << "Passenger belongs to flight "
             << passengerFlight << ", not group flight "
             << group->flightNumber << ".\n";

        cout << "Passenger was not added.\n";
        return;
    }

    if (isPassengerAlreadyGrouped(passengerId))
    {
        cout << "Passenger already belongs to a travel group.\n";
        return;
    }

    // Append to this group's member linked list.
    MemberNode* newMember = new MemberNode(passengerId);

    if (group->members == nullptr)
    {
        group->members = newMember;
    }
    else
    {
        MemberNode* current = group->members;

        while (current->next != nullptr)
            current = current->next;

        current->next = newMember;
    }

    ++group->memberCount;

    cout << "\nPassenger added to the group successfully.\n";
    cout << "Passenger : " << name << '\n';
    cout << "Group     : " << group->groupName << '\n';
    cout << "Flight    : " << group->flightNumber << '\n';
}

// Display group information using live passenger statuses.
void GroupTravelManager::displayGroupStatuses(
    PassengerManager& passengerManager,
    FlightManager& flightManager) const
{
    if (isEmpty())
    {
        cout << "\nNo travel groups have been created.\n";
        return;
    }

    GroupNode* group = head;

    cout << "\n========== Family & Group Travel Status ==========\n";

    while (group != nullptr)
    {
        string flightStatus;

        bool flightExists = flightManager.getFlightStatus(
            group->flightNumber, flightStatus);

        size_t total = 0;
        size_t checkedIn = 0;
        size_t boarded = 0;

        MemberNode* member = group->members;

        while (member != nullptr)
        {
            string name, gender, phone;
            string flight, seat, assistance, status;
            int age;

            bool found = passengerManager.getPassengerQueueDetails(
                member->passengerId,
                name, age, gender, phone,
                flight, seat, assistance, status);

            if (found)
            {
                ++total;
                string normalizedStatus = normalize(status);

                if (normalizedStatus == "checked-in" ||
                    normalizedStatus == "checked in" ||
                    normalizedStatus == "security-cleared" ||
                    normalizedStatus == "security cleared" ||
                    normalizedStatus == "boarded")
                {
                    ++checkedIn;
                }

                if (normalizedStatus == "boarded")
                    ++boarded;
            }

            member = member->next;
        }

        size_t pending = total - checkedIn;

        cout << "\n--------------------------------------------\n";
        cout << "Group ID          : " << group->groupId << '\n';
        cout << "Group Name        : " << group->groupName << '\n';
        cout << "Flight Number     : " << group->flightNumber << '\n';

        cout << "Flight Status     : "
             << (flightExists ? flightStatus : "Flight Not Found")
             << '\n';

        cout << "Registered Members: " << total << '\n';
        cout << "Checked-In        : " << checkedIn << '\n';
        cout << "Pending Check-In  : " << pending << '\n';
        cout << "Boarded           : " << boarded << '\n';

        string boardingStatus;

        if (!flightExists ||
            normalize(flightStatus) == "cancelled" ||
            normalize(flightStatus) == "canceled")
        {
            boardingStatus = "Unavailable - Flight Cancelled/Missing";
        }
        else if (total == 0)
        {
            boardingStatus = "No Registered Members";
        }
        else if (boarded == total)
        {
            boardingStatus = "All Members Boarded";
        }
        else if (boarded > 0)
        {
            boardingStatus = "Partially Boarded";
        }
        else if (checkedIn == total)
        {
            boardingStatus = "All Checked-In - Awaiting Boarding";
        }
        else if (checkedIn > 0)
        {
            boardingStatus = "Partially Checked-In";
        }
        else
        {
            boardingStatus = "Check-In Pending";
        }

        cout << "Group Boarding   : " << boardingStatus << '\n';

        if (total < group->memberCount)
        {
            cout << "Warning           : "
                 << (group->memberCount - total)
                 << " member record(s) no longer exist.\n";
        }

        group = group->next;
    }

    cout << "\n============================================\n";
}