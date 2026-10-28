#include "I2cBusInterface.h"
#include <iostream>

I2cBusInterface::I2cBusInterface(const std::string &devicePath)
    : m_devicePath(devicePath), m_fd(-1)
{
    m_fd = open(devicePath.c_str(), O_RDWR);
    if (m_fd < 0) {
        std::cerr << "[I2C Error] Не удалось открыть шину: " << devicePath << std::endl;
    }
}

bool I2cBusInterface::readReg(int devAddr, unsigned char reg, std::span<unsigned char> outData) {
    if (m_fd < 0 || outData.empty()) return false;

    m_semaphore.acquire();

    // Вспомогательная RAII-структура для автоматического освобождения семафора
    struct SemaphoreGuard {
        std::binary_semaphore& sem;
        ~SemaphoreGuard() { sem.release(); }
    } guard{m_semaphore};

    if (ioctl(m_fd, I2C_SLAVE, devAddr) < 0) return false;
    if (write(m_fd, &reg, 1) != 1) return false;
    if (read(m_fd, outData.data(), outData.size()) != static_cast<ssize_t>(outData.size())) return false;

    return true;
}

bool I2cBusInterface::writeReg(int devAddr, unsigned char reg, unsigned char value) {
    if (m_fd < 0) return false;

    m_semaphore.acquire();
    struct SemaphoreGuard {
        std::binary_semaphore& sem;
        ~SemaphoreGuard() { sem.release(); }
    } guard{m_semaphore};

    if (ioctl(m_fd, I2C_SLAVE, devAddr) < 0) return false;

    unsigned char buf[2] = { static_cast<unsigned char>(reg), value };
    if (write(m_fd, buf, sizeof(buf)) != static_cast<ssize_t>(sizeof(buf))) return false;

    return true;
}