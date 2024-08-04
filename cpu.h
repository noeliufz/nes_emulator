//
//
// Created by Fangzhou Liu on 22/11/2023.
//

#ifndef MYNESEMULATOR__CPU_H_
#define MYNESEMULATOR__CPU_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include "bus.h"
#include "op_code.h"

namespace EM
{
// Registers
struct Registers
{
    uint8_t a = 0x00;   // accumulator
    uint8_t x = 0x00;   // index register x
    uint8_t y = 0x00;   // index register y
    uint8_t sp = 0x00;  // stack pointer
    uint16_t pc = 0x00; // program counter
    uint8_t p = 0x00;   // process status (flag)
};

// Flags
enum CpuFlags
{
    C = (1 << 0), // Carry bit
    Z = (1 << 1), // Zero
    I = (1 << 2), // Interrupt disable
    D = (1 << 3), // Decimal mode (not used in NES emulator)
    B = (1 << 4), // Break
    U = (1 << 5), // *Unused* Break 2
    V = (1 << 6), // Overflow
    N = (1 << 7), // Negative
};

enum class InterruptType
{
    NMI,
};

class Interrupt
{
  public:
    InterruptType itype;
    uint16_t vector_addr;
    uint8_t b_flag_mask;
    uint8_t cpu_cycles;

    Interrupt(InterruptType type, uint16_t addr, uint8_t mask, uint8_t cycles)
        : itype(type), vector_addr(addr), b_flag_mask(mask), cpu_cycles(cycles)
    {
    }
};

class CPU
{
  public:
    CPU();
    CPU(EM::Bus *bus);
    ~CPU();

    // egisters
    EM::Registers registers;

    // Opcode map
    const std::unordered_map<uint8_t, std::shared_ptr<OpCode>> &opcode_map =
        EM::OpCodeSingleton::get_instance().get_opcode_map();
    const std::vector<std::shared_ptr<OpCode>> &opcodes = EM::OpCodeSingleton::get_instance().get_opcodes();

    // Linkage with bus
    EM::Bus *bus = nullptr;

    // Connect with bus
    void connect_bus(EM::Bus *b)
    {
        bus = b;
    }

    // Read & write from & to the bus
    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t data);
    uint16_t read_u16(uint16_t addr) const;
    void write_u16(uint16_t addr, uint16_t data);

    // Load and reset functions
    void reset();
    void load(std::vector<uint8_t> program);
    void load_and_run(std::vector<uint8_t> program);
    void run();
    void run_with_callback(std::function<void(CPU &)> callback);
    void interrupt(Interrupt i);

  private:
    // Get and update flag
    bool get_flag(EM::CpuFlags f);
    void set_flag(EM::CpuFlags f, bool v);
    void update_zero_and_negative_flags(const uint8_t result);
    void update_negative_flags(const uint8_t result);

    // set registers and stack
    void set_register_a(const uint8_t value);
    void add_to_register_a(uint8_t data);
    void sub_from_register_a(uint8_t data);
    uint8_t stack_pop();
    void stack_push(uint8_t data);
    uint16_t stack_pop_u16();
    void stack_push_u16(uint16_t data);
    void asl_accumulator();
    void lsr_accumulator();
    void rol_accumulator();
    void ror_accumulator();

  private:
    /*** Instructions ***/
    void LDY(const AddressingMode &mode);
    void LDX(const AddressingMode &mode);
    void LDA(const AddressingMode &mode);
    void STA(const AddressingMode &mode);
    void AND(const AddressingMode &mode);
    void EOR(const AddressingMode &mode);
    void ORA(const AddressingMode &mode);
    void TAX();
    void INX();
    void INY();
    void SBC(const AddressingMode &mode);
    void ADC(const AddressingMode &mode);
    uint8_t ASL(const AddressingMode &mode);
    uint8_t LSR(const AddressingMode &mode);
    uint8_t ROL(const AddressingMode &mode);
    uint8_t ROR(const AddressingMode &mode);
    uint8_t INC(const AddressingMode &mode);
    void DEX();
    void DEY();
    uint8_t DEC(const AddressingMode &mode);
    void PLA();
    void PLP();
    void PHP();
    void BIT(const AddressingMode &mode);
    void COMPARE(const AddressingMode &mode, uint8_t compare_with);
    void BRANCH(bool condition);

	bool page_cross(uint16_t addr1, uint16_t addr2);

  public:
    std::pair<uint16_t, bool> get_absolute_address(const AddressingMode &mode, uint16_t addr);
    std::pair<uint16_t, bool> get_operand_address(const AddressingMode &mode);

  private:
    const uint16_t STACK = 0x0100;
    const uint16_t STACK_RESET = 0xfd;
};

} // namespace EM

#endif // MYNESEMULATOR__CPU_H_
