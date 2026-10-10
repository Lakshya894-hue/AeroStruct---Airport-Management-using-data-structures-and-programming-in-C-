#include <iostream>

#include "FlightManager.h"
#include "PassengerManager.h"
#include "CheckInQueue.h"
#include "BoardingQueue.h"
#include "PriorityQueue.h"
#include "UndoStack.h"

using namespace std;


// Flight Management Menu
void flightMenu(FlightManager& flightManager)
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
                flightManager.updateFlight();
                break;

            case 5:
                flightManager.deleteFlight();
                break;

            case 6:
                flightManager.cancelFlight();
                break;

            case 7:
                flightManager.sortFlights();
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
void passengerMenu(PassengerManager& passengerManager,
                   UndoStack& undoStack)
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
                passengerManager.addPassenger();
                break;

            case 2:
                passengerManager.displayPassengers();
                break;

            case 3:
                passengerManager.searchPassenger();
                break;

            case 4:
                passengerManager.updatePassenger();
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
void checkInMenu(CheckInQueue& checkInQueue)
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
                checkInQueue.enqueue();
                break;

            case 2:
                checkInQueue.displayQueue();
                break;

            case 3:
                checkInQueue.dequeue();
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
void boardingMenu(BoardingQueue& boardingQueue)
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
                boardingQueue.enqueue();
                break;

            case 2:
                boardingQueue.displayQueue();
                break;

            case 3:
                boardingQueue.dequeue();
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


// Main Function
int main()
{
    FlightManager flightManager;
    PassengerManager passengerManager;
    CheckInQueue checkInQueue;
    BoardingQueue boardingQueue;
    PriorityQueue priorityQueue;
    UndoStack undoStack;

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
        cout << "0. Exit\n";
        cout << "Enter your choice: ";

        cin >> choice;

        switch (choice)
        {
            case 1:
                flightMenu(flightManager);
                break;

            case 2:
                passengerMenu(passengerManager, undoStack);;
                break;

            case 3:
                checkInMenu(checkInQueue);
                break;

            case 4:
                boardingMenu(boardingQueue);
                break;

            case 5:
                priorityMenu(priorityQueue);
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