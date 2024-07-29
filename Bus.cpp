//
// Created by Fangzhou Liu on 22/11/2023.
//

#include "Bus.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace EM
{
using std::ostringstream;

Bus::Bus()
{
    // clear content of RAM
    for (auto &i : ram)
    {
        i = 0x00;
    };
}

Bus::Bus(Rom *rom)
{
    // clear content of RAM
    for (auto &i : ram)
    {
        i = 0x00;
    };

    this->rom = rom;
}

Bus::~Bus() = default;

// write data to RAM
void Bus::write(uint16_t addr, uint8_t data)
{
    // ensure the address range not out of bound
    if (addr >= RAM && addr <= RAM_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00000111'11111111;
        ram[static_cast<size_t>(mirror_down_addr)] = data;
    }
    else if (addr >= PPU_REGISTERS && addr <= PPU_REGISTERS_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00100000'00000111;
        // TODO: PPU is not supported yet
        std::cerr << "PPU is not supported yet" << std::endl;
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        ostringstream oss;
        oss << "Trying to access 0x" << std::hex << addr;
        throw std::runtime_error(oss.str());
    }
    else
    {
        std::cerr << "Ignoring mem write-access at 0x" << std::hex << addr << std::endl;
    }
}

// read data from RAM
uint8_t Bus::read(uint16_t addr)
{
    // ensure the address range not out of bound
    if (addr >= RAM && addr <= RAM_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00000111'11111111;
        return ram[static_cast<size_t>(mirror_down_addr)];
    }
    else if (addr >= PPU_REGISTERS && addr <= PPU_REGISTERS_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00100000'00000111;
        // TODO: PPU is not supported yet
        std::cerr << "PPU is not supported yet" << std::endl;
        return 0;
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        return read_prg_rom(addr);
    }
    else
    {
        std::cout << "Ignoring mem access at 0x" << std::hex << addr << std::endl;
        std::cerr << "Ignoring mem access at 0x" << std::hex << addr << std::endl;
        return 0;
    }
}

// read prg rom
uint8_t Bus::read_prg_rom(uint16_t addr)
{
    addr -= 0x8000;
    if (rom->prg_rom.size() == 0x4000 && addr >= 0x4000)
    {
        // mirror if needed
        addr = addr % 0x4000;
    }
    return rom->prg_rom[static_cast<size_t>(addr)];
}
} // namespace EM
