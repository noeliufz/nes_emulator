//
// Created by Fangzhou Liu on 22/11/2023.
//
#include "CPU.h"
#include "Bus.h"
#include <_types/_uint16_t.h>
#include <_types/_uint8_t.h>

namespace EM {
///////////////////////////////////////////////////////////////////////////////
// Constructor
CPU::CPU() = default;
CPU::~CPU() = default;

///////////////////////////////////////////////////////////////////////////////
// Bus linkage
uint8_t CPU::read(uint16_t addr) const { return bus->read(addr); }

void CPU::write(uint16_t addr, uint8_t data) const { bus->write(addr, data); }

uint16_t CPU::read_u16(uint16_t addr) const {
  uint8_t lo = read(addr);
  uint8_t hi = read(addr + 1);
  uint16_t data = (hi << 8) | lo;
  return data;
}

///////////////////////////////////////////////////////////////////////////////
// Get and update flags
bool CPU::getFlag(FLAG f) const { return (r_p & f); }

void CPU::setFlag(FLAG f, bool v) {
  if (v)
    r_p |= f;
  else
    r_p &= ~f;
}
} // namespace EM
