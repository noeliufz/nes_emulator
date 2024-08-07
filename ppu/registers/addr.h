#ifndef MYNESEMULATOR__ADDR_H_
#define MYNESEMULATOR__ADDR_H_

#include <cstdint>
#include <utility>
#include <array>

namespace EM
{
class AddrRegister
{
  private:
    std::array<uint8_t, 2> value;
    bool hi_ptr;

  public:
    AddrRegister();
    void set(uint16_t data);
    uint16_t get();
    void update(uint8_t data);
    void increment(uint8_t inc);
    void reset_latch();
};
} // namespace EM

#endif
