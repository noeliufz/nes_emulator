#include "bus.h"
#include "cpu.h"
#include "joypad.h"
#include "ppu/ppu.h"
#include "trace.h"
#include <SDL.h>
#include <SDL_pixels.h>
#include <arm_neon.h>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <ostream>
#include <random>
#include <thread>
#include <vector>

std::vector<uint8_t> readFile(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file");
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
    {
        throw std::runtime_error("Error reading file");
    }

    return buffer;
}
int main()
{
    // init sdl window
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // create window
    SDL_Window *window =
        SDL_CreateWindow("Test rom", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 320, SDL_WINDOW_SHOWN);
    if (window == nullptr)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_RaiseWindow(window);

    // create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr)
    {
        SDL_DestroyWindow(window);
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // set scale
    if (SDL_RenderSetScale(renderer, 10.0f, 10.0f) != 0)
    {
        std::cerr << "SDL_RenderSetScale Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // create texture
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, 32, 32);
    if (texture == nullptr)
    {
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // read Nes file
    std::vector<uint8_t> bytes = readFile("../test.nes");
    EM::Rom rom(bytes);

    auto bus = EM::Bus(&rom, [](EM::NesPPU &ppu, EM::Joypad &joypad) {});
    auto cpu = EM::CPU(&bus);
    cpu.reset();
    cpu.registers.pc = 0xC000;

    std::vector<uint8_t> screen_state(32 * 3 * 32, 0);
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(1, 15);

    SDL_Event event;

    cpu.run_with_callback([&](EM::CPU &cpu) {
        std::cout << trace(cpu) << std::endl;
    });

    // clear resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
