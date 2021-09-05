cmake_minimum_required(VERSION 3.16.3)

project(test)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set (CMAKE_C_COMPILER "clang")         # 不知道为什么有 bug
#set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_BUILD_TYPE Debug)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I \"${PROJECT_SOURCE_DIR}/Source/Shared/\" -D PLATFORM_LINUX")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I \"${PROJECT_SOURCE_DIR}/Source/MACLib/\"  -O0")

add_subdirectory(Source)

#add_executable(mac  ./Source/Console/Console.cpp)
add_executable(${PROJECT_NAME}  sample1.cpp)
target_include_directories(${PROJECT_NAME}
    PUBLIC 
        "${PROJECT_SOURCE_DIR}/Source/Shared/"
        "${PROJECT_SOURCE_DIR}/Source/MACLib/"
)

target_link_libraries(${PROJECT_NAME}
        MAC
)