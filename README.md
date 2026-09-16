# AeroStruct – Airport Management System

## Project Description

AeroStruct is an Airport Management System developed using C++,
Data Structures, Algorithms, Object-Oriented Programming and MySQL.
The system is designed to manage flights, passengers, check-in,
boarding, baggage, priority handling and airport routes while
providing passenger-centric assistance and intelligent airport
services.

The project demonstrates the practical application of Data
Structures, Algorithms and OOP concepts to a real-world airport
management scenario.

---

## Problem Statement

Airports handle large numbers of passengers, flights, baggage,
queues and operational activities simultaneously. Inefficient
management of these processes can result in long waiting times,
difficulty in retrieving records, baggage mismatches, poor handling
of priority passengers and difficulty in navigating airport
facilities.

AeroStruct aims to provide a structured and integrated system for
managing these operations efficiently using appropriate Data
Structures, Algorithms, OOP concepts and database management.

---

## Objectives

- Manage flight and passenger records efficiently.
- Handle check-in and boarding queues using appropriate Data Structures.
- Provide priority handling for emergency, VIP and assistance cases.
- Track baggage and detect passenger-baggage inconsistencies.
- Manage airport routes using Graph algorithms.
- Provide passenger-oriented airport navigation and assistance.
- Integrate C++ Data Structures with MySQL persistent storage.
- Demonstrate the practical application of DSA, Algorithms and OOP.

---

# Team Members

| Name | Role | Major Contribution |
|---|---|---|
| Lakshya Sharma | Project Lead | Project planning, architecture, Flight Management, system integration |
| Kuber Dhoundiyal | Developer | Passenger Management, Linked List, Queue, Check-in and Boarding |
| Arihant Singh | Database & Testing | MySQL, Priority Queue, Searching, Sorting, Graph algorithms and Testing |

---

# Technologies Used

- C++17
- Data Structures and Algorithms
- Object-Oriented Programming
- MySQL
- HTML5
- CSS3
- C++ Standard Template Library (STL)
- GCC/G++
- Visual Studio Code
- Git & GitHub

---

# Major Modules

## 1. Flight Management
- Add flight
- Update flight
- Delete flight
- Search flight
- Sort flights
- Cancel flight
- Flight status management

## 2. Passenger Management
- Passenger registration
- Search passenger
- Update passenger
- Delete passenger
- Passenger record management

## 3. Check-in Management
- Passenger check-in
- FIFO queue processing
- Queue status monitoring

## 4. Boarding Management
- Boarding queue
- FIFO processing
- Boarding status management

## 5. Priority Passenger Management
- Emergency passenger handling
- VIP passenger handling
- Senior-citizen and assistance requests
- Priority-based processing

## 6. Baggage Management
- Baggage registration
- Baggage tracking
- Passenger-baggage association
- Baggage status management

## 7. Airport Route Management
- Airport/location representation using Graphs
- Route creation
- BFS traversal
- DFS traversal
- Shortest-path calculation using Dijkstra's Algorithm

## 8. Database Management
- MySQL database integration
- Persistent passenger records
- Persistent flight records
- CRUD operations
- Data retrieval and updating

---

# Advanced Passenger-Centric Features

AeroStruct extends beyond basic airport record management by
introducing passenger-oriented assistance and intelligent services.

## AeroAssist – Personalized Passenger Assistance

AeroAssist is designed to provide a personalized view of a
passenger's airport journey.

Features include:

- Passenger flight information
- Current flight status
- Gate information
- Boarding status
- Next-action guidance
- Queue/waiting-time information
- Passenger-specific notifications

Example workflow:

Booking → Check-in → Security → Gate → Boarding → Departure

---

## Airport Navigator

The Airport Navigator helps passengers find routes between
locations inside the airport.

Possible destinations include:

- Boarding gates
- Check-in counters
- Security checkpoints
- Restrooms
- Medical facilities
- Information desks
- Food and service areas

Graph algorithms can be used to determine suitable routes.

---

## Smart Time-to-Gate Assistance

The system can estimate whether a passenger has sufficient time to
reach the assigned gate based on:

- Passenger's current location
- Gate location
- Estimated route
- Boarding time
- Simulated walking/travel time

The system can provide an appropriate next action such as:

Proceed to gate / Move immediately / Boarding approaching

---

## AeroTrack – Intelligent Baggage Tracking

AeroTrack maintains the relationship between passengers, flights
and baggage.

Possible baggage stages include:

Checked-in → Security → Loaded → In Transit → Arrived → Collected

Each baggage item can have a unique identification code.

---

## Baggage-Passenger Consistency Checker

The system can verify whether baggage is associated with the
correct:

- Passenger
- Flight
- Destination

Possible mismatches can be flagged for staff attention.

---

## Flight Cancellation Assistance

When a flight is cancelled, the system can identify passengers
associated with the affected flight and provide relevant
assistance information.

The module can support:

- Identifying affected passengers
- Identifying associated baggage
- Checking passenger status
- Finding alternative flights
- Updating passenger records

---

## Smart Alternative Flight Finder

For affected or connecting passengers, the system can search for
available alternative flights based on configurable criteria such
as:

- Destination
- Departure time
- Availability
- Number of stops
- Travel duration

Searching and sorting algorithms can be used to process candidate
flights.

---

## Emergency Assistance System

The Emergency Assistance module provides structured handling of
emergency-related passenger requests.

It can:

- Register emergency cases
- Prioritize emergency passengers
- Identify affected passengers
- Locate nearby emergency or medical facilities
- Provide route information
- Generate an incident record

Priority Queues and Graph algorithms are used for the corresponding
operations.

---

## Accessibility Assistance

Passengers requiring assistance can submit requests for services
such as:

- Wheelchair assistance
- Elderly passenger assistance
- Special mobility assistance

Requests can be handled using priority-based processing.

---

## Personalized Airport Notifications

Passengers can receive notifications related to their journey,
such as:

- Check-in reminders
- Boarding reminders
- Gate changes
- Flight status updates
- Delays
- Cancellation information

---

## Family and Group Travel Assistance

For passengers travelling together, the system can provide group
status information such as:

- Number of passengers checked in
- Number of passengers pending
- Group boarding status
- Group flight information

---

## Digital Passenger Journey Timeline

The system can display the passenger's journey as a sequence of
airport stages:

Booking
→ Check-in
→ Security
→ Boarding Gate
→ Boarding
→ Departure
→ Arrival
→ Baggage Collection

This provides passengers with a clear view of their current stage.

---

## Airport Services Finder

Passengers can search for airport facilities such as:

- Medical centre
- Restroom
- Food area
- ATM
- Information desk
- Check-in counter
- Gate

Graph-based route calculation can be used to guide passengers to
these facilities.

---

## Emergency Service Locator

Using the airport Graph, the system can determine a route from the
passenger's current location to the nearest available emergency or
medical facility.

Dijkstra's Algorithm can be used when route distances are weighted.

---

## Waiting-Time Estimator

The system can estimate waiting time at airport service points using
information such as:

- Queue length
- Number of active counters
- Average processing time

This can help passengers understand expected waiting time.

---

## Gate Change Assistance

When a gate changes, the system can display:

Previous gate → New gate

and calculate a route to the new gate using the airport navigation
Graph.

---

## Connection Risk Detection

For connecting passengers, the system can compare:

Arrival time + transfer time

with

Next flight departure time

and identify cases where the available transfer period is below a
configured threshold.

---

## Lost and Unclaimed Baggage Assistance

The system can support:

- Lost baggage reporting
- Baggage detail registration
- Matching reported baggage with stored records
- Unclaimed baggage identification

---

# Data Structures Used

| Data Structure | Application |
|---|---|
| Array | Flight records and fixed-size collections |
| Linked List | Passenger and baggage records |
| Queue | Check-in and boarding |
| Priority Queue | Emergency, VIP and assistance requests |
| Stack | Undo/recent-operation management |
| Graph | Airport locations and routes |

---

# Algorithms Used

- Linear Search
- Binary Search
- Sorting Algorithms
- BFS
- DFS
- Dijkstra's Shortest Path Algorithm

---

# DSA–Feature Mapping

| Feature | Data Structure / Algorithm |
|---|---|
| Passenger records | Linked List |
| Check-in | Queue |
| Boarding | Queue |
| Emergency assistance | Priority Queue |
| VIP handling | Priority Queue |
| Undo operations | Stack |
| Airport navigation | Graph |
| Facility locator | Graph + Dijkstra |
| Airport connectivity | BFS / DFS |
| Flight search | Searching |
| Flight ordering | Sorting |
| Alternative flight finder | Searching + Sorting |
| Baggage association | Linked List / Database |
| Waiting-time estimation | Queue |

---

# System Workflow

```text
User Input
     ↓
Input Validation
     ↓
C++ Application Layer
     ↓
Data Structures & Algorithms
     ↓
MySQL Database
     ↓
Processing / Decision Making
     ↓
Passenger / Admin Output
