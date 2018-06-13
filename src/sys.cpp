#include <fstream>
#include <switch.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sys.h"
#include "ui.h"

static std::fstream deb;

namespace sys
{
	bool devMode = false;

	bool init()
	{
		Result res = 0;
		res = fsdevMountSdmc();
		if(R_FAILED(res))
		{
			printf("MountSdmc failed\n");
			return false;
		}

		res = romfsInit();
		if(R_FAILED(res))
		{
			printf("romfsInit failed\n");
			return false;
		}

		res = hidInitialize();
		if(R_FAILED(res))
		{
			printf("hidInit failed\n");
			return false;
		}

		res = nsInitialize();
		if(R_FAILED(res))
		{
			printf("nsInit failed\n");
			return false;
		}

		mkdir("sdmc:/JKSV", 0777);
		chdir("sdmc:/JKSV");

		deb.open("deb.txt", std::ios::out);

		std::fstream chk("0x4A4B.dev", std::ios::in);
		if(chk.is_open())
		{
			devMode = true;
			chk.close();
		}

		return true;
	}

	bool fini()
	{
		deb.close();
		fsdevUnmountAll();
		romfsExit();
		hidExit();
		nsExit();

		return true;
	}

	void debugWrite(const std::string& out)
	{
		deb.write(out.c_str(), out.length());
	}
}
