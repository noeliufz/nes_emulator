//
// Created by Fangzhou Liu on 22/11/2023.
//

#ifndef MYNESEMULATOR__BUS_H_
#define MYNESEMULATOR__BUS_H_

#include <array>
#include <cstdint>

#include "CPU.h"

namespace EM {
class Bus {
public:
    Bus();
    ~Bus();

    // CPU 6502
    CPU cpu;
    // RAM
    std::array<uint8_t, 0xFFFF> ram{};

    // Read & write from & to the bus
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr);
};
} // namespace EM

#endif // MYNESEMULATOR__BUS_H_
