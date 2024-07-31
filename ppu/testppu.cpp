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
void test_ppu_vram_cross_page()
{
    std::vector<uint8_t> test{0};
    EM::NesPPU ppu{test, EM::Mirroring::HORIZONTAL};
    ppu.write_to_ctrl(0);
    ppu.vram[0x01ff] = 0x66;
    ppu.vram[0x0200] = 0x77;

    ppu.write_to_ppu_addr(0x21);
    ppu.write_to_ppu_addr(0xff);

    ppu.read_data();
    assert(0x66 == ppu.read_data());
    assert(0x77 == ppu.read_data());
    std::cout << "Test ppu vram read cross page ok" << std::endl;
}
void test_ppu_vram_reads_step_32()
{
    std::vector<uint8_t> test{0};
    EM::NesPPU ppu{test, EM::Mirroring::HORIZONTAL};

    ppu.write_to_ctrl(0b100);
    ppu.vram[0x01ff] = 0x66;
    ppu.vram[0x01ff + 32] = 0x77;
    ppu.vram[0x01ff + 64] = 0x88;

    ppu.write_to_ppu_addr(0x21);
    ppu.write_to_ppu_addr(0xff);

    ppu.read_data();

    assert(0x66 == ppu.read_data());
    assert(0x77 == ppu.read_data());
    assert(0x88 == ppu.read_data());

    std::cout << "Test ppu vram read step 32 ok" << std::endl;
}
void test_vram_horizontal_mirror()
{
    std::vector<uint8_t> test{0};
    EM::NesPPU ppu{test, EM::Mirroring::HORIZONTAL};

    ppu.write_to_ppu_addr(0x24);
    ppu.write_to_ppu_addr(0x05);

    ppu.write_to_data(0x66); // write to A

    ppu.write_to_ppu_addr(0x28);
    ppu.write_to_ppu_addr(0x05);

    ppu.write_to_data(0x77); // write to B

    ppu.write_to_ppu_addr(0x20);
    ppu.write_to_ppu_addr(0x05);

    ppu.read_data();                 // load to buffer
    assert(0x66 == ppu.read_data()); // read from a

    ppu.write_to_ppu_addr(0x2C);
    ppu.write_to_ppu_addr(0x05);

    ppu.read_data();

    assert(0x77 == ppu.read_data());

    std::cout << "Test ppu vram read horizontal mirror ok" << std::endl;
}
void test_vram_vertical_mirror()
{
    std::vector<uint8_t> test{0};
    EM::NesPPU ppu{test, EM::Mirroring::VERTICAL};

    ppu.write_to_ppu_addr(0x20);
    ppu.write_to_ppu_addr(0x05);

    ppu.write_to_data(0x66);

    ppu.write_to_ppu_addr(0x2C);
    ppu.write_to_ppu_addr(0x05);

    ppu.write_to_data(0x77);

    ppu.write_to_ppu_addr(0x28);
    ppu.write_to_ppu_addr(0x05);

    ppu.read_data();
    assert(0x66 == ppu.read_data());

    ppu.write_to_ppu_addr(0x24);
    ppu.write_to_ppu_addr(0x05);

    ppu.read_data();
    assert(0x77 == ppu.read_data());

    std::cout << "Test ppu vram read vertical mirror ok" << std::endl;
}
int main()
{
    test_ppu_vram_writes();
    test_ppu_vram_reads();
    test_ppu_vram_cross_page();
    test_ppu_vram_reads_step_32();
    test_vram_horizontal_mirror();
    test_vram_vertical_mirror();
    return 0;
} // namespace EM
