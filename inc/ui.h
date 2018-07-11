#ifndef UI_H
#define UI_H

#include <vector>
#include <string>

#include "data.h"
#include "gfx.h"

//ui headers - split up to keep a bit more organized
#include "kb.h"
#include "menu.h"
#include "miscui.h"
#include "clsui.h"
#include "uiupdate.h"

enum menuState
{
	USR_SEL,
	TTL_SEL,
	FLD_SEL,
	ADV_MDE,
	CLS_USR,
	CLS_TTL
};


namespace ui
{
	//Classic mode/text menus
	extern bool clsMode;

	//Current menu/ui state
	extern int mstate;

	//Both UI modes need access to this
	extern std::string folderMenuInfo;

	//Colors to use now that I added theme detection
	extern uint32_t clearClr, mnuTxt, txtClr, rectLt, rectSh, tboxClr;

	//Button tex
	extern gfx::tex buttonA, buttonB, buttonX, buttonY, buttonMin;
	//Textbox graphics
	extern gfx::tex cornerTopLeft, cornerTopRight, cornerBottomLeft, cornerBottomRight,\
	horEdgeTop, horEdgeBot, vertEdgeLeft, vertEdgeRight;

	extern std::vector<ui::button> selButtons;

	//Loads in the A, B, X, Y button graphics
	void init();
	void exit();

	//Prepares ui
	//Sets up buttons for icon touchin
	void setupSelButtons();

	//Clears and draws general stuff used by multiple screens
	void drawUI();

	//switch case so we don't have problems with multiple main loops like 3DS
	void runApp(const uint64_t& down, const uint64_t& held, const touchPosition& p);
}

#endif
