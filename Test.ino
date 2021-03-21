#include "USBHost_t36.h"
#include <usb_keyboard.h>
#include <Bounce.h>
#include <SPI.h>
#include <SD.h>

#define BUTTON 14
#define UNPRESSED -1
#define SD_READ_INTERVAL 5
const int MODIFIERS[8] = {KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_ALT, KEY_LEFT_GUI, KEY_RIGHT_CTRL, KEY_RIGHT_SHIFT, KEY_RIGHT_ALT, KEY_RIGHT_GUI};

USBHost myusb;
USBHub hub1(myusb);
KeyboardController keyboard1(myusb);

Bounce bouncer = Bounce(BUTTON, 10);

void setup()
{
	pinMode(BUTTON, INPUT_PULLUP);

	myusb.begin();
	keyboard1.attachRawPress(OnRawPress);
	keyboard1.attachRawRelease(OnRawRelease);
	usb_keyboard_configure();
	keyboard1.LEDS(keyboard_leds);
}

void loop()
{
	myusb.Task();
	bouncer.update();

	if (bouncer.fallingEdge())
	{
		WriteFromCard();
	}
}

void OnRawPress(uint8_t key)
{
	PressKey(key);
	keyboard1.LEDS(keyboard_leds);
}

void OnRawRelease(uint8_t key)
{
	ReleaseKey(key);
}

void ReleaseKey(uint8_t key)
{
	if (IsModifier(key))
	{
		keyboard_modifier_keys = (keyboard_modifier_keys ^ GetModifier(key)) | 0xE000;
		usb_keyboard_send();

		return;
	}

	int pressed = CheckPressed(key);

	if (pressed != UNPRESSED)
	{
		keyboard_keys[pressed] = 0;
		usb_keyboard_send();
	}
}

void PressKey(uint8_t key)
{
	if (IsModifier(key))
	{
		keyboard_modifier_keys = keyboard_modifier_keys | GetModifier(key);
		usb_keyboard_send();

		return;
	}

	if (CheckPressed(key) != UNPRESSED)
		return;

	for (int i = 0; i < 6; i++)
	{
		if (keyboard_keys[i] == 0)
		{
			keyboard_keys[i] = key;
			usb_keyboard_send();
			return;
		}
	}
}

bool IsModifier(uint8_t key)
{
	return (key >= 103 && key <= 110);
}

bool IsLock(uint8_t key)
{
	return (key == 57 || key == 71 || key == 83);
}

int GetModifier(uint8_t key)
{
	return MODIFIERS[key - 103];
}

int CheckPressed(uint8_t key)
{
	for (int i = 0; i < 6; i++)
	{
		if (keyboard_keys[i] == key)
			return i;
	}
	return UNPRESSED;
}

void WriteFromCard()
{
	if (!SD.begin(BUILTIN_SDCARD))
	{
		Keyboard.println("Card failed, or not present");
		return;
	}

	File dataFile = SD.open("data.txt");

	if (dataFile)
	{
		char buf[1];
		while (dataFile.available())
		{
			dataFile.read(buf, 1);
			Keyboard.write(buf[0]);
			delay(SD_READ_INTERVAL);

			bouncer.update();

			if (bouncer.fallingEdge())
			{
				dataFile.close();
				return;
			}
		}
		dataFile.close();
	}
	else
	{
		Keyboard.println("Error opening data.txt");
	}
}
