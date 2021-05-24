#pragma once

#include "../utils.hpp"

namespace phosphor
{
namespace led
{
namespace multi
{
namespace purpose
{
namespace monitor
{

using namespace phosphor::led::utils;

class Status
{
/*    Status()
    {
        std::string& purpose = "DebugCard";
        selectPurpose(std::string& purpose);
    }
*/
    DBusHandler dBusHandler;
    void getPropertyValue(const std::string& objectPath,
                     const std::string& interface,
                     const std::string& propertyName);
    void selectPurpose(std::string& purpose);
};

} // namespace monitor
} // namespace purpose
} // namespace multi
} // namespace led
} // namespace phosphor
