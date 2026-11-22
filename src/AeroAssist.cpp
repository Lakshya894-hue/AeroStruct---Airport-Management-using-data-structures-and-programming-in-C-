#include "AeroAssist.h"
#include <algorithm>
#include <string>
#include <cctype>
using namespace std;
namespace //Anonymous Namespace 
{
    string toLower(string value) // converts the string to lowerCase
    {
        transform(value.begin(), value.end(), value.begin(), //starting position, position before final chaarcter, position where transformed characters are written
            [](unsigned char ch)                                   
            {
                return static_cast<char>(tolower(ch));//lambda function(inline function) receives 
                                                           //one character and converts to lowercase
            });
        return value;
    }

    bool contains(const string& text, const string& word) // find word inside text
    {
        return text.find(word) != string::npos;
    }
}

AeroAssistAdvice AeroAssist::getAdvice(const string& assistanceType)
{
    const string type = toLower(assistanceType);//turns the user input to lower case

    if (contains(type, "emergency")) //check whether on string contains another
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
