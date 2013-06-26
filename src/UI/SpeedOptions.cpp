#include "AllWindows.h"
#include "Window.h"

#include "../Widget.h"

#include "../Data/Screen.h"
#include "../Data/Mouse.h"
#include "../Data/Constants.h"
#include "../Data/Settings.h"
#include "../Data/Buttons.h"

static void buttonToggle(int param1, int param2);
static void buttonOk(int param1, int param2);
static void buttonCancel(int param1, int param2);

static void arrowButtonGame(int param1, int param2);
static void arrowButtonScroll(int param1, int param2);

static CustomButton buttons[2] = {
	{
		144, 232, 336, 252,
		buttonOk,
		Widget_Button_doNothing,
		1, 1, 0
	},
	{
		144, 262, 336, 282,
		buttonCancel,
		Widget_Button_doNothing,
		1, 1, 0
	},
};

static ArrowButton arrowButtons[4] = {
	{112, 100, 17, 24, arrowButtonGame, 1, 0},
	{136, 100, 15, 24, arrowButtonGame, 0, 0},
	{112, 136, 17, 24, arrowButtonScroll, 1, 0},
	{136, 136, 15, 24, arrowButtonScroll, 0, 0},
};

static int original_gameSpeed;
static int original_scrollSpeed;

static int focusButtonId;

void UI_SpeedOptions_init()
{
	original_gameSpeed = Data_Settings.gameSpeed;
	original_scrollSpeed = Data_Settings.scrollSpeed;
}

void UI_SpeedOptions_drawForeground()
{
	int baseOffsetX = Data_Screen.offset640x480.x;
	int baseOffsetY = Data_Screen.offset640x480.y;
	
	Widget_Panel_drawOuterPanel(
		baseOffsetX + 96, baseOffsetY + 80,
		18, 14
	);
	
	Widget_Panel_drawSmallLabelButton(4,
		baseOffsetX + 144, baseOffsetY + 232,
		12, 0, focusButtonId == 1 ? 1 : 2
	);
	Widget_Panel_drawSmallLabelButton(4,
		baseOffsetX + 144, baseOffsetY + 262,
		12, 0, focusButtonId == 2 ? 1 : 2
	);
	
	Color colorNormal = 0x02bf;
	Color colorHighlight = 0xfac1;
	Widget_GameText_drawCentered(45, 0,
		baseOffsetX + 128, baseOffsetY + 92,
		224, Font_LargeBlack, Color_Black
	);
	Widget_GameText_drawCentered(45, 4,
		baseOffsetX + 128, baseOffsetY + 236,
		224, Font_SmallBrown, focusButtonId == 1 ? colorHighlight : colorNormal
	);
	Widget_GameText_drawCentered(45, 1,
		baseOffsetX + 128, baseOffsetY + 266,
		224, Font_SmallBrown, focusButtonId == 2 ? colorHighlight : colorNormal
	);
	Widget_GameText_draw(45, 2,
		baseOffsetX + 144, baseOffsetY + 146,
		Font_NormalPlain, Color_Black
	);
	Widget_Text_drawNumber(
		Data_Settings.gameSpeed, 64, "%",
		baseOffsetX + 296, baseOffsetY + 146,
		Font_NormalPlain, Color_Black
	);
	Widget_GameText_draw(45, 3,
		baseOffsetX + 144, baseOffsetY + 182,
		Font_NormalPlain, Color_Black
	);
	Widget_Text_drawNumber(
		Data_Settings.scrollSpeed, 64, "%",
		baseOffsetX + 296, baseOffsetY + 182,
		Font_NormalPlain, Color_Black
	);

	Widget_Button_drawArrowButtons(
		baseOffsetX + 128, baseOffsetY + 40,
		arrowButtons, 8
	);

	// TODO: reset clip region() ?
}

void UI_SpeedOptions_handleMouse()
{
	if (Data_Mouse.isRightClick) {
		// cancel dialog
		UI_Window_goTo(Window_City);
	} else {
		int baseOffsetX = Data_Screen.offset640x480.x;
		int baseOffsetY = Data_Screen.offset640x480.y;
		if (!Widget_Button_handleCustomButtons(
				baseOffsetX, baseOffsetY, buttons, 2, &focusButtonId)) {
			Widget_Button_handleArrowButtons(
				baseOffsetX + 128, baseOffsetY + 40, arrowButtons, 4);
		}
	}
}

static void buttonOk(int param1, int param2)
{
	UI_Window_goTo(Window_City);
}

static void buttonCancel(int param1, int param2)
{
	Data_Settings.gameSpeed = original_gameSpeed;
	Data_Settings.scrollSpeed = original_scrollSpeed;
	UI_Window_goTo(Window_City);
}

static void arrowButtonGame(int param1, int param2)
{
	if (param1 == 1) {
		Data_Settings.gameSpeed -= 10;
	} else if (param1 == 0) {
		Data_Settings.gameSpeed += 10;
	}
	if (Data_Settings.gameSpeed < 10) {
		Data_Settings.gameSpeed = 10;
	}
	if (Data_Settings.gameSpeed > 100) {
		Data_Settings.gameSpeed = 100;
	}
	UI_Window_requestRefresh();
}

static void arrowButtonScroll(int param1, int param2)
{
	if (param1 == 1) {
		Data_Settings.scrollSpeed -= 10;
	} else if (param1 == 0) {
		Data_Settings.scrollSpeed += 10;
	}
	if (Data_Settings.scrollSpeed < 10) {
		Data_Settings.scrollSpeed = 10;
	}
	if (Data_Settings.scrollSpeed > 100) {
		Data_Settings.scrollSpeed = 100;
	}
	UI_Window_requestRefresh();
}