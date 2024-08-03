#ifndef MYNESEMULATOR__CONTROL_H_
#define MYNESEMULATOR__CONTROL_H_

#include <cstdint>
namespace EM
{
class ControlRegister
{
  public:
    enum : uint8_t
    {
        NAMETABLE1 = 0b00000001,
        NAMETABLE2 = 0b00000010,
        VRAM_ADD_INCREMENT = 0b00000100,
        SPRITE_PATTERN_ADDR = 0b00001000,
        BACKGROUND_PATTERN_ADDR = 0b00010000,
        SPRITE_SIZE = 0b00100000,
        MASTER_SLAVE_SELECT = 0b01000000,
        GENERATE_NMI = 0b10000000,
    };

    ControlRegister() : bits(0)
    {
    }

    uint16_t nametable_addr();
    uint8_t vram_addr_increment();
    uint16_t sprt_pattern_addr() const;
    uint16_t bknd_pattern_addr() const;
    uint8_t sprite_size();
    uint8_t master_slave_select();
    bool generate_vblank_nmi();
    void update(uint8_t data);

  private:
    uint8_t bits;
};
} // namespace EM
#endif
