#pragma once

#include "../utils.hpp"

#include <nlohmann/json.hpp>
#include <phosphor-logging/elog.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>

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

using namespace phosphor::led::utils;
using namespace phosphor::logging;

class Status
{
  public:
    Status(sdbusplus::bus::bus& bus) : bus(bus)
    {
        if (loadConfigValues() == "DebugCard")
        {
            matchSignal = std::make_unique<sdbusplus::bus::match_t>(
                bus,
                sdbusplus::bus::match::rules::type::signal() +
                    sdbusplus::bus::match::rules::member("PropertiesChanged") +
                    sdbusplus::bus::match::rules::path(
                        "/xyz/openbmc_project/Chassis/Buttons/Selector0") +
                    sdbusplus::bus::match::rules::interface(
                        "org.freedesktop.DBus.Properties"),
                [this](sdbusplus::message::message& msg) {
                    std::string objectName;
                    std::map<std::string, std::variant<uint16_t>> msgData;
                    msg.read(objectName, msgData);

                    std::cerr << "In match signal \n";
                    for (auto it = msgData.begin(); it != msgData.end(); ++it)
                    {
                        std::cerr << "\t" << it->first << "\n";
                    }
                    auto valPropMap = msgData.find("Position");
                    {
                        if (valPropMap != msgData.end())
                        {
                            DebugCard();
                        }
                    }
                });
        }
        else
        {
            sled();
        }
    }

  private:
    /* @brief sdbusplus D-Bus connection. */
    sdbusplus::bus::bus& bus;

    /** @brief sdbusplus signal matches for Monitor */
    std::unique_ptr<sdbusplus::bus::match_t> matchSignal;

    DBusHandler dBusHandler;
    // Manager manager;
    void sled();

    void DebugCard();

    const PropertyValue getPropertyValue(const std::string& objectPath,
                                         const std::string& interface,
                                         const std::string& propertyName);

    void setLedGroup(const std::string& objectPath, bool value);

    void setPhysicalLed(const std::string& objectPath,
                        const std::string& action, uint8_t dutyOn,
                        uint16_t period);

    void selectLedGroup(uint16_t position, const std::string& powerStatus,
                        const std::string& healthStatus);

    std::string loadConfigValues();
};

} // namespace status
} // namespace purpose
} // namespace multi
} // namespace led
} // namespace phosphor
