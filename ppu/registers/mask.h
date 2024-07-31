#include <cstdint>
#include <vector>
namespace EM
{
enum class Color
{
    Red,
    Green,
    Blue,
};

class MaskRegister
{
  public:
    enum : uint8_t
    {
        GREYSCALE = 0b00000001,
        LEFTMOST_8PXL_BACKGROUND = 0b00000010,
        LEFTMOST_8PXL_SPRITE = 0b00000100,
        SHOW_BACKGROUND = 0b00001000,
        SHOW_SPRITES = 0b00010000,
        EMPHASISE_RED = 0b00100000,
        EMPHASISE_GREEN = 0b01000000,
        EMPHASISE_BLUE = 0b10000000,
    };

    MaskRegister() : bits(0)
    {
    }

    bool is_grayscale() const;
    bool leftmost_8pxl_background() const;
    bool leftmost_8pxl_sprite() const;
    bool show_background() const;
    bool show_sprites() const;
    std::vector<Color> emphasise() const;
    void update(uint8_t data);

  private:
    uint8_t bits;
    void set(uint8_t flag, bool status);
    void remove(uint8_t flag);
    bool contains(uint8_t flag) const;
};

} // namespace EM
