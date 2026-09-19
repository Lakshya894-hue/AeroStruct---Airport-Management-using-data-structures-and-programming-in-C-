#include <iostream>
#include "FlightManager.h"

using namespace std;

int main()
{
    FlightManager manager;

    int choice;

    do
    {
        cout << "\n\n========== AEROSTRUCT ==========\n";
        cout << "1. Add Flight\n";
        cout << "2. Display Flights\n";
        cout << "3. Search Flight\n";
        cout << "4. Update Flight\n";
        cout << "5. Delete Flight\n";
        cout << "6. Cancel Flight\n";
        cout << "7. Sort Flights\n";
        cout << "8. Exit\n";

        cout << "\nEnter your choice: ";
        cin >> choice;

        switch(choice)
        {
            case 1:
                manager.addFlight();
                break;

            case 2:
                manager.displayFlights();
                break;

            case 3:
                manager.searchFlight();
                break;

            case 4:
                manager.updateFlight();
                break;

            case 5:
                manager.deleteFlight();
                break;

            case 6:
                manager.cancelFlight();
                break;

            case 7:
                manager.sortFlights();
                break;

            case 8:
                cout << "Exiting AeroStruct...\n";
                break;

            default:
                cout << "Invalid choice.\n";
        }

    } while (choice != 8);

    return 0;
}