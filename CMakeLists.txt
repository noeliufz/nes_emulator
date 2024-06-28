cmake_minimum_required(VERSION 3.26)
project(MyNesEmulator)

set(CMAKE_CXX_STANDARD 17)

add_executable(cpu_test cpu_test.cpp
        CPU.h
        Bus.cpp
        Bus.h
        CPU.cpp
)
add_executable(Emulator main.cpp
        CPU.h
        Bus.cpp
        Bus.h
        CPU.cpp
)
