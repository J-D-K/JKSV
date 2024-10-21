#pragma once

#include "type.h"
#include <string>

namespace fs
{
    void mkDir(const std::string &_p);
    void mkDirRec(const std::string &_p);
    void delDir(const std::string &_p);
    bool dirNotEmpty(const std::string &_dir);
    bool isDir(const std::string &_path);

    //threadInfo is optional. Only for updating task status.
    void copyDirToDir(const std::string &src, const std::string &dst, threadInfo *t);
    void copyDirToDirThreaded(const std::string &src, const std::string &dst);
    void copyDirToDirCommit(const std::string &src, const std::string &dst, const std::string &dev, threadInfo *t);
    void copyDirToDirCommitThreaded(const std::string &src, const std::string &dst, const std::string &dev);
    void getDirProps(const std::string &path, unsigned &dirCount, unsigned &fileCount, uint64_t &totalSize);
} // namespace fs
