#include "multi-purpose-status.hpp"

#include <phosphor-logging/elog.hpp>

#include <iostream>
#include <string>

namespace phosphor
{
namespace led
{
namespace multi
{
namespace purpose
{
namespace status
{

using namespace phosphor::logging;

constexpr auto KNOB_SELECTOR_OBJPATH =
    "/xyz/openbmc_project/Chassis/Buttons/Selector0";
constexpr auto KNOB_SELECTOR_INTERFACE =
    "xyz.openbmc_project.Chassis.Buttons.Selector";
constexpr auto KNOB_SELECTOR_PROPERTY = "Position";

constexpr auto POWER_STATUS_OBJPATH = "/xyz/openbmc_project/state/chassis";
constexpr auto POWER_STATUS_INTERFACE = "xyz.openbmc_project.State.Chassis";
constexpr auto POWER_STATUS_PROPERTY = "CurrentPowerState";

constexpr auto SENSOR_OBJPATH = "/xyz/openbmc_project/sensors";
constexpr auto SENSOR_INTERFACE =
    "xyz.openbmc_project.Sensor.Threshold.Critical";
constexpr auto SENSOR_CRITICAL_LOW = "CriticalAlarmLow";
constexpr auto SENSOR_CRITICAL_HIGH = "CriticalAlarmHigh";

constexpr auto SLED_OBJPATH = "/xyz/openbmc_project/Sled_identify";

void Status::selectPurpose()
{
    /* Sled, DebugCard, Power, Fault */
    std::string purpose = "DebugCard";

    std::cerr << " In select purpose func \n";
    std::cerr << " PURPOSE : " << purpose << "\n";

    if (purpose == "SLED")
    {
        setLedGroup(SLED_OBJPATH, "true");
    }
    else if (purpose == "DebugCard")
    {
        std::cerr << " In debug card \n";

        /* Get Position property */

        auto pos =
            getPropertyValue(KNOB_SELECTOR_OBJPATH, KNOB_SELECTOR_INTERFACE,
                             KNOB_SELECTOR_PROPERTY);
        uint16_t position = std::get<uint16_t>(pos);
        std::cerr << " POSITION : " << position << "\n";

        if (position == 5)
        {
            setLedGroup("path", "true");
        }
        else
        {
            /* Get Power Status */

            auto power = getPropertyValue(
                POWER_STATUS_OBJPATH + std::to_string(position),
                POWER_STATUS_INTERFACE, POWER_STATUS_PROPERTY);
            std::string powerStatus = std::get<std::string>(power);
            std::cerr << " PowerStatus : " << powerStatus << "\n";

            std::string healthStatus = "Good";

            /* Get Sensor Status */

            auto sensorPath =
                dBusHandler.getSubTreePaths(SENSOR_OBJPATH, SENSOR_INTERFACE);
            std::string ser = std::to_string(position) + "_";
            std::cerr << ser << "\n";
            for (auto& sensor : sensorPath)
            {
                if (sensor.find(ser) != std::string::npos)
                {
                    auto low = getPropertyValue(sensor, SENSOR_INTERFACE,
                                                SENSOR_CRITICAL_LOW);
                    bool sensorPropLow = std::get<bool>(low);

                    auto high = getPropertyValue(sensor, SENSOR_INTERFACE,
                                                 SENSOR_CRITICAL_HIGH);
                    bool sensorPropHigh = std::get<bool>(high);

                    std::cerr << " Low : " << sensorPropLow
                              << " High : " << sensorPropHigh << "\n";

                    if (sensorPropLow || sensorPropHigh)
                    {
                        healthStatus = "Bad";
                    }
                }
            }
            selectLedGroup(powerStatus, healthStatus);
        }
    }
}

const PropertyValue Status::getPropertyValue(const std::string& objectPath,
                                             const std::string& interface,
                                             const std::string& propertyName)
{
    PropertyValue propertyValue{};
    std::cerr << " Objpath   : " << objectPath << "\n";
    std::cerr << " Interface : " << interface << "\n";
    std::cerr << " PropNAme  : " << propertyName << "\n";
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

    return propertyValue;
}

void Status::setLedGroup(const std::string& objectPath, bool value)
{
    try
    {
        dBusHandler.setProperty(objectPath, "xyz.openbmc_project.Led.Group",
                                "Asserted", value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to set Asserted property",
                        entry("ERROR=%s", e.what()),
                        entry("PATH=%s", objectPath.c_str()));
    }
}

void Status::selectLedGroup(const std::string& powerStatus,
                            const std::string& healthStatus)
{
    if ((powerStatus == "On") && (healthStatus == "Good"))
    {
        setLedGroup("path", "true");
    }
    else if ((powerStatus == "On") && (healthStatus == "Bad"))
    {
        setLedGroup("path", "true");
    }
    else if ((powerStatus == "Off") && (healthStatus == "Good"))
    {
        setLedGroup("path", "true");
    }
    else if ((powerStatus == "Off") && (healthStatus == "Bad"))
    {
        setLedGroup("path", "true");
    }
}

} // namespace status
} // namespace purpose
} // namespace multi
} // namespace led
} // namespace phosphor
