#ifndef SYS_H
#define SYS_H

namespace sys
{
    bool init();
    bool fini();

    void debugWrite(const std::string& out);
}

#endif // SYS_H
