#include <iostream>

#include "FlightManager.h"
#include "PassengerManager.h"
#include "CheckInQueue.h"
#include "BoardingQueue.h"
#include "PriorityQueue.h"
#include "UndoStack.h"
#include "AirportSystem.h"
#include "NotificationManager.h"
#include <limits>

using namespace std;


// Flight Management Menu
void flightMenu(FlightManager& flightManager, PassengerManager& passengerManager,
    NotificationManager& notificationManager)
{
    int choice;

    do
    {
        cout << "\n========== Flight Management ==========\n";
        cout << "1. Add Flight\n";
        cout << "2. Display Flights\n";
        cout << "3. Search Flight\n";
        cout << "4. Update Flight\n";
        cout << "5. Delete Flight\n";
        cout << "6. Cancel Flight\n";
        cout << "7. Sort Flights\n";
        cout << "8. Smart Flight Suggestions\n";
        cout << "0. Back\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
                flightManager.addFlight();
                break;

            case 2:
                flightManager.displayFlights();
                break;

            case 3:
                flightManager.searchFlight();
                break;

            case 4:
                flightManager.updateFlight(passengerManager, notificationManager);
                break;

            case 5:
                flightManager.deleteFlight();
                break;

            case 6:
                flightManager.cancelFlight(passengerManager, notificationManager);
                break;

            case 7:
                flightManager.sortFlights();
                break;
            
            case 8:
                flightManager.suggestAlternativeFlights();
                break;

            case 0:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 0);
}


// Passenger Management Menu
void passengerMenu(
    PassengerManager& passengerManager,
    UndoStack& undoStack,
    FlightManager& flightManager)
{
    int choice;

    do
    {
        cout << "\n========== Passenger Management ==========\n";
        cout << "1. Add Passenger\n";
        cout << "2. Display Passengers\n";
        cout << "3. Search Passenger\n";
        cout << "4. Update Passenger\n";
        cout << "5. Delete Passenger\n";
        cout << "6. Undo Last Passenger Deletion\n";
        cout << "7. Display Undo History\n";
        cout << "0. Back\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
                passengerManager.addPassenger(flightManager);;
                break;

            case 2:
                passengerManager.displayPassengers();
                break;

            case 3:
                passengerManager.searchPassenger();
                break;

            case 4:
                passengerManager.updatePassenger(flightManager);;
                break;

            case 5:
                passengerManager.deletePassenger(undoStack);
                break;

            case 6:
                undoStack.undoDelete(passengerManager);
                break;

            case 7:
                undoStack.display();
                break;

            case 0:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 0);
}


// Check-In Menu
void checkInMenu(CheckInQueue& checkInQueue, PassengerManager& passengerManager,
    NotificationManager& notificationManager)
{
    int choice;

    do
    {
        cout << "\n========== Check-In Management ==========\n";
        cout << "1. Add Passenger to Queue\n";
        cout << "2. Display Queue\n";
        cout << "3. Process Passenger\n";
        cout << "0. Back\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
                checkInQueue.enqueue(passengerManager, notificationManager);
                break;

            case 2:
                checkInQueue.displayQueue();
                break;

            case 3:
                checkInQueue.dequeue(passengerManager);
                break;

            case 0:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 0);
}


// Boarding Menu
void boardingMenu(BoardingQueue& boardingQueue,PassengerManager& passengerManager, 
    NotificationManager& notificationManager)
{
    int choice;

    do
    {
        cout << "\n========== Boarding Management ==========\n";
        cout << "1. Add Passenger to Queue\n";
        cout << "2. Display Queue\n";
        cout << "3. Board Passenger\n";
        cout << "0. Back\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
                boardingQueue.enqueue(passengerManager, notificationManager);
                break;

            case 2:
                boardingQueue.displayQueue();
                break;

            case 3:
                boardingQueue.dequeue(passengerManager);
                break;

            case 0:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 0);
}


// Priority Assistance Menu
void priorityMenu(PriorityQueue& priorityQueue)
{
    int choice;

    do
    {
        cout << "\n========== Priority Assistance ==========\n";
        cout << "1. Add Passenger\n";
        cout << "2. Display Priority Queue\n";
        cout << "3. Process Highest-Priority Passenger\n";
        cout << "0. Back\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
                priorityQueue.enqueue();
                break;

            case 2:
                priorityQueue.displayQueue();
                break;

            case 3:
                priorityQueue.dequeue();
                break;

            case 0:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 0);
}

// Personalized Airport Notifications Menu
void notificationMenu(NotificationManager& notificationManager)
{
    int choice;

    do
    {
        cout << "\n========== Personalized Airport Notifications ==========\n";
        cout << "1. Send Check-In Reminder\n";
        cout << "2. Send Boarding Reminder\n";
        cout << "3. Notify Gate Change\n";
        cout << "4. Notify Flight Status Update\n";
        cout << "5. Notify Flight Delay\n";
        cout << "6. Notify Flight Cancellation\n";
        cout << "7. View Passenger Notifications\n";
        cout << "8. View All Notifications\n";
        cout << "9. View Unread Notifications\n";
        cout << "10. Mark Passenger Notifications as Read\n";
        cout << "0. Back\n";
        cout << "Enter your choice: ";

        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        // Clear the newline before reading text with getline().
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        string passengerId, passengerName, flightNumber;
        string gate, oldGate, newGate;
        string status, revisedDeparture, reason;

        switch (choice)
        {
            case 1:
                cout << "Enter Passenger ID: ";
                getline(cin, passengerId);

                cout << "Enter Passenger Name: ";
                getline(cin, passengerName);

                cout << "Enter Flight Number: ";
                getline(cin, flightNumber);

                notificationManager.sendCheckInReminder(
                    passengerId, passengerName, flightNumber);
                break;

            case 2:
                cout << "Enter Passenger ID: ";
                getline(cin, passengerId);

                cout << "Enter Passenger Name: ";
                getline(cin, passengerName);

                cout << "Enter Flight Number: ";
                getline(cin, flightNumber);

                cout << "Enter Boarding Gate: ";
                getline(cin, gate);

                notificationManager.sendBoardingReminder(
                    passengerId, passengerName, flightNumber, gate);
                break;

            case 3:
                cout << "Enter Passenger ID: ";
                getline(cin, passengerId);

                cout << "Enter Passenger Name: ";
                getline(cin, passengerName);

                cout << "Enter Flight Number: ";
                getline(cin, flightNumber);

                cout << "Enter Old Gate: ";
                getline(cin, oldGate);

                cout << "Enter New Gate: ";
                getline(cin, newGate);

                notificationManager.notifyGateChange(
                    passengerId, passengerName, flightNumber,
                    oldGate, newGate);
                break;

            case 4:
                cout << "Enter Passenger ID: ";
                getline(cin, passengerId);

                cout << "Enter Passenger Name: ";
                getline(cin, passengerName);

                cout << "Enter Flight Number: ";
                getline(cin, flightNumber);

                cout << "Enter Flight Status: ";
                getline(cin, status);

                notificationManager.notifyFlightStatus(
                    passengerId, passengerName, flightNumber, status);
                break;

            case 5:
                cout << "Enter Passenger ID: ";
                getline(cin, passengerId);

                cout << "Enter Passenger Name: ";
                getline(cin, passengerName);

                cout << "Enter Flight Number: ";
                getline(cin, flightNumber);

                cout << "Enter Revised Departure Time: ";
                getline(cin, revisedDeparture);

                cout << "Enter Delay Reason: ";
                getline(cin, reason);

                if (reason.empty())
                {
                    notificationManager.notifyDelay(
                        passengerId, passengerName, flightNumber,
                        revisedDeparture);
                }
                else
                {
                    notificationManager.notifyDelay(
                        passengerId, passengerName, flightNumber,
                        revisedDeparture, reason);
                }
                break;

            case 6:
                cout << "Enter Passenger ID: ";
                getline(cin, passengerId);

                cout << "Enter Passenger Name: ";
                getline(cin, passengerName);

                cout << "Enter Flight Number: ";
                getline(cin, flightNumber);

                cout << "Enter Cancellation Reason: ";
                getline(cin, reason);

                if (reason.empty())
                {
                    notificationManager.notifyCancellation(
                        passengerId, passengerName, flightNumber);
                }
                else
                {
                    notificationManager.notifyCancellation(
                        passengerId, passengerName, flightNumber, reason);
                }
                break;

            case 7:
                cout << "Enter Passenger ID: ";
                getline(cin, passengerId);

                notificationManager.displayPassengerNotifications(
                    passengerId);
                break;

            case 8:
                notificationManager.displayAllNotifications();
                break;

            case 9:
                cout << "Enter Passenger ID: ";
                getline(cin, passengerId);

                notificationManager.displayUnreadNotifications(
                    passengerId);
                break;

            case 10:
                cout << "Enter Passenger ID: ";
                getline(cin, passengerId);

                notificationManager.markPassengerNotificationsRead(
                    passengerId);
                break;

            case 0:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice. Please try again.\n";
        }

    } while (choice != 0);
}

// Main Function

int main()
{
    // One AirportSystem object owns all the managers and queues.
    AirportSystem airportSystem;

    // Get references to the components owned by AirportSystem.
    FlightManager& flightManager = airportSystem.getFlightManager();

    PassengerManager& passengerManager = airportSystem.getPassengerManager();

    CheckInQueue& checkInQueue = airportSystem.getCheckInQueue();

    BoardingQueue& boardingQueue = airportSystem.getBoardingQueue();

    PriorityQueue& priorityQueue = airportSystem.getPriorityQueue();

    UndoStack& undoStack = airportSystem.getUndoStack();

    NotificationManager& notificationManager = airportSystem.getNotificationManager();

    int choice;

    do
    {
        cout << "\n";
        cout << "============================================\n";
        cout << "       AEROSTRUCT AIRPORT MANAGEMENT\n";
        cout << "============================================\n";
        cout << "1. Flight Management\n";
        cout << "2. Passenger Management\n";
        cout << "3. Check-In Management\n";
        cout << "4. Boarding Management\n";
        cout << "5. Priority Assistance\n";
        cout << "6. Personalized Airport Notifications\n";
        cout << "0. Exit\n";
        cout << "Enter your choice: ";

        cin >> choice;

        switch (choice)
        {
            case 1:
                flightMenu(flightManager,passengerManager,notificationManager);
                break;

            case 2:
                passengerMenu(passengerManager, undoStack, flightManager);;
                break;

            case 3:
                checkInMenu(checkInQueue, passengerManager, notificationManager);
                break;

            case 4:
                boardingMenu(boardingQueue, passengerManager, notificationManager);
                break;

            case 5:
                priorityMenu(priorityQueue);
                break;
            
            case 6:
                notificationMenu(notificationManager);
                break;

            case 0:
                cout << "\nThank you for using AeroStruct!\n";
                break;

            default:
                cout << "Invalid choice. Please try again.\n";
        }

    } while (choice != 0);

    return 0;
}
