#include "I2cBusInterface.h"
#include <semaphore>
#include <unordered_map>
#include <string>
#include <memory>

/**
 * @class I2cBusManager
 * @brief Глобальный реестр для централизованного управления экземплярами I2C шин.
 *
 * Реализован как Синглтон Майерса. Гарантирует, что для одного и того же
 * системного файла шины (например, `/dev/i2c-1`) в приложении будет создан
 * строго один защищенный объект `I2cBusInterface`.
 */
class I2cBusManager {
public:
    /**
     * @brief Получение единственного глобального экземпляра реестра (Синглтон).
     * @return I2cBusManager& Ссылка на объект реестра.
     */
    static I2cBusManager& instance();

    /**
     * @brief Запрашивает общий доступ к указанной I2C шине.
     *
     * Если объект для данной шины еще не создан, метод инициализирует его и сохраняет в реестре.
     * Если объект уже существует, возвращается существующий умный указатель `std::shared_ptr`.
     *
     * @param devicePath Путь к шине I2C. По умолчанию используется "/dev/i2c-1".
     * @return std::shared_ptr<I2cBusInterface> Умный указатель на потокобезопасный интерфейс шины.
     */
    std::shared_ptr<I2cBusInterface> getBus(const std::string& devicePath = "/dev/i2c-1");

private:
    I2cBusManager() = default;
    ~I2cBusManager() = default;

    // Удаляем конструктор копирования и оператор присваивания для соблюдения паттерна Синглтон
    I2cBusManager(const I2cBusManager&) = delete;
    I2cBusManager& operator=(const I2cBusManager&) = delete;

    std::unordered_map<std::string, std::shared_ptr<I2cBusInterface>> buses; ///< Хранилище созданных интерфейсов шин.
    std::binary_semaphore registrySemaphore{1}; ///< Семафор для защиты структуры данных реестра при вызове из разных потоков.
};