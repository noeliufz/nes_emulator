//
// Created by Fangzhou Liu on 22/11/2023.
//
#include "cpu.h"

#include <arm_neon.h>
#include <cstddef>
#include <cstdint>

#include "bus.h"
#include "op_code.h"
#include <_types/_uint16_t.h>
#include <_types/_uint8_t.h>
#include <iostream>
#include <pthread.h>
#include <stdexcept>
#include <vector>

namespace EM
{

bool CPU::page_cross(uint16_t addr1, uint16_t addr2)
{
    if (static_cast<uint16_t>(addr1 & 0xff00) != static_cast<uint16_t>(addr2 & 0xff00))
    {
        return true;
    }
    else
    {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
// Constructor
CPU::CPU()
{
    const EM::OpCodeSingleton &instance{EM::OpCodeSingleton::get_instance()};
    registers.a = 0;
    registers.x = 0;
    registers.y = 0;
    registers.sp = STACK_RESET;
    registers.p = 0b100100; // status
    registers.pc = 0;       // program counter
};

CPU::CPU(Bus *bus)
{
    const EM::OpCodeSingleton &instance{EM::OpCodeSingleton::get_instance()};
    registers.a = 0;
    registers.x = 0;
    registers.y = 0;
    registers.sp = STACK_RESET;
    registers.p = 0b100100; // status
    registers.pc = 0;       // program counter
    this->bus = bus;
};

CPU::~CPU(){};

//////////////////////////////////////////////////////////////////////////////
// Bus linkage
uint8_t CPU::read(uint16_t addr) const
{
    return bus->read(addr);
}

void CPU::write(uint16_t addr, uint8_t data)
{
    bus->write(addr, data);
}

uint16_t CPU::read_u16(uint16_t addr) const
{
    auto lo = static_cast<uint16_t>(read(addr));
    auto hi = static_cast<uint16_t>(read(addr + 1));
    auto data = static_cast<uint16_t>(static_cast<uint16_t>(hi << 8) | lo);
    return data;
}

void CPU::write_u16(uint16_t addr, uint16_t data)
{
    uint8_t hi = (data >> 8);
    uint8_t lo = (data & 0xff);
    bus->write(addr, lo);
    bus->write(addr + 1, hi);
}

///////////////////////////////////////////////////////////////////////////////
// Reset and load
void CPU::reset()
{
    registers.a = 0;
    registers.x = 0;
    registers.y = 0;
    registers.sp = STACK_RESET;
    registers.p = 0b100100; // status
    registers.pc = read_u16(0xFFFC);
}

void CPU::load(std::vector<uint8_t> program)
{
    std::memcpy((bus->ram).data() + 0x0600, program.data(), program.size());
    // write_u16(0xFFFC, 0x0600);
}

void CPU::load_and_run(std::vector<uint8_t> program)
{
    load(program);
    reset();
    run();
}

void CPU::interrupt(Interrupt i)
{
    stack_push_u16(registers.pc);

    auto flag = registers.p;
    bool v = (i.b_flag_mask & 0b010000) != 0;
    if (v)
        flag |= B; // set status to true
    else
        flag &= ~B; // clear status

    v = (i.b_flag_mask & 0b100000) != 0;
    if (v)
        flag |= U; // set status to true
    else
        flag &= ~U; // clear status

    stack_push(flag);

    set_flag(I, true);

    bus->tick(i.cpu_cycles);
    registers.pc = read_u16(i.vector_addr);
}

///////////////////////////////////////////////////////////////////////////////
// Get and update flags
bool CPU::get_flag(CpuFlags f)
{
    return (registers.p & f);
}

void CPU::set_flag(CpuFlags f, bool v)
{
    if (v)
        registers.p |= f; // set status to true
    else
        registers.p &= ~f; // clear status
}

void CPU::update_zero_and_negative_flags(const uint8_t result)
{
    set_flag(Z, result == 0);      // update zero flag
    set_flag(N, result >> 7 == 1); // update negative flag
}

void CPU::update_negative_flags(const uint8_t result)
{
    set_flag(N, result & 0x80);
}

///////////////////////////////////////////////////////////////////////////////
// Get operand address in different addressing mode
std::pair<uint16_t, bool> CPU::get_absolute_address(const AddressingMode &mode, uint16_t addr)
{
    switch (mode)
    {
    case ZeroPage:
        return {static_cast<uint16_t>(read(addr)), false};

    case Absolute:
        return {read_u16(addr), false};

    case ZeroPage_X: {
        auto pos = read(addr);
        // wrapping add
        auto addr_new = static_cast<uint16_t>(static_cast<uint8_t>(pos + registers.x));
        return {addr_new, false};
    }
    case ZeroPage_Y: {
        auto pos = static_cast<uint16_t>(read(addr));
        // wrapping add
        auto addr_new = static_cast<uint16_t>(static_cast<uint8_t>(pos + registers.y));
        return {addr_new, false};
    }

    case Absolute_X: {
        auto base = read_u16(addr);
        auto addr_new = static_cast<uint16_t>(base + static_cast<uint16_t>(registers.x));
        return {addr_new, page_cross(base, addr_new)};
    }
    case Absolute_Y: {
        auto base = read_u16(addr);
        auto addr_new = static_cast<uint16_t>(base + static_cast<uint16_t>(registers.y));
        return {addr_new, page_cross(base, addr_new)};
    }

    case Indirect_X: {
        auto base = read(addr);
        uint8_t ptr = static_cast<uint8_t>(base + registers.x);
        auto lo = read(static_cast<uint16_t>(ptr));
        auto hi = read(static_cast<uint16_t>((static_cast<uint8_t>(ptr + 1))));
        return {static_cast<uint16_t>((static_cast<uint16_t>(hi) << 8 | (static_cast<uint16_t>(lo)))), false};
    }
    case Indirect_Y: {
        auto base = read(addr);
        auto lo = read(static_cast<uint16_t>(base));
        auto hi = read(static_cast<uint16_t>((static_cast<uint8_t>(base + 1))));
        auto deref_base = static_cast<uint16_t>(static_cast<uint16_t>(hi) << 8 | (static_cast<uint16_t>(lo)));
        auto deref = static_cast<uint16_t>(deref_base + (static_cast<uint16_t>(registers.y)));
        return {deref, page_cross(deref, deref_base)};
    }

    default: {
        throw std::runtime_error("Unknown supported");
    }
    }
}

std::pair<uint16_t, bool> CPU::get_operand_address(const AddressingMode &mode)
{
    switch (mode)
    {
    case Immediate:
        return {registers.pc, false};
    default:
        return get_absolute_address(mode, registers.pc);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Instructions

void CPU::set_register_a(const uint8_t value)
{
    registers.a = value;
    update_zero_and_negative_flags(registers.a);
}

void CPU::add_to_register_a(uint8_t data)
{
    // Calculate the sum including the carry flag
    uint16_t sum = static_cast<uint16_t>(registers.a) + static_cast<uint16_t>(data) + (get_flag(C) ? 1 : 0);

    // Set carry flag
    set_flag(C, sum > 0xFF);

    uint8_t result = static_cast<uint8_t>(sum);

    // Set overflow flag
    set_flag(V, ((data ^ result) & (registers.a ^ result) & 0x80) != 0);

    // Update the register A
    set_register_a(result);
}
void CPU::sub_from_register_a(uint8_t data)
{
    // not sure
    //	auto negated_data = static_cast<int8_t>(-static_cast<int8_t>(data));
    // Perform wrapping subtraction
    //	auto result = static_cast<uint8_t>(negated_data - 1);

    //	add_to_register_a(result);
    uint8_t negated_data = static_cast<uint8_t>(-static_cast<int8_t>(data) - 1);
    add_to_register_a(negated_data);
}

uint8_t CPU::stack_pop()
{
    ++registers.sp;
    return read(static_cast<uint16_t>(static_cast<uint16_t>(STACK) + static_cast<uint16_t>(registers.sp)));
}
void CPU::stack_push(uint8_t data)
{
    write(static_cast<uint16_t>(static_cast<uint16_t>(STACK) + static_cast<uint16_t>(registers.sp)), data);
    --registers.sp;
}
uint16_t CPU::stack_pop_u16()
{
    uint16_t lo = stack_pop();
    uint16_t hi = stack_pop();
    return static_cast<uint16_t>(hi << 8 | lo);
}
void CPU::stack_push_u16(uint16_t data)
{
    auto hi = static_cast<uint8_t>(data >> 8);
    auto lo = static_cast<uint8_t>(data & 0xff);
    stack_push(hi);
    stack_push(lo);
}

void CPU::LDY(const AddressingMode &mode)
{
    auto [addr, page_cross] = get_operand_address(mode);
    auto data = read(addr);
    registers.y = data;
    update_zero_and_negative_flags(registers.y);
    if (page_cross)
    {
        bus->tick(1);
    }
}
void CPU::LDX(const AddressingMode &mode)
{
    auto [addr, page_cross] = get_operand_address(mode);
    auto data = read(addr);
    registers.x = data;
    update_zero_and_negative_flags(registers.x);
    if (page_cross)
    {
        bus->tick(1);
    }
}

void CPU::LDA(const AddressingMode &mode)
{
    auto [addr, page_cross] = get_operand_address(mode);
    uint8_t value = read(addr);
    set_register_a(value);
    if (page_cross)
    {
        bus->tick(1);
    }
}

void CPU::STA(const AddressingMode &mode)
{
    auto [addr, _] = get_operand_address(mode);
    write(addr, registers.a);
}

void CPU::AND(const AddressingMode &mode)
{
    auto [addr, page_cross] = get_operand_address(mode);
    auto data = read(addr);
    set_register_a((static_cast<uint8_t>(data & registers.a)));
    if (page_cross)
    {
        bus->tick(1);
    }
}
void CPU::EOR(const AddressingMode &mode)
{
    auto [addr, page_cross] = get_operand_address(mode);
    auto data = read(addr);
    set_register_a((data ^ registers.a));
    if (page_cross)
    {
        bus->tick(1);
    }
}
void CPU::ORA(const AddressingMode &mode)
{
    auto [addr, page_cross] = get_operand_address(mode);
    auto data = read(addr);
    set_register_a((data | registers.a));
    if (page_cross)
    {
        bus->tick(1);
    }
}

void CPU::TAX()
{
    registers.x = registers.a;
    update_zero_and_negative_flags(registers.x);
}

void CPU::INX()
{
    // actually it is wrapping add
    registers.x = registers.x + 1;
    update_zero_and_negative_flags(registers.x);
}
void CPU::INY()
{
    registers.y = registers.y + 1;
    update_zero_and_negative_flags(registers.y);
}

void CPU::SBC(const AddressingMode &mode)
{
    // A - M - C̅ -> A
    auto [addr, page_cross] = get_operand_address(mode);
    uint8_t data = read(addr);
    auto value = static_cast<uint16_t>(data);
    uint16_t carry_in = get_flag(C) ? 0 : 1;
    uint16_t result = static_cast<uint16_t>(registers.a) - value - carry_in;
    set_flag(N, result & 0x80);
    set_flag(Z, result == 0);
    set_flag(C, result < 0xff);
    set_flag(V, ((registers.a ^ result) & (registers.a ^ data) & 0x80) != 0);
    registers.a = static_cast<uint8_t>(result & 0xFF);
    if (page_cross)
    {
        bus->tick(1);
    }
}

void CPU::ADC(const AddressingMode &mode)
{
    auto [addr, page_cross] = get_operand_address(mode);
    auto value = read(addr);
    add_to_register_a(value);
    if (page_cross)
    {
        bus->tick(1);
    }
}

void CPU::asl_accumulator()
{
    auto data = registers.a;
    // set carry flag
    set_flag(C, data >> 7 == 1);
    data <<= 1;
    set_register_a(data);
}
uint8_t CPU::ASL(const AddressingMode &mode)
{
    auto [addr, _] = get_operand_address(mode);
    auto data = read(addr);

    set_flag(C, data >> 7 == 1);
    data <<= 1;
    write(addr, data);
    update_zero_and_negative_flags(data);
    return data;
}

void CPU::lsr_accumulator()
{
    auto data = registers.a;
    set_flag(C, (data & 1) == 1);
    data >>= 1;
    set_register_a(data);
}
uint8_t CPU::LSR(const AddressingMode &mode)
{
    auto [addr, _] = get_operand_address(mode);
    auto data = read(addr);

    set_flag(C, (data & 1) == 1);

    data >>= 1;
    write(addr, data);
    update_zero_and_negative_flags(data);
    return data;
}

uint8_t CPU::ROL(const AddressingMode &mode)
{
    auto [addr, _] = get_operand_address(mode);
    auto data = read(addr);
    bool old_carry = get_flag(C);

    set_flag(C, data >> 7 == 1);

    data <<= 1;

    if (old_carry)
    {
        data |= 1;
    }

    write(addr, data);
    update_negative_flags(data);

    return data;
}
void CPU::rol_accumulator()
{
    auto data = registers.a;
    bool old_carry = get_flag(C);

    set_flag(C, data >> 7 == 1);

    data <<= 1;

    if (old_carry)
    {
        data |= 1;
    }

    set_register_a(data);
}

uint8_t CPU::ROR(const AddressingMode &mode)
{
    auto [addr, _] = get_operand_address(mode);
    auto data = read(addr);
    bool old_carry = get_flag(C);

    set_flag(C, (data & 1) == 1);

    data >>= 1;

    if (old_carry)
    {
        data |= 0b1000'0000;
    }

    write(addr, data);
    update_negative_flags(data);

    return data;
}
void CPU::ror_accumulator()
{
    auto data = registers.a;
    bool old_carry = get_flag(C);

    set_flag(C, (data & 1) == 1);

    data >>= 1;

    if (old_carry)
    {
        data |= 0b1000'0000;
    }

    set_register_a(data);
}

uint8_t CPU::INC(const AddressingMode &mode)
{
    auto [addr, _] = get_operand_address(mode);
    uint8_t data = read(addr);
    ++data;
    write(addr, data);
    update_zero_and_negative_flags(data);
    return data;
}

void CPU::DEX()
{
    registers.x = registers.x - 1;
    update_zero_and_negative_flags(registers.x);
}
void CPU::DEY()
{
    --registers.y;
    update_zero_and_negative_flags(registers.y);
}
uint8_t CPU::DEC(const AddressingMode &mode)
{
    auto [addr, _] = get_operand_address(mode);
    auto data = read(addr);
    --data;
    write(addr, data);
    update_zero_and_negative_flags(data);
    return data;
}

void CPU::PLA()
{
    auto data = stack_pop();
    set_register_a(data);
}
void CPU::PLP()
{
    registers.p = stack_pop();
    // Clear BREAK flag
    set_flag(B, false);
    // Set BREAK2 flag
    set_flag(U, true);
}
void CPU::PHP()
{
    // Save previous status
    auto flags = registers.p;
    flags |= B;
    flags |= U;
    stack_push(flags);
}

void CPU::BIT(const AddressingMode &mode)
{
    auto [addr, _] = get_operand_address(mode);
    auto data = read(addr);
    auto and_result = registers.a & data;
    set_flag(Z, and_result == 0);
    set_flag(N, (data & 0b1000'0000) > 0);
    set_flag(V, (data & 0b0100'1000) > 0);
}

void CPU::COMPARE(const AddressingMode &mode, uint8_t compare_with)
{
    auto [addr, page_cross] = get_operand_address(mode);
    auto data = read(addr);
    set_flag(C, data <= compare_with);
    update_zero_and_negative_flags(compare_with - data);
    if (page_cross)
    {
        bus->tick(1);
    }
}

void CPU::BRANCH(bool condition)
{
    if (condition)
    {
        bus->tick(1);

        auto jump = static_cast<int8_t>(read(registers.pc));
        auto jump_addr = static_cast<uint16_t>(static_cast<uint16_t>(registers.pc + 1) + static_cast<uint16_t>(jump));

        if ((static_cast<uint16_t>(registers.pc + 1) & 0xff00) != (jump_addr & 0xff00))
        {
            bus->tick(1);
        }

        registers.pc = jump_addr;
    }
}
} // namespace EM
