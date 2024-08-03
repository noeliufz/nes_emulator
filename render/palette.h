#include <array>
#include <cstdint>
#include <tuple>

namespace EM
{
class SystemPalette
{
  public:
    static const std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 64> &get_palette();
    static const std::array<std::tuple<uint8_t, uint8_t, uint8_t>, 64> palette;
};
} // namespace EM
