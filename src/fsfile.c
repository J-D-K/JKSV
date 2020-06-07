#include <switch.h>
#include <string.h>
#include <stdint.h>
#include <malloc.h>

#include "fsfile.h"

char *getDeviceFromPath(char *dev, size_t _max, const char *path)
{
    memset(dev, 0, _max);
    char *c = strchr(path, ':');
    if(c - path > _max)
        return NULL;

    //probably not good? idk
    memcpy(dev, path, c - path);

    return dev;
}

char *getFilePath(char *pathOut, size_t _max, const char *path)
{
    memset(pathOut, 0, _max);
    char *c = strchr(path, '/');
    size_t pLength = strlen(c);
    if(pLength > _max)
        return NULL;

    memcpy(pathOut, c, pLength);

    return pathOut;
}

int fsremove(const char *_p)
{
    char devStr[16];
    char path[FS_MAX_PATH];
    if(!getDeviceFromPath(devStr, 16, _p) || !getFilePath(path, FS_MAX_PATH, _p))
        return -1;

    Result res = fsFsDeleteFile(fsdevGetDeviceFileSystem(devStr), path);
    return res;
}

Result fsDelDirRec(const char *_p)
{
    char devStr[16];
    char path[FS_MAX_PATH];
    if(!getDeviceFromPath(devStr, 16, _p) || ! getFilePath(path, FS_MAX_PATH, _p))
        return 1;

    return fsFsDeleteDirectoryRecursively(fsdevGetDeviceFileSystem(devStr), path);
}

FSFILE *fsfopen(const char *_p, uint32_t mode)
{
    char devStr[16];
    char filePath[FS_MAX_PATH];
    if(!getDeviceFromPath(devStr, 16, _p) || !getFilePath(filePath, FS_MAX_PATH, _p))
        return NULL;

    FsFileSystem *s = fsdevGetDeviceFileSystem(devStr);
    if(s == NULL)
        return NULL;

    if(mode & FsOpenMode_Write)
    {
        fsFsDeleteFile(s, filePath);
        fsFsCreateFile(s, filePath, 0, 0);
    }
    else if(mode & FsOpenMode_Append)
        fsFsCreateFile(s, filePath, 0, 0);

    FSFILE *ret = malloc(sizeof(FSFILE));
    ret->error = fsFsOpenFile(s, filePath, mode, &ret->_f);
    if(R_FAILED(ret->error))
    {
        free(ret);
        return NULL;
    }
    fsFileGetSize(&ret->_f, &ret->fsize);
    ret->offset = (mode & FsOpenMode_Append) ?  ret->fsize : 0;

    return ret;
}

FSFILE *fsfopenWithSystem(FsFileSystem *_s, const char *_p, uint32_t mode)
{
    if(mode & FsOpenMode_Write)
    {
        fsFsDeleteFile(_s, _p);
        fsFsCreateFile(_s, _p, 0, 0);
    }
    else if(mode & FsOpenMode_Append)
        fsFsCreateFile(_s, _p, 0, 0);

    FSFILE *ret = malloc(sizeof(FSFILE));
    ret->error = fsFsOpenFile(_s, _p, mode, &ret->_f);
    if(R_FAILED(ret->error))
    {
        free(ret);
        return NULL;
    }
    fsFileGetSize(&ret->_f, &ret->fsize);
    ret->offset = (mode & FsOpenMode_Append) ?  ret->fsize : 0;

    return ret;
}

size_t fsfwrite(const void *buf, size_t sz, size_t count, FSFILE *_f)
{
    size_t fullSize = sz * count;
    if(_f->offset + fullSize > _f->fsize)
    {
        s64 newSize = (_f->fsize + fullSize) - (_f->fsize - _f->offset);
        fsFileSetSize(&_f->_f, newSize);
        _f->fsize = newSize;
    }
    _f->error = fsFileWrite(&_f->_f, _f->offset, buf, fullSize, FsWriteOption_Flush);
    _f->offset += fullSize;

    return fullSize;
}
