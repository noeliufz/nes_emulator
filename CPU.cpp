//
// Created by Fangzhou Liu on 22/11/2023.
//
#include "CPU.h"

#include <arm_neon.h>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "Bus.h"
#include <_types/_uint16_t.h>
#include <_types/_uint8_t.h>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace EM
{
///////////////////////////////////////////////////////////////////////////////
// Constructor
CPU::CPU()
{
    const EM::OpCodeSingleton &instance{EM::OpCodeSingleton::get_instance()};
    opcodes = instance.get_opcodes();
    opcode_map = instance.get_opcode_map();
    registers.a = 0;
    registers.x = 0;
    registers.y = 0;
    registers.sp = STACK_RESET;
    registers.p = 0b100100; // status
    registers.pc = 0;       // program counter

    bus = new Bus();
};

CPU::~CPU()
{
    for (auto [key, value] : *opcode_map)
    {
        delete value;
        value = nullptr;
    }
    for (auto value : *opcodes)
    {
        delete value;
        value = nullptr;
    }
    delete opcodes;
    delete opcode_map;
    opcodes = nullptr;
    opcode_map = nullptr;
};

///////////////////////////////////////////////////////////////////////////////
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
    uint8_t lo = read(addr);
    uint8_t hi = read(addr + 1);
    uint16_t data = (hi << 8) | lo;
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
    write_u16(0xFFFC, 0x0600);
}

void CPU::load_and_run(std::vector<uint8_t> program)
{
    load(program);
    reset();
    run();
}

void CPU::run()
{
    run_with_callback([this](CPU &) {});
}

void CPU::run_with_callback(std::function<void(CPU &)> callback)
{
    uint8_t data0x10 = read(0x10);
    while (true)
    {
        auto code = read(registers.pc);
        ++registers.pc;
        auto state = registers.pc;

        const EM::OpCode *op = nullptr;
        try
        {
            auto it = opcode_map->find(code);
            if (it != opcode_map->end())
            {
                op = it->second;
            }
            else
            {
                std::ostringstream oss;
                oss << "Opcode " << std::hex << static_cast<int>(code) << " is not recognized. ";
                oss << "Program counter: " << std::hex << registers.pc;
                throw std::runtime_error(oss.str());
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << e.what() << std::endl;
        }

        switch (code)
        {
        case 0xa9:
        case 0xa5:
        case 0xb5:
        case 0xad:
        case 0xbd:
        case 0xb9:
        case 0xa1:
        case 0xb1: {
            LDA(op->mode);
            break;
        }

        case 0xaa: {
            TAX();
            break;
        }

        case 0xe8: {
            INX();
            break;
        }

        case 0x00:
            return;

        /* CLD */
        case 0xd8: {
            set_flag(D, false);
            break;
        }
        /* CLI */
        case 0x58: {
            set_flag(I, false);
            break;
        }
        /* CLV */
        case 0xb8: {
            set_flag(V, false);
            break;
        }
        /* CLC */
        case 0x18: {
            set_flag(C, false);
            break;
        }
        /* SEC */
        case 0x38: {
            set_flag(C, true);
            break;
        }
        /* SEI */
        case 0x78: {
            set_flag(I, true);
            break;
        }
        /* SED */
        case 0xf8: {
            set_flag(D, true);
            break;
        }
        /* PHA */
        case 0x48: {
            stack_push(registers.a);
            break;
        }
        /* PLA */
        case 0x68: {
            PLA();
            break;
        }
        /* PHP */
        case 0x08: {
            PHP();
            break;
        }
        /* PLP */
        case 0x28: {
            PLP();
            break;
        }
        /* ADC */
        case 0x69:
        case 0x65:
        case 0x75:
        case 0x6d:
        case 0x7d:
        case 0x79:
        case 0x61:
        case 0x71: {
            ADC(op->mode);
            break;
        }
        /* SBC */
        case 0xe9:
        case 0xe5:
        case 0xf5:
        case 0xed:
        case 0xfd:
        case 0xf9:
        case 0xe1:
        case 0xf1: {
            SBC(op->mode);
            break;
        }
        /* AND */
        case 0x29:
        case 0x25:
        case 0x35:
        case 0x2d:
        case 0x3d:
        case 0x39:
        case 0x21:
        case 0x31: {
            AND(op->mode);
            break;
        }

        /* EOR */
        case 0x49:
        case 0x45:
        case 0x55:
        case 0x4d:
        case 0x5d:
        case 0x59:
        case 0x41:
        case 0x51: {
            EOR(op->mode);
            break;
        }
        /* ORA */
        case 0x09:
        case 0x05:
        case 0x15:
        case 0x0d:
        case 0x1d:
        case 0x19:
        case 0x01:
        case 0x11: {
            ORA(op->mode);
            break;
        }

        /* LSR */
        case 0x4a: {
            lsr_accumulator();
            break;
        }
        /* LSR */
        case 0x46:
        case 0x56:
        case 0x4e:
        case 0x5e: {
            LSR(op->mode);
            break;
        }

        /* ASL */
        case 0x0a: {
            asl_accumulator();
            break;
        }
        /* ASL */
        case 0x06:
        case 0x16:
        case 0x0e:
        case 0x1e: {
            ASL(op->mode);
            break;
        }

        /* ROL */
        case 0x2a: {
            rol_accumulator();
            break;
        }
        /* ROL */
        case 0x26:
        case 0x36:
        case 0x2e:
        case 0x3e: {
            ROL(op->mode);
            break;
        }

        /* ROR */
        case 0x6a: {
            ror_accumulator();
            break;
        }
        case 0x66:
        case 0x76:
        case 0x6e:
        case 0x7e: {
            ROR(op->mode);
            break;
        }

        /* INC */
        case 0xe6:
        case 0xf6:
        case 0xee:
        case 0xfe: {
            INC(op->mode);
            break;
        }

        /* INY */
        case 0xc8: {
            INY();
            break;
        }

        /* DEC */
        case 0xc6:
        case 0xd6:
        case 0xce:
        case 0xde: {
            DEC(op->mode);
            break;
        }

        /* DEX */
        case 0xca: {
            DEX();
            break;
        }

        /* DEY */
        case 0x88: {
            DEY();
            break;
        }

        /* CMP */
        case 0xc9:
        case 0xc5:
        case 0xd5:
        case 0xcd:
        case 0xdd:
        case 0xd9:
        case 0xc1:
        case 0xd1: {
            COMPARE(op->mode, registers.a);
            break;
        }

        /* CPY */
        case 0xc0:
        case 0xc4:
        case 0xcc: {
            COMPARE(op->mode, registers.y);
            break;
        }

        /* CPX */
        case 0xe0:
        case 0xe4:
        case 0xec: {
            COMPARE(op->mode, registers.x);
            break;
        }

        /* JMP Absolute */
        case 0x4c: {
            auto addr = read_u16(registers.pc);
            registers.pc = addr;
            break;
        }

        /* JMP Indirect */
        case 0x6c: {
            auto addr = read_u16(registers.pc);
            // bug info from
            // https://github.com/bugzmanov/nes_ebook/blob/master/code/ch3.4/src/cpu.rs

            uint16_t ref;
            if ((addr & 0x00FF) == 0x00FF)
            {
                auto lo = read(addr);
                auto hi = read(addr & 0xFF00);
                ref = (static_cast<uint16_t>(hi)) << 8 | (static_cast<uint16_t>(lo));
            }
            else
            {
                ref = read_u16(addr);
            }
            registers.pc = ref;
            break;
        }

        /* JSR */
        case 0x20: {
            stack_push_u16(registers.pc + 1);
            auto target = read_u16(registers.pc);
            registers.pc = target;
            break;
        }

        /* RTS */
        case 0x60: {
            registers.pc = stack_pop_u16() + 1;
            break;
        }

        /* RTI */
        case 0x40: {
            registers.p = stack_pop();
            set_flag(B, false);
            set_flag(U, true);
            registers.pc = stack_pop_u16();
            break;
        }

        /* BNE */
        case 0xd0: {
            BRANCH(!get_flag(Z));
            break;
        }
        /* BVS */
        case 0x70: {
            BRANCH(get_flag(V));
            break;
        }
        /* BVC */
        case 0x50: {
            BRANCH(!get_flag(V));
            break;
        }
        /* BPL */
        case 0x10: {
            BRANCH(!get_flag(N));
            break;
        }
        /* BMI */
        case 0x30: {
            BRANCH(get_flag(N));
            break;
        }
        /* BEQ */
        case 0xf0: {
            BRANCH(get_flag(Z));
            break;
        }
        /* BCS */
        case 0xb0: {
            BRANCH(get_flag(C));
            break;
        }
        /* BCC */
        case 0x90: {
            BRANCH(!get_flag(C));
            break;
        }

        /* BIT */
        case 0x24:
        case 0x2c: {
            BIT(op->mode);
            break;
        }

        /* STA */
        case 0x85:
        case 0x95:
        case 0x8d:
        case 0x9d:
        case 0x99:
        case 0x81:
        case 0x91: {
            STA(op->mode);
            break;
        }

        /* STX */
        case 0x86:
        case 0x96:
        case 0x8e: {
            auto addr = get_operand_address(op->mode);
            write(addr, registers.x);
            break;
        }
        /* STY */
        case 0x84:
        case 0x94:
        case 0x8c: {
            auto addr = get_operand_address(op->mode);
            write(addr, registers.y);
            break;
        }

        /* LDX */
        case 0xa2:
        case 0xa6:
        case 0xb6:
        case 0xae:
        case 0xbe: {
            LDX(op->mode);
            break;
        }
        /* LDY */
        case 0xa0:
        case 0xa4:
        case 0xb4:
        case 0xac:
        case 0xbc: {
            LDY(op->mode);
            break;
        }

        /* NOP */
        case 0xea: {
            break;
        }

        /* TAY */
        case 0xa8: {
            registers.y = registers.a;
            update_zero_and_negative_flags(registers.y);
            break;
        }

        /* TSX */
        case 0xba: {
            registers.x = registers.sp;
            update_zero_and_negative_flags(registers.x);
            break;
        }

        /* TXA */
        case 0x8a: {
            registers.a = registers.x;
            update_zero_and_negative_flags(registers.a);
            break;
        }
        /* TXS */
        case 0x9a: {
            registers.sp = registers.x;
            break;
        }

        /* TYA */
        case 0x98: {
            registers.a = registers.y;
            update_zero_and_negative_flags(registers.a);
            break;
        }

        default:
            std::cerr << "Not implemented code: " << std::hex << static_cast<int>(code) << std::endl;
            break;
            // todo
        }

        if (state == registers.pc)
        {
            registers.pc += static_cast<uint16_t>((op->len - 1));
        }

        callback(*this);
    }
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
    set_flag(Z, result == 0);        // update zero flag
    set_flag(N, (result >> 7) == 1); // update negative flag
}

void CPU::update_negative_flags(const uint8_t result)
{
    set_flag(N, (result >> 7) == 1);
}

///////////////////////////////////////////////////////////////////////////////
// Get operand address in different addressing mode
uint16_t CPU::get_operand_address(const AddressingMode &mode)
{
    switch (mode)
    {
    case Immediate:
        return registers.pc;

    case ZeroPage:
        return static_cast<uint16_t>(read(registers.pc));

    case Absolute:
        return read_u16(registers.pc);

    case ZeroPage_X: {
        auto pos = read(registers.pc);
        // wrapping add
        uint16_t addr = static_cast<uint16_t>((pos + registers.x));
        return addr;
    }
    case ZeroPage_Y: {
        auto pos = read(registers.pc);
        // wrapping add
        uint16_t addr = static_cast<uint16_t>((pos + registers.y));
        return addr;
    }

    case Absolute_X: {
        auto base = read_u16(registers.pc);
        uint16_t addr = base + static_cast<uint16_t>(registers.x);
        return addr;
    }
    case Absolute_Y: {
        auto base = read_u16(registers.pc);
        uint16_t addr = base + static_cast<uint16_t>(registers.y);
        return addr;
    }

    case Indirect_X: {
        auto base = read(registers.pc);
        uint8_t ptr = static_cast<uint8_t>(base) + registers.x;
        auto lo = read(static_cast<uint16_t>(ptr));
        auto hi = read(static_cast<uint16_t>((ptr + 1)));
        return static_cast<uint16_t>((static_cast<uint16_t>(hi) << 8 | (static_cast<uint16_t>(lo))));
    }
    case Indirect_Y: {
        auto base = read(registers.pc);
        auto lo = read(static_cast<uint16_t>(base));
        auto hi = read(static_cast<uint16_t>((base + 1)));
        auto deref_base = (static_cast<uint16_t>(hi) << 8 | (static_cast<uint16_t>(lo)));
        auto deref = deref_base + (static_cast<uint16_t>(registers.y));
        return deref;
    }

    case NoneAddressing: {
        throw std::runtime_error("Unknown supported");
    }
    }

    return 0; // todo
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

    uint8_t result = static_cast<uint8_t>(sum & 0xFF);

    // Set overflow flag
    set_flag(V, ((registers.a ^ result) & (data ^ result) & 0x80) != 0);

    // Update the register A
    set_register_a(result);
}
uint8_t CPU::stack_pop()
{
    ++registers.sp;
    return read(STACK + static_cast<uint16_t>(registers.sp));
}
void CPU::stack_push(uint8_t data)
{
    write(STACK + static_cast<uint16_t>(registers.sp), data);
    --registers.sp;
}
uint16_t CPU::stack_pop_u16()
{
    uint16_t lo = stack_pop();
    uint16_t hi = stack_pop();
    return hi << 8 | lo;
}
void CPU::stack_push_u16(uint16_t data)
{
    uint8_t hi = data >> 8;
    uint8_t lo = data & 0xff;
    stack_push(hi);
    stack_push(lo);
}

void CPU::LDY(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
    auto data = read(addr);
    registers.y = data;
    update_zero_and_negative_flags(registers.y);
}
void CPU::LDX(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
    auto data = read(addr);
    registers.x = data;
    update_zero_and_negative_flags(registers.x);
}

void CPU::LDA(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
    auto value = read(addr);
    set_register_a(value);
}

void CPU::STA(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
    write(addr, registers.a);
}

void CPU::AND(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
    auto data = read(addr);
    set_register_a((data & registers.a));
}
void CPU::EOR(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
    auto data = read(addr);
    set_register_a((data ^ registers.a));
}
void CPU::ORA(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
    auto data = read(addr);
    set_register_a((data | registers.a));
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
    uint16_t addr = get_operand_address(mode);
    uint8_t data = read(addr);

    uint16_t value = static_cast<uint16_t>(data);
    uint16_t carry_in = get_flag(C) ? 0 : 1;
    uint16_t result = static_cast<uint16_t>(registers.a) - value - carry_in;

    registers.a = static_cast<uint8_t>(result & 0xFF);

    set_flag(N, registers.a & 0x80);
    set_flag(Z, registers.a == 0);
    set_flag(C, result < 0x100);
    set_flag(V, ((registers.a ^ result) & (registers.a ^ data) & 0x80) != 0);
}

void CPU::ADC(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
    auto value = read(addr);
    add_to_register_a(value);
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
    auto addr = get_operand_address(mode);
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
    auto addr = get_operand_address(mode);
    auto data = read(addr);

    set_flag(C, (data & 1) == 1);

    data >>= 1;
    write(addr, data);
    update_zero_and_negative_flags(data);
    return data;
}

uint8_t CPU::ROL(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
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
    auto addr = get_operand_address(mode);
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
    auto addr = get_operand_address(mode);
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
    auto addr = get_operand_address(mode);
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
    // Set BREAK
    set_flag(B, true);
    // Set BREAK2
    set_flag(U, true);
    stack_push(registers.p);

    // Copy back
    registers.p = flags;
}

void CPU::BIT(const AddressingMode &mode)
{
    auto addr = get_operand_address(mode);
    auto data = read(addr);
    auto and_result = registers.a & data;
    set_flag(Z, and_result == 0);
    set_flag(N, (data & 0b1000'0000) > 0);
    set_flag(V, (data & 0b0100'1000) > 0);
}

void CPU::COMPARE(const AddressingMode &mode, uint8_t compare_with)
{
    auto addr = get_operand_address(mode);
    auto data = read(addr);
    set_flag(C, data <= compare_with);
    update_zero_and_negative_flags(compare_with - data);
}

void CPU::BRANCH(bool condition)
{
    if (condition)
    {
        int8_t jump = static_cast<int8_t>(read(registers.pc));
        auto jump_addr = registers.pc + 1 + static_cast<uint16_t>(jump);
        registers.pc = jump_addr;
    }
}
} // namespace EM
