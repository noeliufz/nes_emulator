//
// Created by Fangzhou Liu on 4/8/2024.
//
#include "cpu.h"

#include <cstddef>
#include <cstdint>
#include <iostream>

#include "bus.h"
#include "op_code.h"
#include "trace.h"

#include <_types/_uint16_t.h>
#include <_types/_uint8_t.h>
#include <pthread.h>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace EM
{
bool DEBUG = false;

const double CPU_FREQUENCY = 1660000.0;        // NTSC版6502 CPU频率（Hz）
const double CYCLE_TIME = 1.0 / CPU_FREQUENCY; // 每个CPU周期的时间（秒）
uint64_t total_cycles = 0;                     // 累计执行的周期数
void CPU::run()
{
    run_with_callback([](CPU &) {});
}

void CPU::run_with_callback(std::function<void(CPU &)> callback)
{
    while (true)
    {
        auto nmi = bus->poll_nmi_status();

        if (nmi.has_value())
        {
            interrupt(NMI);
        }

        callback(*this);

        auto code = read(registers.pc);
        ++registers.pc;
        auto state = registers.pc;

        const EM::OpCode *op = nullptr;
        try
        {
            auto it = opcode_map.find(code);
            if (it != opcode_map.end())
            {
                op = it->second.get();
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

        if (DEBUG)
        {
            std::cout << op->mnemonic << " ";
            std::cout << std::hex << static_cast<int>(op->code) << " ";
            std::cout << "X: " << static_cast<int>(registers.x) << " ";
            std::cout << "Y: " << static_cast<int>(registers.y) << " ";
            std::cout << "A: " << static_cast<int>(registers.a) << " ";
            std::cout << "SP: " << static_cast<int>(registers.sp) << " ";
            std::cout << std::endl;
        }
        double expected_time = op->cycles * CYCLE_TIME;
        auto emulation_start_time = std::chrono::high_resolution_clock::now();

        try
        {

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

            case 0x00: {
                // return;
            }

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
                    ref = static_cast<uint16_t>(static_cast<uint16_t>(static_cast<uint16_t>(hi)) << 8 |
                                                (static_cast<uint16_t>(lo)));
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
                stack_push_u16(static_cast<uint16_t>(registers.pc + 2 - 1));
                auto target = read_u16(registers.pc);
                registers.pc = target;
                break;
            }

                /* RTS */
            case 0x60: {
                registers.pc = static_cast<uint16_t>(stack_pop_u16());
                ++registers.pc;
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
                auto [addr, _] = get_operand_address(op->mode);
                write(addr, registers.x);
                break;
            }
                /* STY */
            case 0x84:
            case 0x94:
            case 0x8c: {
                auto [addr, _] = get_operand_address(op->mode);
                //			std::cout << "STY" << std::hex << static_cast<int>(addr) << std::endl;
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

                /* Unofficial */
                /* DCP */
            case 0xc7:
            case 0xd7:
            case 0xcf:
            case 0xdf:
            case 0xdb:
            case 0xd3:
            case 0xc3: {
                const auto [addr, _] = get_operand_address(op->mode);
                auto data = read(addr);
                --data;
                write(addr, data);
                if (data <= registers.a)
                {
                    set_flag(C, true);
                }
                update_zero_and_negative_flags(registers.a - data);
                break;
            }

                /* RLA */
            case 0x27:
            case 0x37:
            case 0x2f:
            case 0x3f:
            case 0x3b:
            case 0x33:
            case 0x23: {
                auto data = ROL(op->mode);
                set_register_a(data & registers.a);
                break;
            }

                /* SLO */
            case 0x07:
            case 0x17:
            case 0x0f:
            case 0x1f:
            case 0x1b:
            case 0x03:
            case 0x13: {
                auto data = ASL(op->mode);
                set_register_a(data | registers.a);
                break;
            }

                /* SRE */
            case 0x47:
            case 0x57:
            case 0x4f:
            case 0x5f:
            case 0x5b:
            case 0x43:
            case 0x53: {
                auto data = LSR(op->mode);
                set_register_a(data ^ registers.a);
                break;
            }

                /* SKB */
            case 0x80:
            case 0x82:
            case 0x89:
            case 0xc2:
            case 0xe2: {
                break;
            }

                /* AXS */
            case 0xcb: {
                const auto [addr, _] = get_operand_address(op->mode);
                auto data = read(addr);
                auto x_and_a = static_cast<uint8_t>(registers.x & registers.a);
                auto result = static_cast<uint8_t>(x_and_a - data);

                if (data <= x_and_a)
                {
                    set_flag(C, true);
                }

                update_zero_and_negative_flags(result);
                registers.x = result;
                break;
            }

                /* ARR */
            case 0x6b: {
                auto [addr, _] = get_operand_address(op->mode);
                auto data = read(addr);
                set_register_a(data & registers.a);
                ror_accumulator();

                auto result = registers.a;
                auto bit_5 = (result >> 5) & 1;
                auto bit_6 = (result >> 6) & 1;

                if (bit_6 == 1)
                {
                    set_flag(C, true);
                }
                else
                {
                    set_flag(C, false);
                }

                if ((bit_5 ^ bit_6) == 1)
                {
                    set_flag(V, true);
                }
                else
                {
                    set_flag(V, false); // TODO: !
                }

                update_zero_and_negative_flags(result);

                break;
            }

                /* Unofficial SBC */
            case 0xeb: {
                auto [addr, _] = get_operand_address(op->mode);
                auto data = read(addr);
                // TODO: not sure
                sub_from_register_a(data);
                break;
            }

                /* ANC */
            case 0x0b:
            case 0x2b: {
                auto [addr, _] = get_operand_address(op->mode);
                auto data = read(addr);
                set_register_a(data & registers.a);
                if (get_flag(N))
                {
                    set_flag(C, true);
                }
                else
                {
                    set_flag(C, false);
                }
                break;
            }

                /* ALR */
            case 0x4b: {
                auto [addr, _] = get_operand_address(op->mode);
                auto data = read(addr);
                set_register_a(data & registers.a);
                lsr_accumulator();
                break;
            }

                /* NOP read */
            case 0x04:
            case 0x44:
            case 0x64:
            case 0x14:
            case 0x34:
            case 0x54:
            case 0x74:
            case 0xd4:
            case 0xf4:
            case 0x0c:
            case 0x1c:
            case 0x3c:
            case 0x5c:
            case 0x7c:
            case 0xdc:
            case 0xfc: {
                auto [addr, page_cross] = get_operand_address(op->mode);
                auto data = read(addr);
                if (page_cross)
                {
                    bus->tick(1);
                }
                break;
            }

                /* RRA */
            case 0x67:
            case 0x77:
            case 0x6f:
            case 0x7f:
            case 0x7b:
            case 0x63:
            case 0x73: {
                auto data = ROR(op->mode);
                add_to_register_a(data);
                break;
            }

                /* ISB */
            case 0xe7:
            case 0xf7:
            case 0xef:
            case 0xff:
            case 0xfb:
            case 0xe3:
            case 0xf3: {
                auto data = INC(op->mode);
                sub_from_register_a(data);
                break;
            }

                /* NOPs */
            case 0x02:
            case 0x12:
            case 0x22:
            case 0x32:
            case 0x42:
            case 0x52:
            case 0x62:
            case 0x72:
            case 0x92:
            case 0xb2:
            case 0xd2:
            case 0xf2: {
                break;
            }

            case 0x1a:
            case 0x3a:
            case 0x5a:
            case 0x7a:
            case 0xda:
            case 0xfa: {
                break;
            }

                /* LAX */
            case 0xa7:
            case 0xb7:
            case 0xaf:
            case 0xbf:
            case 0xa3:
            case 0xb3: {
                auto [addr, _] = get_operand_address(op->mode);
                auto data = read(addr);
                set_register_a(data);
                registers.x = registers.a;
                break;
            }

                /* SAX */
            case 0x87:
            case 0x97:
            case 0x8f:
            case 0x83: {
                auto data = static_cast<uint8_t>(registers.a & registers.x);
                auto [addr, _] = get_operand_address(op->mode);
                write(addr, data);
                break;
            }

                /* LXA */
            case 0xab: {
                LDA(op->mode);
                TAX();
                break;
            }

                /* XAA */
            case 0x8b: {
                registers.a = registers.x;
                update_zero_and_negative_flags(registers.a);
                auto [addr, _] = get_operand_address(op->mode);
                auto data = read(addr);
                set_register_a(data & registers.a);
                break;
            }

                /* LAS */
            case 0xbb: {
                auto [addr, _] = get_operand_address(op->mode);
                auto data = read(addr);
                data = data & registers.sp;
                registers.a = data;
                registers.x = data;
                registers.sp = data;
                update_zero_and_negative_flags(data);
                break;
            }

                /* TAS */
            case 0x9b: {
                uint8_t data = registers.a & registers.x;
                registers.sp = data;
                uint16_t mem_addr = read_u16(registers.pc) + static_cast<uint16_t>(registers.y);
                data = (static_cast<uint8_t>(mem_addr >> 8) + 1) & registers.sp;
                write(mem_addr, data);
                break;
            }

                /* AHX Indirect Y */
            case 0x93: {
                auto pos = read(registers.pc);
                uint16_t mem_addr = read_u16(static_cast<uint16_t>(pos)) + static_cast<uint16_t>(registers.y);
                uint8_t data = registers.a & registers.x & static_cast<uint8_t>(mem_addr >> 8);
                write(mem_addr, data);
                break;
            }

                /* AHX Absolute Y */
            case 0x9f: {
                auto mem_addr = static_cast<uint16_t>(read_u16(registers.pc) + static_cast<uint16_t>(registers.y));
                uint8_t data = registers.a & registers.x & static_cast<uint8_t>(mem_addr >> 8);
                write(mem_addr, data);
                break;
            }

                /* SHX */
            case 0x9e: {
                uint16_t mem_addr = read_u16(registers.pc) + static_cast<uint16_t>(registers.y);
                uint8_t data = registers.x & (static_cast<uint8_t>(mem_addr >> 8) + 1);
                write(mem_addr, data);
                break;
            }

                /* SHY */
            case 0x9c: {
                uint16_t mem_addr = read_u16(registers.pc) + static_cast<uint16_t>(registers.x);
                uint8_t data = registers.y & (static_cast<uint8_t>(mem_addr >> 8) + 1);
                write(mem_addr, data);
                break;
            }

            default:
                std::cerr << "Not implemented code: " << std::hex << static_cast<int>(code) << std::endl;
                break;
                // todo
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Runtime error: " << e.what();
            std::cerr << " Opcode: " << op->mnemonic << " 0x" << std::hex << static_cast<int>(op->code);
            std::cerr << " PC: " << registers.pc;
            std::cerr << std::endl;
        }
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = current_time - emulation_start_time;
        double actual_time = elapsed.count();
        if (expected_time > actual_time)
        {
            double sleep_time = expected_time - actual_time;
            if (sleep_time > 0)
            {
                // std::this_thread::sleep_for(std::chrono::duration<double>(sleep_time));
            }
        }

        bus->tick(op->cycles);

        if (state == registers.pc)
        {
            registers.pc += static_cast<uint16_t>((op->len - 1));
        }
    }
}
} // namespace EM
