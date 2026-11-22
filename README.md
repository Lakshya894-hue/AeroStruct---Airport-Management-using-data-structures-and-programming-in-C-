# ✈️ AeroStruct – Airport Management System

**A C++17 and Data Structures Based Airport Management System**

AeroStruct is a full-stack Airport Management System designed to simplify airport operations, manage passenger services and improve the overall travel experience. It combines C++ business logic, Data Structures and Algorithms (DSA), MySQL database management and a web-based frontend.

The system provides separate portals for administrators and passengers, allowing airport operations to be managed securely while giving passengers access to their own travel-related services.

---

## 📌 Project Overview

AeroStruct aims to provide an integrated platform for managing essential airport operations, including:

- Passenger registration and management.
- Flight scheduling and management.
- Check-in and boarding operations.
- Baggage registration and tracking.
- Emergency and priority passenger handling.
- Airport navigation and route management.
- Group travel management.
- Notifications and passenger assistance.

The project demonstrates how fundamental and advanced data structures can be integrated into a practical, real-world management application.

## ✨ Key Features

### 1. Flight Management
- Add, view, search and update flight records.
- Manage flight schedules and statuses.
- Search and sort flight information.
- Maintain flight-related database records.

### 2. Passenger Management
- Register and manage passenger information.
- Maintain passenger records.
- Support administrator and passenger access.
- Restrict passenger access to their own information.

### 3. Check-in and Boarding
- Manage passenger check-in queues.
- Maintain boarding queues.
- Process passenger boarding operations.
- Manage queue-based operations.

### 4. Baggage Management
- Register passenger baggage.
- Track baggage records and status.
- Handle baggage collection and reported issues.
- Manage baggage handling queues.

### 5. Emergency Management
- Maintain emergency case records.
- Associate affected passengers with incidents.
- Manage emergency status and related operations.

### 6. Priority Passenger Management
- Handle VIP and emergency passenger priority.
- Process passengers according to priority.

### 7. AeroAssist
- Provide airport assistance and passenger support.
- Integrate assistance-related functionality.

### 8. Smart Flight Suggestions
- Provide flight suggestions based on available flight information.

### 9. Airport Navigation
- Represent airport locations and connections.
- Manage airport paths.
- Support graph-based navigation and route exploration.

### 10. Group Travel
- Manage travel groups.
- Maintain group membership information.

### 11. Notification Management
- Store and manage passenger notifications.
- Support notification reading and tracking.

### 12. Authentication and Access Control
- Separate administrator and passenger login.
- Password-based authentication.
- Session-based authentication.
- Role-based access restrictions.
- Protected API operations.

---

## 🧰 Technology Stack

| Technology | Purpose |
|---|---|
| C++17 | Backend business logic |
| C | Data structures and algorithm implementations where applicable |
| HTML5 | Frontend structure |
| CSS3 | Frontend styling and responsive layouts |
| JavaScript | Frontend functionality and API communication |
| MySQL | Persistent database storage |
| CMake | Backend build configuration |
| MySQL X DevAPI | C++ database connectivity |
| REST APIs | Frontend-backend communication |
| libsodium | Authentication-related cryptographic functionality |
| Git and GitHub | Version control and collaboration |

---

## 🧠 Data Structures and Algorithms

AeroStruct uses different data structures to handle airport operations efficiently.

| Data Structure / Algorithm | Application |
|---|---|
| Linked List | Passenger record management |
| Queue | Check-in operations |
| Queue | Boarding operations |
| Priority Queue | VIP and emergency passengers |
| Stack | Undo recent operations |
| Graph | Airport navigation and connectivity |
| BFS / DFS | Graph traversal |
| Dijkstra's Algorithm | Shortest-path navigation |
| Binary Search | Flight searching |
| Sorting Algorithms | Flight sorting |

The project demonstrates the practical application of data structures in managing real-world airport operations.

---

## 🗄️ Database Architecture

AeroStruct uses MySQL for persistent data storage.

**Database Name:** `aerostruct_db`

The database currently contains 18 tables:

| Table | Purpose |
|---|---|
| `airports` | Airport information |
| `airport_locations` | Airport location records |
| `airport_paths` | Connections between airport locations |
| `passengers` | Passenger records |
| `flights` | Flight information |
| `users` | Authentication accounts |
| `auth_sessions` | Authentication sessions |
| `checkin_queue` | Check-in queue records |
| `boarding_queue` | Boarding queue records |
| `baggage_records` | Baggage information |
| `baggage_handling_queue` | Baggage handling operations |
| `emergency_cases` | Emergency incident records |
| `emergency_case_passengers` | Passengers associated with incidents |
| `assistance_queue` | Assistance requests |
| `assistance_request_history` | Assistance request history |
| `travel_groups` | Group travel records |
| `travel_group_members` | Group membership information |
| `notifications` | Passenger notifications |

The database supports persistent storage and retrieval of information across the application's modules.

---

## 🏗️ System Architecture

AeroStruct follows a frontend-backend-database architecture.

```text
             AEROSTRUCT APPLICATION
                      |
           +----------+----------+
           |                     |
      ADMIN PORTAL          PASSENGER PORTAL
           |                     |
           +----------+----------+
                      |
                HTML / CSS
                JavaScript
                      |
                  REST APIs
                      |
               C++17 API SERVER
                      |
          +-----------+-----------+
          |           |           |
       Flight      Passenger   Baggage
       Routes       Routes      Routes
          |           |           |
          +-----------+-----------+
                      |
                   MySQL
                      |
                 aerostruct_db
```

The frontend communicates with the C++ API server through HTTP requests. The backend handles application logic, authorization and database operations.

---

## 📂 Project Structure

The repository is organized into the following major components:

```text
AeroStruct/
│
├── FrontEnd/
│   ├── index.html
│   ├── admin.html
│   ├── passenger.html
│   ├── style.css
│   ├── app.js
│   └── portal.js
│
├── server/
│   ├── routes/
│   ├── CMakeLists.txt
│   └── api_main.cpp
│
├── src/
│   ├── Flight.cpp
│   ├── Flight.h
│   ├── Passenger.cpp
│   ├── Passenger.h
│   └── other project modules
│
├── README.md
└── .gitignore
```

*The tree represents the principal project components; additional files and modules are maintained in their respective directories.*

---

## ⚙️ Installation and Setup

### Prerequisites

Install and configure the following:

- Windows operating system.
- Visual Studio with C++ development tools.
- CMake.
- MySQL Server.
- MySQL X Plugin / X DevAPI support.
- vcpkg and required C++ dependencies.
- Node.js for frontend JavaScript syntax checking.
- Google Chrome or another modern browser.

### 1. Clone the Repository

```bash
git clone <YOUR_GITHUB_REPOSITORY_URL>
```

Navigate to the project directory:

```bash
cd AeroStruct---Airport-Management-using-data-structures-and-programming-in-C-
```

### 2. Database Configuration

Create the database:

```sql
CREATE DATABASE aerostruct_db;
```

Configure the MySQL connection using the project's existing server configuration and launcher setup.

The application uses MySQL connectivity through the MySQL X DevAPI service.

### 3. Build the Backend

Open PowerShell in the project root and execute:

```powershell
cmake --build .\server\build-auth --config Release
```

### 4. Start the API Server

Use the project launcher:

```powershell
.\Start-AeroStruct.bat
```

The API server runs at:

```text
http://127.0.0.1:8080
```

Keep the server terminal running while using the application.

### 5. Launch the Frontend

Open the `FrontEnd` directory using Visual Studio Code.

Start the frontend through Live Server.

The application provides the following entry points:

| Portal | File |
|---|---|
| Homepage | `index.html` |
| Administrator Portal | `admin.html` |
| Passenger Portal | `passenger.html` |

For example, with Live Server running on port 5501:

```text
http://127.0.0.1:5501/index.html
http://127.0.0.1:5501/admin.html
http://127.0.0.1:5501/passenger.html
```

---

## 🔐 Authentication and Security

AeroStruct implements separate authentication flows for administrators and passengers.

### Administrator
- Administrative dashboard access.
- Passenger account provisioning.
- Flight and operational management.
- Authorized administrative operations.

### Passenger
- Personal passenger portal.
- Access to authorized passenger information.
- Passenger-related services.
- Restricted access to administrative operations.

Authentication is handled through protected API requests and database-backed sessions.

**Security note:** The current application is intended for local development and demonstration. Production deployment should include HTTPS, secure session management, credential protection, rate limiting and additional security review.

---

## 🧪 Testing and Verification

The application can be tested through:

- Frontend functionality testing.
- REST API endpoint testing.
- MySQL record verification.
- Administrator and passenger authentication testing.
- Role-based authorization testing.
- Data structure operation testing.
- Database persistence testing.
- Responsive layout testing.

MySQL Workbench can be used to inspect database tables and verify that application operations are persisted.

---

## 🚀 Future Enhancements

Potential future improvements include:

- Advanced airport analytics.
- Database backup and recovery interface.
- Comprehensive audit logging.
- Enhanced passenger notifications.
- Advanced reporting and operational insights.
- Further UI/UX improvements.

---

## 👨‍💻 Project Development

**Project:** AeroStruct – Airport Management System Using Data Structures and C++

**Development Approach:** Team-based development

**Version Control:** GitHub

The project is developed collaboratively, with team members contributing to frontend development, backend modules, data structures, database integration and testing.

---

## 📜 Conclusion

AeroStruct demonstrates the integration of C++ programming, Data Structures and Algorithms, MySQL database management and web development to address practical airport management requirements.

By combining modular backend services, persistent database storage, role-based authentication and dedicated user portals, the project provides an integrated approach to airport operations management.

**AeroStruct – Smarter Operations. Better Airport Experiences.** ✈️
