//
// Created by Fangzhou Liu on 22/11/2023.
//

#ifndef MYNESEMULATOR__CPU_H_
#define MYNESEMULATOR__CPU_H_

#include <cstdint>

namespace EM {
class Bus;

class CPU {
public:
  CPU();
  ~CPU();

public:
  // Registers
  uint8_t r_a = 0x00;   // accumulator
  uint8_t r_x = 0x00;   // index register x
  uint8_t r_y = 0x00;   // index register y
  uint8_t r_sp = 0x00;  // stack pointer
  uint16_t r_pc = 0x00; // program counter
  uint8_t r_p = 0x00;   // process status (flag)

  // Flags
  enum FLAG {
    C = (1 << 0), // Carry bit
    Z = (1 << 1), // Zero
    I = (1 << 2), // Interrupt disable
    D = (1 << 3), // Decimal mode (not used in NES emulator)
    B = (1 << 4), // Break
    U = (1 << 5), // *Unused*
    V = (1 << 6), // Overflow
    N = (1 << 7), // Negative
  };

  // Linkage with bus
  EM::Bus *bus = nullptr;

public:
  // Connect with bus
  void connectBus(Bus *b) { bus = b; }

  // Read & write from & to the bus
  uint8_t read(uint16_t addr) const;
  void write(uint16_t addr, uint8_t data) const;

  uint16_t read_u16(uint16_t addr) const;
  void write_u16(uint16_t addr, uint8_t data) const;

private:
  // Get and update flag
  bool getFlag(FLAG f) const;
  void setFlag(FLAG f, bool v);
};
} // namespace EM

#endif // MYNESEMULATOR__CPU_H_
