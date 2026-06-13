#pragma once

#include <windows.h>
#include <xinput.h>

#pragma comment(lib, "xinput.lib")

namespace XController {
	// XInput button masks
	enum ButtonMask {
		BTN_A = XINPUT_GAMEPAD_A,
		BTN_B = XINPUT_GAMEPAD_B,
		BTN_X = XINPUT_GAMEPAD_X,
		BTN_Y = XINPUT_GAMEPAD_Y,
		BTN_LB = 0x0100,  // LEFT_SHOULDER
		BTN_RB = 0x0200,  // RIGHT_SHOULDER
		BTN_BACK = XINPUT_GAMEPAD_BACK,
		BTN_START = XINPUT_GAMEPAD_START,
		BTN_VIEW = XINPUT_GAMEPAD_BACK,  // Same as BACK (View button)
		BTN_MENU = XINPUT_GAMEPAD_START,  // Same as START (Menu button)
		BTN_LEFT_THUMB = XINPUT_GAMEPAD_LEFT_THUMB,
		BTN_RIGHT_THUMB = XINPUT_GAMEPAD_RIGHT_THUMB,
		BTN_DPAD_UP = XINPUT_GAMEPAD_DPAD_UP,
		BTN_DPAD_DOWN = XINPUT_GAMEPAD_DPAD_DOWN,
		BTN_DPAD_LEFT = XINPUT_GAMEPAD_DPAD_LEFT,
		BTN_DPAD_RIGHT = XINPUT_GAMEPAD_DPAD_RIGHT,
	};

	// Initialize controller input
	void Init();
	void Shutdown();

	// Check button state (current frame)
	bool IsButtonDown(int controller, ButtonMask button);
	bool IsButtonJustPressed(int controller, ButtonMask button);
	bool IsButtonLongPress(int controller, ButtonMask button, DWORD holdTimeMs = 500);

	// Get stick values
	void GetLeftStick(int controller, float& x, float& y);
	void GetRightStick(int controller, float& x, float& y);

	// Get trigger values
	float GetLeftTrigger(int controller);
	float GetRightTrigger(int controller);

	// Utility
	bool IsControllerConnected(int controller);
	void UpdateState();
};
