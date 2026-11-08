#include <gtest/gtest.h>

#include "I2cBusManager.h"
#include "I2cBusInterface.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>
#include <span>

namespace {

constexpr const char* BUS_PATH     = "/dev/i2c-1";
constexpr int         PCF8574_ADDR = 0x20;     // адрес из i2cdetect
constexpr int         BAD_ADDR     = 0x7F;     // на шине пусто
constexpr const char* BAD_BUS_PATH = "/dev/i2c-99";

inline void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace

// ===========================================================================
//  Fixture: один раз на тест берём шину и приводим PCF8574 в известное
//  состояние (все выходы = 0). В TearDown возвращаем то же самое.
// ===========================================================================
class Pcf8574Test : public ::testing::Test {
protected:
    void SetUp() override {
        bus = I2cBusManager::instance().getBus(BUS_PATH);
        ASSERT_NE(bus, nullptr)    << "getBus вернул nullptr";
        ASSERT_TRUE(bus->isValid())<< "Шина /dev/i2c-1 не открыта. "
                                      "Проверьте i2cdetect и группу i2c.";
        resetOutputs();
    }

    void TearDown() override {
        if (bus && bus->isValid()) resetOutputs();
    }

    void resetOutputs() {
        const unsigned char zero[] = {0x00};
        (void)bus->writeRaw(PCF8574_ADDR, zero);
        sleep_ms(5);
    }

    std::shared_ptr<I2cBusInterface> bus;
};

// ---------------------------------------------------------------------------
//  1. Базовое API и синглтон
// ---------------------------------------------------------------------------

TEST_F(Pcf8574Test, BusIsValid) {
    EXPECT_TRUE(bus->isValid());
    EXPECT_EQ(bus->getDevicePath(), std::string(BUS_PATH));
}

TEST(I2cBusManagerTest, SingletonReturnsSameManager) {
    auto& m1 = I2cBusManager::instance();
    auto& m2 = I2cBusManager::instance();
    EXPECT_EQ(&m1, &m2);
}

TEST(I2cBusManagerTest, GetBusReturnsSameInstanceForSamePath) {
    auto a = I2cBusManager::instance().getBus("/dev/i2c-1");
    auto b = I2cBusManager::instance().getBus("/dev/i2c-1");
    EXPECT_EQ(a.get(), b.get())
        << "getBus обязан возвращать тот же указатель для того же пути";
}

TEST(I2cBusManagerTest, GetBusForDifferentPathReturnsDifferentInstance) {
    auto good = I2cBusManager::instance().getBus("/dev/i2c-1");
    auto bad  = I2cBusManager::instance().getBus(BAD_BUS_PATH);
    EXPECT_NE(good.get(), bad.get());
}

TEST(I2cBusManagerTest, BadBusPathIsInvalid) {
    auto bad = I2cBusManager::instance().getBus(BAD_BUS_PATH);
    ASSERT_NE(bad, nullptr);
    EXPECT_FALSE(bad->isValid());
}

// ---------------------------------------------------------------------------
//  2. writeRaw — управление 8 выходами PCF8574
// ---------------------------------------------------------------------------

TEST_F(Pcf8574Test, WriteAllLow) {
    const unsigned char tx[] = {0x00};
    EXPECT_TRUE(bus->writeRaw(PCF8574_ADDR, tx));
}

TEST_F(Pcf8574Test, WriteAllHigh) {
    const unsigned char tx[] = {0xFF};
    EXPECT_TRUE(bus->writeRaw(PCF8574_ADDR, tx));
}

TEST_F(Pcf8574Test, WriteAlternatingPatterns) {
    for (unsigned char p : {0xAA, 0x55, 0xF0, 0x0F}) {
        const unsigned char tx[] = {p};
        EXPECT_TRUE(bus->writeRaw(PCF8574_ADDR, tx))
            << "pattern 0x" << std::hex << int(p);
        sleep_ms(20);
    }
}

TEST_F(Pcf8574Test, WalkSingleBits) {
    // По очереди поднимаем каждый вывод — удобно визуально проверять с LED
    for (int bit = 0; bit < 8; ++bit) {
        const unsigned char tx[] = {static_cast<unsigned char>(1u << bit)};
        EXPECT_TRUE(bus->writeRaw(PCF8574_ADDR, tx)) << "bit " << bit;
        sleep_ms(50);
    }
}

TEST_F(Pcf8574Test, WriteToMissingDeviceFails) {
    const unsigned char tx[] = {0x00};
    EXPECT_FALSE(bus->writeRaw(BAD_ADDR, tx))
        << "Запись на несуществующий адрес должна провалиться";
}

// ---------------------------------------------------------------------------
//  3. readReg — у PCF8574 нет «регистров», но метод должен корректно
//     работать как write(reg) → read(1). Если записать 0x00, все выходы
//     принудительно LOW, и чтение вернёт 0x00.
// ---------------------------------------------------------------------------

TEST_F(Pcf8574Test, ReadRegAllLowReturnsZero) {
    unsigned char buf[1] = {0xFF};
    ASSERT_TRUE(bus->readReg(PCF8574_ADDR, 0x00,
                             std::span<unsigned char>(buf, 1)));
    EXPECT_EQ(buf[0], 0x00)
        << "Выводы принудительно LOW, чтение обязано вернуть 0x00";
}

TEST_F(Pcf8574Test, ReadRegFromMissingDeviceFails) {
    unsigned char buf[1] = {0};
    EXPECT_FALSE(bus->readReg(BAD_ADDR, 0x00,
                              std::span<unsigned char>(buf, 1)));
}

// ---------------------------------------------------------------------------
//  4. transfer — атомарная запись + пауза + чтение
// ---------------------------------------------------------------------------

TEST_F(Pcf8574Test, TransferWriteZeroReadBackZero) {
    const unsigned char tx[] = {0x00};
    unsigned char rx[1] = {0xFF};
    ASSERT_TRUE(bus->transfer(
        PCF8574_ADDR,
        std::span<const unsigned char>(tx, 1),
        std::span<unsigned char>(rx, 1),
        5));
    EXPECT_EQ(rx[0], 0x00);
}

TEST_F(Pcf8574Test, TransferEmptyWriteStillReads) {
    unsigned char rx[1] = {0xFF};
    EXPECT_TRUE(bus->transfer(
        PCF8574_ADDR,
        std::span<const unsigned char>(),
        std::span<unsigned char>(rx, 1),
        0));
}

TEST_F(Pcf8574Test, TransferToMissingDeviceFails) {
    const unsigned char tx[] = {0x00};
    unsigned char rx[1] = {0};
    EXPECT_FALSE(bus->transfer(
        BAD_ADDR,
        std::span<const unsigned char>(tx, 1),
        std::span<unsigned char>(rx, 1),
        0));
}

// ---------------------------------------------------------------------------
//  5. Конкурентность — проверяем, что семафор действительно сериализует
//     доступ к шине. PCF8574: биты, записанные 0, жёстко прижаты к земле.
//     Значит, после любой записи чтение обязано вернуть 0 в этих битах.
// ---------------------------------------------------------------------------

TEST_F(Pcf8574Test, ConcurrentTransfersAreAtomic) {
    constexpr int kThreads    = 8;
    constexpr int kIterations = 200;

    std::atomic<int> failures{0};
    std::vector<std::thread> workers;
    workers.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            // У каждого потока свой битовый шаблон
            const unsigned char pattern =
                static_cast<unsigned char>(0xFFu & ~(1u << (t % 8)));
            for (int i = 0; i < kIterations; ++i) {
                const unsigned char tx[] = {pattern};
                unsigned char rx[1] = {0xFF};
                const bool ok = bus->transfer(
                    PCF8574_ADDR,
                    std::span<const unsigned char>(tx, 1),
                    std::span<unsigned char>(rx, 1),
                    1);
                // Биты, записанные как 0, обязаны прочитаться как 0.
                // Биты, записанные как 1, — «не важно» (floating/вход).
                if (!ok || (rx[0] & static_cast<unsigned char>(~pattern)) != 0) {
                    ++failures;
                }
            }
        });
    }
    for (auto& w : workers) w.join();

    EXPECT_EQ(failures.load(), 0)
        << failures.load() << " транзакций вернули несогласованные данные — "
        << "семафор не защищает шину";
}

TEST_F(Pcf8574Test, ConcurrentRawWritesDoNotCrash) {
    constexpr int kThreads = 4;
    constexpr int kIters   = 500;

    std::atomic<int> failures{0};
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (int i = 0; i < kIters; ++i) {
                const unsigned char tx[] =
                    {static_cast<unsigned char>(t * 50 + i)};
                if (!bus->writeRaw(PCF8574_ADDR, tx)) ++failures;
            }
        });
    }
    for (auto& w : workers) w.join();

    EXPECT_EQ(failures.load(), 0);
}

// ---------------------------------------------------------------------------
//  6. writeReg — для PCF8574 «адрес регистра» не имеет смысла (каждый байт
//     трактуется как новое состояние выходов), но вызов не должен падать.
// ---------------------------------------------------------------------------

TEST_F(Pcf8574Test, WriteRegCallSucceeds) {
    EXPECT_TRUE(bus->writeReg(PCF8574_ADDR, 0xFF, 0x00));
    sleep_ms(10);
    resetOutputs(); // вернуть шину в известное состояние
}

TEST_F(Pcf8574Test, WriteRegToMissingDeviceFails) {
    EXPECT_FALSE(bus->writeReg(BAD_ADDR, 0x00, 0x00));
}