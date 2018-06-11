#ifndef FILE_H
#define FILE_H

#include <string>
#include <vector>
#include <switch.h>
#include <dirent.h>

#include "data.h"

namespace fs
{
    bool mountSave(data::user& usr, data::titledata& open);
    void copyDirToDir(const std::string& from, const std::string& to);
    void copyDirToDirCommit(const std::string& from, const std::string& to, const std::string& dev);
    void delDir(const std::string& path);

    class dirList
    {
        public:
            dirList(const std::string& _path);
            void rescan();

            const std::string getItem(int index);
            bool isDir(int index);
            unsigned getCount();

        private:
            DIR *d;
            struct dirent *ent;
            std::string path;
            std::vector<std::string> item;
    };
}

#endif // FILE_H
