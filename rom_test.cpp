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
SDL_Colour get_color(uint8_t byte)
{
    switch (byte)
    {
    case 0:
        return SDL_Color{0, 0, 0, 255}; // black
    case 1:
        return SDL_Color{255, 255, 255, 255}; // white
    case 2:
    case 9:
        return SDL_Color{128, 128, 128, 255}; // grey
    case 3:
    case 10:
        return SDL_Color{255, 0, 0, 255}; // red
    case 4:
    case 11:
        return SDL_Color{0, 255, 0, 255}; // green
    case 5:
    case 12:
        return SDL_Color{0, 0, 255, 255}; // blue
    case 6:
    case 13:
        return SDL_Color{255, 0, 255, 255}; // magenta
    case 7:
    case 14:
        return SDL_Color{255, 255, 0, 255}; // yellow
    default:
        return SDL_Color{0, 255, 255, 255}; // cyan
    }
}

bool read_screen_state(const EM::CPU &cpu, std::vector<uint8_t> &frame)
{
    size_t frame_idx = 0;
    bool update = false;
    // check if there is pixels to update
    for (uint16_t i = 0x0200; i < 0x0600; ++i)
    {
        uint8_t color_idx = cpu.read(i);
        SDL_Color color = get_color(color_idx);
        uint8_t b1 = color.r;
        uint8_t b2 = color.g;
        uint8_t b3 = color.b;

        if (frame[frame_idx] != b1 || frame[frame_idx + 1] != b2 || frame[frame_idx + 2] != b3)
        {
            frame[frame_idx] = b1;
            frame[frame_idx + 1] = b2;
            frame[frame_idx + 2] = b3;
            update = true;
        }
        frame_idx += 3;
    }
    return update;
}
void handle_user_input(EM::CPU &cpu, SDL_Event &event)
{
    switch (event.type)
    {
    case SDL_QUIT:
        std::exit(0);
        break;
    case SDL_KEYDOWN:
        switch (event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
            std::exit(0);
            break;
        case SDLK_w:
            cpu.write(0xff, 0x77);
            break;
        case SDLK_s:
            cpu.write(0xff, 0x73);
            break;
        case SDLK_a:
            cpu.write(0xff, 0x61);
            break;
        case SDLK_d:
            cpu.write(0xff, 0x64);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
}
std::vector<uint8_t> readFile(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open file");
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
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
        // SDL_Event event;
        // while (SDL_PollEvent(&event))
        // {
        // handle_user_input(cpu, event);
        // }
        // cpu.write(0xfe, dist(rng));

        // if (read_screen_state(cpu, screen_state))
        // {
        //     SDL_UpdateTexture(texture, nullptr, screen_state.data(), 32 * 3);
        //     SDL_RenderClear(renderer);
        //     SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        //     SDL_RenderPresent(renderer);
        // }
        //
        // std::this_thread::sleep_for(std::chrono::nanoseconds(70'000));
    });

    // clear resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
