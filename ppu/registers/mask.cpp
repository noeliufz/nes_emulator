#include "mask.h"

#include <cstdint>
#include <vector>
namespace EM
{

bool MaskRegister::is_grayscale() const
{
    return contains(GREYSCALE);
}

bool MaskRegister::leftmost_8pxl_background() const
{
    return contains(LEFTMOST_8PXL_BACKGROUND);
}

bool MaskRegister::leftmost_8pxl_sprite() const
{
    return contains(LEFTMOST_8PXL_SPRITE);
}

bool MaskRegister::show_background() const
{
    return contains(SHOW_BACKGROUND);
}

bool MaskRegister::show_sprites() const
{
    return contains(SHOW_SPRITES);
}

std::vector<Color> MaskRegister::emphasise() const
{
    std::vector<Color> result;
    if (contains(EMPHASISE_RED))
    {
        result.push_back(Color::Red);
    }
    if (contains(EMPHASISE_BLUE))
    {
        result.push_back(Color::Blue);
    }
    if (contains(EMPHASISE_GREEN))
    {
        result.push_back(Color::Green);
    }
    return result;
}

void MaskRegister::update(uint8_t data)
{
    bits = data;
}

void MaskRegister::set(uint8_t flag, bool status)
{
    if (status)
    {
        bits |= flag;
    }
    else
    {
        bits &= ~flag;
    }
}

void MaskRegister::remove(uint8_t flag)
{
    bits &= ~flag;
}

bool MaskRegister::contains(uint8_t flag) const
{
    return bits & flag;
}
} // namespace EM
