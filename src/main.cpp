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

			ui::runApp(down, held);
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
