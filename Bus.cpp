//
// Created by Fangzhou Liu on 22/11/2023.
//

#include "Bus.h"
#include <iostream>

namespace EM
{
Bus::Bus()
{
    // connect cpu with bus
    cpu.connect_bus(this);
    // clear content of RAM
    for (auto &i : ram)
    {
        i = 0x00;
    }
}

Bus::~Bus() = default;

// write data to RAM
void Bus::write(uint16_t addr, uint8_t data)
{
    // ensure the address range not out of bound
    if (addr < 0x00 || addr > 0xFFFF)
    {
        std::cerr << "Write data out of range of RAM" << std::endl;
        exit(1);
    }

    ram[addr] = data;
}

// read data from RAM
uint8_t Bus::read(uint16_t addr)
{
    // ensure the address range not out of bound
    if (addr < 0x00 || addr > 0xFFFF)
    {
        std::cerr << "Read data out of range of RAM" << std::endl;
        exit(1);
    }

    return ram[addr];
}

} // namespace EM
