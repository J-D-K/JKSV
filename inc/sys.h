#ifndef SYS_H
#define SYS_H

namespace sys
{
	//Inits services, romfs, sdmc
	bool init();
	bool fini();

	//Writes to 'deb.txt' in JKSV folder
	void debugWrite(const std::string& out);
}

#endif // SYS_H
