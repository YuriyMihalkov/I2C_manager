#include "I2cBusInterface.h"
#include <linux/i2c-dev.h>
#include <iostream>
#include <semaphore>
#include <span>

I2cBusInterface::I2cBusInterface(const std::string &devicePath)
    : devicePath(devicePath), fd(-1)
{
    fd = open(devicePath.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "[I2C Error] Не удалось открыть шину: " << devicePath << std::endl;
    }
}

bool I2cBusInterface::readReg(int devAddr, unsigned char reg, std::span<unsigned char> outData) {
    if (fd < 0 || outData.empty()) return false;

    semaphore.acquire();

    // Вспомогательная RAII-структура для автоматического освобождения семафора
    struct SemaphoreGuard {
        std::binary_semaphore& sem;
        ~SemaphoreGuard() { sem.release(); }
    } guard{semaphore};

    if (ioctl(fd, I2C_SLAVE, devAddr) < 0) return false;
    if (write(fd, &reg, 1) != 1) return false;
    if (read(fd, outData.data(), outData.size()) != static_cast<ssize_t>(outData.size())) return false;

    return true;
}

bool I2cBusInterface::writeReg(int devAddr, unsigned char reg, unsigned char value) {
    if (fd < 0) return false;

    semaphore.acquire();
    struct SemaphoreGuard {
        std::binary_semaphore& sem;
        ~SemaphoreGuard() { sem.release(); }
    } guard{semaphore};

    if (ioctl(fd, I2C_SLAVE, devAddr) < 0) return false;

    unsigned char buf[2] = { static_cast<unsigned char>(reg), value };
    if (write(fd, buf, sizeof(buf)) != static_cast<ssize_t>(sizeof(buf))) return false;

    return true;
}

bool I2cBusInterface::writeRaw(int devAddr, std::span<const unsigned char> data) {
    if (fd < 0 || data.empty()) return false;

    semaphore.acquire();
    struct SemaphoreGuard {
        std::binary_semaphore& sem;
        ~SemaphoreGuard() { sem.release(); }
    } guard{semaphore};

    if (ioctl(fd, I2C_SLAVE, devAddr) < 0) return false;

    return write(fd, data.data(), data.size()) == static_cast<ssize_t>(data.size());
}

bool I2cBusInterface::transfer(int devAddr,
                               std::span<const unsigned char> txData,
                               std::span<unsigned char> rxData,
                               int postWriteDelayMs) {
    if (fd < 0 || rxData.empty()) return false;

    semaphore.acquire();
    struct SemaphoreGuard {
        std::binary_semaphore& sem;
        ~SemaphoreGuard() { sem.release(); }
    } guard{semaphore};

    if (ioctl(fd, I2C_SLAVE, devAddr) < 0) return false;

    if (!txData.empty()) {
        if (write(fd, txData.data(), txData.size()) != static_cast<ssize_t>(txData.size())) {
            return false;
        }
    }

    if (postWriteDelayMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(postWriteDelayMs));
    }

    return read(fd, rxData.data(), rxData.size()) == static_cast<ssize_t>(rxData.size());
}