#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <string>

// A console-based notification module for personalized passenger updates.
// Notifications are stored in a linked list as an in-app notification history.
// This module simulates notifications; it does not send SMS, email, or push alerts.

class NotificationManager
{
private:
    enum class NotificationType
    {
        CheckInReminder,
        BoardingReminder,
        GateChange,
        FlightStatus,
        Delay,
        Cancellation
    };

    struct Node
    {
        int notificationId;
        std::string passengerId;
        std::string passengerName;
        std::string flightNumber;
        NotificationType type;
        std::string message;
        bool isRead;
        Node* next;

        Node(int id,
             const std::string& pId,
             const std::string& pName,
             const std::string& fNumber,
             NotificationType notificationType,
             const std::string& notificationMessage);
    };

    Node* head;
    Node* tail;
    int size;
    int nextNotificationId;

    void addNotification(const std::string& passengerId,
                         const std::string& passengerName,
                         const std::string& flightNumber,
                         NotificationType type,
                         const std::string& message);

    static std::string getTypeName(NotificationType type);
    static bool hasRequiredDetails(const std::string& passengerId,
                                   const std::string& passengerName,
                                   const std::string& flightNumber);

public:
    NotificationManager();
    ~NotificationManager();

    // Personalized journey notifications
    void sendCheckInReminder(const std::string& passengerId,
                             const std::string& passengerName,
                             const std::string& flightNumber);

    void sendBoardingReminder(const std::string& passengerId,
                              const std::string& passengerName,
                              const std::string& flightNumber,
                              const std::string& gate);

    void notifyGateChange(const std::string& passengerId,
                          const std::string& passengerName,
                          const std::string& flightNumber,
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

    // Notification history and read/unread handling
    void displayPassengerNotifications(const std::string& passengerId) const;
    void displayAllNotifications() const;
    void displayUnreadNotifications(const std::string& passengerId) const;
    void markPassengerNotificationsRead(const std::string& passengerId);

    bool isEmpty() const;
    int getSize() const;
};

#endif
