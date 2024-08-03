#include "../ppu/ppu.h"
#include "frame.h"
#include "palette.h"

#include <array>
#include <cstddef>

namespace EM
{
std::array<uint8_t, 4> bg_palette(const NesPPU &ppu, size_t tile_column, size_t tile_row);
std::array<uint8_t, 4> sprite_pallete(const NesPPU &ppu, uint8_t pallete_idx);
void render(const NesPPU &ppu, Frame &frame);
} // namespace EM
