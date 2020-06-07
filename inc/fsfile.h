#ifndef FSFILE_H
#define FSFILE_H

#include <switch.h>
#include <stdint.h>

//Bare minimum wrapper around switch fs for JKSV
#define FS_SEEK_SET 0
#define FS_SEEK_CUR 1
#define FS_SEEK_END 2

#ifdef __cplusplus
extern "C"
{
#endif
typedef struct
{
    FsFile _f;
    Result error;
    s64 offset, fsize;
} FSFILE;

int fsremove(const char *_p);
Result fsDelDirRec(const char *_p);

char *getDeviceFromPath(char *dev, size_t _max, const char *path);
char *getFilePath(char *pathOut, size_t _max, const char *path);

/*Opens file. Device is fetched from path. Libnx romfs doesn't work with this.
Mode needs to be:
    FsOpenMode_Read
    FsOpenMode_Write
    FsOpenMode_Append
*/
FSFILE *fsfopen(const char *_p, uint32_t mode);

/*Same as above, but FsFileSystem _s is used. Path cannot have device in it*/
FSFILE *fsfopenWithSystem(FsFileSystem *_s, const char *_p, uint32_t mode);

//Closes _f
inline void fsfclose(FSFILE *_f)
{
    if(_f != NULL)
    {
        fsFileClose(&_f->_f);
        free(_f);
    }
}

//Seeks like stdio
inline void fsfseek(FSFILE *_f, int offset, int origin)
{
    switch(origin)
    {
        case FS_SEEK_SET:
            _f->offset = offset;
            break;

        case FS_SEEK_CUR:
            _f->offset += offset;
            break;

        case FS_SEEK_END:
            _f->offset = offset + _f->fsize;
            break;
    }
}

//Returns offset
inline size_t fsftell(FSFILE *_f) { return _f->offset; }

//Writes buf to file. Automatically resizes _f to fit buf
size_t fsfwrite(const void *buf, size_t sz, size_t count, FSFILE *_f);

//Reads to buff
inline size_t fsfread(void *buf, size_t sz, size_t count, FSFILE *_f)
{
    uint64_t read = 0;
    _f->error = fsFileRead(&_f->_f, _f->offset, buf, sz * count, 0, &read);
    _f->offset += read;
    return read;
}

//Gets byte from file
inline char fsfgetc(FSFILE *_f)
{
    char ret = 0;
    uint64_t read = 0;
    _f->error = fsFileRead(&_f->_f, _f->offset++, &ret, 1, 0, &read);
    return ret;
}

//Writes byte to file
inline void fsfputc(int ch, FSFILE *_f) { fsfwrite(&ch, 1, 1, _f); }
#ifdef __cplusplus
}
#endif

#endif
