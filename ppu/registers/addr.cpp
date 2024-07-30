#include "addr.h"
#include <cstdint>

namespace EM
{

EM::AddrRegister::AddrRegister()
{
    value = {0, 0};
    hi_ptr = true;
}

void EM::AddrRegister::set(uint16_t data)
{
    value.first = static_cast<uint8_t>((data >> 8));
    value.second = static_cast<uint8_t>((data & 0xff));
}

uint16_t EM::AddrRegister::get()
{
    return (static_cast<uint16_t>(value.first) << 8) | static_cast<uint16_t>(value.second);
}

void EM::AddrRegister::update(uint8_t data)
{
    if (hi_ptr)
    {
        value.first = data;
    }
    else
    {
        value.second = data;
    }

    if (get() > 0x3fff)
    {
        // mirro down addr above 0x3fff
        set(get() & 0b11111111111111);
    }

    hi_ptr = !hi_ptr;
}

void EM::AddrRegister::increment(uint8_t inc)
{
    auto lo = value.second;
    value.second = value.second + inc;
    if (lo > value.second)
    {
        value.first = value.first + 1;
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
