#ifndef AEROASSIST_H
#define AEROASSIST_H

#include <string>

// Rule-based assistance recommendation for the airport help desk.
// Lower priority values represent more urgent requests.
struct AeroAssistAdvice
{
    int priority;
    std::string category;
    std::string supportTeam;
    std::string recommendedAction;
};

class AeroAssist
{
public:
    // Suggests a priority and support team from the assistance type.
    static AeroAssistAdvice getAdvice(const std::string& assistanceType);
};

#endif
