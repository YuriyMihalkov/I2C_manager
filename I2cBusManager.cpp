#include "I2cBusManager.h"

#include <memory>
#include <string>

std::shared_ptr<I2cBusInterface> I2cBusManager::getBus(const std::string &devicePath) {
    registrySemaphore.acquire();
    struct SemaphoreGuard {
        std::binary_semaphore& sem;
        ~SemaphoreGuard() { sem.release(); }
    } guard{registrySemaphore};

    auto it = buses.find(devicePath);
    if (it == buses.end()) {
        auto bus = std::make_shared<I2cBusInterface>(devicePath);
        buses[devicePath] = bus;
        return bus;
    }

    return it->second;
}

I2cBusManager &I2cBusManager::instance() {
    static I2cBusManager registry;
    return registry;
}
