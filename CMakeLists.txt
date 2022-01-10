cmake_minimum_required(VERSION 3.16.3)

project(test)

# some variables 如果使用 llvm 的链接器，需要加上下面的 flags
set (LINK_FLAGS "-lc++ -lc++abi -fuse-ld=lld")

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set (CMAKE_C_COMPILER "clang")         # 不知道为什么有 bug
#set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_BUILD_TYPE Debug)
# -DMICRO_SECONDS logs 开启毫秒精度， -DPLATFORM_LINUX 指定编译平台 -DLOG_TEST 开启 log 工具
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I \"${PROJECT_SOURCE_DIR}/Source/Shared/\" -D PLATFORM_LINUX -DMICRO_SECONDS -DLOG_TEST")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I \"${PROJECT_SOURCE_DIR}/Source/MACLib/\" -I \"${PROJECT_SOURCE_DIR}/Source/utils/\" -O0 -g")

# 要完全使用 clang 编译器，就需要链接 libc++ 以及 libc++abi。以及使用链接器 lld 如果使用 gcc就不需要这些选项
set (CMAKE_SHARED_LINKER_FLAGS "${LINK_FLAGS}")
set (CMAKE_EXE_LINKER_FLAGS "${LINK_FLAGS}")

# 尝试使用 asan 
#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

add_subdirectory(Source)

#add_executable(mac  ./Source/Console/Console.cpp)
#add_executable(${PROJECT_NAME}  compress_demo.cpp)
add_executable(${PROJECT_NAME}  compress_demo2.cpp)
target_include_directories(${PROJECT_NAME}
    PUBLIC 
        "${PROJECT_SOURCE_DIR}/Source/Shared/"
        "${PROJECT_SOURCE_DIR}/Source/MACLib/"
        "${PROJECT_SOURCE_DIR}/Source/utils/"
)

target_link_libraries(${PROJECT_NAME}
        MAC
)