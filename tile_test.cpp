#include "bus.h"
#include "cpu.h"
#include "render/frame.h"
#include "render/palette.h"
#include "render/render.h"
#include "trace.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <arm_neon.h>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <ostream>
#include <random>
#include <vector>

EM::Frame *show_tile(std::vector<uint8_t> chr_rom, size_t bank, size_t tile_n)
{
    auto *frame = new EM::Frame();
    bank = bank * 0x1000;
    auto tile_start = chr_rom.begin() + (bank + tile_n * 16);
    auto tile_end = chr_rom.begin() + (bank + tile_n * 16 + 16);

    const std::vector<uint8_t> tile_data(tile_start, tile_end);

    for (auto y = 0; y <= 7; ++y)
    {
        auto upper = tile_data[y];
        auto lower = tile_data[y + 8];

        for (int x = 7; x >= 0; --x)
        {
            //			auto value = ((1 & lower) << 1) | (1 & upper);
            auto value = ((1 & upper) << 1) | (1 & lower);
            upper >>= 1;
            lower >>= 1;
            std::array<uint8_t, 3> rgb;
            switch (value)
            {
            case 0: {
                rgb = EM::SystemPalette::palette[0x01];
                break;
            }
            case 1: {
                rgb = EM::SystemPalette::palette[0x23];
                break;
            }
            case 2: {
                rgb = EM::SystemPalette::palette[0x27];
                break;
            }
            case 3: {
                rgb = EM::SystemPalette::palette[0x30];
                break;
            }
            default:
                throw std::runtime_error("cannot be");
            }

            frame->set_pixel(x, y, rgb);
        }
    }
    return frame;
}
EM::Frame *show_tile_bank(std::vector<uint8_t> chr_rom, size_t bank)
{
    auto *frame = new EM::Frame();
    auto tile_y = 0;
    auto tile_x = 0;
    bank = bank * 0x1000;

    for (auto tile_n = 0; tile_n < 255; ++tile_n)
    {
        if (tile_n != 0 && tile_n % 20 == 0)
        {
            tile_y += 10;
            tile_x = 0;
        }

        auto tile_start = chr_rom.begin() + (bank + tile_n * 16);
        auto tile_end = chr_rom.begin() + (bank + tile_n * 16 + 16);
        const std::vector<uint8_t> tile_data(tile_start, tile_end);

        for (auto y = 0; y <= 7; ++y)
        {
            auto upper = tile_data[y];
            auto lower = tile_data[y + 8];

            for (int x = 7; x >= 0; --x)
            {
                auto value = ((1 & upper) << 1) | (1 & lower);
                upper >>= 1;
                lower >>= 1;
                std::array<uint8_t, 3> rgb;
                switch (value)
                {
                case 0: {
                    rgb = EM::SystemPalette::palette[0x01];
                    break;
                }
                case 1: {
                    rgb = EM::SystemPalette::palette[0x23];
                    break;
                }
                case 2: {
                    rgb = EM::SystemPalette::palette[0x27];
                    break;
                }
                case 3: {
                    rgb = EM::SystemPalette::palette[0x30];
                    break;
                }
                default:
                    throw std::runtime_error("cannot be");
                }

                frame->set_pixel(tile_x + x, tile_y + y, rgb);
            }
        }
        tile_x += 10;
    }
    return frame;
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
    SDL_Window *window = SDL_CreateWindow("Nes emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256.0 * 3.0,
                                          240.0 * 3.0, SDL_WINDOW_SHOWN);
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
    if (SDL_RenderSetScale(renderer, 3.0f, 3.0f) != 0)
    {
        std::cerr << "SDL_RenderSetScale Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // create texture
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, 256, 240);
    if (texture == nullptr)
    {
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // read Nes file
    std::vector<uint8_t> bytes = readFile("../game.nes");
    EM::Rom rom(bytes);

    EM::Frame *frame = show_tile_bank(rom.chr_rom, 1);

    std::vector<uint8_t> screen_state(32 * 3 * 32, 0);
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(1, 15);

    SDL_Event event;

    SDL_UpdateTexture(texture, nullptr, frame->data.data(), 256 * 3);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    while (true)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    std::exit(0);
                }
                break;
            default:
                break;
            }
        }
    }
}
