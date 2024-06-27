#include "Bus.h"
#include "CPU.h"
#include <cassert>
#include <iostream>

void test_ops_working_together() {
    auto bus = EM::Bus();
    bus.cpu.load_and_run(std::vector<uint8_t>{0xa9, 0xc0, 0xaa, 0xe8, 0x00});
    assert(0xc1 == bus.cpu.registers.x);
}

void test_0xa9_lda_immediate_load_data() {
    auto bus = EM::Bus();
    bus.cpu.load_and_run(std::vector<uint8_t>{0xa9, 0x05, 0x00});

    assert(0x5 == bus.cpu.registers.a);
    assert(0x5 == bus.cpu.read(0x8001));
    assert(0b00 == (bus.cpu.registers.p & 0b0000'0010));
    assert(0b00 == (bus.cpu.registers.p & 0b0000'0000));
}

int main() {
    std::cout << "Test Bus and CPU" << std::endl;

    // auto bus = EM::Bus();
    // bus.cpu.write(0x100, 0x10);
    // assert(0x10 == bus.cpu.read(0x100));
    // assert(0x10 == bus.ram[0x100]);
    //
    // const EM::OpCode *code = bus.cpu.get_opcode(0x1b);
    // std::cout << static_cast<unsigned>(code->code) << std::endl;
    // std::cout << code->mnemonic << std::endl;
    // std::cout << static_cast<unsigned>(code->len) << std::endl;

    test_ops_working_together();
    test_0xa9_lda_immediate_load_data();

    return 0;
}
