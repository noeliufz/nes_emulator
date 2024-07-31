//
// Created by Fangzhou Liu on 22/11/2023.
//

#ifndef MYNESEMULATOR__BUS_H_
#define MYNESEMULATOR__BUS_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "CPU.h"
#include "cartridge.h"

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
    Bus(Rom *rom);
    ~Bus();

    size_t cycles;
    std::array<uint8_t, 2048> ram{};
    Rom *rom = nullptr;

    std::unique_ptr<NesPPU> ppu;

    // Read & write from & to the bus
    void write(uint16_t addr, uint8_t data);
    uint8_t read(uint16_t addr);
    uint8_t read_prg_rom(uint16_t addr);

    void tick(uint8_t cycle);
};
} // namespace EM

#endif // MYNESEMULATOR__BUS_H_
