#ifndef MYNESEMULATOR__FRAME_H_
#define MYNESEMULATOR__FRAME_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace EM
{
class Frame
{
  public:
    static constexpr std::size_t WIDTH = 256;
    static constexpr std::size_t HEIGHT = 240;

    Frame() : data(WIDTH * HEIGHT * 3, 0)
    {
    }

    void set_pixel(std::size_t x, std::size_t y, std::array<uint8_t, 3> &rgb)
    {
        std::size_t base = y * 3 * WIDTH + x * 3;
        if (base + 2 < data.size())
        {
            data[base] = rgb[0];
            data[base + 1] = rgb[1];
            data[base + 2] = rgb[2];
        }
    }

    std::vector<uint8_t> data;
};
} // namespace EM

#endif
