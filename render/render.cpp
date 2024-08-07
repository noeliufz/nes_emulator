#include "render.h"
#include "palette.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>
namespace EM
{

std::array<uint8_t, 4> bg_palette(const NesPPU &ppu, const uint8_t *attribute_table, size_t tile_column,
                                  size_t tile_row)
{
    auto attr_table_idx = tile_row / 4 * 8 + tile_column / 4;
    auto attr_byte = attribute_table[attr_table_idx];

    auto i = tile_column % 4 / 2;
    auto j = tile_row % 4 / 2;

    size_t palette_idx;

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

    size_t palette_start = 1 + static_cast<size_t>(palette_idx) * 4;
    return {
        ppu.palette_table[0],
        ppu.palette_table[palette_start],
        ppu.palette_table[palette_start + 1],
        ppu.palette_table[palette_start + 2],
    };
}

std::array<uint8_t, 4> sprite_palette(const NesPPU &ppu, uint8_t palette_idx)
{
    size_t start = 0x11 + static_cast<std::size_t>(palette_idx * 4);
    return {
        0,
        ppu.palette_table[start],
        ppu.palette_table[start + 1],
        ppu.palette_table[start + 2],
    };
}

void render_name_table(const NesPPU &ppu, Frame &frame, std::vector<uint8_t> &name_table, Rect view_port, std::ptrdiff_t shift_x,
                       std::ptrdiff_t shift_y)
{
    auto bank = ppu.ctrl.bknd_pattern_addr();
//    auto start = name_table.begin() + 0x3c0;
//    auto end = name_table.begin() + 0x400;
//    auto attribute_table = std::vector<uint8_t>(start, end);
	auto attribute_table = name_table.data() + 0x3c0;
    for (auto i = 0; i < 0x3c0; ++i)
    {
        auto tile_column = i % 32;
        auto tile_row = i / 32;
        auto tile_idx = static_cast<uint16_t>(name_table[i]);
        auto tile = &(ppu.chr_rom[bank + tile_idx * 16]);
        auto p = bg_palette(ppu, attribute_table, tile_column, tile_row);

        for (auto y = 0; y <= 7; ++y)
        {
            auto upper = tile[y];
            auto lower = tile[y + 8];

            for (int x = 7; x >= 0; --x)
            {
                auto value = ((1 & lower) << 1) | (1 & upper);
                upper >>= 1;
                lower >>= 1;
                std::array<uint8_t, 3> rgb;
                switch (value)
                {
                case 0: {
                    rgb = SystemPalette::palette[ppu.palette_table[0]];
                    break;
                }
                case 1: {
                    rgb = SystemPalette::palette[p[1]];
                    break;
                }
                case 2: {
                    rgb = SystemPalette::palette[p[2]];
                    break;
                }
                case 3: {
                    rgb = SystemPalette::palette[p[3]];
                    break;
                }
                default:
                    throw std::runtime_error("cannot be");
                }
                auto pixel_x = static_cast<size_t >(tile_column * 8 + x);
                auto pixel_y = static_cast<size_t >(tile_row * 8 + y);


                if (pixel_x >= view_port.x1 && pixel_x < view_port.x2 && pixel_y >= view_port.y1 &&
                    pixel_y < view_port.y2)
                {
                    frame.set_pixel(static_cast<size_t>(shift_x + static_cast<std::ptrdiff_t>(pixel_x)),
                                    static_cast<size_t>(shift_y + static_cast<std::ptrdiff_t>(pixel_y)), rgb);
                }
            }
        }
    }
}

void render(const NesPPU &ppu, Frame &frame)
{
    auto scroll_x = static_cast<size_t>(ppu.scroll.scroll_x);
    auto scroll_y = static_cast<size_t>(ppu.scroll.scroll_y);


    //    auto bank = ppu.ctrl.bknd_pattern_addr();
    auto &mirr = ppu.mirroring;
    auto nametable_addr = ppu.ctrl.nametable_addr();

    auto main_nametable_start = ppu.vram.begin();
    auto main_nametable_end = ppu.vram.begin();
    auto second_nametable_start = ppu.vram.begin();
    auto second_nametable_end = ppu.vram.begin();

    if ((mirr == Mirroring::VERTICAL && (nametable_addr == 0x2000 || nametable_addr == 0x2800)) ||
        (mirr == Mirroring::HORIZONTAL && (nametable_addr == 0x2000 || nametable_addr == 0x2400)))
    {
        main_nametable_start = ppu.vram.begin();
        main_nametable_end = ppu.vram.begin() + 0x400;
        second_nametable_start = ppu.vram.begin() + 0x400;
        second_nametable_end = ppu.vram.begin() + 0x800;
    }
    else if ((mirr == Mirroring::VERTICAL && (nametable_addr == 0x2400 || nametable_addr == 0x2c00)) ||
             (mirr == Mirroring::HORIZONTAL && (nametable_addr == 0x2800 || nametable_addr == 0x2c00)))
    {
        main_nametable_start = ppu.vram.begin() + 0x400;
        main_nametable_end = ppu.vram.begin() + 0x800;
        second_nametable_start = ppu.vram.begin();
        second_nametable_end = ppu.vram.begin() + 0x400;
    }
    else
    {
        throw std::runtime_error("Not support type");
    }

    std::vector<uint8_t> main_nametable(main_nametable_start, main_nametable_end);
    std::vector<uint8_t> second_nametable(second_nametable_start, second_nametable_end);

    Rect r{scroll_x, scroll_y, 256, 240};
    render_name_table(ppu, frame, main_nametable, r, -static_cast<std::ptrdiff_t>(scroll_x), -static_cast<std::ptrdiff_t>(scroll_y));

    if (scroll_x > 0)
    {
        Rect r{0, 0, scroll_x, 240};
        render_name_table(ppu, frame, second_nametable, r, static_cast<std::ptrdiff_t>(256 - scroll_x), static_cast<std::ptrdiff_t>(0));
    }
    else if (scroll_y > 0)
    {
        Rect r{0, 0, 256, scroll_y};
        render_name_table(ppu, frame, second_nametable, r, static_cast<std::ptrdiff_t >(0), static_cast<std::ptrdiff_t>(240 - scroll_y));
    }

    std::array<uint8_t, 3> rgb;
    for (int i = ppu.oam_data.size() - 4; i >= 0; i = i - 4)
    {
        uint16_t tile_idx = ppu.oam_data[i + 1];
        size_t tile_x = ppu.oam_data[i + 3];
        size_t tile_y = ppu.oam_data[i];

        bool flip_vertical = (ppu.oam_data[i + 2] >> 7) & 1;
        bool flip_horizontal = (ppu.oam_data[i + 2] >> 6) & 1;

        uint8_t palette_idx = ppu.oam_data[i + 2] & 0b11;
        auto sp = sprite_palette(ppu, palette_idx);
        auto bank = ppu.ctrl.sprt_pattern_addr();

        const auto *tile_data = &ppu.chr_rom[bank + tile_idx * 16];

        for (int y = 0; y <= 7; ++y)
        {
            uint8_t upper = tile_data[y];
            uint8_t lower = tile_data[y + 8];
            for (int x = 7; x >= 0; --x)
            {
                auto value = (lower & 1) << 1 | (upper & 1);

                upper >>= 1;
                lower >>= 1;

                if (value == 0)
                {
                    // skipping coloring the pixel
                    continue;
                }

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

                size_t final_x = tile_x + (flip_horizontal ? 7 - x : x);
                size_t final_y = tile_y + (flip_vertical ? 7 - y : y);
                frame.set_pixel(final_x, final_y, rgb);
            }
        }
    }
}

} // namespace EM
