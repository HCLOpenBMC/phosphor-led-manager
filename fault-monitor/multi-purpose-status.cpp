#include "multi-purpose-status.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/elog.hpp>

#include <fstream>
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

static constexpr auto PHY_LED_IFACE = "xyz.openbmc_project.Led.Physical";
static constexpr auto GROUP_LED_IFACE = "xyz.openbmc_project.Led.Group";

std::map<std::string, std::string> sledDbusNames = {{"SledObjpath", ""},
                                                    {"LedOff", ""}};

std::map<std::string, std::string> dbusNames = {{"KnobSelectorObjpath", ""},
                                                {"KnobSelectorInterface", ""},
                                                {"KnobSelectorProperty", ""},
                                                {"PowerStatusObjpath", ""},
                                                {"PowerStatusInterface", ""},
                                                {"PowerStatusProperty", ""},
                                                {"SensorObjpath", ""},
                                                {"SensorInterface", ""},
                                                {"SensorCriticalLow", ""},
                                                {"SensorCriticalHigh", ""},
                                                {"SledObjpath", ""},
                                                {"BmcObjpath", ""},
                                                {"PowerLed", ""},
                                                {"SystemLed", ""}};

void Status::sled()
{
    std::cerr << " In sled \n";
    setLedGroup(dbusNames["BmcObjpath"], true);
    setLedGroup(dbusNames["SledObjpath"], true);
#if 0
    for (int pos = 1; pos < 5; pos++)
    {
        setPhysicalLed("/xyz/openbmc_project/led/physical/power" +
                           std::to_string(pos),
                       "On", 0, 0);

        setPhysicalLed("/xyz/openbmc_project/led/physical/system" +
                           std::to_string(pos),
                       "Blink", 20, 200);
    }
#endif
}

void Status::DebugCard()
{
    std::cerr << " In debug card \n";

    /* Get Position property */
    //    setLedGroup(dbusNames["SledObjPath"], true);

    auto pos = getPropertyValue(dbusNames["KnobSelectorObjpath"],
                                dbusNames["KnobSelectorInterface"],
                                dbusNames["KnobSelectorProperty"]);
    uint16_t position = std::get<uint16_t>(pos);
    std::cerr << " POSITION : " << position << "\n";

    if (position == 5)
    {
        for (int pos = 1; pos < position; pos++)
        {
            setPhysicalLed(dbusNames["SystemLed"] + std::to_string(pos), "Off",
                           0, 0);
            setPhysicalLed(dbusNames["PowerLed"] + std::to_string(pos), "Blink",
                           50, 500);
        }
    }
    else
    {
        /* Get Power Status */

        auto power = getPropertyValue(dbusNames["PowerStatusObjpath"] +
                                          std::to_string(position),
                                      dbusNames["PowerStatusInterface"],
                                      dbusNames["PowerStatusProperty"]);

        std::string powerState = std::get<std::string>(power);
        std::string powerStatus = powerState.substr(45);
        std::cerr << " PowerStatus : " << powerStatus << "\n";

        std::string healthStatus = "Good";

        /* Get Sensor Status */

        auto sensorPath = dBusHandler.getSubTreePaths(
            dbusNames["SensorObjpath"], dbusNames["SensorInterface"]);
        std::string ser = "/" + std::to_string(position) + "_";
        std::cerr << ser << "\n";
        for (auto& sensor : sensorPath)
        {
            if (sensor.find(ser) != std::string::npos)
            {
                auto low =
                    getPropertyValue(sensor, dbusNames["SensorInterface"],
                                     dbusNames["SensorCriticalLow"]);
                bool sensorPropLow = std::get<bool>(low);

                auto high =
                    getPropertyValue(sensor, dbusNames["SensorInterface"],
                                     dbusNames["SensorCriticalHigh"]);
                bool sensorPropHigh = std::get<bool>(high);

                std::cerr << " Low : " << sensorPropLow
                          << " High : " << sensorPropHigh << "\n";

                if (sensorPropLow || sensorPropHigh)
                {
                    healthStatus = "Bad";
                }
            }
        }
        selectLedGroup(position, powerStatus, healthStatus);
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
        return {};
    }

    return propertyValue;
}

void Status::setLedGroup(const std::string& objectPath, bool value)
{
    std::cerr << " In set led group \n";
    std::cerr << " Objpath : " << objectPath << "\n";
    std::cerr << " Value : " << value << "\n";
    try
    {
        std::cerr << " Inside try block \n";
        PropertyValue assertedValue{value};
        dBusHandler.setProperty(objectPath, GROUP_LED_IFACE, "Asserted",
                                assertedValue);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        log<level::ERR>("Failed to set Asserted property",
                        entry("ERROR=%s", e.what()),
                        entry("PATH=%s", objectPath.c_str()));
    }
}

void Status::setPhysicalLed(const std::string& objPath,
                            const std::string& action, uint8_t dutyOn,
                            uint16_t period)
{
    std::cerr << " In set physical led \n";

    try
    {
        if (action == "Blink")
        {
            PropertyValue dutyOnValue{dutyOn};
            PropertyValue periodValue{period};

            dBusHandler.setProperty(objPath, PHY_LED_IFACE, "DutyOn",
                                    dutyOnValue);
            dBusHandler.setProperty(objPath, PHY_LED_IFACE, "Period",
                                    periodValue);
        }

        std::string ledAction =
            "xyz.openbmc_project.Led.Physical.Action." + action;
        std::cerr << "Led action : " << ledAction << "\n";

        PropertyValue actionValue{ledAction};
        dBusHandler.setProperty(objPath, PHY_LED_IFACE, "State", actionValue);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Error setting property for physical LED",
                        entry("ERROR=%s", e.what()),
                        entry("OBJECT_PATH=%s", objPath.c_str()));
    }
}

void Status::selectLedGroup(uint16_t position, const std::string& powerStatus,
                            const std::string& healthStatus)
{
    std::cerr << " Position     : " << position << "\n";
    std::cerr << " PowerStatus  : " << powerStatus << "\n";
    std::cerr << " HealthStatus : " << healthStatus << "\n";

    if ((powerStatus == "On") && (healthStatus == "Good"))
    {
        setPhysicalLed(dbusNames["SystemLed"] + std::to_string(position), "Off",
                       0, 0);
        setPhysicalLed(dbusNames["PowerLed"] + std::to_string(position),
                       "Blink", 90, 900);
    }
    else if ((powerStatus == "On") && (healthStatus == "Bad"))
    {
        setPhysicalLed(dbusNames["PowerLed"] + std::to_string(position), "On",
                       0, 0);

        setPhysicalLed(dbusNames["SystemLed"] + std::to_string(position),
                       "Blink", 90, 900);
    }
    else if ((powerStatus == "Off") && (healthStatus == "Good"))
    {
        setPhysicalLed(dbusNames["SystemLed"] + std::to_string(position), "Off",
                       0, 0);

        setPhysicalLed(dbusNames["PowerLed"] + std::to_string(position),
                       "Blink", 10, 100);
    }
    else if ((powerStatus == "Off") && (healthStatus == "Bad"))
    {
        setPhysicalLed(dbusNames["PowerLed"] + std::to_string(position), "On",
                       0, 0);

        setPhysicalLed(dbusNames["SystemLed"] + std::to_string(position),
                       "Blink", 10, 100);
    }
}

std::string Status::loadConfigValues()
{
    std::cerr << " In load config values \n";
    const std::string configFilePath =
        "/usr/share/phosphor-led-manager/config.json";

    std::ifstream configFile(configFilePath.c_str());

    if (!configFile.is_open())
    {
        log<level::ERR>("loadConfigValues : Cannot open config path");
        return 0;
    }

    std::cerr << "After opening config file \n";
    auto data = nlohmann::json::parse(configFile);

    auto ledPurpose = data["purpose"];
    std::cerr << "Led purpose : " << ledPurpose << "\n";
    std::string opt;
    for (auto& led : ledPurpose)
    {
        std::cerr << "LED : " << led << "\n";
        opt = led;
        if (led == "DebugCard" || led == "SLED")
        {
            auto dbus = data["dbus_names"];
            for (auto& name : dbusNames)
            {
                //                std::cerr << "SLED :" << name.first << "\n";
                if (dbus.contains(name.first.c_str()))
                {
                    name.second = dbus[name.first];
                    //                    std::cerr << name.second << "\n";
                }
            }
        }
        else
        {
            throw std::runtime_error("Failed to select LED name");
        }
    }
    return opt;
}
#if 0
        if (led == "SLED")
        {
            std::cerr << " Inside SLED \n";
            auto sledDbus = data["sled_dbus"];
            for (auto& name : sledDbusNames)
            {
                std::cerr << "SLED :" << name.first << "\n";
                if (sledDbus.contains(name.first.c_str()))
                {
                    name.second = sledDbus[name.first];
                    std::cerr << name.second << "\n";
                }
            }
        }
        else if (led == "DebugCard")
        {
            std::cerr << " Inside debug card \n";
            auto debugCardDbus = data["debugcard_dbus"];
            for (auto& name : debugCardDbusNames)
            {
                if (debugCardDbus.contains(name.first.c_str()))
                {
                    name.second = debugCardDbus[name.first];
                }
            }
        }
        else
        {
            throw std::runtime_error("Failed to select LED name");
        }
    }
    return opt;
}
#endif

} // namespace status
} // namespace purpose
} // namespace multi
} // namespace led
} // namespace phosphor
