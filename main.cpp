#include "Bus.h"
#include "CPU.h"
#include <cassert>
#include <iostream>

int main() {
  std::cout << "Test Bus and CPU" << std::endl;

  EM::Bus bus = EM::Bus();
  bus.cpu.write(0x100, 0x10);
  assert(0x10 == bus.cpu.read(0x100));
  assert(0x10 == bus.ram[0x100]);

  return 0;
}
