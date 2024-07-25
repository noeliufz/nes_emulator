//
// Created by Fangzhou Liu on 22/11/2023.
//

#include "Bus.h"
#include <iostream>

namespace EM
{
Bus::Bus()
{
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
    if (addr >= RAM && addr <= RAM_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00000111'11111111;
        ram[mirror_down_addr] = data;
    }
    else if (addr >= PPU_REGISTERS && addr <= PPU_REGISTERS_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00100000'00000111;
        // TODO: PPU is not supported yet
        std::cerr << "PPU is not supported yet" << std::endl;
    }
    else
    {
        std::cerr << "Ignoring mem write-access at " << addr << std::endl;
    }
}

// read data from RAM
uint8_t Bus::read(uint16_t addr)
{
    // ensure the address range not out of bound
    if (addr >= RAM && addr <= RAM_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00000111'11111111;
        return ram[mirror_down_addr];
    }
    else if (addr >= PPU_REGISTERS && addr <= PPU_REGISTERS_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00100000'00000111;
        // TODO: PPU is not supported yet
        std::cerr << "PPU is not supported yet" << std::endl;
        return 0;
    }
    else
    {
        std::cerr << "Ignoring mem access at " << addr << std::endl;
        return 0;
    }
}

} // namespace EM
