cmake_minimum_required(VERSION 3.16.3)

project(test)

# some variables 如果使用 llvm 的链接器，需要加上下面的 flags
set (LINK_FLAGS "-lc++ -lc++abi -fuse-ld=lld")

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
#set (CMAKE_C_COMPILER "clang")         # 不知道为什么有 bug
#set (CMAKE_CXX_COMPILER "clang++")
set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_BUILD_TYPE Debug)

# 要完全使用 clang 编译器，就需要链接 libc++ 以及 libc++abi。以及使用链接器 lld 如果使用 gcc就不需要这些选项
set (CMAKE_SHARED_LINKER_FLAGS "${LINK_FLAGS}")
set (CMAKE_EXE_LINKER_FLAGS "${LINK_FLAGS}")

add_executable(${PROJECT_NAME}  linux_alsa.c)

target_link_libraries(${PROJECT_NAME}
        pthread
        simplelogs
        asound
)