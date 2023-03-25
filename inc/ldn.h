#pragma once

#include <switch.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <lz4.h>
#include <lz4frame.h>
#include <lz4frame_static.h>
#include <mutex>

#include "type.h"

#define SAVE_DATA_SERVER_PORT 25789
#define SOCKET_BUFFER_SIZE 4096
#define LENGTH_OF_LISTEN_QUEUE 20

enum LDN_COMMUNICATE_TYPE {
    UPDATE_FILE,
    UPDATE_ABORT,
    UPDATE_OK,
    UPDATE_DONE,
};

struct LZ4_readFile_s {
  LZ4F_dctx *dctxPtr;
  int socket;
  LZ4_byte *srcBuf;
  size_t srcBufNext;
  size_t srcBufSize;
  size_t srcBufMaxSize;
};

struct LZ4_writeFile_s {
  LZ4F_cctx *cctxPtr;
  int socket;
  LZ4_byte *dstBuf;
  size_t maxWriteSize;
  size_t dstBufMaxSize;
  LZ4F_errorCode_t errCode;
};

namespace LDN {
    typedef struct {
        // point to server's socket fd;
        int serverFD;
        // point to communicate socket fd, send meta data
        int commFD;
        // for client to bind with server, communicate create file socket fd
        struct sockaddr_in serverAddr;
    } LDNCommunicate;

    typedef struct {
        LDNCommunicate *comm;
        unsigned int filesize = 0, writeLimit = 0;
        std::string fullPath;
        std::mutex bufferLock;
        std::condition_variable cond;
        std::vector<uint8_t> sharedBuffer;
        bool bufferIsFull = false;
        std::string dst, dev;
        LZ4_writeFile_s* lz4fWrite;
    } LDNfcopyArgs;

    typedef struct {
        u32 type;
        size_t fsz;
    } commMeta;

    void destroyLDN();

    LDN::LDNCommunicate *createCommunicate(void);

    void destroyCommunicate(LDNCommunicate *comm);

    Result createLDNServer(LDNCommunicate *comm);

    Result createLDNClient(LDNCommunicate *comm);

    int bindClient(int serverFD);

    int bindServer(sockaddr_in *serverAddr);

    bool waitForOK(int socketfd);
    bool waitForDONE(int socketfd);
    void sendOK(int socket_fd);
    void sendDONE(int socket_fd);
    void sendAbort(int socket_fd);
    void reciveMeta(commMeta *meta, int socketfd);
    void sendMeta(commMeta *meta, int socketfd);
    void copySaveFileToRemote(const std::string &local, threadInfo *t);
    void copyRemoteSaveFile(threadInfo *t);
};