#pragma once

#include <string>
#include <cstdio>
#include <vector>
#include <switch.h>
#include <dirent.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>

#include "fs.h"
#include "data.h"
#include "ui.h"

namespace fs
{
    //Copy args are optional and only used if passed and threaded
    void copyFile(const std::string& src, const std::string& dst, threadInfo *t);
    void copyFileThreaded(const std::string& src, const std::string& dst);
    void copyFileCommit(const std::string& src, const std::string& dst, const std::string& dev, threadInfo *t);
    void copyFileCommitThreaded(const std::string& src, const std::string& dst, const std::string& dev);
    void fileDrawFunc(void *a);

    //deletes file
    void delfile(const std::string& _p);

    //Loads paths to filter from backup/deletion
    void loadPathFilters(const uint64_t& tid);
    bool pathIsFiltered(const std::string& _path);
    void freePathFilters();

    void wipeSave();

    //Dumps all titles for current user
    void dumpAllUserSaves();

    void getShowFileProps(const std::string& _path);
    void getShowDirProps(const std::string& _path);

    bool fileExists(const std::string& _path);
    //Returns file size
    size_t fsize(const std::string& _f);

    class dataFile
    {
        public:
            dataFile(const std::string& _path);
            ~dataFile();
            void close(){ fclose(f); }

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

    //Take a pointer to backupArgs^
    void createNewBackup(void *a);
    void overwriteBackup(void *a);
    void restoreBackup(void *a);
    void deleteBackup(void *a);

    void logOpen();
    void logWrite(const char *fmt, ...);
    void logClose();
}
