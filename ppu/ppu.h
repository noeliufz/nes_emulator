#ifndef MYNESEMULATOR__PPU_H_
#define MYNESEMULATOR__PPU_H_

#include "../cartridge.h"
#include "registers/addr.h"
#include "registers/control.h"
#include "registers/mask.h"
#include "registers/scroll.h"
#include "registers/status.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
namespace EM
{
class NesPPU
{
  public:
    std::vector<uint8_t> chr_rom;
    std::array<uint8_t, 32> palette_table;
    std::array<uint8_t, 2048> vram;

    uint8_t oam_addr;
    std::array<uint8_t, 256> oam_data;

    AddrRegister address_register;
    ControlRegister ctrl;
    StatusRegister status;
    ScrollRegister scroll;
    MaskRegister mask;

    uint8_t internal_data_buf;

    size_t cycles;
    uint16_t scanline;

    std::optional<uint8_t> nmi_interrupt;

    Mirroring mirroring;

  public:
    NesPPU(const std::vector<uint8_t> &chr_rom, const Mirroring &mirroring) : chr_rom(chr_rom), mirroring(mirroring)
    {
        std::fill(palette_table.begin(), palette_table.end(), 0);
        std::fill(vram.begin(), vram.end(), 0);
        std::fill(oam_data.begin(), oam_data.end(), 0);
        oam_addr = 0;
        cycles = 0;
        scanline = 0;
        nmi_interrupt = std::nullopt;
    }

    void write_to_ppu_addr(uint8_t value);

    void write_to_ctrl(uint8_t value);
    void write_to_data(uint8_t value);

    void write_to_oam_addr(uint8_t value);
    void write_to_oam_data(uint8_t value);
    uint8_t read_oam_data();
    void write_oam_dma(const std::array<uint8_t, 256> &data);

    void write_to_mask(uint8_t value);

    void write_to_scroll(uint8_t value);

    void increment_vram_addr();
    uint8_t read_data();
    uint8_t read_status();
    uint16_t mirror_vram_addr(uint16_t addr);

    bool tick(uint8_t cycle);
};
} // namespace EM
#endif
