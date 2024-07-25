cmake_minimum_required(VERSION 3.26)
project(MyNesEmulator)

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

# 添加cpu_test可执行文件
add_executable(cpu_test
    Bus.cpp
    Bus.h
    cpu_test.cpp
    CPU.h
    CPU.cpp
    Opcode.h
    Opcode.cpp
)

# 链接SDL2库到cpu_test
target_link_libraries(cpu_test ${SDL2_LIBRARIES})

# 添加cpu_game_test可执行文件
add_executable(cpu_game_test
    cartridge.h
    cartridge.cpp
    cpu_game_test.cpp
    CPU.h
    Bus.cpp
    Bus.h
    CPU.cpp
    Opcode.h
    Opcode.cpp
)

# 链接SDL2库到cpu_test
target_link_libraries(cpu_game_test ${SDL2_LIBRARIES})


# 添加cpu_game_test可执行文件
add_executable(rom_test
    cartridge.h
    cartridge.cpp
    rom_test.cpp
    CPU.h
    Bus.cpp
    Bus.h
    CPU.cpp
    Opcode.h
    Opcode.cpp
)

# 链接SDL2库到cpu_test
target_link_libraries(rom_test ${SDL2_LIBRARIES})

# 添加Emulator可执行文件
add_executable(Emulator
    main.cpp
    CPU.h
    Bus.cpp
    Bus.h
    CPU.cpp
    Opcode.h
    Opcode.cpp
)

# 链接SDL2库到Emulator
target_link_libraries(Emulator ${SDL2_LIBRARIES})
