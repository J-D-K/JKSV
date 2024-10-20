#pragma once

#include <string>
#include "type.h"

namespace fs
{
    void mkDir(const std::string& _p);
    void mkDirRec(const std::string& _p);
    void delDir(const std::string& _p);
    bool dirNotEmpty(const std::string& _dir);
    bool isDir(const std::string& _path);

    //threadInfo is optional. Only for updating task status.
    void copyDirToDir(const std::string& src, const std::string& dst, threadInfo *t);
    void copyDirToDirThreaded(const std::string& src, const std::string& dst);
    void copyDirToDirCommit(const std::string& src, const std::string& dst, const std::string& dev, threadInfo *t);
    void copyDirToDirCommitThreaded(const std::string& src, const std::string& dst, const std::string& dev);
    void getDirProps(const std::string& path, unsigned& dirCount, unsigned& fileCount, uint64_t& totalSize);

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
            dirList(const std::string& _path, bool ignoreDotFiles = false);
            void reassign(const std::string& _path);
            void rescan();

            std::string getItem(int index) const { return item[index].getItm(); }
            std::string getItemExt(int index) const { return item[index].getExt(); }
            bool isDir(int index) const { return item[index].isDir(); }
            unsigned getCount() const { return item.size(); }
            fs::dirItem *getDirItemAt(unsigned int _ind) { return &item[_ind]; }

        private:
            std::string path;
            std::vector<dirItem> item;
    };
}
