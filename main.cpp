#include "Bus.h"
#include "CPU.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "Test Bus and CPU" << std::endl;

  const auto bus = EM::Bus();
  bus.cpu.write(0x100, 0x10);
  assert(0x10 == bus.cpu.read(0x100));
  assert(0x10 == bus.ram[0x100]);

  const EM::OpCode* code = bus.cpu.get_opcode(0x1b);
  std::cout << static_cast<unsigned>(code->code) << std::endl;
  std::cout << code->mnemonic << std::endl;
  std::cout << static_cast<unsigned>(code->len) << std::endl;

  return 0;
}
