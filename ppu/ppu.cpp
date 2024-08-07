#include "ppu.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <vector>

namespace EM
{
void EM::NesPPU::write_to_ppu_addr(uint8_t value)
{
    address_register.update(value);
}
void EM::NesPPU::write_to_ctrl(uint8_t value)
{
    auto before_nmi_status = ctrl.generate_vblank_nmi();
    ctrl.update(value);
    if (!before_nmi_status && ctrl.generate_vblank_nmi() && status.is_in_vblank())
    {
        nmi_interrupt = 1;
    }
}
void EM::NesPPU::write_to_data(uint8_t data)
{
    auto addr = address_register.get();
    if (addr >= 0 && addr <= 0x1fff)
    {
        std::cerr << "attempt to write to chr rom space" << std::endl;
    }
    else if (addr >= 0x2000 && addr <= 0x2fff)
    {
        vram[mirror_vram_addr(addr)] = data;
    }
    else if (addr >= 0x3000 && addr <= 0x3eff)
    {
        throw std::runtime_error("addr " + std::to_string(addr) + " shouldn't be used in reality");
    }
    else if (addr == 0x3F10 || addr == 0x3F14 || addr == 0x3F18 || addr == 0x3F1C)
    {
        uint16_t add_mirror = addr - 0x10;
        palette_table[add_mirror - 0x3F00] = data;
    }
    else if (addr >= 0x3f00 && addr <= 0x3fff)
    {
        palette_table[addr - 0x3f00] = data;
    }
    else
    {
        throw std::runtime_error("unexpected access to mirrored space " + std::to_string(addr));
    }
    increment_vram_addr();
}
void EM::NesPPU::increment_vram_addr()
{
    address_register.increment(ctrl.vram_addr_increment());
}
uint8_t EM::NesPPU::read_status()
{
    auto data = status.snapshot();
    status.reset_vblank_status();
    address_register.reset_latch();
    scroll.reset_latch();
    return data;
}
uint8_t EM::NesPPU::read_data()
{
    auto address = address_register.get();
    increment_vram_addr();

    if (address >= 0 && address <= 0x1fff)
    {
        auto result = internal_data_buf;
        internal_data_buf = chr_rom[address];
        return result;
    }
    else if (address >= 0x2000 && address <= 0x2fff)
    {
        auto result = internal_data_buf;
        internal_data_buf = vram[mirror_vram_addr(address)];
        return result;
    }
    else if (address >= 0x3000 && address <= 0x3eff)
    {
        throw std::runtime_error("address space 0x3000..0x3eff is not expected to be used, requested = " +
                                 std::to_string(address));
    }
    else if (address == 0x3f10 || address == 0x3f14 || address == 0x3f18 || address == 0x3f1c)
    {
        uint16_t add_mirror = address - 0x10;
        return palette_table[(add_mirror - 0x3f00)];
    }
    else if (address >= 0x3f00 && address <= 0x3fff)
    {
        return palette_table[(address - 0x3f00)];
    }
    else
    {
        throw std::runtime_error("unexpected access to mirrored space " + std::to_string(address));
    }
}
uint16_t EM::NesPPU::mirror_vram_addr(uint16_t addr) const
{
    uint16_t mirrored_vram = addr & 0b10111111111111;
    uint16_t vram_index = mirrored_vram - 0x2000;
    uint16_t name_table = vram_index / 0x400;
    if ((mirroring == Mirroring::VERTICAL && (name_table == 2 || name_table == 3)) ||
        (mirroring == Mirroring::HORIZONTAL && name_table == 3))
    {
        return vram_index - 0x800;
    }
    else if (mirroring == Mirroring::HORIZONTAL && (name_table == 1 || name_table == 2))
    {
        return vram_index - 0x400;
    }
    else
    {
        return vram_index;
    }
}
bool EM::NesPPU::tick(uint8_t cycle)
{
    cycles += static_cast<size_t>(cycle);
    if (cycles >= 341)
    {
        if (is_sprite_0_hit(cycles))
        {
            status.set_sprite_zero_hit(true);
        }
        cycles -= 341;
        scanline += 1;

        if (scanline == 241)
        {
            status.set_vblank_status(true);
            status.set_sprite_zero_hit(false);
            if (ctrl.generate_vblank_nmi())
            {
                nmi_interrupt = 1;
            }
        }
        if (scanline >= 262)
        {
            scanline = 0;
            nmi_interrupt = std::nullopt;
            status.set_sprite_zero_hit(false);
            status.reset_vblank_status();
            return true;
        }
    }
    return false;
}

bool NesPPU::is_sprite_0_hit(size_t cycle)
{
    auto y = static_cast<size_t>(oam_data[0]);
    auto x = static_cast<size_t>(oam_data[3]);
    return (y == static_cast<size_t>(scanline) && x <= cycle && mask.show_sprites());
}

void NesPPU::write_to_oam_addr(uint8_t value)
{
    oam_addr = value;
}
void NesPPU::write_to_oam_data(uint8_t value)
{
    oam_data[oam_addr] = value;
    ++oam_addr;
}
uint8_t NesPPU::read_oam_data()
{
    return oam_data[oam_addr];
}

void NesPPU::write_oam_dma(const std::array<uint8_t, 256> &data)
{
    for (const auto &x : data)
    {
        oam_data[oam_addr] = x;
        ++oam_addr;
    }
}

void NesPPU::write_to_mask(uint8_t value)
{
    mask.update(value);
}

void NesPPU::write_to_scroll(uint8_t value)
{
    scroll.write(value);
}

} // namespace EM
