#include "AuthMiddleware.h"

#include <sodium.h>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
    template <typename T>
    std::string valueToString(const T& value)
    {
        std::ostringstream output;
        output << value;
        return output.str();
    }

    std::string normalizeId(std::string value)
    {
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

    void sendAuthError(
        httplib::Response& response,
        int status,
        const std::string& message)
    {
        response.status = status;

        response.set_content(
            "{\"status\":\"error\",\"message\":\"" +
            message + "\"}",
            "application/json"
        );
    }
}

std::string extractBearerToken(const httplib::Request& request)
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

std::string hashSessionToken(const std::string& token)
{
    if (sodium_init() < 0)
    {
        throw std::runtime_error(
            "libsodium initialization failed"
        );
    }

    unsigned char digest[crypto_hash_sha256_BYTES] = {};
    char digestHex[crypto_hash_sha256_BYTES * 2 + 1] = {};

    if (crypto_hash_sha256(
            digest,
            reinterpret_cast<const unsigned char*>(token.data()),
            static_cast<unsigned long long>(token.size())
        ) != 0)
    {
        throw std::runtime_error(
            "Session token hashing failed"
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

bool authorizeRequest(
    mysqlx::Session& session,
    const httplib::Request& request,
    httplib::Response& response,
    AuthUser& authenticatedUser,
    const std::string& requiredRole)
{
    const std::string token = extractBearerToken(request);

    if (token.empty())
    {
        response.set_header(
            "WWW-Authenticate",
            "Bearer realm=\"AeroStruct\""
        );

        sendAuthError(
            response,
            401,
            "Authentication required"
        );

        return false;
    }

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
    {
        response.set_header(
            "WWW-Authenticate",
            "Bearer realm=\"AeroStruct\""
        );

        sendAuthError(
            response,
            401,
            "Session is invalid, expired or revoked"
        );

        return false;
    }

    authenticatedUser.userId =
        std::stoull(valueToString(row[0]));

    authenticatedUser.username =
        valueToString(row[1]);

    authenticatedUser.role =
        valueToString(row[2]);

    authenticatedUser.passengerId =
        valueToString(row[3]);

    if (!requiredRole.empty() &&
        authenticatedUser.role != requiredRole)
    {
        sendAuthError(
            response,
            403,
            "You do not have permission to access this resource"
        );

        return false;
    }

    return true;
}

bool canAccessPassenger(
    const AuthUser& authenticatedUser,
    const std::string& passengerId)
{
    if (authenticatedUser.role == "admin")
        return true;

    if (authenticatedUser.role != "passenger")
        return false;

    return normalizeId(authenticatedUser.passengerId) ==
           normalizeId(passengerId);
}
