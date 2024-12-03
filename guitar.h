#pragma once
#include "framework.h"

typedef enum tagGUITAR_BUTTON
{
	GREEN,
	RED,
	YEELOW,
	BLUE,
	ORANGE,

	UP_STRUM,
	DOWN_STRUM,
	GRAPH_LAST = DOWN_STRUM,

	LEFT,
	RIGHT,
	START,
	SELECT,

	BUTTON_COUNT
} GUITAR_BUTTON;

typedef struct tagBUTTON_INFO
{
	WORD wXButton;
	WORD wDialogId;
	WORD wCtlIdCounter;
	WORD wCtlIdTimer;

	HWND hControl;
	HWND hLabelCounter;
	HWND hLabelTimer;
}
BUTTON_INFO;

extern BUTTON_INFO buttons[BUTTON_COUNT];
