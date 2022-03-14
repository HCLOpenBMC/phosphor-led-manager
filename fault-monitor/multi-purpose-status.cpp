#include "multi-purpose-status.hpp"

#include <nlohmann/json.hpp>
#include <xyz/openbmc_project/Chassis/Buttons/HostSelector/server.hpp>
#include <xyz/openbmc_project/Led/Physical/server.hpp>

#include <algorithm>
#include <fstream>
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

using namespace sdbusplus::xyz::openbmc_project::Led::server;
using namespace sdbusplus::xyz::openbmc_project::Chassis::Buttons::server;

static constexpr size_t bmcPosition = 0;

template <typename T>
T Status::getPropertyValue(const std::string& objectPath,
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
        throw std::runtime_error("Failed to get property value");
    }

    return std::get<T>(propertyValue);
}

PowerState Status::powerStatus(const std::string& hostSelection)
{
    PowerState powerStatus;

    auto power = getPropertyValue<std::string>(
        power::POWER_STATE_OBJPATH + hostSelection, power::POWER_STATE_IFACE,
        power::POWER_STATE_PROPERTY);

    std::cerr << " Power : " << power << "\n";

    if (power == "xyz.openbmc_project.State.Chassis.PowerState.On")
    {
        powerStatus = PowerState::On;
    }
    else if (power == "xyz.openbmc_project.State.Chassis.PowerState.Off")
    {
        powerStatus = PowerState::Off;
    }
    else
    {
        log<level::ERR>("Failed to get property of powerstate",
                        entry("PATH=%s", power::POWER_STATE_OBJPATH));
    }
    return powerStatus;
}

std::string Status::healthStatus(const std::string& hostSelection)
{
    std::string healthStatus = "Good";
    auto sensorPath = dBusHandler.getSubTreePaths(sensor::SENSOR_OBJPATH,
                                                  sensor::SENSOR_THRES_IFACE);

    if (sensorPath.empty())
    {
        log<level::INFO>("Failed to get sensor paths");
        return "Bad";
    }
    std::string ser = "/" + hostSelection + "_";

    for (auto& sensor : sensorPath)
    {
        if (sensor.find(ser) != std::string::npos)
        {
            // std::cerr << " Sensor : " << sensor << "\n";
            auto low = getPropertyValue<bool>(
                sensor, sensor::SENSOR_THRES_IFACE, sensor::SENSOR_CRI_LOW);
            auto high = getPropertyValue<bool>(
                sensor, sensor::SENSOR_THRES_IFACE, sensor::SENSOR_CRI_HIGH);
            if (low || high)
            {
                healthStatus = "Bad";
            }
        }
    }
    std::cerr << " Health : " << healthStatus << "\n";
    return healthStatus;
}

void Status::setPhysicalLed(const std::string& objPath, Action action,
                            DutyOn dutyOn = 0, Period period = 0)
{
#if 0
    std::cerr << " Path   : " << objPath << "\n";
    std::cerr << " Action : " << static_cast<int>(action) << "\n";
    std::cerr << " Duty   : " << static_cast<int>(dutyOn) << "\n";
    std::cerr << " Period : " << static_cast<int>(period) << "\n";
#endif
    try
    {
        if (action == Action::Blink)
        {
            PropertyValue dutyOnValue{dutyOn};
            PropertyValue periodValue{period};

            dBusHandler.setProperty(objPath, led::PHY_LED_IFACE, "DutyOn",
                                    dutyOnValue);
            dBusHandler.setProperty(objPath, led::PHY_LED_IFACE, "Period",
                                    periodValue);
        }

        std::string ledAction;

        if (action == Action::On)
        {
            ledAction = convertForMessage(Physical::Action::On);
        }
        else if (action == Action::Blink)
        {
            ledAction = convertForMessage(Physical::Action::Blink);
        }
        else
        {
            ledAction = convertForMessage(Physical::Action::Off);
        }

        PropertyValue actionValue{ledAction};
        dBusHandler.setProperty(objPath, led::PHY_LED_IFACE, "State",
                                actionValue);
    }
    catch (const std::exception& e)
    {
        log<level::ERR>("Error setting property for physical LED",
                        entry("ERROR=%s", e.what()),
                        entry("OBJECT_PATH=%s", objPath.c_str()));
    }
}

void Status::selectPhysicalLed(const std::string& host, PowerState powerStatus,
                               const std::string& healthStatus)
{
    std::cerr << "Host   : " << host << "\n";
    //    std::cerr << "Power  : " << static_cast<int>(powerStatus) << "\n";
    //    std::cerr << "Health : " << healthStatus << "\n";

    switch (powerStatus)
    {
        case PowerState::On:
            if (healthStatus == "Good")
            {
                setPhysicalLed(led::SYSTEM_LED + host, Action::Off);
                setPhysicalLed(led::POWER_LED + host, Action::Blink, 90, 900);
            }
            else
            {
                setPhysicalLed(led::POWER_LED + host, Action::Off);
                setPhysicalLed(led::SYSTEM_LED + host, Action::Blink, 90, 900);
            }
            break;

        case PowerState::Off:
            if (healthStatus == "Good")
            {
                setPhysicalLed(led::SYSTEM_LED + host, Action::Off);
                setPhysicalLed(led::POWER_LED + host, Action::Blink, 10, 100);
            }
            else
            {
                setPhysicalLed(led::POWER_LED + host, Action::Off);
                setPhysicalLed(led::SYSTEM_LED + host, Action::Blink, 10, 100);
            }
            break;

        default:
            log<level::ERR>("Error in selecting physical LED");
            return;
    }
}

void Status::ledSlot(const std::string& host, PowerState power,
                     const std::string& health)
{
    //    std::cerr << " Slot  : " << host << "\n";
    //    std::cerr << " Power : " << static_cast<size_t>(power) << "\n";
    //    std::cerr << "Helath : " << health << "\n";

    if (power == PowerState::On)
    {
        if (health == "Good")
        {
            setPhysicalLed(led::POWER_LED + host, Action::On);
            setPhysicalLed(led::SYSTEM_LED + host, Action::Off);
        }
        else
        {
            setPhysicalLed(led::POWER_LED + host, Action::Off);
            setPhysicalLed(led::SYSTEM_LED + host, Action::On);
        }
    }
    else
    {
        setPhysicalLed(led::POWER_LED + host, Action::Off);
        setPhysicalLed(led::SYSTEM_LED + host, Action::Off);
    }
}

void Status::ledHandler()
{
    std::cerr << " In led handler \n";
    size_t maxPos = getPropertyValue<size_t>(hostselector::SELECTOR_OBJPATH,
                                             hostselector::SELECTOR_INTERFACE,
                                             hostselector::SELECTOR_MAXPOS);
    std::cerr << " Max pos : " << maxPos << "\n";
    while (true)
    {
        PowerState powerState;
        std::string healthState;

        bool sled = getPropertyValue<bool>(
            led::SLED_DBUS_PATH, led::GROUP_LED_IFACE, led::SLED_PROP);

        if (sled)
        {
            std::cerr << " In sled \n";
            for (size_t slotPos = 1; slotPos <= maxPos; slotPos++)
            {
                setPhysicalLed(led::POWER_LED + std::to_string(slotPos),
                               Action::Off);
                setPhysicalLed(led::SYSTEM_LED + std::to_string(slotPos),
                               Action::Blink, 20, 200);
            }
            continue;
        }

        size_t pos = getPropertyValue<size_t>(hostselector::SELECTOR_OBJPATH,
                                              hostselector::SELECTOR_INTERFACE,
                                              hostselector::SELECTOR_POSITION);
        std::cerr << " Pos : " << pos << "\n";
        if (pos == bmcPosition)
        {
            for (size_t slotPos = 1; slotPos <= maxPos; slotPos++)
            {
                setPhysicalLed(led::SYSTEM_LED + std::to_string(slotPos),
                               Action::Off);
                setPhysicalLed(led::POWER_LED + std::to_string(slotPos),
                               Action::Blink, 50, 500);
            }
            continue;
        }

        for (size_t slot = 1; slot <= maxPos; slot++)
        {
            std::string slotNum = std::to_string(slot);
            std::string str = "_" + std::to_string(slot - 1);

            auto fruPath =
                dBusHandler.getSubTreePaths(fru::FRU_OBJPATH, fru::FRU_IFACE);

            if (fruPath.empty())
            {
                log<level::INFO>("Failed to get FRU paths");
                continue;
            }
            for (auto& fru : fruPath)
            {
                if (fru.find(str) != std::string::npos)
                {
                    std::cerr << "FRU : " << fru << "\n";

                    auto prop = getPropertyValue<std::string>(
                        fru, fru::FRU_IFACE, fru::FRU_PROP);

                    if (prop.empty())
                    {
                        std::cerr << " Prop is empty \n";
                        powerState = PowerState::Off;
                        healthState = "Good";
                    }
                    else
                    {
                        powerState = powerStatus(slotNum);
                        healthState = healthStatus(slotNum);
                    }
                }
            }
            if (pos == slot)
            {
                std::cerr << " Pos equals slot \n";
                selectPhysicalLed(slotNum, powerState, healthState);
            }
            else
            {
                ledSlot(slotNum, powerState, healthState);
            }
        }
    }
}

} // namespace status
} // namespace purpose
} // namespace multi
} // namespace led
} // namespace phosphor
