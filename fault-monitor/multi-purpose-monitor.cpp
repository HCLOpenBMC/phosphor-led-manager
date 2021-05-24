#include "multi-purpose-monitor.hpp"
#include "../utils.hpp"
#include <phosphor-logging/elog.hpp>

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
using namespace phosphor::logging;

constexpr auto KNOB_SELECTOR_OBJPATH =
    "/xyz/openbmc_project/Chassis/Buttons/Selector0";
constexpr auto KNOB_SELECTOR_INTERFACE =
    "xyz.openbmc_project.Chassis.Buttons.Selector";
constexpr auto KNOB_SELECTOR_PROPERTY = "Position";

void Status::selectPurpose(std::string& purpose)
{
    /* Sled
       DebugCard
       Power
       Fault
    */

    if (purpose == "SLED")
    {
        //getPropertyValue();
        getPropertyValue(KNOB_SELECTOR_OBJPATH, KNOB_SELECTOR_INTERFACE,
                         KNOB_SELECTOR_PROPERTY);
    }
    else if (purpose == "DebugCard")
    {
        getPropertyValue(KNOB_SELECTOR_OBJPATH, KNOB_SELECTOR_INTERFACE,
                         KNOB_SELECTOR_PROPERTY);
    }
}

void Status::getPropertyValue(const std::string& objectPath,
                              const std::string& interface,
                              const std::string& propertyName)
{
    PropertyValue propertyValue{};
    try
    {
        propertyValue =
            dBusHandler.getProperty(objectPath, interface, propertyName);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to get property", entry("ERROR=%s", e.what()),
                        entry("PATH=%s", objectPath.c_str()));
        //        return {};
    }
}

} // namespace monitor
} // namespace purpose
} // namespace multi
} // namespace led
} // namespace phosphor
