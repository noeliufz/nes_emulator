#include "../cartridge.h"
#include "registers/addr.h"
#include "registers/control.h"

#include <array>
#include <cstdint>
#include <vector>
namespace EM
{
class NesPPU
{
  public:
    std::vector<uint8_t> chr_rom;
    std::array<uint8_t, 32> palette_table;
    std::array<uint8_t, 2048> vram;
    std::array<uint8_t, 256> oam_data;

    AddrRegister address_register;
    ControlRegister ctrl;

    uint8_t internal_data_buf;

    Mirroring mirroring;

  public:
    NesPPU(const std::vector<uint8_t> &chr_rom, const Mirroring &mirroring) : chr_rom(chr_rom), mirroring(mirroring)
    {
        std::fill(palette_table.begin(), palette_table.end(), 0);
        std::fill(vram.begin(), vram.end(), 0);
        std::fill(oam_data.begin(), oam_data.end(), 0);
    }

    void write_to_ppu_addr(uint8_t value);
    void write_to_ctrl(uint8_t value);
    void write_to_data(uint8_t value);

    void increment_vram_addr();
    uint8_t read_data();
    uint8_t read_status();
    uint16_t mirror_vram_addr(uint16_t addr);
};
} // namespace EM
