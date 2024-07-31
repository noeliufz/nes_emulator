//
// Created by Fangzhou Liu on 31/7/2024.
//
#include "scroll.h"
#include <cstdint>
namespace EM
{
// Write method
void ScrollRegister::write(uint8_t data)
{
    if (!latch)
    {
        scroll_x = data;
    }
    else
    {
        scroll_y = data;
    }
    latch = !latch;
}

// Reset latch method
void ScrollRegister::reset_latch()
{
    latch = false;
}
} // namespace EM
