#ifndef SYS_H
#define SYS_H

namespace sys
{
	//Inits services, romfs, sdmc
	bool init();
	bool fini();

	//Uh oh
	extern bool sysSave, devMenu, forceMountable;
}

#endif // SYS_H
