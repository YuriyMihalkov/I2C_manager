project(I2C)

file(GLOB_RECURSE SOURCE "*.cpp" "*.cxx")
file(GLOB_RECURSE HEADER "*.h" "*.hpp")

add_library(${PROJECT_NAME} SHARED ${SOURCE} ${HEADER})

target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR})