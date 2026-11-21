#ifndef AUTH_MIDDLEWARE_H
#define AUTH_MIDDLEWARE_H

#include "httplib.h"
#include <mysqlx/xdevapi.h>

#include <cstdint>
#include <string>

struct AuthUser
{
    std::uint64_t userId = 0;
    std::string username;
    std::string role;
    std::string passengerId;
};

// Extract and hash the bearer token.
std::string extractBearerToken(const httplib::Request& request);
std::string hashSessionToken(const std::string& token);

// Validate the session and optionally enforce a required role.
// Returns false and sets an HTTP error response when access is denied.
bool authorizeRequest(
    mysqlx::Session& session,
    const httplib::Request& request,
    httplib::Response& response,
    AuthUser& authenticatedUser,
    const std::string& requiredRole = ""
);

// Admins can access any passenger record.
// Passengers can access only their own record.
bool canAccessPassenger(
    const AuthUser& authenticatedUser,
    const std::string& passengerId
);

#endif
