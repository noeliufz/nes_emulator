#ifndef MYNESEMULATOR__FRAME_H_
#define MYNESEMULATOR__FRAME_H_

#include <cstddef>
#include <cstdint>
#include <tuple>
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

    void set_pixel(std::size_t x, std::size_t y, std::tuple<uint8_t, uint8_t, uint8_t> rgb)
    {
        std::size_t base = y * 3 * WIDTH + x * 3;
        if (base + 2 < data.size())
        {
            data[base] = std::get<0>(rgb);
            data[base + 1] = std::get<1>(rgb);
            data[base + 2] = std::get<2>(rgb);
        }
    }

    std::vector<uint8_t> data;
};
} // namespace EM

#endif
