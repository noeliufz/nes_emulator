#include "CPU.h"
#include <string>

namespace EM
{
std::string trace(EM::CPU &cpu);
std::string to_hex(uint16_t value);
std::string to_hex(uint8_t value);
} // namespace EM
