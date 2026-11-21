#include "AuthRoutes.h"

#include <mysqlx/xdevapi.h>
#include <sodium.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// Existing helpers defined in api_main.cpp.
const char* getDbUser();
const char* getDbPassword();

void setCorsHeaders(httplib::Response& response);
bool databaseCredentialsAvailable();
std::string trim(const std::string& value);
std::string jsonEscape(const std::string& value);

namespace
{
    constexpr unsigned long long SESSION_DURATION_HOURS = 12;
    constexpr unsigned int MAX_FAILED_ATTEMPTS = 5;

    struct AuthUser
    {
        unsigned long long userId = 0;
        std::string username;
        std::string role;
        std::string passengerId;
    };

    template <typename T>
    std::string mysqlValueString(const T& value)
    {
        std::ostringstream output;
        output << value;
        return output.str();
    }

    std::string readParam(
        const httplib::Request& request,
        const std::string& name)
    {
        if (!request.has_param(name))
            return {};

        return request.get_param_value(name);
    }

    std::string normalizeUsername(std::string value)
    {
        value = trim(value);

        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            }
        );

        return value;
    }

    std::string normalizePassengerId(std::string value)
    {
        value = trim(value);

        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::toupper(ch));
            }
        );

        return value;
    }

    bool validUsername(const std::string& username)
    {
        if (username.size() < 3 || username.size() > 80)
            return false;

        for (unsigned char ch : username)
        {
            if (!std::isalnum(ch) &&
                ch != '.' &&
                ch != '_' &&
                ch != '-')
            {
                return false;
            }
        }

        return true;
    }

    bool validPassword(const std::string& password)
    {
        return password.size() >= 12 &&
               password.size() <= 128 &&
               password.find('\0') == std::string::npos;
    }

    bool constantTimeEquals(
        const std::string& first,
        const std::string& second)
    {
        if (first.empty() || first.size() != second.size())
            return false;

        return sodium_memcmp(
            first.data(),
            second.data(),
            first.size()
        ) == 0;
    }

    std::string hashPassword(const std::string& password)
    {
        char hash[crypto_pwhash_STRBYTES] = {};

        const int result = crypto_pwhash_str_alg(
            hash,
            password.c_str(),
            static_cast<unsigned long long>(password.size()),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_ARGON2ID13
        );

        if (result != 0)
        {
            throw std::runtime_error(
                "Password hashing failed. Insufficient system resources."
            );
        }

        return std::string(hash);
    }

    bool verifyPassword(
        const std::string& storedHash,
        const std::string& password)
    {
        return crypto_pwhash_str_verify(
            storedHash.c_str(),
            password.c_str(),
            static_cast<unsigned long long>(password.size())
        ) == 0;
    }

    std::string generateSessionToken()
    {
        unsigned char randomBytes[32] = {};
        char tokenHex[65] = {};

        randombytes_buf(randomBytes, sizeof(randomBytes));

        sodium_bin2hex(
            tokenHex,
            sizeof(tokenHex),
            randomBytes,
            sizeof(randomBytes)
        );

        sodium_memzero(randomBytes, sizeof(randomBytes));

        return std::string(tokenHex);
    }

    std::string hashSessionToken(const std::string& token)
    {
        unsigned char digest[crypto_hash_sha256_BYTES] = {};
        char digestHex[crypto_hash_sha256_BYTES * 2 + 1] = {};

        if (crypto_hash_sha256(
                digest,
                reinterpret_cast<const unsigned char*>(token.data()),
                static_cast<unsigned long long>(token.size())
            ) != 0)
        {
            throw std::runtime_error(
                "Could not hash the session token"
            );
        }

        sodium_bin2hex(
            digestHex,
            sizeof(digestHex),
            digest,
            sizeof(digest)
        );

        sodium_memzero(digest, sizeof(digest));

        return std::string(digestHex);
    }

    std::string extractBearerToken(
        const httplib::Request& request)
    {
        const std::string header =
            request.get_header_value("Authorization");

        const std::string prefix = "Bearer ";

        if (header.size() <= prefix.size() ||
            header.compare(0, prefix.size(), prefix) != 0)
        {
            return {};
        }

        const std::string token = header.substr(prefix.size());

        if (token.size() != 64)
            return {};

        for (unsigned char ch : token)
        {
            if (!std::isxdigit(ch))
                return {};
        }

        return token;
    }

    void sendJson(
        httplib::Response& response,
        int status,
        const std::string& body)
    {
        response.status = status;
        response.set_content(body, "application/json");
    }

    void sendError(
        httplib::Response& response,
        int status,
        const std::string& message)
    {
        sendJson(
            response,
            status,
            "{\"status\":\"error\",\"message\":\"" +
            jsonEscape(message) + "\"}"
        );
    }

    bool databaseReady(httplib::Response& response)
    {
        if (!databaseCredentialsAvailable())
        {
            sendError(
                response,
                500,
                "Database credentials are not configured"
            );

            return false;
        }

        return true;
    }

    bool findAuthenticatedUser(
        mysqlx::Session& session,
        const std::string& token,
        AuthUser& user)
    {
        if (token.empty())
            return false;

        const std::string tokenHash = hashSessionToken(token);

        auto result = session.sql(
            "SELECT "
            "u.user_id, "
            "u.username, "
            "u.role, "
            "COALESCE(u.passenger_id, '') "
            "FROM auth_sessions s "
            "JOIN users u ON u.user_id = s.user_id "
            "LEFT JOIN passengers p "
            "ON p.passenger_id = u.passenger_id "
            "WHERE s.token_hash = ? "
            "AND s.revoked_at IS NULL "
            "AND s.expires_at > UTC_TIMESTAMP() "
            "AND u.is_active = TRUE "
            "AND (u.role = 'admin' OR p.passenger_id IS NOT NULL) "
            "LIMIT 1"
        ).bind(tokenHash).execute();

        auto row = result.fetchOne();

        if (!row)
            return false;

        user.userId = std::stoull(mysqlValueString(row[0]));
        user.username = mysqlValueString(row[1]);
        user.role = mysqlValueString(row[2]);
        user.passengerId = mysqlValueString(row[3]);

        return true;
    }

    bool createSession(
        mysqlx::Session& session,
        unsigned long long userId,
        std::string& rawToken)
    {
        rawToken = generateSessionToken();

        const std::string tokenHash =
            hashSessionToken(rawToken);

        session.sql(
            "INSERT INTO auth_sessions "
            "(user_id, token_hash, expires_at) "
            "VALUES (?, ?, "
            "DATE_ADD(UTC_TIMESTAMP(), INTERVAL 12 HOUR))"
        ).bind(userId).bind(tokenHash).execute();

        return true;
    }

    std::string userJson(const AuthUser& user)
    {
        return
            "{\"username\":\"" + jsonEscape(user.username) + "\","
            "\"role\":\"" + jsonEscape(user.role) + "\","
            "\"passengerId\":" +
            (
                user.passengerId.empty()
                    ? "null"
                    : "\"" + jsonEscape(user.passengerId) + "\""
            ) +
            "}";
    }
}

void registerAuthRoutes(httplib::Server& server)
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error(
            "libsodium initialization failed"
        );
    }

    // =========================================================
    // FIRST ADMIN ACCOUNT
    // POST /api/auth/admin/bootstrap
    // =========================================================

    server.Post(
        "/api/auth/admin/bootstrap",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            if (!databaseReady(response))
                return;

            try
            {
                const char* configuredKey =
                    std::getenv("AEROSTRUCT_ADMIN_SETUP_KEY");

                if (configuredKey == nullptr ||
                    *configuredKey == '\0')
                {
                    sendError(
                        response,
                        503,
                        "Admin bootstrap is not configured"
                    );

                    return;
                }

                const std::string suppliedKey =
                    readParam(request, "setupKey");

                if (!constantTimeEquals(
                        configuredKey,
                        suppliedKey))
                {
                    sendError(
                        response,
                        403,
                        "Invalid admin setup key"
                    );

                    return;
                }

                const std::string username =
                    normalizeUsername(readParam(request, "username"));

                const std::string password =
                    readParam(request, "password");

                if (!validUsername(username))
                {
                    sendError(
                        response,
                        400,
                        "Username must contain 3-80 letters, digits, dots, underscores or hyphens"
                    );

                    return;
                }

                if (!validPassword(password))
                {
                    sendError(
                        response,
                        400,
                        "Password must contain between 12 and 128 characters"
                    );

                    return;
                }

                mysqlx::Session session(
                    "localhost",
                    33060,
                    getDbUser(),
                    getDbPassword()
                );

                session.sql("USE aerostruct_db").execute();

                auto countResult = session.sql(
                    "SELECT COUNT(*) FROM users WHERE role = 'admin'"
                ).execute();

                auto countRow = countResult.fetchOne();

                if (!countRow)
                    throw std::runtime_error(
                        "Could not check existing admin accounts"
                    );

                const unsigned long long adminCount =
                    std::stoull(mysqlValueString(countRow[0]));

                if (adminCount > 0)
                {
                    sendError(
                        response,
                        409,
                        "An admin account already exists. Bootstrap is disabled."
                    );

                    return;
                }

                const std::string passwordHash =
                    hashPassword(password);

                session.sql(
                    "INSERT INTO users "
                    "(username, password_hash, role, passenger_id) "
                    "VALUES (?, ?, 'admin', NULL)"
                ).bind(username).bind(passwordHash).execute();

                sendJson(
                    response,
                    201,
                    "{\"status\":\"success\","
                    "\"message\":\"Initial administrator created. Please log in.\"}"
                );
            }
            catch (const std::exception& error)
            {
                std::cerr
                    << "Admin bootstrap failed: "
                    << error.what() << '\n';

                sendError(
                    response,
                    503,
                    "Could not create the administrator account"
                );
            }
        }
    );

    // =========================================================
    // ADMIN-PROVISIONED PASSENGER ACCOUNT
    // POST /api/auth/passengers/provision
    // =========================================================

    server.Post(
        "/api/auth/passengers/provision",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            if (!databaseReady(response))
                return;

            try
            {
                mysqlx::Session session(
                    "localhost",
                    33060,
                    getDbUser(),
                    getDbPassword()
                );

                session.sql("USE aerostruct_db").execute();

                AuthUser admin;

                if (!findAuthenticatedUser(
                        session,
                        extractBearerToken(request),
                        admin))
                {
                    sendError(
                        response,
                        401,
                        "A valid login session is required"
                    );

                    return;
                }

                if (admin.role != "admin")
                {
                    sendError(
                        response,
                        403,
                        "Administrator access is required"
                    );

                    return;
                }

                const std::string passengerId =
                    normalizePassengerId(
                        readParam(request, "passengerId")
                    );

                const std::string username =
                    normalizeUsername(readParam(request, "username"));

                const std::string password =
                    readParam(request, "password");

                if (passengerId.empty() ||
                    passengerId.size() > 30)
                {
                    sendError(
                        response,
                        400,
                        "A valid Passenger ID is required"
                    );

                    return;
                }

                if (!validUsername(username))
                {
                    sendError(
                        response,
                        400,
                        "Invalid username"
                    );

                    return;
                }

                if (!validPassword(password))
                {
                    sendError(
                        response,
                        400,
                        "Password must contain between 12 and 128 characters"
                    );

                    return;
                }

                auto passengerResult = session.sql(
                    "SELECT passenger_id FROM passengers "
                    "WHERE passenger_id = ? LIMIT 1"
                ).bind(passengerId).execute();

                if (!passengerResult.fetchOne())
                {
                    sendError(
                        response,
                        404,
                        "Passenger record does not exist"
                    );

                    return;
                }

                auto existingResult = session.sql(
                    "SELECT user_id FROM users "
                    "WHERE username = ? OR passenger_id = ? "
                    "LIMIT 1"
                ).bind(username).bind(passengerId).execute();

                if (existingResult.fetchOne())
                {
                    sendError(
                        response,
                        409,
                        "Username or Passenger ID is already assigned to an account"
                    );

                    return;
                }

                const std::string passwordHash =
                    hashPassword(password);

                session.sql(
                    "INSERT INTO users "
                    "(username, password_hash, role, passenger_id) "
                    "VALUES (?, ?, 'passenger', ?)"
                ).bind(username)
                 .bind(passwordHash)
                 .bind(passengerId)
                 .execute();

                sendJson(
                    response,
                    201,
                    "{\"status\":\"success\","
                    "\"message\":\"Passenger login account created\"}"
                );
            }
            catch (const std::exception& error)
            {
                std::cerr
                    << "Passenger account provisioning failed: "
                    << error.what() << '\n';

                sendError(
                    response,
                    503,
                    "Could not create the passenger account"
                );
            }
        }
    );

    // =========================================================
    // LOGIN
    // POST /api/auth/login
    // =========================================================

    server.Post(
        "/api/auth/login",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            if (!databaseReady(response))
                return;

            try
            {
                const std::string username =
                    normalizeUsername(readParam(request, "username"));

                const std::string password =
                    readParam(request, "password");

                if (username.empty() || password.empty())
                {
                    sendError(
                        response,
                        400,
                        "Username and password are required"
                    );

                    return;
                }

                mysqlx::Session session(
                    "localhost",
                    33060,
                    getDbUser(),
                    getDbPassword()
                );

                session.sql("USE aerostruct_db").execute();

                auto result = session.sql(
                    "SELECT "
                    "user_id, "
                    "password_hash, "
                    "role, "
                    "COALESCE(passenger_id, ''), "
                    "is_active, "
                    "failed_login_attempts, "
                    "(locked_until IS NOT NULL "
                    "AND locked_until > UTC_TIMESTAMP()) "
                    "FROM users "
                    "WHERE username = ? "
                    "LIMIT 1"
                ).bind(username).execute();

                auto row = result.fetchOne();

                if (!row)
                {
                    sendError(
                        response,
                        401,
                        "Invalid username or password"
                    );

                    return;
                }

                const unsigned long long userId =
                    std::stoull(mysqlValueString(row[0]));

                const std::string storedHash =
                    mysqlValueString(row[1]);

                const std::string role =
                    mysqlValueString(row[2]);

                const std::string passengerId =
                    mysqlValueString(row[3]);

                const std::string active =
                    mysqlValueString(row[4]);

                const std::string locked =
                    mysqlValueString(row[6]);

                if (active != "1")
                {
                    sendError(
                        response,
                        403,
                        "This account is disabled"
                    );

                    return;
                }

                if (locked == "1")
                {
                    sendError(
                        response,
                        429,
                        "Account temporarily locked after repeated failed attempts"
                    );

                    return;
                }

                if (!verifyPassword(storedHash, password))
                {
                    session.sql(
                        "UPDATE users SET "
                        "failed_login_attempts = failed_login_attempts + 1, "
                        "locked_until = CASE "
                        "WHEN failed_login_attempts + 1 >= 5 "
                        "THEN DATE_ADD(UTC_TIMESTAMP(), INTERVAL 15 MINUTE) "
                        "ELSE locked_until END "
                        "WHERE user_id = ?"
                    ).bind(userId).execute();

                    sendError(
                        response,
                        401,
                        "Invalid username or password"
                    );

                    return;
                }

                if (role == "passenger")
                {
                    auto passengerResult = session.sql(
                        "SELECT passenger_id FROM passengers "
                        "WHERE passenger_id = ? LIMIT 1"
                    ).bind(passengerId).execute();

                    if (!passengerResult.fetchOne())
                    {
                        sendError(
                            response,
                            403,
                            "The linked passenger record is unavailable. Contact the operations desk."
                        );

                        return;
                    }
                }

                session.sql(
                    "UPDATE users SET "
                    "failed_login_attempts = 0, "
                    "locked_until = NULL "
                    "WHERE user_id = ?"
                ).bind(userId).execute();

                std::string token;

                createSession(session, userId, token);

                AuthUser user;
                user.userId = userId;
                user.username = username;
                user.role = role;
                user.passengerId = passengerId;

                sendJson(
                    response,
                    200,
                    "{\"status\":\"success\","
                    "\"token\":\"" + token + "\","
                    "\"expiresIn\":43200,"
                    "\"user\":" + userJson(user) + "}"
                );
            }
            catch (const std::exception& error)
            {
                std::cerr
                    << "Authentication login failed: "
                    << error.what() << '\n';

                sendError(
                    response,
                    503,
                    "Login service is temporarily unavailable"
                );
            }
        }
    );

    // =========================================================
    // CURRENT USER / SESSION VALIDATION
    // GET /api/auth/me
    // =========================================================

    server.Get(
        "/api/auth/me",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            if (!databaseReady(response))
                return;

            try
            {
                mysqlx::Session session(
                    "localhost",
                    33060,
                    getDbUser(),
                    getDbPassword()
                );

                session.sql("USE aerostruct_db").execute();

                AuthUser user;

                if (!findAuthenticatedUser(
                        session,
                        extractBearerToken(request),
                        user))
                {
                    sendError(
                        response,
                        401,
                        "Session is invalid, expired or revoked"
                    );

                    return;
                }

                sendJson(
                    response,
                    200,
                    "{\"status\":\"success\","
                    "\"user\":" + userJson(user) + "}"
                );
            }
            catch (const std::exception& error)
            {
                std::cerr
                    << "Session validation failed: "
                    << error.what() << '\n';

                sendError(
                    response,
                    503,
                    "Could not validate the session"
                );
            }
        }
    );

    // =========================================================
    // LOGOUT
    // POST /api/auth/logout
    // =========================================================

    server.Post(
        "/api/auth/logout",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            if (!databaseReady(response))
                return;

            try
            {
                const std::string token =
                    extractBearerToken(request);

                if (token.empty())
                {
                    sendError(
                        response,
                        401,
                        "A valid bearer token is required"
                    );

                    return;
                }

                mysqlx::Session session(
                    "localhost",
                    33060,
                    getDbUser(),
                    getDbPassword()
                );

                session.sql("USE aerostruct_db").execute();

                const std::string tokenHash =
                    hashSessionToken(token);

                session.sql(
                    "UPDATE auth_sessions "
                    "SET revoked_at = UTC_TIMESTAMP() "
                    "WHERE token_hash = ? "
                    "AND revoked_at IS NULL"
                ).bind(tokenHash).execute();

                sendJson(
                    response,
                    200,
                    "{\"status\":\"success\","
                    "\"message\":\"Logged out successfully\"}"
                );
            }
            catch (const std::exception& error)
            {
                std::cerr
                    << "Authentication logout failed: "
                    << error.what() << '\n';

                sendError(
                    response,
                    503,
                    "Could not complete logout"
                );
            }
        }
    );
}
