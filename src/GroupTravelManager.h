#ifndef GROUPTRAVELMANAGER_H
#define GROUPTRAVELMANAGER_H

#include <string>
#include <cstddef>

class PassengerManager;
class FlightManager;

class GroupTravelManager
{
private:
    // Linked list of passengers belonging to one group.
    struct MemberNode
    {
        std::string passengerId;
        MemberNode* next;

        MemberNode(const std::string& id)
            : passengerId(id), next(nullptr)
        {}
    };

    // Linked list of travel groups.
    struct GroupNode
    {
        std::string groupId;
        std::string groupName;
        std::string flightNumber;

        MemberNode* members;
        std::size_t memberCount;

        GroupNode* next;

        GroupNode(
            const std::string& id,
            const std::string& name,
            const std::string& flight)
            : groupId(id),
              groupName(name),
              flightNumber(flight),
              members(nullptr),
              memberCount(0),
              next(nullptr)
        {}
    };

    GroupNode* head;
    std::size_t groupCount;

    GroupNode* findGroup(const std::string& groupId) const;

    bool isPassengerAlreadyGrouped(
        const std::string& passengerId) const;

    static std::string normalize(std::string value);

public:
    GroupTravelManager();
    ~GroupTravelManager();

    void createGroup(FlightManager& flightManager);

    void addPassengerToGroup(
        PassengerManager& passengerManager,
        FlightManager& flightManager);

    void displayGroupStatuses(
        PassengerManager& passengerManager,
        FlightManager& flightManager) const;

    bool isEmpty() const;
};

#endif