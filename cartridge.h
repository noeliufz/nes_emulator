#ifndef MYNESEMULATOR__CARTRIDGE_H_
#define MYNESEMULATOR__CARTRIDGE_H_

#include <cstdint>
#include <vector>
namespace EM
{

enum class Mirroring
{
    VERTICAL,
    HORIZONTAL,
    FOUR_SCREEN
};

class Rom
{
  public:
    std::vector<uint8_t> prg_rom;
    std::vector<uint8_t> chr_rom;
    uint8_t mapper;
    Mirroring screen_mirroring;

    Rom();

    Rom(const std::vector<uint8_t> &raw);

  private:
    static constexpr size_t PRG_ROM_PAGE_SIZE = 16384;
    static constexpr size_t CHR_ROM_PAGE_SIZE = 8192;
    static constexpr const char *NES_TAG = "NES\x1A";
};
} // namespace EM
#endif
