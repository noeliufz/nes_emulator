#include "bus.h"
#include "cpu.h"
#include "joypad.h"
#include "render/frame.h"
#include "render/render.h"
#include "trace.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_keycode.h>
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
    SDL_Window *window = SDL_CreateWindow("Nes emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256 * 3,
                                          240 * 3, SDL_WINDOW_SHOWN);
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
    if (SDL_RenderSetScale(renderer, 2.0f, 2.0f) != 0)
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

    EM::Frame frame;

    std::vector<uint8_t> screen_state(32 * 3 * 32, 0);
    std::random_device rd;
    //    std::uniform_int_distribution<int> dist(1, 15);

    std::unordered_map<SDL_Keycode, EM::JoypadButton> key_map = {
        {SDLK_DOWN, EM::JoypadButton::DOWN},    {SDLK_UP, EM::JoypadButton::UP},
        {SDLK_RIGHT, EM::JoypadButton::RIGHT},  {SDLK_LEFT, EM::JoypadButton::LEFT},
        {SDLK_SPACE, EM::JoypadButton::SELECT}, {SDLK_RETURN, EM::JoypadButton::START},
        {SDLK_a, EM::JoypadButton::BUTTON_A},   {SDLK_s, EM::JoypadButton::BUTTON_A},
    };

    auto gameloop_callback = [&](EM::NesPPU &ppu, EM::Joypad &joypad) {
        render(ppu, frame);
        SDL_UpdateTexture(texture, nullptr, frame.data.data(), 256 * 3);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

		SDL_Event event;
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
			  else if (auto keycode = key_map.find(event.key.keysym.sym); keycode != key_map.end())
			  {
				  joypad.set_button_pressed_status(keycode->second, true);
			  }
			  break;
		  case SDL_KEYUP: {
			  if (auto keycode = key_map.find(event.key.keysym.sym); keycode != key_map.end())
			  {
				  joypad.set_button_pressed_status(keycode->second, false);
			  }
			  break;
		  }
		  default:
			  break;
		  }
		}
    };

    auto bus = EM::Bus(&rom, gameloop_callback);
    auto cpu = EM::CPU(&bus);
    cpu.reset();
//    cpu.run();
    cpu.run_with_callback([&](EM::CPU &cpu) {
//		std::cout << trace(cpu) << std::endl;
	});
}
