#include "controller.h"
#include <map>

namespace XController {
	struct ControllerState {
		XINPUT_STATE currentState;
		XINPUT_STATE previousState;
		DWORD lastPress[16];  // Track press times for long press detection
		bool wasConnected;
	};

	static ControllerState g_controllers[4];

	void Init() {
		ZeroMemory(g_controllers, sizeof(g_controllers));
		for (int i = 0; i < 4; i++) {
			g_controllers[i].wasConnected = false;
			ZeroMemory(g_controllers[i].lastPress, sizeof(g_controllers[i].lastPress));
		}
	}

	void Shutdown() {
		// Nothing special needed for XInput
	}

	bool IsControllerConnected(int controller) {
		if (controller < 0 || controller >= 4) return false;
		
		XINPUT_STATE state;
		return XInputGetState(controller, &state) == ERROR_SUCCESS;
	}

	void UpdateState() {
		for (int i = 0; i < 4; i++) {
			g_controllers[i].previousState = g_controllers[i].currentState;
			
			DWORD result = XInputGetState(i, &g_controllers[i].currentState);
			if (result == ERROR_SUCCESS) {
				g_controllers[i].wasConnected = true;
			} else {
				g_controllers[i].wasConnected = false;
				ZeroMemory(&g_controllers[i].currentState, sizeof(XINPUT_STATE));
			}
		}
	}

	bool IsButtonDown(int controller, ButtonMask button) {
		if (controller < 0 || controller >= 4) return false;
		if (!g_controllers[controller].wasConnected) return false;

		return (g_controllers[controller].currentState.Gamepad.wButtons & button) != 0;
	}

	bool IsButtonJustPressed(int controller, ButtonMask button) {
		if (controller < 0 || controller >= 4) return false;
		if (!g_controllers[controller].wasConnected) return false;

		WORD currentButtons = g_controllers[controller].currentState.Gamepad.wButtons;
		WORD previousButtons = g_controllers[controller].previousState.Gamepad.wButtons;

		// Button pressed if it's down now and wasn't down before
		return (currentButtons & button) && !(previousButtons & button);
	}

	bool IsButtonLongPress(int controller, ButtonMask button, DWORD holdTimeMs) {
		if (controller < 0 || controller >= 4) return false;
		if (!g_controllers[controller].wasConnected) return false;

		WORD currentButtons = g_controllers[controller].currentState.Gamepad.wButtons;

		if (currentButtons & button) {
			// Button is currently held
			DWORD& lastPressTime = g_controllers[controller].lastPress[button & 0xFF];
			
			if (GetTickCount() - lastPressTime >= holdTimeMs) {
				return true;  // Long press detected
			}
		} else {
			// Button released - reset timer
			g_controllers[controller].lastPress[button & 0xFF] = GetTickCount();
		}

		return false;
	}

	void GetLeftStick(int controller, float& x, float& y) {
		if (controller < 0 || controller >= 4) {
			x = y = 0.0f;
			return;
		}
		if (!g_controllers[controller].wasConnected) {
			x = y = 0.0f;
			return;
		}

		SHORT stickX = g_controllers[controller].currentState.Gamepad.sThumbLX;
		SHORT stickY = g_controllers[controller].currentState.Gamepad.sThumbLY;

		// Deadzone
		const SHORT DEADZONE = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
		if (abs(stickX) < DEADZONE) stickX = 0;
		if (abs(stickY) < DEADZONE) stickY = 0;

		// Normalize to -1.0 to 1.0
		x = stickX / 32768.0f;
		y = stickY / 32768.0f;
	}

	void GetRightStick(int controller, float& x, float& y) {
		if (controller < 0 || controller >= 4) {
			x = y = 0.0f;
			return;
		}
		if (!g_controllers[controller].wasConnected) {
			x = y = 0.0f;
			return;
		}

		SHORT stickX = g_controllers[controller].currentState.Gamepad.sThumbRX;
		SHORT stickY = g_controllers[controller].currentState.Gamepad.sThumbRY;

		// Deadzone
		const SHORT DEADZONE = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
		if (abs(stickX) < DEADZONE) stickX = 0;
		if (abs(stickY) < DEADZONE) stickY = 0;

		// Normalize to -1.0 to 1.0
		x = stickX / 32768.0f;
		y = stickY / 32768.0f;
	}

	float GetLeftTrigger(int controller) {
		if (controller < 0 || controller >= 4) return 0.0f;
		if (!g_controllers[controller].wasConnected) return 0.0f;

		BYTE trigger = g_controllers[controller].currentState.Gamepad.bLeftTrigger;
		return trigger / 255.0f;  // Normalize to 0.0 - 1.0
	}

	float GetRightTrigger(int controller) {
		if (controller < 0 || controller >= 4) return 0.0f;
		if (!g_controllers[controller].wasConnected) return 0.0f;

		BYTE trigger = g_controllers[controller].currentState.Gamepad.bRightTrigger;
		return trigger / 255.0f;  // Normalize to 0.0 - 1.0
	}
}
