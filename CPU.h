//
// Created by Fangzhou Liu on 22/11/2023.
//

#ifndef MYNESEMULATOR__CPU_H_
#define MYNESEMULATOR__CPU_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace EM {
// Registers
struct Registers {
    uint8_t a = 0x00;   // accumulator
    uint8_t x = 0x00;   // index register x
    uint8_t y = 0x00;   // index register y
    uint8_t sp = 0x00;  // stack pointer
    uint16_t pc = 0x00; // program counter
    uint8_t p = 0x00;   // process status (flag)
};

// Flags
enum CpuFlags {
    C = (1 << 0),
    // Carry bit
    Z = (1 << 1),
    // Zero
    I = (1 << 2),
    // Interrupt disable
    D = (1 << 3),
    // Decimal mode (not used in NES emulator)
    B = (1 << 4),
    // Break
    U = (1 << 5),
    // *Unused*
    V = (1 << 6),
    // Overflow
    N = (1 << 7),
    // Negative
};

// Addressing modes
enum AddressingMode {
    Immediate,
    ZeroPage,
    ZeroPage_X,
    ZeroPage_Y,
    Absolute,
    Absolute_X,
    Absolute_Y,
    Indirect_X,
    Indirect_Y,
    NoneAddressing,
};

struct OpCode {
    uint8_t code;
    std::string mnemonic;
    uint8_t len;
    uint8_t cycles;
    AddressingMode mode;
    // constructor
    OpCode(int i_code, const char *str, int i_len, int i_cycles,
           AddressingMode addressing_mode)
        : code(i_code), mnemonic(str), len(i_len), cycles(i_cycles),
          mode(addressing_mode) {}
};

class Bus;

class CPU {
public:
    CPU();
    ~CPU();

    // Registers
    Registers registers;
    // Opcode map
    std::unordered_map<uint8_t, const OpCode *> *opcode_map;
    std::vector<const OpCode *> *opcodes = nullptr;
    // Linkage with bus
    EM::Bus *bus = nullptr;

    // Connect with bus
    void connect_bus(Bus *b) { bus = b; }

    // Read & write from & to the bus
    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t data) const;
    uint16_t read_u16(uint16_t addr) const;
    void write_u16(uint16_t addr, uint8_t data) const; // todo

    void init_opcodes();
    void init_opcode_map();
    const OpCode *get_opcode(uint8_t code) const;

private:
    // Get and update flag
    bool get_flag(CpuFlags f) const;
    void set_flag(CpuFlags f, bool v);
};
} // namespace EM

#endif // MYNESEMULATOR__CPU_H_
