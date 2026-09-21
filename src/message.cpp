#include "core/message.h"
#include <string>
using namespace std;

Message::Message()
    : role_(Role::System), content_(""){}

Message::Message(Role role, std::string content)
    : role_(role), content_(content){}

Role Message::role() const noexcept
{
    return role_;
}

const std::string& Message::content() const noexcept
{
    return content_;
}