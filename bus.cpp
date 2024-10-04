//
// Created by Fangzhou Liu on 22/11/2023.
//

#include "bus.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace EM
{
using std::ostringstream;

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
    else if (addr == 0x2000)
    {
        ppu->write_to_ctrl(data);
    }
    else if (addr == 0x2001)
    {
        ppu->write_to_mask(data);
    }
    else if (addr == 0x2002)
    {
        throw std::runtime_error("attemp to write to PPU status register");
    }
    else if (addr == 0x2003)
    {
        ppu->write_to_oam_addr(data);
    }
    else if (addr == 0x2004)
    {
        ppu->write_to_oam_data(data);
    }
    else if (addr == 0x2005)
    {
        ppu->write_to_scroll(data);
    }
    else if (addr == 0x2006)
    {
        ppu->write_to_ppu_addr(data);
    }
    else if (addr == 0x2007)
    {
        ppu->write_to_data(data);
    }
    else if ((addr >= 0x4000 && addr <= 0x4013) || addr == 0x4015)
    {
        // ignore APU
    }
    else if (addr == 0x4016)
    {
        joypad1.write(data);
    }
    else if (addr == 0x4017)
    {
        // ignore joypad 2;
    }
    else if (addr == 0x4014)
    {
        std::array<uint8_t, 256> buffer = {0};
        auto hi = static_cast<uint16_t>(static_cast<uint16_t>(data) << 8);
        for (uint16_t i = 0; i < 256; ++i)
        {
            buffer[i] = read(hi + i);
        }

        ppu->write_oam_dma(buffer);
    }
    else if (addr >= 0x2008 && addr <= PPU_REGISTERS_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00100000'00000111;
        write(mirror_down_addr, data);
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        ostringstream oss;
        oss << "Attempt to write to cartridge ROM space 0x" << std::hex << addr;
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
        return ram[mirror_down_addr];
    }
    else if (addr == 0x2000 || addr == 0x2001 || addr == 0x2003 || addr == 0x2005 || addr == 0x2006 || addr == 0x4014)
    {
        throw std::runtime_error("Attempt to read from write-only PPU address 0x" + std::to_string(addr));
        return 0;
    }
    else if (addr == 0x2002)
    {
        return ppu->read_status();
    }
    else if (addr == 0x2004)
    {
        return ppu->read_oam_data();
    }
    else if (addr == 0x2007)
    {
        return ppu->read_data();
    }
    else if (addr >= 0x4000 && addr <= 0x4015)
    {
        // ignore APU
        return 0;
    }
    else if (addr == 0x4016)
    {
        return joypad1.read();
    }
    else if (addr == 0x4017)
    {
        // ignore joypad 2
        return 0;
    }

    else if (addr >= 0x2008 && addr <= PPU_REGISTERS_MIRRORS_END)
    {
        uint16_t mirror_down_addr = addr & 0b00100000'00000111;
        return read(mirror_down_addr);
    }
    else if (addr >= 0x8000 && addr <= 0xFFFF)
    {
        return read_prg_rom(addr);
    }
    else
    {
        std::cerr << "Ignoring mem access at 0x" << std::hex << addr << std::endl;
        return 0;
    }
}

// read prg rom
uint8_t Bus::read_prg_rom(uint16_t addr) const
{
    addr -= 0x8000;
    if (rom->prg_rom.size() == 0x4000 && addr >= 0x4000)
    {
        // std::cout << "Mirror in PRG" << std::endl;
        // mirror if needed
        addr = addr % 0x4000;
    }
    // std::cout << "PRG read: 0x" << std::hex << addr << std::endl;
    return rom->prg_rom[addr];
}

void Bus::tick(uint8_t cycle)
{
    cycles += static_cast<size_t>(cycle);

    auto nmi_before = ppu->nmi_interrupt.has_value();
    auto f = ppu->tick(cycle * 3);
    auto nmi_after = ppu->nmi_interrupt.has_value();

    if (!nmi_before && nmi_after)
    // if (f)
    {
        gameloop_callback(*ppu, joypad1);
    }
}

std::optional<uint8_t> Bus::poll_nmi_status() const
{
    if (ppu->nmi_interrupt.has_value())
    {
        auto result = ppu->nmi_interrupt;
        ppu->nmi_interrupt.reset();
        return result;
    }

    return ppu->nmi_interrupt;
}
} // namespace EM
