#include "render.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
namespace EM
{
std::array<uint8_t, 4> bg_palette(const NesPPU &ppu, size_t tile_column, size_t tile_row)
{
    auto attr_table_idx = tile_row / 4 * 8 + tile_column / 4;
    auto attr_byte = ppu.vram[0x3c0 + attr_table_idx];

    auto i = tile_column % 4 / 2;
    auto j = tile_row % 4 / 2;

    auto palette_idx = attr_byte;

    if (i == 0 && j == 0)
    {
        palette_idx = attr_byte & 0b11;
    }
    else if (i == 1 && j == 0)
    {
        palette_idx = (attr_byte >> 2) & 0b11;
    }
    else if (i == 0 && j == 1)
    {
        palette_idx = (attr_byte >> 4) & 0b11;
    }
    else if (i == 1 && j == 1)
    {
        palette_idx = (attr_byte >> 6) & 0b11;
    }
    else
    {
        throw std::runtime_error("should not happen");
    }

    size_t palette_start = 1 + palette_idx * 4;
    return {
        ppu.palette_table[0],
        ppu.palette_table[palette_start],
        ppu.palette_table[palette_start + 1],
        ppu.palette_table[palette_start + 1],
    };
}

std::array<uint8_t, 4> sprite_palette(const NesPPU &ppu, uint8_t palette_idx)
{
    size_t start = 0x11 + (palette_idx * 4);
    return {
        0,
        ppu.palette_table[start],
        ppu.palette_table[start + 1],
        ppu.palette_table[start + 2],
    };
}

void render(const NesPPU &ppu, Frame &frame)
{
    auto bank = ppu.ctrl.bknd_pattern_addr();

    for (auto i = 0; i < 0x3c0; ++i)
    {
        uint16_t tile = ppu.vram[i];
        size_t tile_column = i % 32;
        size_t tile_row = i / 32;
        const auto tile_data = &ppu.chr_rom[(bank + tile * 16)];
        auto palette = bg_palette(ppu, tile_column, tile_row);

        for (auto y = 0; y <= 7; ++y)
        {
            auto upper = tile_data[y];
            auto lower = tile_data[y + 8];

            std::tuple<uint8_t, uint8_t, uint8_t> rgb;
            for (auto x = 7; x >= 0; --x)
            {
                auto value = (1 & lower) << 1 | (1 & upper);
                upper >>= 1;
                lower >>= 1;
                switch (value)
                {
                case 0: {
                    rgb = SystemPalette::palette[ppu.palette_table[0]];
                    break;
                }
                case 1: {
                    rgb = SystemPalette::palette[palette[1]];
                    break;
                }
                case 2: {
                    rgb = SystemPalette::palette[palette[2]];
                    break;
                }
                case 3: {
                    rgb = SystemPalette::palette[palette[3]];
                    break;
                }
                default:
                    throw std::runtime_error("cannot be");
                }
                frame.set_pixel(tile_column * 8, tile_row * 8, rgb);
            }
        }
    }

    for (size_t i = ppu.oam_data.size(); i > 0; i -= 4)
    {
        uint16_t tile_idx = ppu.oam_data[i + 1];
        size_t tile_x = ppu.oam_data[i + 3];
        size_t tile_y = ppu.oam_data[i];

        bool flip_vertical = (ppu.oam_data[i + 2] >> 7) & 1;
        bool flip_horizontal = (ppu.oam_data[i + 2] >> 6) & 1;

        uint8_t palette_idx = ppu.oam_data[i + 2] & 0b11;
        auto sp = sprite_palette(ppu, palette_idx);
        uint16_t bank = ppu.ctrl.sprt_pattern_addr();

        const auto tile_data = &ppu.chr_rom[(bank + tile_idx * 16)];

        for (int y = 0; y <= 7; ++y)
        {
            uint8_t upper = tile_data[y];
            uint8_t lower = tile_data[y + 8];
            for (int x = 7; x >= 0; --x)
            {
                auto value = (1 & lower) << 1 | (1 & upper);

                upper >>= 1;
                lower >>= 1;

                if (value == 0)
                {
                    // skipping coloring the pixel
                    continue;
                }

                std::tuple<uint8_t, uint8_t, uint8_t> rgb;
                switch (value)
                {
                case 1: {
                    rgb = SystemPalette::palette[sp[1]];
                    break;
                }
                case 2: {
                    rgb = SystemPalette::palette[sp[2]];
                    break;
                }
                case 3: {
                    rgb = SystemPalette::palette[sp[3]];
                    break;
                }
                default:
                    throw std::runtime_error("cannot be");
                }

                if (!flip_horizontal && !flip_vertical)
                {
                    frame.set_pixel(tile_x + x, tile_y + y, rgb);
                }
                else if (flip_horizontal && !flip_vertical)
                {
                    frame.set_pixel(tile_x + 7 - x, tile_y + y, rgb);
                }
                else if (!flip_horizontal && flip_vertical)
                {
                    frame.set_pixel(tile_x + x, tile_y + 7 - y, rgb);
                }
                else if (flip_horizontal && flip_vertical)
                {
                    frame.set_pixel(tile_x + 7 - x, tile_y + 7 - y, rgb);
                }
            }
        }
    }
}

} // namespace EM
