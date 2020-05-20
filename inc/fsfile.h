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
void fsfclose(FSFILE *_f);

//Seeks like stdio
void fsfseek(FSFILE *_f, int offset, int origin);

//Returns offset
size_t fsftell(FSFILE *_f);

//Writes buf to file. Automatically resizes _f to fit buf
size_t fsfwrite(const void *buf, size_t sz, size_t count, FSFILE *_f);

//Reads to buff
size_t fsfread(void *buf, size_t sz, size_t count, FSFILE *_f);

//Gets byte from file
char fsfgetc(FSFILE *_f);

//Writes byte to file
void fsfputc(int ch, FSFILE *_f);
#ifdef __cplusplus
}
#endif

#endif
