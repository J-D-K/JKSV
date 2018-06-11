#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <switch.h>

#include "gfx.h"
#include "sys.h"
#include "data.h"
#include "file.h"
#include "util.h"
#include "ui.h"

int main(int argc, const char *argv[])
{
	bool init = false;

	init = gfx::init(64);
	if(init) init = sys::init();
	if(init) init = data::init();

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
			touchPosition p;
			hidTouchRead(&p, 0);

            if(down & KEY_PLUS)
                break;

            gfx::clearConsoleColor(0x3B3B3BFF);
			gfx::drawText("JKSV - 6/10/2018", 16, 16, 64, 0xFFFFFFFF);
			gfx::drawRectangle(16, 64, 1248, 2, 0xFFFFFFFF);
			gfx::drawRectangle(384, 64, 2, 592, 0xFFFFFFFF);
			gfx::drawRectangle(16, 656, 1248, 2, 0xFFFFFFFF);

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
