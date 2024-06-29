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
#include <sstream>
#include <stdexcept>
#include <vector>

namespace EM
{
///////////////////////////////////////////////////////////////////////////////
// Constructor
CPU::CPU()
{
    init_opcodes();
    init_opcode_map();
    registers.a = 0;
    registers.x = 0;
    registers.y = 0;
    registers.sp = STACK_RESET;
    registers.p = 0b100100; // status
    registers.pc = 0;       // program counter
};

CPU::~CPU()
{
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
                ref = ((uint16_t)hi) << 8 | ((uint16_t)lo);
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
            registers.pc += (uint16_t)(op->len - 1);
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

void CPU::init_opcodes()
{
    if (opcodes)
    {
        std::cout << "Opcodes vector already inited." << std::endl;
        return;
    }

    opcodes = new std::vector<const OpCode *>;

    opcodes->emplace_back(new OpCode(0x00, "BRK", 1, 7, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xea, "NOP", 1, 2, AddressingMode::NoneAddressing));

    /* Arithmetic */
    opcodes->emplace_back(new OpCode(0x69, "ADC", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0x65, "ADC", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x75, "ADC", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x6d, "ADC", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x7d, "ADC", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x79, "ADC", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0x61, "ADC", 2, 6, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0x71, "ADC", 2, 5 /*+1 if page crossed*/, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0xe9, "SBC", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0xe5, "SBC", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xf5, "SBC", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xed, "SBC", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xfd, "SBC", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0xf9, "SBC", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0xe1, "SBC", 2, 6, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0xf1, "SBC", 2, 5 /*+1 if page crossed*/, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0x29, "AND", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0x25, "AND", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x35, "AND", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x2d, "AND", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x3d, "AND", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x39, "AND", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0x21, "AND", 2, 6, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0x31, "AND", 2, 5 /*+1 if page crossed*/, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0x49, "EOR", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0x45, "EOR", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x55, "EOR", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x4d, "EOR", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x5d, "EOR", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x59, "EOR", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0x41, "EOR", 2, 6, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0x51, "EOR", 2, 5 /*+1 if page crossed*/, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0x09, "ORA", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0x05, "ORA", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x15, "ORA", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x0d, "ORA", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x1d, "ORA", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x19, "ORA", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0x01, "ORA", 2, 6, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0x11, "ORA", 2, 5 /*+1 if page crossed*/, AddressingMode::Indirect_Y));

    /* Shifts */
    opcodes->emplace_back(new OpCode(0x0a, "ASL", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x06, "ASL", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x16, "ASL", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x0e, "ASL", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x1e, "ASL", 3, 7, AddressingMode::Absolute_X));

    opcodes->emplace_back(new OpCode(0x4a, "LSR", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x46, "LSR", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x56, "LSR", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x4e, "LSR", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x5e, "LSR", 3, 7, AddressingMode::Absolute_X));

    opcodes->emplace_back(new OpCode(0x2a, "ROL", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x26, "ROL", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x36, "ROL", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x2e, "ROL", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x3e, "ROL", 3, 7, AddressingMode::Absolute_X));

    opcodes->emplace_back(new OpCode(0x6a, "ROR", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x66, "ROR", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x76, "ROR", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x6e, "ROR", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x7e, "ROR", 3, 7, AddressingMode::Absolute_X));

    opcodes->emplace_back(new OpCode(0xe6, "INC", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xf6, "INC", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xee, "INC", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xfe, "INC", 3, 7, AddressingMode::Absolute_X));

    opcodes->emplace_back(new OpCode(0xe8, "INX", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xc8, "INY", 1, 2, AddressingMode::NoneAddressing));

    opcodes->emplace_back(new OpCode(0xc6, "DEC", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xd6, "DEC", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xce, "DEC", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xde, "DEC", 3, 7, AddressingMode::Absolute_X));

    opcodes->emplace_back(new OpCode(0xca, "DEX", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x88, "DEY", 1, 2, AddressingMode::NoneAddressing));

    opcodes->emplace_back(new OpCode(0xc9, "CMP", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0xc5, "CMP", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xd5, "CMP", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xcd, "CMP", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xdd, "CMP", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0xd9, "CMP", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0xc1, "CMP", 2, 6, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0xd1, "CMP", 2, 5 /*+1 if page crossed*/, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0xc0, "CPY", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0xc4, "CPY", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xcc, "CPY", 3, 4, AddressingMode::Absolute));

    opcodes->emplace_back(new OpCode(0xe0, "CPX", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0xe4, "CPX", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xec, "CPX", 3, 4, AddressingMode::Absolute));

    /* Branching */

    opcodes->emplace_back(new OpCode(0x4c, "JMP", 3, 3, AddressingMode::NoneAddressing));
    // AddressingMode that acts as Immidiate
    opcodes->emplace_back(new OpCode(0x6c, "JMP", 3, 5, AddressingMode::NoneAddressing));
    // AddressingMode:Indirect with 6502 bug

    opcodes->emplace_back(new OpCode(0x20, "JSR", 3, 6, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x60, "RTS", 1, 6, AddressingMode::NoneAddressing));

    opcodes->emplace_back(new OpCode(0x40, "RTI", 1, 6, AddressingMode::NoneAddressing));

    opcodes->emplace_back(
        new OpCode(0xd0, "BNE", 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, AddressingMode::NoneAddressing));
    opcodes->emplace_back(
        new OpCode(0x70, "BVS", 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, AddressingMode::NoneAddressing));
    opcodes->emplace_back(
        new OpCode(0x50, "BVC", 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, AddressingMode::NoneAddressing));
    opcodes->emplace_back(
        new OpCode(0x30, "BMI", 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, AddressingMode::NoneAddressing));
    opcodes->emplace_back(
        new OpCode(0xf0, "BEQ", 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, AddressingMode::NoneAddressing));
    opcodes->emplace_back(
        new OpCode(0xb0, "BCS", 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, AddressingMode::NoneAddressing));
    opcodes->emplace_back(
        new OpCode(0x90, "BCC", 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, AddressingMode::NoneAddressing));
    opcodes->emplace_back(
        new OpCode(0x10, "BPL", 2, 2 /*(+1 if branch succeeds +2 if to a new page)*/, AddressingMode::NoneAddressing));

    opcodes->emplace_back(new OpCode(0x24, "BIT", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x2c, "BIT", 3, 4, AddressingMode::Absolute));

    /* Stores, Loads */
    opcodes->emplace_back(new OpCode(0xa9, "LDA", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0xa5, "LDA", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xb5, "LDA", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xad, "LDA", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xbd, "LDA", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0xb9, "LDA", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0xa1, "LDA", 2, 6, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0xb1, "LDA", 2, 5 /*+1 if page crossed*/, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0xa2, "LDX", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0xa6, "LDX", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xb6, "LDX", 2, 4, AddressingMode::ZeroPage_Y));
    opcodes->emplace_back(new OpCode(0xae, "LDX", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xbe, "LDX", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_Y));

    opcodes->emplace_back(new OpCode(0xa0, "LDY", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0xa4, "LDY", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xb4, "LDY", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xac, "LDY", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xbc, "LDY", 3, 4 /*+1 if page crossed*/, AddressingMode::Absolute_X));

    opcodes->emplace_back(new OpCode(0x85, "STA", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x95, "STA", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x8d, "STA", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x9d, "STA", 3, 5, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x99, "STA", 3, 5, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0x81, "STA", 2, 6, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0x91, "STA", 2, 6, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0x86, "STX", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x96, "STX", 2, 4, AddressingMode::ZeroPage_Y));
    opcodes->emplace_back(new OpCode(0x8e, "STX", 3, 4, AddressingMode::Absolute));

    opcodes->emplace_back(new OpCode(0x84, "STY", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x94, "STY", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x8c, "STY", 3, 4, AddressingMode::Absolute));

    /* Flags clear */

    opcodes->emplace_back(new OpCode(0xD8, "CLD", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x58, "CLI", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xb8, "CLV", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x18, "CLC", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x38, "SEC", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x78, "SEI", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xf8, "SED", 1, 2, AddressingMode::NoneAddressing));

    opcodes->emplace_back(new OpCode(0xaa, "TAX", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xa8, "TAY", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xba, "TSX", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x8a, "TXA", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x9a, "TXS", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x98, "TYA", 1, 2, AddressingMode::NoneAddressing));

    /* Stack */
    opcodes->emplace_back(new OpCode(0x48, "PHA", 1, 3, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x68, "PLA", 1, 4, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x08, "PHP", 1, 3, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x28, "PLP", 1, 4, AddressingMode::NoneAddressing));

    /* unofficial */

    opcodes->emplace_back(new OpCode(0xc7, "*DCP", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xd7, "*DCP", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xCF, "*DCP", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xdF, "*DCP", 3, 7, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0xdb, "*DCP", 3, 7, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0xd3, "*DCP", 2, 8, AddressingMode::Indirect_Y));
    opcodes->emplace_back(new OpCode(0xc3, "*DCP", 2, 8, AddressingMode::Indirect_X));

    opcodes->emplace_back(new OpCode(0x27, "*RLA", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x37, "*RLA", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x2F, "*RLA", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x3F, "*RLA", 3, 7, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x3b, "*RLA", 3, 7, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0x33, "*RLA", 2, 8, AddressingMode::Indirect_Y));
    opcodes->emplace_back(new OpCode(0x23, "*RLA", 2, 8, AddressingMode::Indirect_X));

    opcodes->emplace_back(new OpCode(0x07, "*SLO", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x17, "*SLO", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x0F, "*SLO", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x1f, "*SLO", 3, 7, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x1b, "*SLO", 3, 7, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0x03, "*SLO", 2, 8, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0x13, "*SLO", 2, 8, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0x47, "*SRE", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x57, "*SRE", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x4F, "*SRE", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x5f, "*SRE", 3, 7, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x5b, "*SRE", 3, 7, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0x43, "*SRE", 2, 8, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0x53, "*SRE", 2, 8, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0x80, "*NOP", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0x82, "*NOP", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0x89, "*NOP", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0xc2, "*NOP", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0xe2, "*NOP", 2, 2, AddressingMode::Immediate));

    opcodes->emplace_back(new OpCode(0xCB, "*AXS", 2, 2, AddressingMode::Immediate));

    opcodes->emplace_back(new OpCode(0x6B, "*ARR", 2, 2, AddressingMode::Immediate));

    opcodes->emplace_back(new OpCode(0xeb, "*SBC", 2, 2, AddressingMode::Immediate));

    opcodes->emplace_back(new OpCode(0x0b, "*ANC", 2, 2, AddressingMode::Immediate));
    opcodes->emplace_back(new OpCode(0x2b, "*ANC", 2, 2, AddressingMode::Immediate));

    opcodes->emplace_back(new OpCode(0x4b, "*ALR", 2, 2, AddressingMode::Immediate));
    // opcodes->emplace_back(new OpCode(0xCB, "IGN", 3,4 /* or 5*/,
    // AddressingMode::Absolute_X));

    opcodes->emplace_back(new OpCode(0x04, "*NOP", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x44, "*NOP", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x64, "*NOP", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x14, "*NOP", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x34, "*NOP", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x54, "*NOP", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x74, "*NOP", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xd4, "*NOP", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xf4, "*NOP", 2, 4, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x0c, "*NOP", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x1c, "*NOP", 3, 4 /*or 5*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x3c, "*NOP", 3, 4 /*or 5*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x5c, "*NOP", 3, 4 /*or 5*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x7c, "*NOP", 3, 4 /*or 5*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0xdc, "*NOP", 3, 4 /* or 5*/, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0xfc, "*NOP", 3, 4 /* or 5*/, AddressingMode::Absolute_X));

    opcodes->emplace_back(new OpCode(0x67, "*RRA", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x77, "*RRA", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0x6f, "*RRA", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x7f, "*RRA", 3, 7, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0x7b, "*RRA", 3, 7, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0x63, "*RRA", 2, 8, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0x73, "*RRA", 2, 8, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0xe7, "*ISB", 2, 5, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xf7, "*ISB", 2, 6, AddressingMode::ZeroPage_X));
    opcodes->emplace_back(new OpCode(0xef, "*ISB", 3, 6, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xff, "*ISB", 3, 7, AddressingMode::Absolute_X));
    opcodes->emplace_back(new OpCode(0xfb, "*ISB", 3, 7, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0xe3, "*ISB", 2, 8, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0xf3, "*ISB", 2, 8, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0x02, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x12, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x22, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x32, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x42, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x52, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x62, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x72, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x92, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xb2, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xd2, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xf2, "*NOP", 1, 2, AddressingMode::NoneAddressing));

    opcodes->emplace_back(new OpCode(0x1a, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x3a, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x5a, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0x7a, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xda, "*NOP", 1, 2, AddressingMode::NoneAddressing));
    // opcodes->emplace_back(new OpCode(0xea, "NOP", 1,2,
    // AddressingMode::NoneAddressing));
    opcodes->emplace_back(new OpCode(0xfa, "*NOP", 1, 2, AddressingMode::NoneAddressing));

    opcodes->emplace_back(new OpCode(0xab, "*LXA", 2, 3,
                                     AddressingMode::Immediate)); // todo: highly unstable and not used
    // http://visual6502.org/wiki/index.php?title=6502_Opcode_8B_%28XAA,_ANE%29
    opcodes->emplace_back(new OpCode(0x8b, "*XAA", 2, 3,
                                     AddressingMode::Immediate)); // todo: highly unstable and not used
    opcodes->emplace_back(new OpCode(0xbb, "*LAS", 3, 2, AddressingMode::Absolute_Y));
    // todo: highly unstable and not used
    opcodes->emplace_back(new OpCode(0x9b, "*TAS", 3, 2, AddressingMode::Absolute_Y));
    // todo: highly unstable and not used
    opcodes->emplace_back(new OpCode(0x93, "*AHX", 2, /* guess */ 8, AddressingMode::Indirect_Y));
    // todo: highly unstable and not used
    opcodes->emplace_back(new OpCode(0x9f, "*AHX", 3,
                                     /* guess */ 4 /* or 5*/, AddressingMode::Absolute_Y));
    // todo: highly unstable and not used
    opcodes->emplace_back(new OpCode(0x9e, "*SHX", 3,
                                     /* guess */ 4 /* or 5*/, AddressingMode::Absolute_Y));
    // todo: highly unstable and not used
    opcodes->emplace_back(new OpCode(0x9c, "*SHY", 3,
                                     /* guess */ 4 /* or 5*/, AddressingMode::Absolute_X));
    // todo: highly unstable and not used

    opcodes->emplace_back(new OpCode(0xa7, "*LAX", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0xb7, "*LAX", 2, 4, AddressingMode::ZeroPage_Y));
    opcodes->emplace_back(new OpCode(0xaf, "*LAX", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0xbf, "*LAX", 3, 4, AddressingMode::Absolute_Y));
    opcodes->emplace_back(new OpCode(0xa3, "*LAX", 2, 6, AddressingMode::Indirect_X));
    opcodes->emplace_back(new OpCode(0xb3, "*LAX", 2, 5, AddressingMode::Indirect_Y));

    opcodes->emplace_back(new OpCode(0x87, "*SAX", 2, 3, AddressingMode::ZeroPage));
    opcodes->emplace_back(new OpCode(0x97, "*SAX", 2, 4, AddressingMode::ZeroPage_Y));
    opcodes->emplace_back(new OpCode(0x8f, "*SAX", 3, 4, AddressingMode::Absolute));
    opcodes->emplace_back(new OpCode(0x83, "*SAX", 2, 6, AddressingMode::Indirect_X));
}

void CPU::init_opcode_map()
{
    if (opcode_map)
    {
        std::cout << "Opcode Map already inited." << std::endl;
        return;
    }

    opcode_map = new std::unordered_map<uint8_t, const OpCode *>;

    for (const auto &code : *opcodes)
    {
        (*opcode_map)[code->code] = code;
    }
}
const OpCode *CPU::get_opcode(uint8_t code)
{
    return opcode_map->find(code) != opcode_map->end() ? (*opcode_map)[code] : nullptr;
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
        return (uint16_t)read(registers.pc);

    case Absolute:
        return read_u16(registers.pc);

    case ZeroPage_X: {
        auto pos = read(registers.pc);
        // wrappign add
        uint16_t addr = (uint16_t)(pos + registers.x);
        return addr;
    }
    case ZeroPage_Y: {
        auto pos = read(registers.pc);
        // wrappign add
        uint16_t addr = (uint16_t)(pos + registers.y);
        return addr;
    }

    case Absolute_X: {
        auto base = read_u16(registers.pc);
        uint16_t addr = base + (uint16_t)registers.x;
        return addr;
    }
    case Absolute_Y: {
        auto base = read_u16(registers.pc);
        uint16_t addr = base + (uint16_t)registers.y;
        return addr;
    }

    case Indirect_X: {
        auto base = read(registers.pc);
        uint8_t ptr = (uint8_t)base + registers.x;
        auto lo = read((uint16_t)ptr);
        auto hi = read((uint16_t)(ptr + 1));
        return (uint16_t)((uint16_t)hi) << 8 | ((uint16_t)lo);
    }
    case Indirect_Y: {
        auto base = read(registers.pc);
        auto lo = read((uint16_t)base);
        auto hi = read((uint16_t)(base + 1));
        auto deref_base = ((uint16_t)hi) << 8 | ((uint16_t)lo);
        auto deref = deref_base + ((uint16_t)registers.y);
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
    return read(STACK + (uint16_t)registers.sp);
}
void CPU::stack_push(uint8_t data)
{
    write(STACK + (uint16_t)registers.sp, data);
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
