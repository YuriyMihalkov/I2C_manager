project(I2C)

cmake_minimum_required(VERSION 3.28)


# =====================================================================
# 0. Настройки
# =====================================================================

project(I2C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")

# =====================================================================
# 1. Подготовка
# ====================================================================

file(GLOB_RECURSE SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/**.h")

if(NOT DEFINED I2C_DISABLE_TEST)
    option(I2C_DISABLE_TEST "Disable tests" FALSE)
endif()

# =====================================================================
# 2. Сборка
# =====================================================================

add_library(${PROJECT_NAME} SHARED ${SOURCE} ${HEADER})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include())


# =====================================================================
# 3. Тесты
# =====================================================================
if(NOT I2C_DISABLE_TEST)
    find_package(GTest CONFIG REQUIRED)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()