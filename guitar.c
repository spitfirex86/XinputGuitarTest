#include "framework.h"
#include "guitar.h"
#include "resource.h"
#include <Xinput.h>

BUTTON_INFO buttons[BUTTON_COUNT] = {
	[GREEN] = { XINPUT_GAMEPAD_A, IDC_GREEN, IDC_GREENNB, IDC_GREENMS },
	[RED] = { XINPUT_GAMEPAD_B, IDC_RED, IDC_REDNB, IDC_REDMS },
	[YEELOW] = { XINPUT_GAMEPAD_Y, IDC_YEELOW, IDC_YEELOWNB, IDC_YEELOWMS },
	[BLUE] = { XINPUT_GAMEPAD_X, IDC_BLUE, IDC_BLUENB, IDC_BLUEMS },
	[ORANGE] = { XINPUT_GAMEPAD_LEFT_SHOULDER, IDC_ORANGE, IDC_ORANGENB, IDC_ORANGEMS },

	[UP_STRUM] = { XINPUT_GAMEPAD_DPAD_UP, IDC_UP, IDC_UPNB, IDC_UPMS },
	[DOWN_STRUM] = { XINPUT_GAMEPAD_DPAD_DOWN, IDC_DOWN, IDC_DOWNNB, IDC_DOWNMS },
	[LEFT] = { XINPUT_GAMEPAD_DPAD_LEFT, IDC_LEFT },
	[RIGHT] = { XINPUT_GAMEPAD_DPAD_RIGHT, IDC_RIGHT },

	[START] = { XINPUT_GAMEPAD_START, IDC_START },
	[SELECT] = { XINPUT_GAMEPAD_BACK, IDC_SELECT }
};
