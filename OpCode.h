#ifndef MYNESEMULATOR__OPCODE_H_
#define MYNESEMULATOR__OPCODE_H_

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace EM
{

// Addressing modes
enum AddressingMode
{
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

struct OpCode
{
    uint8_t code;
    std::string mnemonic;
    uint8_t len;
    uint8_t cycles;
    AddressingMode mode;
    // constructor
    OpCode(int i_code, const char *str, int i_len, int i_cycles, AddressingMode addressing_mode)
        : code(i_code), mnemonic(str), len(i_len), cycles(i_cycles), mode(addressing_mode)
    {
    }
};

class OpCodeSingleton
{
  public:
    static const OpCodeSingleton &get_instance()
    {
        static const OpCodeSingleton instance;
        return instance;
    }

    static std::unique_ptr<std::unordered_map<uint8_t,  std::unique_ptr<OpCode>>> get_opcode_map()
    {
        return std::move(opcode_map);
    }

    static std::unique_ptr<std::vector< std::unique_ptr<OpCode>>> get_opcodes()
    {
        return std::move(opcodes);
    }

    OpCodeSingleton(const OpCodeSingleton &) = delete;
    OpCodeSingleton &operator=(const OpCodeSingleton &) = delete;

  private:
    static std::unique_ptr<std::unordered_map<uint8_t,  std::unique_ptr<OpCode>>> opcode_map;
    static std::unique_ptr<std::vector< std::unique_ptr<OpCode>>> opcodes;
    static void init_opcodes();
    static void init_opcode_map();

    OpCodeSingleton()
    {
        init_opcodes();
        init_opcode_map();
    }
};
} // namespace EM
#endif
