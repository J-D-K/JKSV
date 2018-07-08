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

namespace ui
{
	extern bool clsMode;
	//Colors to use now that I added theme detection
	extern uint32_t clearClr, mnuTxt, txtClr, rectLt, rectSh, tboxClr;

	//Button tex
	extern gfx::tex buttonA, buttonB, buttonX, buttonY, buttonMin;
	//Textbox graphics
	extern gfx::tex cornerTopLeft, cornerTopRight, cornerBottomLeft, cornerBottomRight, horEdgeTop, horEdgeBot, vertEdgeLeft, vertEdgeRight;

	//Loads in the A, B, X, Y button graphics
	void init();
	void exit();

	//Prepares ui
	//Sets up buttons for icon touchin
	void setupSelButtons();
	void folderMenuPrepare(data::user& usr, data::titledata& dat);

	//Clears, draws UI + menus
	void drawUI();

	//Handles input and actions for UI menus. Sometimes draw things specific to each
	void updateUserMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p);
	void updateTitleMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p);
	void updateFolderMenu(const uint64_t& down, const uint64_t& held, const touchPosition& p);
	void updateAdvMode(const uint64_t& down, const uint64_t& held, const touchPosition& p);

	//switch case so we don't have problems with multiple main loops like 3DS
	void runApp(const uint64_t& down, const uint64_t& held, const touchPosition& p);
}

#endif
