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
    C = (1 << 0), // Carry bit
    Z = (1 << 1), // Zero
    I = (1 << 2), // Interrupt disable
    D = (1 << 3), // Decimal mode (not used in NES emulator)
    B = (1 << 4), // Break
    U = (1 << 5), // *Unused* Break 2
    V = (1 << 6), // Overflow
    N = (1 << 7), // Negative
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
    std::unordered_map<uint8_t, const OpCode *> *opcode_map = nullptr;
    std::vector<const OpCode *> *opcodes = nullptr;

    // Linkage with bus
    EM::Bus *bus = nullptr;

    // Connect with bus
    void connect_bus(Bus *b) { bus = b; }

    // Read & write from & to the bus
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t data);
    uint16_t read_u16(uint16_t addr);
    void write_u16(uint16_t addr, uint16_t data);

    // Load and reset functions
    void reset();
    void load(std::vector<uint8_t> program);
    void load_and_run(std::vector<uint8_t> program);
    void run();

    void init_opcodes();
    void init_opcode_map();
    const OpCode *get_opcode(uint8_t code);

    void interpret(std::vector<uint8_t> program);

private:
    // Get and update flag
    bool get_flag(CpuFlags f);
    void set_flag(CpuFlags f, bool v);
    void update_zero_and_negative_flags(uint8_t result);

    // set registers and stack
    void set_register_a(uint8_t value);
    void add_to_register_a(uint8_t data);
    uint8_t stack_pop();
    void stack_push(uint8_t data);
    uint16_t stack_pop_u16();
    void stack_push_u16(uint16_t data);
    void asl_accumulator();
    void lsr_accumulator();
    void rol_accumulator();
    void ror_accumulator();

private:
    uint16_t get_operand_address(AddressingMode mode);
    /*** Instructions ***/
    void LDY(AddressingMode mode);
    void LDX(AddressingMode mdoe);
    void LDA(AddressingMode mode);
    void STA(AddressingMode mode);
    void AND(AddressingMode mode);
    void EOR(AddressingMode mode);
    void ORA(AddressingMode mode);
    void TAX();
    void INX();
    void INY();
    void SBC(AddressingMode mode);
    void ADC(AddressingMode mode);
    uint8_t ASL(AddressingMode mode);
    uint8_t LSR(AddressingMode mode);
    uint8_t ROL(AddressingMode mode);
    uint8_t ROR(AddressingMode mode);
    uint8_t INC(AddressingMode mode);
    void DEX();
    void DEY();
    uint8_t DEC(AddressingMode mode);
    void PLA();
    void PLP();
    void PHP();
    void BIT(AddressingMode mode);
    void COMPARE(AddressingMode mode, uint8_t compare_with);
    void BRANCH(bool condition);

private:
    const uint16_t STACK = 0x0100;
    const uint16_t STACK_RESET = 0xfd;
};

} // namespace EM

#endif // MYNESEMULATOR__CPU_H_
