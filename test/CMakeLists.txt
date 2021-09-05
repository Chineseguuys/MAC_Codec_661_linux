cmake_minimum_required(VERSION 3.16.0)

project(test)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_BUILD_TYPE Debug)
set (CMAKE_CXX_FLAGS "-D PLATFORM_LINUX")

add_executable(${PROJECT_NAME} sample1.cpp)

target_include_directories(${PROJECT_NAME}
PUBLIC  
    "../Shared/"
)

target_link_libraries(${PROJECT_NAME}
    MAC_STATIC
)

target_link_directories(${PROJECT_NAME}
PUBLIC
    ../build/Source
)