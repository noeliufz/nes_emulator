//
// Created by Fangzhou Liu on 22/11/2023.
//

#ifndef MYNESEMULATOR__BUS_H_
#define MYNESEMULATOR__BUS_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include "cartridge.h"

#include "joypad.h"
#include "ppu/ppu.h"

namespace EM
{
const uint16_t RAM = 0x0000;
const uint16_t RAM_MIRRORS_END = 0x1FFF;
const uint16_t PPU_REGISTERS = 0x2000;
const uint16_t PPU_REGISTERS_MIRRORS_END = 0x3FFF;

class Bus
{
  public:
    Bus();
    // Bus(Rom *rom);
    ~Bus();

    Joypad joypad1;

    size_t cycles;
    std::array<uint8_t, 2048> ram{};
    Rom *rom = nullptr;

    std::unique_ptr<NesPPU> ppu;

    std::function<void(NesPPU &, Joypad &)> gameloop_callback;
    template <typename F> Bus(Rom *rom, F gameloop_callback);

    // Read & write from & to the bus
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr);
    uint8_t read_prg_rom(uint16_t addr) const;

    void tick(uint8_t cycle);
    std::optional<uint8_t> poll_nmi_status() const;
};
// Template constructor implementation
template <typename F> Bus::Bus(Rom *rom, F gameloop_callback) : gameloop_callback(gameloop_callback)
{
    this->rom = rom;
    cycles = 0;
    // Initialize RAM to zero
    std::fill(ram.begin(), ram.end(), 0x00);
    // Initialize PPU with ROM data
    ppu = std::make_unique<NesPPU>(rom->chr_rom, rom->screen_mirroring);
}
} // namespace EM
#endif // MYNESEMULATOR__BUS_H_
