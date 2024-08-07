#include "addr.h"
#include <cstdint>
#include <array>

namespace EM
{

EM::AddrRegister::AddrRegister()
{
    value={0,0};
    hi_ptr = true;
}

void EM::AddrRegister::set(uint16_t data)
{
    value[0] = static_cast<uint8_t>((data >> 8));
    value[1] = static_cast<uint8_t>((data & 0xff));
}

uint16_t EM::AddrRegister::get()
{
    return static_cast<uint16_t>(static_cast<uint16_t>(value[0]) << 8 | static_cast<uint16_t>(value[1]));
}

void EM::AddrRegister::update(uint8_t data)
{
    if (hi_ptr)
    {
        value[0] = data;
    }
    else
    {
        value[1] = data;
    }

    if (get() > 0x3fff)
    {
        // mirror down addr above 0x3fff
        set(get() & 0b11111111111111);
    }

    hi_ptr = !hi_ptr;
}

void EM::AddrRegister::increment(uint8_t inc)
{
    auto lo = value[1];
    value[1] = value[1] + inc;
    if (lo > value[1])
    {
        value[0] = value[0] + 1;
    }
    if (get() > 0x3fff)
    {
        // mirro down addr above 0x3fff
        set(get() & 0b11111111111111);
    }
}

void EM::AddrRegister::reset_latch()
{
    hi_ptr = true;
}

} // namespace EM
