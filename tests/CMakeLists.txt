project(i2c_pcf8574_tests)

find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

file(GLOB SOURCES "*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
    I2C
)

include(GoogleTest)
gtest_discover_tests(i2c_pcf8574_tests)