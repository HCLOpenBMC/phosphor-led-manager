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

std::map<std::string, std::string> sledDbusNames = {{"SledObjpath", ""},
                                                    {"PowerLedOff", ""}};

std::map<std::string, std::string> debugCardDbusNames = {
    {"KnobSelectorObjpath", ""},  {"KnobSelectorInterface", ""},
    {"KnobSelectorProperty", ""}, {"PowerStatusObjpath", ""},
    {"PowerStatusInterface", ""}, {"PowerStatusProperty", ""},
    {"SensorObjpath", ""},        {"SensorInterface", ""},
    {"SensorCriticalLow", ""},    {"SensorCriticalHigh", ""},
    {"SledObjpath", ""},          {"LedBmcObjpath", ""},
    {"PowerLedOff", ""},          {"LedPowerOn", ""},
    {"LedPowerOff", ""},          {"LedSystemOn", ""},
    {"LedSystemOff", ""},         {"PowerOff", ""},
    {"SystemOff", ""}};

void Status::selectPurpose(const std::string& purpose)
{
    /* Sled, DebugCard, Power, Fault */
    // std::string purpose = "DebugCard";

    std::cerr << " In select purpose func \n";
    std::cerr << " PURPOSE : " << purpose << "\n";

    if (purpose == "SLED")
    {
        setLedGroup(sledDbusNames["PowerLedOff"], "true");
        setLedGroup(sledDbusNames["SledObjPath"], "true");
    }
    else if (purpose == "DebugCard")
    {
        std::cerr << " In debug card \n";

        /* Get Position property */

        auto pos = getPropertyValue(debugCardDbusNames["KnobSelectorObjpath"],
                                    debugCardDbusNames["KnobSelectorInterface"],
                                    debugCardDbusNames["KnobSelectorProperty"]);
        uint16_t position = std::get<uint16_t>(pos);
        std::cerr << " POSITION : " << position << "\n";

        if (position == 5)
        {
            setLedGroup(debugCardDbusNames["SledObjpath"], "false");
            setLedGroup(debugCardDbusNames["LedBmcObjpath"], "true");
        }
        else
        {
            /* Get Power Status */

            auto power =
                getPropertyValue(debugCardDbusNames["PowerStatusObjpath"] +
                                     std::to_string(position),
                                 debugCardDbusNames["PowerStatusInterface"],
                                 debugCardDbusNames["PowerStatusProperty"]);
            std::string powerState = std::get<std::string>(power);
            std::string powerStatus = powerState.substr(45);
            std::cerr << " PowerStatus : " << powerStatus << "\n";

            std::string healthStatus = "Good";

            /* Get Sensor Status */

            auto sensorPath = dBusHandler.getSubTreePaths(
                debugCardDbusNames["SensorObjpath"],
                debugCardDbusNames["SensorInterface"]);
            std::string ser = std::to_string(position) + "_";
            std::cerr << ser << "\n";
            for (auto& sensor : sensorPath)
            {
                if (sensor.find(ser) != std::string::npos)
                {
                    auto low = getPropertyValue(
                        sensor, debugCardDbusNames["SensorInterface"],
                        debugCardDbusNames["SensorCriticalLow"]);
                    bool sensorPropLow = std::get<bool>(low);

                    auto high = getPropertyValue(
                        sensor, debugCardDbusNames["SensorInterface"],
                        debugCardDbusNames["SensorCriticalHigh"]);
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

void Status::selectLedGroup(uint16_t position, const std::string& powerStatus,
                            const std::string& healthStatus)
{
    std::cerr << " Position     : " << position << "\n";
    std::cerr << " PowerStatus  : " << powerStatus << "\n";
    std::cerr << " HealthStatus : " << healthStatus << "\n";

    if ((powerStatus == "On") && (healthStatus == "Good"))
    {
        setLedGroup(debugCardDbusNames["PowerOff"] + std::to_string(position),
                    "true");
        setLedGroup(debugCardDbusNames["SystemOff"] + std::to_string(position),
                    "true");
        setLedGroup(debugCardDbusNames["LedPowerOn"] + std::to_string(position),
                    "true");
    }
    else if ((powerStatus == "On") && (healthStatus == "Bad"))
    {
        setLedGroup(debugCardDbusNames["PowerOff"] + std::to_string(position),
                    "true");
        setLedGroup(debugCardDbusNames["SystemOff"] + std::to_string(position),
                    "true");
        setLedGroup(debugCardDbusNames["LedSystemOn"] +
                        std::to_string(position),
                    "true");
    }
    else if ((powerStatus == "Off") && (healthStatus == "Good"))
    {
        setLedGroup(debugCardDbusNames["PowerOff"] + std::to_string(position),
                    "true");
        setLedGroup(debugCardDbusNames["SystemOff"] + std::to_string(position),
                    "true");
        setLedGroup(debugCardDbusNames["LedPowerOff"] +
                        std::to_string(position),
                    "true");
    }
    else if ((powerStatus == "Off") && (healthStatus == "Bad"))
    {
        setLedGroup(debugCardDbusNames["PowerOff"] + std::to_string(position),
                    "true");
        setLedGroup(debugCardDbusNames["SystemOff"] + std::to_string(position),
                    "true");
        setLedGroup(debugCardDbusNames["LedSystemrOff"] +
                        std::to_string(position),
                    "true");
    }
}

void Status::loadConfigValues()
{
    std::cerr << " In load config values \n";
    const std::string configFilePath =
        "/usr/share/phosphor-led-manager/config.json";

    std::ifstream configFile(configFilePath.c_str());

    if (!configFile.is_open())
    {
        log<level::ERR>("loadConfigValues : Cannot open config path");
        return;
    }

    std::cerr << "After opening config file \n";
    auto data = nlohmann::json::parse(configFile);

    auto ledPurpose = data["purpose"];
    std::cerr << "Led purpose : " << ledPurpose << "\n";

    for (auto& led : ledPurpose)
    {
        std::cerr << "LED : " << led << "\n";
        if (led == "SLED")
        {
            std::cerr << " Inside SLED \n";
            auto sledDbus = data["sled_dbus"];
            for (auto& name : sledDbusNames)
            {
                if (sledDbus.contains(name.first.c_str()))
                {
                    name.second = sledDbus[name.first];
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
        std::cerr << " before select purpose led \n";
        selectPurpose(led);
    }
}

} // namespace status
} // namespace purpose
} // namespace multi
} // namespace led
} // namespace phosphor
