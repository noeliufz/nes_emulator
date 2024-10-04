#include "trace.h"
#include "../cpu/cpu.h"
#include "../cpu/op_code.h"
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace EM
{
std::string trace(EM::CPU &cpu)
{
    const auto opcode_map = EM::OpCodeSingleton::get_instance().get_opcode_map();
    const auto code = cpu.read(cpu.registers.pc);

    const auto it = opcode_map.find(code);
    const EM::OpCode *op = nullptr;

    if (it != opcode_map.end())
    {
        op = it->second.get();
    }
    else
    {
        std::ostringstream oss;
        oss << "Opcode " << std::hex << static_cast<int>(code) << " is not recognized. ";
        oss << "Program counter: " << std::hex << cpu.registers.pc;
        throw std::runtime_error(oss.str());
    }

    const auto begin = cpu.registers.pc;
    auto hex_dump = std::vector<uint8_t>();
    hex_dump.emplace_back(code);

    uint16_t mem_addr = 0;
    uint8_t stored_value = 0;

    switch (op->mode)
    {
    case EM::AddressingMode::Immediate:
    case EM::AddressingMode::NoneAddressing:
        mem_addr = 0;
        stored_value = 0;
        break;
    default:
        auto [addr, _] = cpu.get_absolute_address(op->mode, begin + 1);
        mem_addr = addr;
        stored_value = cpu.read(addr);
    }

    std::string tmp;

    if (op->len == 1)
    {
        switch (op->code)
        {
        case 0x0a:
        case 0x4a:
        case 0x2a:
        case 0x6a:
            tmp = "A ";
            break;
        default:
            tmp = "";
            break;
        }
    }
    else if (op->len == 2)
    {
        auto addr = cpu.read(begin + 1);
        hex_dump.emplace_back(addr);

        switch (op->mode)
        {
        case EM::AddressingMode::Immediate:
            tmp = "#" + to_hex(addr);
            break;
        case EM::AddressingMode::ZeroPage:
            tmp = "$" + to_hex(addr) + " = " + to_hex(stored_value);
            break;
        case EM::AddressingMode::ZeroPage_X:
            tmp = "$" + to_hex(addr) + ",X @ " + to_hex(mem_addr) + " = " + to_hex(stored_value);
            break;
        case EM::AddressingMode::ZeroPage_Y:
            tmp = "$" + to_hex(addr) + ",Y @ " + to_hex(mem_addr) + " = " + to_hex(stored_value);
            break;
        case EM::AddressingMode::Indirect_X:
            tmp = "($" + to_hex(addr) + ",X) @ " + to_hex(static_cast<uint8_t>((addr + cpu.registers.x) & 0xFF)) +
                  " = " + to_hex(mem_addr) + " = " + to_hex(stored_value);
            break;
        case EM::AddressingMode::Indirect_Y:
            tmp = "($" + to_hex(addr) +
                  "),Y = " + to_hex(static_cast<uint16_t>((mem_addr - cpu.registers.y) & 0xFFFF)) + " @ " +
                  to_hex(mem_addr) + " = " + to_hex(stored_value);
            break;
        case EM::AddressingMode::NoneAddressing: {
            auto offset = static_cast<int8_t>(addr);
            auto jmp_address = static_cast<uint16_t>(begin + 2 + offset);
            tmp = "$" + to_hex(jmp_address);
            break;
        }
        default:
            throw std::runtime_error("Unexpected addressing mode with length 2.");
        }
    }
    else if (op->len == 3)
    {
        uint8_t address_lo = cpu.read(begin + 1);
        uint8_t address_hi = cpu.read(begin + 2);
        hex_dump.emplace_back(address_lo);
        hex_dump.emplace_back(address_hi);

        auto address = static_cast<uint16_t>((address_hi << 8) | address_lo);

        switch (op->mode)
        {
        case EM::AddressingMode::NoneAddressing:
            if (op->code == 0x6c)
            { // JMP indirect
                uint16_t jmp_addr;
                if ((address & 0x00FF) == 0x00FF)
                {
                    uint8_t lo = cpu.read(address);
                    uint8_t hi = cpu.read(address & 0xFF00);
                    jmp_addr = static_cast<uint16_t>((hi << 8) | lo);
                }
                else
                {
                    jmp_addr = cpu.read_u16(address);
                }
                tmp = "($" + to_hex(address) + ") = " + to_hex(jmp_addr);
            }
            else
            {
                tmp = "$" + to_hex(address);
            }
            break;
        case EM::AddressingMode::Absolute:
            tmp = "$" + to_hex(mem_addr) + " = " + to_hex(stored_value);
            break;
        case EM::AddressingMode::Absolute_X:
            tmp = "$" + to_hex(address) + ",X @ " + to_hex(mem_addr) + " = " + to_hex(stored_value);
            break;
        case EM::AddressingMode::Absolute_Y:
            tmp = "$" + to_hex(address) + ",Y @ " + to_hex(mem_addr) + " = " + to_hex(stored_value);
            break;
        default:
            throw std::runtime_error("Unexpected addressing mode with length 3.");
        }
    }

    std::stringstream hex_stream;
    for (size_t i = 0; i < hex_dump.size(); ++i)
    {
        if (i > 0)
            hex_stream << " ";
        hex_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hex_dump[i]);
    }

    std::string hex_str = hex_stream.str();

    std::stringstream asm_stream;
    asm_stream << std::hex << std::setw(4) << std::setfill('0') << begin << "  " << std::left << std::setw(8) << hex_str
               << std::right << std::setw(4) << op->mnemonic << " " << tmp;

    std::string asm_str = asm_stream.str();

    std::stringstream final_stream;
    final_stream << std::uppercase << asm_str << "  A:" << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>(cpu.registers.a) << " X:" << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>(cpu.registers.x) << " Y:" << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>(cpu.registers.y) << " P:" << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>(cpu.registers.p) << " SP:" << std::hex << std::setw(2) << std::setfill('0')
                 << static_cast<int>(cpu.registers.sp) << " PPU:" << static_cast<int>(cpu.bus->ppu->cycles) << ","
                 << static_cast<int>(cpu.bus->ppu->scanline) << " CYC:" << static_cast<int>(cpu.bus->cycles);

    return final_stream.str();
}

std::string to_hex(uint16_t value)
{
    std::stringstream stream;
    stream << std::hex << std::setw(4) << std::setfill('0') << value;
    return stream.str();
}

std::string to_hex(uint8_t value)
{
    std::stringstream stream;
    stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(value);
    return stream.str();
}
} // namespace EM
