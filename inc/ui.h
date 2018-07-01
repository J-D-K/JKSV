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
	//Colors to use now that I added theme detection
	extern uint32_t clearClr, mnuTxt, txtClr, rectLt, rectSh, tboxClr;

	//Textbox graphics
	extern gfx::tex cornerTopLeft, cornerTopRight, cornerBottomLeft, cornerBottomRight, horEdgeTop, horEdgeBot, vertEdgeLeft, vertEdgeRight;

	//Loads in the A, B, X, Y button graphics
	void init();
	void exit();

	//Prepares menus for use
	void userMenuInit();
	void titleMenuPrepare(data::user& usr);
	void folderMenuPrepare(data::user& usr, data::titledata& dat);

	//Draws title bar at top
	void drawTitleBar(const std::string& txt);

	//Clears, draws UI + menus
	void drawUI();

	//Handles input and actions for UI menus. Sometimes draw thing specific to each
	void updateUserMenu(const uint64_t& down, const uint64_t& held);
	void updateTitleMenu(const uint64_t& down, const uint64_t& held);
	void updateFolderMenu(const uint64_t& down, const uint64_t& held);
	void updateAdvMode(const uint64_t& down, const uint64_t& held);

	//switch case so we don't have problems with multiple main loops like 3DS
	void runApp(const uint64_t& down, const uint64_t& held);
}

#endif
