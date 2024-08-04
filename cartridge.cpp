#include "cartridge.h"
#include <iostream>
namespace EM
{
Rom::Rom()
{
    prg_rom.clear();
    chr_rom.clear();
    mapper = 0;
    screen_mirroring = Mirroring::HORIZONTAL;
}

Rom::Rom(const std::vector<uint8_t> &raw)
{
    if (raw.size() < 16 || std::memcmp(&raw[0], "NES\x1A", 4) != 0)
    {
        throw std::invalid_argument("File is not in iNES file format");
    }

    mapper = (raw[7] & 0b1111'0000) | (raw[6] >> 4);

    uint8_t ines_ver = (raw[7] >> 2) & 0b11;
    if (ines_ver != 0)
    {
        throw std::invalid_argument("NES2.0 format is not supported");
    }

    bool four_screen = raw[6] & 0b1000;
    bool vertical_mirroring = raw[6] & 0b1;
    if (four_screen)
    {
        screen_mirroring = Mirroring::FOUR_SCREEN;
    }
    else if (vertical_mirroring)
    {
        screen_mirroring = Mirroring::VERTICAL;
    }
    else
    {
        screen_mirroring = Mirroring::HORIZONTAL;
    }

    size_t prg_rom_size = raw[4] * PRG_ROM_PAGE_SIZE;
    size_t chr_rom_size = raw[5] * CHR_ROM_PAGE_SIZE;

    bool skip_trainer = raw[6] & 0b100;

    size_t prg_rom_start = 16 + (skip_trainer ? 512 : 0);
    size_t chr_rom_start = prg_rom_start + prg_rom_size;


    if (raw.size() < chr_rom_start + chr_rom_size || raw.size() < prg_rom_start + prg_rom_size)
    {
        throw std::invalid_argument("File is too small for specified PRG and CHR ROM sizes");
    }


    prg_rom.assign(raw.begin() + prg_rom_start, raw.begin() + prg_rom_start + prg_rom_size);
    chr_rom.assign(raw.begin() + chr_rom_start, raw.begin() + chr_rom_start + chr_rom_size);
}

} // namespace EM
