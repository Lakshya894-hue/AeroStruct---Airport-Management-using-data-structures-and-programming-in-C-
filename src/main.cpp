#include <iostream>
#include "FlightManager.h"
#include "PassengerManager.h"

using namespace std;

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
                cout << "Invalid choice. Try again.\n";
        }

    } while (choice != 0);
}


void passengerMenu(PassengerManager& passengerManager)
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
                passengerManager.deletePassenger();
                break;

            case 0:
                cout << "Returning to main menu...\n";
                break;

            default:
                cout << "Invalid choice. Try again.\n";
        }

    } while (choice != 0);
}


int main()
{
    FlightManager flightManager;
    PassengerManager passengerManager;

    int choice;

    do
    {
        cout << "\n========================================\n";
        cout << "        AEROSTRUCT AIRPORT SYSTEM       \n";
        cout << "========================================\n";
        cout << "1. Flight Management\n";
        cout << "2. Passenger Management\n";
        cout << "0. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice)
        {
            case 1:
                flightMenu(flightManager);
                break;

            case 2:
                passengerMenu(passengerManager);
                break;

            case 0:
                cout << "Thank you for using AeroStruct!\n";
                break;

            default:
                cout << "Invalid choice. Try again.\n";
        }

    } while (choice != 0);

    return 0;
}