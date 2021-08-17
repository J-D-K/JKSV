#pragma once

#include <string>
#include <cstdio>
#include <vector>
#include <switch.h>
#include <dirent.h>
#include <minizip/zip.h>
#include <minizip/unzip.h>

#include "fsfile.h"
#include "fsthrd.h"
#include "data.h"
#include "miscui.h"

#define BUFF_SIZE 0xC0000

namespace fs
{
    void init();
    void exit();

    //Mounts usr's save data for open. Returns false it fails
    bool mountSave(const FsSaveDataInfo& _m);
    inline bool unmountSave() { return fsdevUnmountDevice("sv") == 0; }
    bool commitToDevice(const std::string& dev);
    void createSaveData(FsSaveDataType _type, uint64_t _tid, AccountUid _userID);

    void copyFile(const std::string& from, const std::string& to);
    void copyFileCommit(const std::string& from, const std::string& to, const std::string& dev);

    //Recursively copies 'from' to 'to'
    void copyDirToDir(const std::string& from, const std::string& to);

    //Copies from to zipFile to
    void copyDirToZip(const std::string& from, zipFile to);

    //Same as above, but commits data to 'dev' after every file is closed
    void copyDirToDirCommit(const std::string& from, const std::string& to, const std::string& dev);

    //Copies unzfile to 'to'
    void copyZipToDir(unzFile unz, const std::string& to, const std::string& dev);

    bool dirNotEmpty(const std::string& _dir);
    bool zipNotEmpty(unzFile unzip);

    void mkDir(const std::string& _p);
    void mkDirRec(const std::string& _p);
    //deletes file
    void delfile(const std::string& path);
    //Recursively deletes 'path'
    void delDir(const std::string& path);

    //Loads paths to filter from backup/deletion
    void loadPathFilters(const std::string& _file);
    bool pathIsFiltered(const std::string& _path);
    void freePathFilters();

    void wipeSave();

    //Dumps all titles for 'user'. returns false to bail
    bool dumpAllUserSaves(const data::user& u);

    //returns file properties as C++ string
    std::string getFileProps(const std::string& _path);

    //Recursively retrieves info about dir _path
    void getDirProps(const std::string& _path, uint32_t& dirCount, uint32_t& fileCount, uint64_t& totalSize);

    bool fileExists(const std::string& _path);
    //Returns file size
    size_t fsize(const std::string& _f);
    bool isDir(const std::string& _path);

    std::string getWorkDir();
    void setWorkDir(const std::string& _w);

    class dirItem
    {
        public:
            dirItem(const std::string& pathTo, const std::string& sItem);
            std::string getItm() const { return itm; }
            std::string getName() const;
            std::string getExt() const;
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
            std::string getItemExt(int index) const { return item[index].getExt(); }
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

    //Structs to send data to threads
    typedef struct
    {
        ui::menu *m;
        fs::dirList *d;
    } backupArgs;

    typedef struct
    {
        std::string to, from, dev;
        zipFile z;
        unzFile unz;
        bool cleanup = false;
        uint64_t offset = 0;
        ui::progBar *prog;
        threadStatus *thrdStatus;
        Mutex arglck = 0;
        void argLock() { mutexLock(&arglck); }
        void argUnlock() { mutexUnlock(&arglck); }
    } copyArgs;

    typedef struct
    {
        FsSaveDataType type;
        uint64_t tid;
        AccountUid account;
        uint16_t index;
    } svCreateArgs;

    copyArgs *copyArgsCreate(const std::string& from, const std::string& to, const std::string& dev, zipFile z, unzFile unz, bool _cleanup);
    void copyArgsDestroy(copyArgs *c);

    //Take a pointer to backupArgs^
    void createNewBackup(void *a);
    void overwriteBackup(void *a);
    void restoreBackup(void *a);
    void deleteBackup(void *a);

    void logOpen();
    void logWrite(const char *fmt, ...);
    void logClose();
}
