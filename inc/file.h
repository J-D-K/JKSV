#ifndef FILE_H
#define FILE_H

#include <string>
#include <cstdio>
#include <vector>
#include <switch.h>
#include <dirent.h>

#include "data.h"

namespace fs
{
    void init();
    void exit();

    //Mounts usr's save data for open. Returns false it fails
    bool mountSave(data::user& usr, data::titledata& open);

    void copyFile(const std::string& from, const std::string& to);
    void copyFileCommit(const std::string& from, const std::string& to, const std::string& dev);

    //Recursively copies 'from' to 'to'
    void copyDirToDir(const std::string& from, const std::string& to);

    //Same as above, but commits data to 'dev' after every file is closed
    void copyDirToDirCommit(const std::string& from, const std::string& to, const std::string& dev);

    //Recursively deletes 'path'
    void delDir(const std::string& path);

    //Dumps all titles for 'user'
    void dumpAllUserSaves(data::user& u);

    //returns file properties as C++ string
    std::string getFileProps(const std::string& _path);

    //Recursively retrieves info about dir _path
    void getDirProps(const std::string& _path, uint32_t& dirCount, uint32_t& fileCount, uint64_t& totalSize);

    bool fileExists(const std::string& _path);
    bool isDir(const std::string& _path);

    std::string getWorkDir();

    class dirItem
    {
        public:
            dirItem(const std::string& pathTo, const std::string& sItem);
            std::string getItm() { return itm; }
            bool isDir() { return dir; }

        private:
            std::string itm;
            bool dir = false;
    };

    //Just retrieves a listing for _path and stores it in item vector
    class dirList
    {
        public:
            dirList(const std::string& _path);
            void reassign(const std::string& _path);
            void rescan();

            std::string getItem(int index);
            bool isDir(int index);
            unsigned getCount();

        private:
            DIR *d;
            struct dirent *ent;
            std::string path;
            std::vector<dirItem> item;
    };

    class dataFile
    {
        public:
            dataFile(const std::string& _path);
            ~dataFile();

            bool isOpen(){ return opened; }

            //Gets next good line. Returns empty string if no more.
            std::string getNextLine();

        private:
            FILE *f;
            bool opened = false;
    };

    void logOpen();
    void logWrite(const char *fmt, ...);
    void logClose();
}

#endif // FILE_H
