//
// Created by Fangzhou Liu on 5/8/2024.
//

#ifndef MYNESEMULATOR__JOYPAD_H_
#define MYNESEMULATOR__JOYPAD_H_


#include <cstdint>

namespace EM
{

	enum class JoypadButton : uint8_t
	{
		RIGHT = 0b10000000,
		LEFT = 0b01000000,
		DOWN = 0b00100000,
		UP = 0b00010000,
		START = 0b00001000,
		SELECT = 0b00000100,
		BUTTON_B = 0b00000010,
		BUTTON_A = 0b00000001,
	};

	inline JoypadButton operator|(JoypadButton a, JoypadButton b)
	{
		return static_cast<JoypadButton>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
	}

	inline JoypadButton operator&(JoypadButton a, JoypadButton b)
	{
		return static_cast<JoypadButton>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
	}

	inline JoypadButton operator~(JoypadButton a)
	{
		return static_cast<JoypadButton>(~static_cast<uint8_t>(a));
	}

	class Joypad
	{
	 public:
		Joypad() : strobe(false), button_index(0), button_status(static_cast<JoypadButton>(0))
		{
		}

		void write(uint8_t data)
		{
			strobe = (data & 1) == 1;
			if (strobe)
			{
				button_index = 0;
			}
		}

		uint8_t read()
		{
			if (button_index > 7)
			{
				return 1;
			}
			uint8_t response = (static_cast<uint8_t>(button_status) & (1 << button_index)) >> button_index;
			if (!strobe && button_index <= 7)
			{
				button_index++;
			}
			return response;
		}

		void set_button_pressed_status(JoypadButton button, bool pressed)
		{
			if (pressed)
			{
				button_status = button_status | button;
			}
			else
			{
				button_status = button_status & ~button;
			}
		}

	 private:
		bool strobe;
		uint8_t button_index;
		JoypadButton button_status;
	};
}

#endif //MYNESEMULATOR__JOYPAD_H_
