#pragma once

#include "../utils.hpp"

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

class Status
{
  public:
    Status()
    {
        std::cerr << "In status constructor \n";
        //        auto purpose = "DebugCard";
        //        selectPurpose(purpose);
    }

    //  private:
    DBusHandler dBusHandler;

    void selectPurpose();

    const PropertyValue getPropertyValue(const std::string& objectPath,
                                         const std::string& interface,
                                         const std::string& propertyName);

    void setLedGroup(const std::string& objectPath, bool value);

    void selectLedGroup(const std::string& powerStatus,
                        const std::string& healthStatus);
};

} // namespace status
} // namespace purpose
} // namespace multi
} // namespace led
} // namespace phosphor
