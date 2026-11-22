#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <string>
#include <cstddef>
// A console-based notification module for personalized passenger updates.
// Notifications are stored in a linked list as an in-app notification history.
// This module simulates notifications; it does not send SMS, email, or push alerts.
class FlightManager;
class PassengerManager;

// API-friendly notification record.
struct NotificationRecord
{
    long long notificationId = 0;

    std::string passengerId;
    std::string passengerName;
    std::string flightNumber;

    std::string type;
    std::string message;

    bool isRead = false;
    std::string createdAt;
};
class NotificationManager
{
private:
    enum class NotificationType //enum conatins named set of fixed values
    {
        CheckInReminder,
        BoardingReminder,
        GateChange,
        FlightStatus,
        Delay,
        Cancellation,
        ConnectionRisk
    };

    struct Node
    {
        long long notificationId;
        std::string passengerId;
        std::string passengerName;
        std::string flightNumber;
        NotificationType type;
        std::string message;
        bool isRead;
        std::string typeLabel;
        std::string createdAt;

        Node* next;

        Node(long long id,const std::string& pId,const std::string& pName,const std::string& fNumber,
             NotificationType notificationType,
             const std::string& notificationMessage);
    };

    Node* head;
    Node* tail;
    int size;
    long long nextNotificationId;
    FlightManager* flightManager;
    PassengerManager* passengerManager;

    void addNotification(const std::string& passengerId,
                         const std::string& passengerName,
                         const std::string& flightNumber,
                         NotificationType type,
                         const std::string& message);

    static std::string getTypeName(NotificationType type);
    static bool hasRequiredDetails(const std::string& passengerId,const std::string& passengerName,
                                   const std::string& flightNumber);

public:
    NotificationManager();
    ~NotificationManager();

    // Personalized journey notifications
    void sendCheckInReminder(const std::string& passengerId,const std::string& passengerName,
                            const std::string& flightNumber);

    void sendBoardingReminder(const std::string& passengerId,const std::string& passengerName,
                              const std::string& flightNumber,
                              const std::string& gate);

    void notifyGateChange(const std::string& passengerId,const std::string& passengerName,const std::string& flightNumber,
                          const std::string& oldGate,
                          const std::string& newGate);

    void notifyFlightStatus(const std::string& passengerId,
                            const std::string& passengerName,
                            const std::string& flightNumber,
                            const std::string& status);

    void notifyDelay(const std::string& passengerId,
                     const std::string& passengerName,
                     const std::string& flightNumber,
                     const std::string& revisedDeparture,
                     const std::string& reason = "Operational reasons");

    void notifyCancellation(const std::string& passengerId,
                            const std::string& passengerName,
                            const std::string& flightNumber,
                            const std::string& reason = "The flight has been cancelled");
    // Detect a tight connection and notify the passenger if risky.
    bool notifyConnectionRisk(
    const std::string& passengerId,
    const std::string& passengerName,
    const std::string& arrivingFlightNumber,
    const std::string& arrivalTime,
    int transferMinutes,
    const std::string& connectingFlightNumber,
    const std::string& nextDepartureTime,
    int minimumBufferMinutes = 30);

    // Notification history and read/unread handling
    void displayPassengerNotifications(const std::string& passengerId) const;
    void displayAllNotifications() const;
    void displayUnreadNotifications(const std::string& passengerId) const;
    void markPassengerNotificationsRead(const std::string& passengerId);
    void setFlightManager(FlightManager& manager);
    void setPassengerManager(PassengerManager& manager);

    bool isEmpty() const;
    int getSize() const;

    // API-friendly notification operations.
    bool appendNotificationRecord(
    const NotificationRecord& record,
    std::string& message
    );

    bool getNotificationAt(
    std::size_t index,
    NotificationRecord& record
    ) const;

    bool setNotificationReadRecord(
    long long notificationId,
    bool isRead,
    std::string& message
    );

    std::size_t markAllNotificationRecordsRead(
    const std::string& passengerId = ""
    );
};

#endif
