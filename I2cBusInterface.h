/**
 * @file I2cBusManager.hpp
 * @brief Потокобезопасный универсальный менеджер I2C шин для Raspberry Pi.
 * @author Ваше Имя
 * @date 2026-10-23
 *
 * Данный файл содержит реализацию классов для безопасной параллельной работы
 * с несколькими физическими шинами I2C на стандартах C++20 с использованием
 * бинарных семафоров и безопасных буферов std::span.
 */

#pragma once

#include <semaphore>
#include <span>
#include <fcntl.h>
#include <string>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

/**
 * @class I2cBusInterface
 * @brief Класс для управления и потокобезопасного доступа к одной физической шине I2C.
 *
 * Использует бинарный семафор std::binary_semaphore для синхронизации потоков.
 * Обеспечивает раздельный доступ: операции на одной шине не блокируют работу других шин.
 */
class I2cBusInterface {
public:
    /**
     * @brief Конструктор интерфейса I2C шины.
     * @param devicePath Системный путь к файлу устройства (например, "/dev/i2c-1").
     */
    explicit I2cBusInterface(const std::string& devicePath);

    /**
     * @brief Деструктор класса. Освобождает системные ресурсы и закрывает файл устройства.
     */
    ~I2cBusInterface() {
        m_semaphore.acquire(); // Гарантируем, что в момент закрытия шина не используется
        if (m_fd >= 0) {
            close(m_fd);
        }
    }

    /**
     * @brief Проверяет, успешно ли открыт файл устройства I2C шины.
     * @return true, если дескриптор файла валиден, иначе false.
     */
    [[nodiscard]] bool isValid() const noexcept { return m_fd >= 0; }

    /**
     * @brief Возвращает системный путь к файлу шины.
     * @return std::string Путь к устройству.
     */
    [[nodiscard]] std::string devicePath() const noexcept { return m_devicePath; }

    /**
     * @brief Потокобезопасное чтение данных из регистра устройства.
     *
     * Метод захватывает семафор шины, выбирает ведомое устройство (I2C Slave),
     * отправляет адрес регистра и считывает запрошенное количество байт.
     *
     * @param devAddr Семибитный I2C-адрес целевого датчика/устройства.
     * @param reg Адрес регистра, из которого необходимо начать чтение.
     * @param outData Безопасный буфер (std::span) для записи полученных данных.
     * @return true в случае успешного чтения, false при ошибке на шине.
     */
    bool readReg(int devAddr, unsigned char reg, std::span<unsigned char> outData);

    /**
     * @brief Потокобезопасная запись одного байта в регистр устройства.
     *
     * @param devAddr Семибитный I2C-адрес целевого датчика/устройства.
     * @param reg Адрес регистра, в который производится запись.
     * @param value Байт данных для записи.
     * @return true в случае успешной записи, false при ошибке.
     */
    bool writeReg(int devAddr, unsigned char reg, unsigned char value);

private:
    std::string m_devicePath; ///< Путь к файлу устройства Linux.
    int m_fd;                  ///< Дескриптор файла I2C шины.
    std::binary_semaphore m_semaphore{1}; ///< Бинарный семафор C++20 для атомарного доступа к шине.
};


