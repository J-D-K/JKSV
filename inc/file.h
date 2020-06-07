#ifndef FILE_H
#define FILE_H

#include <string>
#include <cstdio>
#include <vector>
#include <switch.h>
#include <dirent.h>
#include <minizip/zip.h>

#include "fsfile.h"
#include "data.h"

namespace fs
{
    void init();
    void exit();

    //Mounts usr's save data for open. Returns false it fails
    bool mountSave(const data::user& usr, const data::titledata& open);
    inline bool unmountSave(){ return fsdevUnmountDevice("sv") == 0; }

    void copyFile(const std::string& from, const std::string& to);
    void copyFileCommit(const std::string& from, const std::string& to, const std::string& dev);

    //Recursively copies 'from' to 'to'
    void copyDirToDir(const std::string& from, const std::string& to);

    //Copies from to zipFile to
    void copyDirToZip(const std::string& from, zipFile *to);

    //Same as above, but commits data to 'dev' after every file is closed
    void copyDirToDirCommit(const std::string& from, const std::string& to, const std::string& dev);

    //deletes file
    void delfile(const std::string& path);
    //Recursively deletes 'path'
    void delDir(const std::string& path);

    //Dumps all titles for 'user'. returns false to bail
    bool dumpAllUserSaves(const data::user& u);

    //returns file properties as C++ string
    std::string getFileProps(const std::string& _path);

    //Recursively retrieves info about dir _path
    void getDirProps(const std::string& _path, uint32_t& dirCount, uint32_t& fileCount, uint64_t& totalSize);

    bool fileExists(const std::string& _path);
    //Returns file size
    size_t fsize(const std::string& _f);
    //Returns if device in path has space needed. Device is gotten from file path.
    bool hasFreeSpace(const std::string& _f, int needed);
    bool isDir(const std::string& _path);

    std::string getWorkDir();

    class dirItem
    {
        public:
            dirItem(const std::string& pathTo, const std::string& sItem);
            std::string getItm() const { return itm; }
            bool isDir() const { return dir; }

        private:
            std::string itm;
            bool dir = false;
    };

    //Just retrieves a listing for _path and stores it in item vector
    class dirList
    {
        public:
            dirList() = default;
            dirList(const std::string& _path);
            void reassign(const std::string& _path);
            void rescan();

            std::string getItem(int index) const { return item[index].getItm(); }
            bool isDir(int index) const { return item[index].isDir(); }
            unsigned getCount() const { return item.size(); }

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

            bool isOpen() const { return opened; }

            bool readNextLine(bool proc);
            //Finds where variable name ends. When a '(' or '=' is hit. Strips spaces
            void procLine();
            std::string getLine() const { return line; }
            std::string getName() const { return name; }
            //Reads until ';', ',', or '\n' is hit and returns as string.
            std::string getNextValueStr();
            int getNextValueInt();

        private:
            FILE *f;
            std::string line, name;
            size_t lPos = 0;
            bool opened = false;
    };

    void logOpen();
    void logWrite(const char *fmt, ...);
    void logClose();
}

#endif // FILE_H
