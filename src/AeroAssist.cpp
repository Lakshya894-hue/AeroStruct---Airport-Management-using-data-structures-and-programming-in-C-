#include "AeroAssist.h"
#include <algorithm>
#include <string>
#include <cctype>

namespace
{
    std::string toLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
        return value;
    }

    bool contains(const std::string& text, const std::string& word)
    {
        return text.find(word) != std::string::npos;
    }
}

AeroAssistAdvice AeroAssist::getAdvice(const std::string& assistanceType)
{
    const std::string type = toLower(assistanceType);

    if (contains(type, "emergency"))
    {
        return {1, "Emergency", "Emergency Response Team",
                "Contact the designated emergency staff immediately."};
    }

    if (contains(type, "medical") || contains(type, "medicine"))
    {
        return {2, "Medical Assistance", "Airport Medical Team",
                "Notify the medical team and confirm the passenger's location."};
    }

    if (contains(type, "wheelchair") || contains(type, "accessibility") ||
        contains(type, "mobility"))
    {
        return {3, "Accessibility", "Accessibility Assistance Desk",
                "Arrange wheelchair or mobility support as requested."};
    }

    if (contains(type, "elderly") || contains(type, "senior"))
    {
        return {3, "Accessibility", "Passenger Assistance Desk",
                "Offer an escort or mobility assistance as requested."};
    }

    if (contains(type, "vip"))
    {
        return {4, "VIP", "Guest Relations Desk",
                "Notify guest relations for personalized support."};
    }

    return {5, "General", "Passenger Service Desk",
            "Direct the passenger to the information or service desk."};
}
