#include <cstdint>
namespace EM
{
class ScrollRegister
{
  public:
    uint8_t scroll_x;
    uint8_t scroll_y;
    bool latch;

    // Constructor
    ScrollRegister() : scroll_x(0), scroll_y(0), latch(false)
    {
    }

    // Write method
    void write(uint8_t data);

    // Reset latch method
    void reset_latch();
};
} // namespace EM
