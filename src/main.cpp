#include <string>
#include <vector>
#include <fstream>
#include <switch.h>

#include "gfx.h"
#include "sys.h"
#include "data.h"
#include "ui.h"

int main(int argc, const char *argv[])
{
	bool init = false;

	init = gfx::init();
	if(init)
		init = sys::init();
	if(init)
		init = data::init();

	if(init)
	{
		data::loadDataInfo();
		gfx::switchMode();
		ui::init();
		ui::userMenuInit();

		bool run = true;
		while(appletMainLoop() && run)
		{
			hidScanInput();

			uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);
			uint64_t held = hidKeysHeld(CONTROLLER_P1_AUTO);

			if(down & KEY_PLUS)
				break;

			gfx::clearBufferColor(0xFF3B3B3B);
			ui::drawTitleBar("JKSV - 06/17/2018");
			gfx::drawRectangle(448, 64, 1, 592, 0xFF7B7B7B);
			gfx::drawRectangle(449, 64, 2, 592, 0xFF2B2B2B);

			gfx::drawRectangle(16, 656, 1248, 1, 0xFF7B7B7B);
			gfx::drawRectangle(16, 657, 1248, 2, 0xFF2B2B2B);

			ui::runApp(down, held);

			gfx::handleBuffs();
		}
	}
	else
	{
		while(appletMainLoop())
		{
			hidScanInput();

			uint64_t down = hidKeysDown(CONTROLLER_P1_AUTO);

			if(down & KEY_PLUS)
				break;

			gfx::handleBuffs();
		}
	}

	gfx::fini();
	sys::fini();
	data::fini();
}
