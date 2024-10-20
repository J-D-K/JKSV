#include "rfs.h"

std::vector<uint8_t> rfs::downloadBuffer;

void rfs::writeThread_t(void *a)
{
    rfs::dlWriteThreadStruct *in = (rfs::dlWriteThreadStruct *)a;
    std::vector<uint8_t> localBuff;
    unsigned written = 0;

    FILE *out = fopen(in->cfa->path.c_str(), "wb");

    while(written < in->cfa->size)
    {
        std::unique_lock<std::mutex> dataLock(in->dataLock);
        in->cond.wait(dataLock, [in]{ return in->bufferFull; });
        localBuff.clear();
        localBuff.assign(in->sharedBuffer.begin(), in->sharedBuffer.end());
        in->sharedBuffer.clear();
        in->bufferFull = false;
        dataLock.unlock();
        in->cond.notify_one();

        written += fwrite(localBuff.data(), 1, localBuff.size(), out);
    }
    fclose(out);
    rfs::downloadBuffer.clear();
}

size_t rfs::writeDataBufferThreaded(uint8_t *buff, size_t sz, size_t cnt, void *u)
{
    rfs::dlWriteThreadStruct *in = (rfs::dlWriteThreadStruct *)u;
    rfs::downloadBuffer.insert(rfs::downloadBuffer.end(), buff, buff + (sz * cnt));
    in->downloaded += sz * cnt;

    if(in->downloaded == in->cfa->size || rfs::downloadBuffer.size() == DOWNLOAD_BUFFER_SIZE)
    {
        std::unique_lock<std::mutex> dataLock(in->dataLock);
        in->cond.wait(dataLock, [in]{ return in->bufferFull == false; });
        in->sharedBuffer.assign(rfs::downloadBuffer.begin(), rfs::downloadBuffer.end());
        rfs::downloadBuffer.clear();
        in->bufferFull = true;
        dataLock.unlock();
        in->cond.notify_one();
    }

    if(in->cfa->o)
        *in->cfa->o = in->downloaded;

    return sz * cnt;
}
