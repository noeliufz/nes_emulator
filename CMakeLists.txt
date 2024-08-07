cmake_minimum_required(VERSION 3.26)
project(MyNesEmulator)
# add_compile_options(-Wconversion -Werror)
set(CMAKE_CXX_STANDARD 17)

# 指定编译器路径
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)

# 自动启用生成 compile_commands.json 文件
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找SDL2库
find_package(SDL2 REQUIRED)

# 打印SDL2库查找结果
if(SDL2_FOUND)
    message(STATUS "SDL2 found!")
    message(STATUS "SDL2 include dirs: ${SDL2_INCLUDE_DIRS}")
    message(STATUS "SDL2 libraries: ${SDL2_LIBRARIES}")
else()
    message(FATAL_ERROR "SDL2 not found!")
endif()

# 包含SDL2头文件
include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB_RECURSE PPU_SOURCES "${CMAKE_SOURCE_DIR}/ppu/*.cpp")
file(GLOB_RECURSE RENDER_SOURCES "${CMAKE_SOURCE_DIR}/render/*.cpp")

set(TESTPPU_FILE "${CMAKE_SOURCE_DIR}/ppu/testppu.cpp")

# 首先将所有源文件添加到 SOURCES 列表中
list(APPEND SOURCES ${PPU_SOURCES} ${RENDER_SOURCES})

# 从 SOURCES 列表中移除 TESTPPU_FILE
list(REMOVE_ITEM SOURCES ${TESTPPU_FILE})

#
add_executable(rom_test
    cartridge.h
    cartridge.cpp
    rom_test.cpp
    trace.h
    trace.cpp
        cpu.h
        bus.cpp
        bus.h
        cpu.cpp
        op_code.h
        op_code.cpp
    ${SOURCES}
        cpu_run.cpp
        joypad.cpp
        joypad.h
)
#
target_link_libraries(rom_test ${SDL2_LIBRARIES})
#
#
add_executable(emulator
    cartridge.h
    cartridge.cpp
    emulator.cpp
    trace.h
    trace.cpp
        cpu.h
        bus.cpp
        bus.h
        cpu.cpp
        op_code.h
        op_code.cpp
    ${SOURCES}
        cpu_run.cpp
        joypad.cpp
        joypad.h
)

target_link_libraries(emulator ${SDL2_LIBRARIES})

add_executable(tile_test
    cartridge.h
    cartridge.cpp
    tile_test.cpp
    trace.h
    trace.cpp
        cpu.h
        bus.cpp
        bus.h
        cpu.cpp
        op_code.h
        op_code.cpp
    ${SOURCES}
        cpu_run.cpp
)

target_link_libraries(tile_test ${SDL2_LIBRARIES})

