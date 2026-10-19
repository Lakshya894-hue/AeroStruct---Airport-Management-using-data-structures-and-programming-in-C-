#ifndef BAGGAGE_MANAGER_H
#define BAGGAGE_MANAGER_H

#include <cstddef>
#include <string>

class PassengerManager;
class FlightManager;

// In-memory baggage tracking module: linked-list records + FIFO handling queue.
// Menu options are handled by baggageMenu() in main.cpp.
class BaggageManager
{
public:
    BaggageManager();
    ~BaggageManager();

    // This class owns dynamically allocated linked-list nodes, so copying is unsafe.
    BaggageManager(const BaggageManager&) = delete;
    BaggageManager& operator=(const BaggageManager&) = delete;

    bool isEmpty() const;
    std::size_t getBaggageCount() const;

    // Public operations called by baggageMenu() in main.cpp.
    void registerBaggage(PassengerManager& passengerManager,
                         FlightManager& flightManager);
    void processNextBaggage(FlightManager& flightManager);
    void markBaggageArrived(FlightManager& flightManager);
    void markBaggageCollected();
    void requeueHeldBaggage(FlightManager& flightManager);
    void searchBaggage() const;
    void displayAllBaggage() const;
    void displayPassengerBaggage() const;
    void displayHandlingQueue() const;
    void reportBaggageIssue();
    void updateBaggageIssueStatus();

private:
    struct BaggageNode
    {
        std::string baggageTag;
        std::string passengerId;
        std::string passengerName;
        std::string flightNumber;
        std::string seatNumber;
        double weightKg;
        std::string description;
        std::string registeredAt;
        std::string status;
        std::string issueType;
        std::string issueDescription;
        std::string issueStatus;
        BaggageNode* next;
    };

    struct QueueNode
    {
        std::string baggageTag;
        QueueNode* next;
    };

    BaggageNode* head;
    BaggageNode* tail;
    QueueNode* queueFront;
    QueueNode* queueRear;
    std::size_t baggageCount;
    unsigned long long nextBaggageNumber;

    static constexpr double MIN_BAGGAGE_WEIGHT_KG = 0.1;
    static constexpr double MAX_BAGGAGE_WEIGHT_KG = 50.0; // Demo limit, not airline policy.
    static constexpr std::size_t MAX_BAGGAGE_RECORDS = 500;

    std::string trim(const std::string& value) const;
    std::string normalize(const std::string& value) const;
    std::string currentTimestamp() const;
    std::string generateBaggageTag();

    BaggageNode* findBaggage(const std::string& baggageTag);
    const BaggageNode* findBaggage(const std::string& baggageTag) const;
    void enqueueHandlingTag(const std::string& baggageTag);
    bool dequeueHandlingTag(std::string& baggageTag);
    void clearHandlingQueue();

    bool isUnavailableFlightStatus(const std::string& status) const;
    bool isCancelledFlightStatus(const std::string& status) const;

    void displayBaggage(const BaggageNode& baggage) const;
};

#endif // BAGGAGE_MANAGER_H
