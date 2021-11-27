cmake_minimum_required(VERSION 3.16.0)
project(library_build)

set (CMAKE_BUILD_TYPE Debug)

aux_source_directory(Console EXE_LIST)
#aux_source_directory(MACDll MACDLL_LIST)
aux_source_directory(MACLib MACLIB_LIST)
aux_source_directory(MACLib/Old MACLIB_OLD_LIST)
# 其他 在 windows 系统当中的文件都不需要
set (MACDLL_LIST ./MACDll/MACDll.cpp)
set (SHARED_LIST ./Shared/CharacterHelper.cpp ./Shared/CircleBuffer.cpp ./Shared/CRC.cpp ./Shared/GlobalFunctions.cpp ./Shared/StdLibFileIO.cpp ./Shared/WinFileIO.cpp)

# log 子目录
aux_source_directory(utils LOGS_LIST)

add_library(MAC SHARED ${SHARED_LIST} ${EXE_LIST} ${MACLIB_LIST} ${MACLIB_OLD_LIST} ${MACDLL_LIST} ${LOGS_LIST})
add_library(MAC_STATIC STATIC ${SHARED_LIST} ${EXE_LIST} ${MACLIB_LIST} ${MACLIB_OLD_LIST} ${MACDLL_LIST} ${LOGS_LIST})