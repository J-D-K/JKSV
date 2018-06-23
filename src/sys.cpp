#include <fstream>
#include <switch.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sys.h"

namespace sys
{
	bool sysSave = false;
	bool devMenu = false;
	bool forceMountable = true;

	void loadCfg()
	{
		std::fstream devIn("0x4A4B.dev", std::ios::in | std::ios::binary);
		if(devIn.is_open())
		{
			uint8_t byte = 0;
			devIn.read((char *)&byte, 1);
			devIn.close();

			sysSave = byte >> 7 & 1;
			devMenu = byte >> 6 & 1;
			forceMountable = byte >> 5 & 1;

			//Make sure the rest are 0
			for(int i = 4; i > -1; i--)
			{
				bool chk = byte >> i & 1;
				if(chk == true)
				{
					//Disable
					sysSave = false;
					devMenu = false;
				}
			}

			if(sysSave)
				printf("System save dumping enabled.\n");
			if(devMenu)
				printf("DevMenu enabled\n");
		}
	}

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

		res = setsysInitialize();
		if(R_FAILED(res))
		{
			printf("setInit() failed\n");
			return false;
		}

		mkdir("sdmc:/JKSV", 0777);
		chdir("sdmc:/JKSV");

		loadCfg();

		return true;
	}

	bool fini()
	{
		fsdevUnmountAll();
		romfsExit();
		hidExit();
		nsExit();
		setsysExit();

		return true;
	}
}
