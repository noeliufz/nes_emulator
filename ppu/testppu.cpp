#include "ppu.h"
#include <cassert>
#include <iostream>
/* tests */
void test_ppu_vram_writes()
{
    std::vector<uint8_t> test{0};
    EM::NesPPU ppu{test, EM::Mirroring::HORIZONTAL};
    ppu.write_to_ppu_addr(0x23);
    ppu.write_to_ppu_addr(0x05);
    ppu.write_to_data(0x66);
    assert(0x66 == ppu.vram[0x0305]);
    std::cout << "Test ppu vram writes ok" << std::endl;
}
void test_ppu_vram_reads()
{
    std::vector<uint8_t> test{0};
    EM::NesPPU ppu{test, EM::Mirroring::HORIZONTAL};
    ppu.write_to_ctrl(0);
    ppu.vram[0x0305] = 0x66;
    ppu.write_to_ppu_addr(0x23);
    ppu.write_to_ppu_addr(0x05);
    ppu.read_data();
    assert(0x2306 == ppu.address_register.get());
    assert(0x66 == ppu.read_data());
    std::cout << "Test ppu vram reads ok" << std::endl;
}
int main()
{
    test_ppu_vram_writes();
    test_ppu_vram_reads();
    return 0;
} // namespace EM
