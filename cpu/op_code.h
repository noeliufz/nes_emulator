#ifndef MYNESEMULATOR__OP_CODE_H_
#define MYNESEMULATOR__OP_CODE_H_

#include <cstdint>
#include <memory>
#include <mutex>
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
    OpCode(uint8_t i_code, const char *str, uint8_t i_len, uint8_t i_cycles, AddressingMode addressing_mode)
        : code(i_code), mnemonic(str), len(i_len), cycles(i_cycles), mode(addressing_mode)
    {
    }
};

class OpCodeSingleton
{
  public:
    static const OpCodeSingleton &get_instance()
    {
        std::call_once(initInstanceFlag, &OpCodeSingleton::initSingleton);
        return *instance;
    }

    static const std::unordered_map<uint8_t, std::shared_ptr<OpCode>> &get_opcode_map()
    {
        return *opcode_map;
    }

    static const std::vector<std::shared_ptr<OpCode>> &get_opcodes()
    {
        return *opcodes;
    }

    OpCodeSingleton(const OpCodeSingleton &) = delete;
    OpCodeSingleton &operator=(const OpCodeSingleton &) = delete;

  private:
    static void initSingleton()
    {
        instance.reset(new OpCodeSingleton());
    }

    static std::unique_ptr<OpCodeSingleton> instance;
    static std::once_flag initInstanceFlag;

    static std::shared_ptr<std::unordered_map<uint8_t, std::shared_ptr<OpCode>>> opcode_map;
    static std::shared_ptr<std::vector<std::shared_ptr<OpCode>>> opcodes;
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
