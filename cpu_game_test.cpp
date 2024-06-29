#include "Bus.h"
#include "CPU.h"
#include <SDL.h>
#include <SDL_pixels.h>
#include <arm_neon.h>
#include <cassert>
#include <cstdint>
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
        SDL_CreateWindow("Snake game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 320, 320, SDL_WINDOW_SHOWN);
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

    std::vector<uint8_t> game_code{
        0x20, 0x06, 0x06, 0x20, 0x38, 0x06, 0x20, 0x0d, 0x06, 0x20, 0x2a, 0x06, 0x60, 0xa9, 0x02, 0x85, 0x02, 0xa9,
        0x04, 0x85, 0x03, 0xa9, 0x11, 0x85, 0x10, 0xa9, 0x10, 0x85, 0x12, 0xa9, 0x0f, 0x85, 0x14, 0xa9, 0x04, 0x85,
        0x11, 0x85, 0x13, 0x85, 0x15, 0x60, 0xa5, 0xfe, 0x85, 0x00, 0xa5, 0xfe, 0x29, 0x03, 0x18, 0x69, 0x02, 0x85,
        0x01, 0x60, 0x20, 0x4d, 0x06, 0x20, 0x8d, 0x06, 0x20, 0xc3, 0x06, 0x20, 0x19, 0x07, 0x20, 0x20, 0x07, 0x20,
        0x2d, 0x07, 0x4c, 0x38, 0x06, 0xa5, 0xff, 0xc9, 0x77, 0xf0, 0x0d, 0xc9, 0x64, 0xf0, 0x14, 0xc9, 0x73, 0xf0,
        0x1b, 0xc9, 0x61, 0xf0, 0x22, 0x60, 0xa9, 0x04, 0x24, 0x02, 0xd0, 0x26, 0xa9, 0x01, 0x85, 0x02, 0x60, 0xa9,
        0x08, 0x24, 0x02, 0xd0, 0x1b, 0xa9, 0x02, 0x85, 0x02, 0x60, 0xa9, 0x01, 0x24, 0x02, 0xd0, 0x10, 0xa9, 0x04,
        0x85, 0x02, 0x60, 0xa9, 0x02, 0x24, 0x02, 0xd0, 0x05, 0xa9, 0x08, 0x85, 0x02, 0x60, 0x60, 0x20, 0x94, 0x06,
        0x20, 0xa8, 0x06, 0x60, 0xa5, 0x00, 0xc5, 0x10, 0xd0, 0x0d, 0xa5, 0x01, 0xc5, 0x11, 0xd0, 0x07, 0xe6, 0x03,
        0xe6, 0x03, 0x20, 0x2a, 0x06, 0x60, 0xa2, 0x02, 0xb5, 0x10, 0xc5, 0x10, 0xd0, 0x06, 0xb5, 0x11, 0xc5, 0x11,
        0xf0, 0x09, 0xe8, 0xe8, 0xe4, 0x03, 0xf0, 0x06, 0x4c, 0xaa, 0x06, 0x4c, 0x35, 0x07, 0x60, 0xa6, 0x03, 0xca,
        0x8a, 0xb5, 0x10, 0x95, 0x12, 0xca, 0x10, 0xf9, 0xa5, 0x02, 0x4a, 0xb0, 0x09, 0x4a, 0xb0, 0x19, 0x4a, 0xb0,
        0x1f, 0x4a, 0xb0, 0x2f, 0xa5, 0x10, 0x38, 0xe9, 0x20, 0x85, 0x10, 0x90, 0x01, 0x60, 0xc6, 0x11, 0xa9, 0x01,
        0xc5, 0x11, 0xf0, 0x28, 0x60, 0xe6, 0x10, 0xa9, 0x1f, 0x24, 0x10, 0xf0, 0x1f, 0x60, 0xa5, 0x10, 0x18, 0x69,
        0x20, 0x85, 0x10, 0xb0, 0x01, 0x60, 0xe6, 0x11, 0xa9, 0x06, 0xc5, 0x11, 0xf0, 0x0c, 0x60, 0xc6, 0x10, 0xa5,
        0x10, 0x29, 0x1f, 0xc9, 0x1f, 0xf0, 0x01, 0x60, 0x4c, 0x35, 0x07, 0xa0, 0x00, 0xa5, 0xfe, 0x91, 0x00, 0x60,
        0xa6, 0x03, 0xa9, 0x00, 0x81, 0x10, 0xa2, 0x00, 0xa9, 0x01, 0x81, 0x10, 0x60, 0xa2, 0x00, 0xea, 0xea, 0xca,
        0xd0, 0xfb, 0x60};

    auto bus = EM::Bus();
    bus.cpu.load(game_code);
    bus.cpu.reset();
    bus.cpu.registers.pc = 0x0600;

    std::vector<uint8_t> screen_state(32 * 3 * 32, 0);
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(1, 15);

    SDL_Event event;

    bus.cpu.run_with_callback([&](EM::CPU &cpu) {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            handle_user_input(cpu, event);
        }
        cpu.write(0xfe, dist(rng));

        if (read_screen_state(cpu, screen_state))
        {
            SDL_UpdateTexture(texture, nullptr, screen_state.data(), 32 * 3);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(70'000));
    });

    // clear resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
