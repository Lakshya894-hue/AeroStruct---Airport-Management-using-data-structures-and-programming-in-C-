#include "AeroAssistRoutes.h"

#include "AeroAssist.h"

#include <string>
#include <sstream>

// Shared helpers defined in api_main.cpp
void setCorsHeaders(httplib::Response& response);
std::string trim(const std::string& value);
std::string jsonEscape(const std::string& value);

void registerAeroAssistRoutes(httplib::Server& server)
{
    // ================================================
    // AEROASSIST: GET SUPPORT RECOMMENDATION
    // ================================================

    server.Get(
        "/api/assistance/advice",
        [](const httplib::Request& request,
           httplib::Response& response)
        {
            setCorsHeaders(response);

            if (!request.has_param("assistanceType"))
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Assistance type is required"})",
                    "application/json"
                );
                return;
            }

            std::string assistanceType =
                trim(request.get_param_value("assistanceType"));

            if (assistanceType.empty() || assistanceType.size() > 100)
            {
                response.status = 400;
                response.set_content(
                    R"({"status":"error","message":"Invalid assistance type"})",
                    "application/json"
                );
                return;
            }

            AeroAssistAdvice advice =
                AeroAssist::getAdvice(assistanceType);

            std::ostringstream body;

            body << "{"
                 << "\"priority\":" << advice.priority << ","
                 << "\"category\":\""
                 << jsonEscape(advice.category) << "\","
                 << "\"supportTeam\":\""
                 << jsonEscape(advice.supportTeam) << "\","
                 << "\"recommendedAction\":\""
                 << jsonEscape(advice.recommendedAction) << "\""
                 << "}";

            response.status = 200;
            response.set_content(
                body.str(),
                "application/json"
            );
        }
    );
}
