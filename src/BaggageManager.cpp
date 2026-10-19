#include "BaggageManager.h"
#include "PassengerManager.h"
#include "FlightManager.h"

#include <cctype>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

using namespace std;

namespace
{
    bool readIntegerInRange(const string& prompt, int minimum,
                            int maximum, int& value)
    {
        while (true)
        {
            cout << prompt;
            string line;
            if (!getline(cin >> ws, line)) return false;

            istringstream input(line);
            long long parsed = 0;
            char extra = '\0';
            if (!(input >> parsed) || (input >> extra) ||
                parsed < numeric_limits<int>::min() ||
                parsed > numeric_limits<int>::max())
            {
                cout << "Invalid input. Enter a whole number.\n";
                continue;
            }
            if (parsed < minimum || parsed > maximum)
            {
                cout << "Enter a value from " << minimum << " to "
                     << maximum << ".\n";
                continue;
            }
            value = static_cast<int>(parsed);
            return true;
        }
    }

    bool readWeightInRange(const string& prompt, double minimum,
                           double maximum, double& value)
    {
        while (true)
        {
            cout << prompt;
            string line;
            if (!getline(cin >> ws, line)) return false;

            istringstream input(line);
            double parsed = 0.0;
            char extra = '\0';
            if (!(input >> parsed) || (input >> extra) || !isfinite(parsed))
            {
                cout << "Invalid weight. Enter a finite number, such as 18.5.\n";
                continue;
            }
            if (parsed < minimum || parsed > maximum)
            {
                cout << "For this demonstration, weight must be between "
                     << minimum << " and " << maximum << " kg.\n";
                continue;
            }
            value = parsed;
            return true;
        }
    }

    bool readRequiredLine(const string& prompt, string& value,
                          size_t maximumLength)
    {
        while (true)
        {
            cout << prompt;
            if (!getline(cin, value)) return false;
            if (value.size() > maximumLength)
            {
                cout << "Input must not exceed " << maximumLength
                     << " characters. Try again.\n";
                continue;
            }

            bool hasNonWhitespace = false;
            for (unsigned char ch : value)
            {
                if (!isspace(ch)) { hasNonWhitespace = true; break; }
            }
            if (!hasNonWhitespace)
            {
                cout << "This field is required. Try again.\n";
                continue;
            }
            return true;
        }
    }

    string issueTypeFromChoice(int choice)
    {
        switch (choice)
        {
            case 1: return "Delayed";
            case 2: return "Missing";
            case 3: return "Damaged";
            default: return "Other";
        }
    }
}

BaggageManager::BaggageManager()
    : head(nullptr), tail(nullptr), queueFront(nullptr), queueRear(nullptr),
      baggageCount(0), nextBaggageNumber(1)
{
}

BaggageManager::~BaggageManager()
{
    BaggageNode* current = head;
    while (current != nullptr)
    {
        BaggageNode* toDelete = current;
        current = current->next;
        delete toDelete;
    }
    head = tail = nullptr;
    baggageCount = 0;
    clearHandlingQueue();
}

bool BaggageManager::isEmpty() const
{
    return head == nullptr;
}

size_t BaggageManager::getBaggageCount() const
{
    return baggageCount;
}

string BaggageManager::trim(const string& value) const
{
    size_t first = 0;
    while (first < value.size() &&
           isspace(static_cast<unsigned char>(value[first]))) ++first;

    size_t last = value.size();
    while (last > first &&
           isspace(static_cast<unsigned char>(value[last - 1]))) --last;

    return value.substr(first, last - first);
}

string BaggageManager::normalize(const string& value) const
{
    string result = trim(value);
    for (char& ch : result)
        ch = static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    return result;
}

string BaggageManager::currentTimestamp() const
{
    const time_t now = time(nullptr);
    const tm* local = localtime(&now);
    if (local == nullptr) return "Timestamp unavailable";

    char buffer[32] = {};
    if (strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local) == 0)
        return "Timestamp unavailable";
    return buffer;
}

string BaggageManager::generateBaggageTag()
{
    while (true)
    {
        ostringstream builder;
        builder << "BG" << setfill('0') << setw(6) << nextBaggageNumber++;
        const string candidate = builder.str();
        if (findBaggage(candidate) == nullptr) return candidate;
    }
}

BaggageManager::BaggageNode* BaggageManager::findBaggage(const string& tag)
{
    const string key = normalize(tag);
    BaggageNode* current = head;
    while (current != nullptr)
    {
        if (normalize(current->baggageTag) == key) return current;
        current = current->next;
    }
    return nullptr;
}

const BaggageManager::BaggageNode* BaggageManager::findBaggage(const string& tag) const
{
    const string key = normalize(tag);
    const BaggageNode* current = head;
    while (current != nullptr)
    {
        if (normalize(current->baggageTag) == key) return current;
        current = current->next;
    }
    return nullptr;
}

void BaggageManager::enqueueHandlingTag(const string& tag)
{
    QueueNode* node = new QueueNode{tag, nullptr};
    if (queueRear == nullptr)
        queueFront = queueRear = node;
    else
    {
        queueRear->next = node;
        queueRear = node;
    }
}

bool BaggageManager::dequeueHandlingTag(string& tag)
{
    if (queueFront == nullptr) return false;
    QueueNode* node = queueFront;
    tag = node->baggageTag;
    queueFront = queueFront->next;
    if (queueFront == nullptr) queueRear = nullptr;
    delete node;
    return true;
}

void BaggageManager::clearHandlingQueue()
{
    string ignored;
    while (dequeueHandlingTag(ignored)) { }
}

bool BaggageManager::isUnavailableFlightStatus(const string& status) const
{
    const string value = normalize(status);
    return value == "cancelled" || value == "canceled" || value == "completed";
}

bool BaggageManager::isCancelledFlightStatus(const string& status) const
{
    const string value = normalize(status);
    return value == "cancelled" || value == "canceled";
}

void BaggageManager::registerBaggage(PassengerManager& passengerManager,
                                     FlightManager& flightManager)
{
    if (baggageCount >= MAX_BAGGAGE_RECORDS)
    {
        cout << "Demo baggage-record limit reached (500 records).\n";
        return;
    }

    string passengerId;
    if (!readRequiredLine("Passenger ID: ", passengerId, 30)) return;
    passengerId = trim(passengerId);

    string passengerName, gender, phone, passengerFlight, seat, assistance, passengerStatus;
    int age = 0;
    if (!passengerManager.getPassengerQueueDetails(
            passengerId, passengerName, age, gender, phone,
            passengerFlight, seat, assistance, passengerStatus))
    {
        cout << "Passenger not found. Register the passenger first.\n";
        return;
    }

    const string passengerState = normalize(passengerStatus);
    if (passengerState != "checked-in" && passengerState != "security-cleared")
    {
        cout << "Baggage can only be registered after check-in. Current passenger status: "
             << passengerStatus << ".\n";
        return;
    }

    string flightStatus;
    if (trim(passengerFlight).empty() ||
        !flightManager.getFlightStatus(passengerFlight, flightStatus))
    {
        cout << "Baggage registration denied: the passenger's flight was not found.\n";
        return;
    }
    if (isUnavailableFlightStatus(flightStatus))
    {
        cout << "Baggage registration denied: flight " << passengerFlight
             << " is " << flightStatus << ".\n";
        return;
    }

    double weightKg = 0.0;
    if (!readWeightInRange("Baggage weight in kg (0.1-50.0): ",
                           MIN_BAGGAGE_WEIGHT_KG, MAX_BAGGAGE_WEIGHT_KG, weightKg))
        return;

    string description;
    if (!readRequiredLine(
            "Baggage description (e.g. blue suitcase, max 100 characters): ",
            description, 100)) return;
    description = trim(description);

    const string tag = generateBaggageTag();
    BaggageNode* node = new BaggageNode{
        tag, trim(passengerId), trim(passengerName), trim(passengerFlight),
        trim(seat), weightKg, description, currentTimestamp(),
        "Queued for Handling", "None", "", "None", nullptr
    };

    if (tail == nullptr) head = tail = node;
    else { tail->next = node; tail = node; }

    enqueueHandlingTag(tag);
    ++baggageCount;

    cout << "\nBaggage registered successfully.\n"
         << "Baggage tag : " << tag << '\n'
         << "Passenger   : " << node->passengerName << " (" << node->passengerId << ")\n"
         << "Flight      : " << node->flightNumber << '\n'
         << "Weight      : " << fixed << setprecision(2) << node->weightKg
         << " kg\n" << defaultfloat
         << "Status      : " << node->status << '\n'
         << "The bag has been added to the FIFO handling queue.\n"
         << "Note: 50 kg is a demo validation limit, not an airline policy.\n";
}

void BaggageManager::processNextBaggage(FlightManager& flightManager)
{
    if (queueFront == nullptr)
    {
        cout << "The baggage handling queue is empty.\n";
        return;
    }

    string tag;
    while (dequeueHandlingTag(tag))
    {
        BaggageNode* baggage = findBaggage(tag);
        if (baggage == nullptr || baggage->status != "Queued for Handling")
            continue;

        string flightStatus;
        if (!flightManager.getFlightStatus(baggage->flightNumber, flightStatus) ||
            isUnavailableFlightStatus(flightStatus))
        {
            baggage->status = "On Hold";
            cout << "Baggage " << baggage->baggageTag
                 << " was placed On Hold because its flight is missing, cancelled or completed.\n";
            continue;
        }

        baggage->status = "Loaded";
        cout << "Next baggage processed successfully.\n"
             << "Tag    : " << baggage->baggageTag << '\n'
             << "Flight : " << baggage->flightNumber << '\n'
             << "Status : Loaded\n";
        return;
    }

    cout << "No eligible baggage remained in the handling queue.\n";
}

void BaggageManager::markBaggageArrived(FlightManager& flightManager)
{
    string tag;
    if (!readRequiredLine("Baggage tag: ", tag, 20)) return;

    BaggageNode* baggage = findBaggage(trim(tag));
    if (baggage == nullptr)
    {
        cout << "Baggage tag not found.\n";
        return;
    }
    if (baggage->status != "Loaded")
    {
        cout << "Only baggage with status Loaded can be marked Arrived. Current status: "
             << baggage->status << ".\n";
        return;
    }

    string flightStatus;
    if (!flightManager.getFlightStatus(baggage->flightNumber, flightStatus))
    {
        cout << "Cannot update baggage: the assigned flight was not found.\n";
        return;
    }
    if (isCancelledFlightStatus(flightStatus))
    {
        cout << "Cannot mark baggage Arrived because its flight is cancelled.\n";
        return;
    }

    baggage->status = "Arrived";
    cout << "Baggage " << baggage->baggageTag << " marked Arrived.\n";
}

void BaggageManager::markBaggageCollected()
{
    string tag;
    if (!readRequiredLine("Baggage tag: ", tag, 20)) return;

    BaggageNode* baggage = findBaggage(trim(tag));
    if (baggage == nullptr)
    {
        cout << "Baggage tag not found.\n";
        return;
    }
    if (baggage->status != "Arrived")
    {
        cout << "Only Arrived baggage can be marked Collected. Current status: "
             << baggage->status << ".\n";
        return;
    }

    string pickupPassengerId;
    if (!readRequiredLine("Passenger ID collecting the bag: ", pickupPassengerId, 30))
        return;
    pickupPassengerId = trim(pickupPassengerId);
    if (normalize(pickupPassengerId) != normalize(baggage->passengerId))
    {
        cout << "Collection denied: passenger ID does not match the baggage record.\n";
        return;
    }

    baggage->status = "Collected";
    cout << "Baggage " << baggage->baggageTag
         << " marked Collected by passenger " << baggage->passengerId << ".\n";
}

void BaggageManager::requeueHeldBaggage(FlightManager& flightManager)
{
    string tag;
    if (!readRequiredLine("Baggage tag to requeue: ", tag, 20)) return;

    BaggageNode* baggage = findBaggage(trim(tag));
    if (baggage == nullptr)
    {
        cout << "Baggage tag not found.\n";
        return;
    }
    if (baggage->status != "On Hold")
    {
        cout << "Only baggage with status On Hold can be requeued. Current status: "
             << baggage->status << ".\n";
        return;
    }

    string flightStatus;
    if (!flightManager.getFlightStatus(baggage->flightNumber, flightStatus))
    {
        cout << "Cannot requeue: the assigned flight was not found.\n";
        return;
    }
    if (isUnavailableFlightStatus(flightStatus))
    {
        cout << "Cannot requeue while the flight is " << flightStatus
             << ". Change the flight to an active status first.\n";
        return;
    }

    baggage->status = "Queued for Handling";
    enqueueHandlingTag(baggage->baggageTag);
    cout << "Baggage " << baggage->baggageTag
         << " requeued at the back of the FIFO handling queue.\n";
}

void BaggageManager::displayBaggage(const BaggageNode& baggage) const
{
    cout << "\n--------------------------------------------------\n"
         << "Baggage tag   : " << baggage.baggageTag << '\n'
         << "Passenger     : " << baggage.passengerName << " ("
         << baggage.passengerId << ")\n"
         << "Flight        : " << baggage.flightNumber << '\n'
         << "Seat          : " << baggage.seatNumber << '\n'
         << "Weight        : " << fixed << setprecision(2)
         << baggage.weightKg << " kg\n" << defaultfloat
         << "Description   : " << baggage.description << '\n'
         << "Registered at : " << baggage.registeredAt << '\n'
         << "Status        : " << baggage.status << '\n'
         << "Issue type    : " << baggage.issueType << '\n';

    if (baggage.issueType != "None")
        cout << "Issue details : " << baggage.issueDescription << '\n'
             << "Issue status  : " << baggage.issueStatus << '\n';
}

void BaggageManager::searchBaggage() const
{
    string tag;
    if (!readRequiredLine("Enter baggage tag (e.g. BG000001): ", tag, 20)) return;

    const BaggageNode* baggage = findBaggage(trim(tag));
    if (baggage == nullptr)
    {
        cout << "No baggage record matches that tag.\n";
        return;
    }
    displayBaggage(*baggage);
}

void BaggageManager::displayAllBaggage() const
{
    if (isEmpty())
    {
        cout << "No baggage records are available.\n";
        return;
    }

    cout << "\n========== All Baggage Records =========="
         << "\nTotal records: " << baggageCount << '\n';
    const BaggageNode* current = head;
    while (current != nullptr)
    {
        displayBaggage(*current);
        current = current->next;
    }
}

void BaggageManager::displayPassengerBaggage() const
{
    string passengerId;
    if (!readRequiredLine("Passenger ID: ", passengerId, 30)) return;
    passengerId = trim(passengerId);

    bool found = false;
    const BaggageNode* current = head;
    while (current != nullptr)
    {
        if (normalize(current->passengerId) == normalize(passengerId))
        {
            displayBaggage(*current);
            found = true;
        }
        current = current->next;
    }
    if (!found)
        cout << "No baggage records were found for passenger " << passengerId << ".\n";
}

void BaggageManager::displayHandlingQueue() const
{
    if (queueFront == nullptr)
    {
        cout << "The baggage handling queue is empty.\n";
        return;
    }

    cout << "\n========== Baggage Handling Queue (FIFO) ==========" << '\n';
    const QueueNode* current = queueFront;
    size_t position = 1;
    while (current != nullptr)
    {
        const BaggageNode* baggage = findBaggage(current->baggageTag);
        cout << position << ". " << current->baggageTag;
        if (baggage != nullptr)
            cout << " | " << baggage->passengerName
                 << " | Flight " << baggage->flightNumber
                 << " | " << baggage->status;
        cout << '\n';
        ++position;
        current = current->next;
    }
    cout << "The first item listed will be processed next.\n";
}

void BaggageManager::reportBaggageIssue()
{
    string tag;
    if (!readRequiredLine("Baggage tag: ", tag, 20)) return;

    BaggageNode* baggage = findBaggage(trim(tag));
    if (baggage == nullptr)
    {
        cout << "Baggage tag not found.\n";
        return;
    }
    if (baggage->issueType != "None")
    {
        cout << "An issue is already recorded for this tag ("
             << baggage->issueStatus
             << "). Update that issue instead of overwriting its history.\n";
        return;
    }

    cout << "\nIssue type:\n"
         << "1. Delayed baggage\n"
         << "2. Missing baggage\n"
         << "3. Damaged baggage\n"
         << "4. Other issue\n";
    int choice = 0;
    if (!readIntegerInRange("Choose issue type (1-4): ", 1, 4, choice)) return;

    string description;
    if (!readRequiredLine("Issue description (max 200 characters): ", description, 200))
        return;

    baggage->issueType = issueTypeFromChoice(choice);
    baggage->issueDescription = trim(description);
    baggage->issueStatus = "Reported";
    cout << "Issue recorded for " << baggage->baggageTag
         << ". Status: Reported.\n";
}

void BaggageManager::updateBaggageIssueStatus()
{
    string tag;
    if (!readRequiredLine("Baggage tag: ", tag, 20)) return;

    BaggageNode* baggage = findBaggage(trim(tag));
    if (baggage == nullptr)
    {
        cout << "Baggage tag not found.\n";
        return;
    }
    if (baggage->issueType == "None")
    {
        cout << "No issue has been reported for this baggage tag.\n";
        return;
    }
    if (baggage->issueStatus == "Resolved")
    {
        cout << "This baggage issue is already Resolved.\n";
        return;
    }

    cout << "Current issue status: " << baggage->issueStatus << '\n';
    if (baggage->issueStatus == "Reported")
    {
        cout << "1. Under Review\n2. Resolved\n";
        int choice = 0;
        if (!readIntegerInRange("Choose new issue status (1-2): ", 1, 2, choice))
            return;
        baggage->issueStatus = (choice == 1) ? "Under Review" : "Resolved";
    }
    else if (baggage->issueStatus == "Under Review")
    {
        cout << "1. Resolved\n";
        int choice = 0;
        if (!readIntegerInRange("Enter 1 to resolve this issue: ", 1, 1, choice))
            return;
        baggage->issueStatus = "Resolved";
    }
    else
    {
        cout << "Unknown issue status. No changes made.\n";
        return;
    }

    cout << "Issue status for " << baggage->baggageTag
         << " updated to " << baggage->issueStatus << ".\n";
}
