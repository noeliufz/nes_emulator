#ifndef MYNESEMULATOR__PALETTE_H_
#define MYNESEMULATOR__PALETTE_H_

#include <array>
#include <cstdint>

namespace EM
{
class SystemPalette
{
  public:
    static const std::array<std::array<uint8_t, 3>, 64> palette;
};
} // namespace EM

#endif
