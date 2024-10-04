#include "../ppu/ppu.h"
#include "frame.h"

#include <array>
#include <cstddef>

namespace EM
{

struct Rect
{
    size_t x1;
    size_t x2;
    size_t y1;
    size_t y2;

    Rect(size_t x1, size_t x2, size_t y1, size_t y2) : x1(x1), x2(x2), y1(y1), y2(y2)
    {
    }
};

std::array<uint8_t, 4> bg_palette(const NesPPU &ppu, std::vector<uint8_t> &attribute_table, size_t tile_column,
                                  size_t tile_row);
std::array<uint8_t, 4> sprite_palette(const NesPPU &ppu, uint8_t pallete_idx);
void render(const NesPPU &ppu, Frame &frame);
void render_name_table(const NesPPU &ppu, Frame &frame, std::vector<uint8_t> &name_table, Rect &view_port, int shift_x,
                       int shift_y);
} // namespace EM
