#include "control.h"
#include <stdexcept>
namespace EM
{
uint16_t EM::ControlRegister::nametable_addr()
{
    switch (bits & 0b11)
    {
    case 0:
        return 0x2000;
    case 1:
        return 0x2400;
    case 2:
        return 0x2800;
    case 3:
        return 0x2c00;
    default:
        throw std::runtime_error("not possible");
    }
}

uint8_t EM::ControlRegister::vram_addr_increment()
{
    return (bits & VRAM_ADD_INCREMENT) ? 32 : 1;
}

uint16_t EM::ControlRegister::sprt_pattern_addr() const
{
    return (bits & SPRITE_PATTERN_ADDR) ? 0x1000 : 0;
}

uint16_t EM::ControlRegister::bknd_pattern_addr() const
{
    return (bits & BACKGROUND_PATTERN_ADDR) ? 0x1000 : 0;
}

uint8_t EM::ControlRegister::sprite_size()
{
    return (bits & SPRITE_SIZE) ? 16 : 8;
}

uint8_t EM::ControlRegister::master_slave_select()
{
    return (bits & MASTER_SLAVE_SELECT) ? 1 : 0;
}

bool EM::ControlRegister::generate_vblank_nmi()
{
    return bits & GENERATE_NMI;
}

void EM::ControlRegister::update(uint8_t data)
{
    bits = data;
}
} // namespace EM
