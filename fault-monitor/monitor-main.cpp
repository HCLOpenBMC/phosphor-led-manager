#include "config.h"

#ifdef MONITOR_OPERATIONAL_STATUS
#include "operational-status-monitor.hpp"
#endif

#ifdef FRU_FAULT_MONITOR
#include "fru-fault-monitor.hpp"
#endif

#ifdef MULTI_PURPOSE_MONITOR
#include "multi-purpose-status.hpp"
#endif

#include <iostream>

int main(void)
{
    std::cerr << " In monitor main \n";
    /** @brief Dbus constructs used by Fault Monitor */
    sdbusplus::bus::bus bus = sdbusplus::bus::new_default();

#ifdef MULTI_PURPOSE_MONITOR
    std::cerr << " In multi purpose monitor \n";
    phosphor::led::multi::purpose::status::Status Status;
    Status.selectPurpose();
#endif

#ifdef MONITOR_OPERATIONAL_STATUS
    phosphor::led::Operational::status::monitor::Monitor monitor(bus);
#endif

#ifdef FRU_FAULT_MONITOR
    phosphor::led::fru::fault::monitor::Add monitor(bus);
#endif

    /** @brief Wait for client requests */
    while (true)
    {
        /** @brief process dbus calls / signals discarding unhandled */
        bus.process_discard();
        bus.wait();
    }
    return 0;
}
