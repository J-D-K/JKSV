#include "ldn.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <unistd.h>

#include "fs.h"
#include "ui.h"
#include "file.h"
#include "util.h"

static const u8 sec_data[0x10] = {0x04, 0xb9, 0x9d, 0x4d, 0x58, 0xbc,
                                  0x65, 0xe1, 0x77, 0x13, 0xc2, 0xb8,
                                  0xd1, 0xb8, 0xec, 0xf6};

Result create_network(const LdnSecurityConfig *sec_config,
                      const LdnUserConfig *user_config,
                      const LdnNetworkConfig *netconfig, const void *advert,
                      size_t advert_size) {
  Result rc = 0;

  rc = ldnOpenAccessPoint();
  if (R_FAILED(rc)) {
    return rc;
  }

  rc = ldnCreateNetwork(sec_config, user_config, netconfig);
  if (R_FAILED(rc)) {
    goto error_close;
  }

  rc = ldnSetAdvertiseData(advert, advert_size);
  if (R_FAILED(rc))
    goto error_close;

  return rc;
error_close:
  ldnCloseAccessPoint();
  return rc;
}

Result connect_network(const LdnScanFilter *filter,
                       const LdnSecurityConfig *sec_config,
                       const LdnUserConfig *user_config, const void *advert,
                       size_t advert_size) {
  Result rc = 0;
  s32 total_out = 0;
  LdnNetworkInfo netinfo_list[0x18];

  rc = ldnOpenStation();

  if (R_SUCCEEDED(rc)) {
    rc = ldnScan(0, filter, netinfo_list, 0x18, &total_out);
  }

  // In an actual app you'd display the output netinfo_list and let the user
  // select which one to connect to, however in this example we'll just
  // connect to the first one.

  if (R_SUCCEEDED(rc) && !total_out) {
    rc = MAKERESULT(Module_Libnx, LibnxError_NotFound);
  }

  if (R_SUCCEEDED(rc)) { // Handle this / parse it with any method you want.
    if (netinfo_list[0].advertise_data_size != advert_size ||
        memcmp(netinfo_list[0].advertise_data, advert, advert_size) != 0) {
      rc = MAKERESULT(Module_Libnx, LibnxError_NotFound);
    }
  }

  if (R_SUCCEEDED(rc)) {
    rc = ldnConnect(sec_config, user_config, 0, 0, &netinfo_list[0]);
  }

  if (R_FAILED(rc))
    ldnCloseStation();

  return rc;
}

static void leave_network(void) {
  Result rc = 0;
  LdnState state;

  rc = ldnGetState(&state);
  if (R_SUCCEEDED(rc)) {
    if (state == LdnState_AccessPointOpened ||
        state == LdnState_AccessPointCreated) {
      if (state == LdnState_AccessPointCreated) {
        rc = ldnDestroyNetwork();
      }
      rc = ldnCloseAccessPoint();
    }

    if (state == LdnState_StationOpened || state == LdnState_StationConnected) {
      if (state == LdnState_StationConnected) {
        rc = ldnDisconnect();
      }
      rc = ldnCloseStation();
    }
  }
}

void LDN::destroyLDN() {
  leave_network();
  ldnExit();
}

LDN::LDNCommunicate* LDN::createCommunicate(void)
{
  LDN::LDNCommunicate* comm = new LDN::LDNCommunicate;
  comm->serverFD = -1;
  comm->commFD = -1;
  return comm;
}

void LDN::destroyCommunicate(LDNCommunicate *comm) {
  if (comm->commFD >= 0)
    close(comm->commFD);
  if (comm->serverFD >= 0)
    close(comm->serverFD);
  delete comm;
}

Result LDN::createLDNServer(LDNCommunicate *comm) {
  Result rc;
  LdnSecurityConfig sec_config = {0};
  LdnUserConfig user_config = {0};
  LdnNetworkConfig netconfig = {0};
  LdnState state;
  data::user *user = data::getCurrentUser();
  data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
  /*
   * Use Title ID to the advert, so if advert not match
   * LDN connect between client and server will bind fail
   */
  std::string advertStr = std::to_string(utinfo->tid);
  int serverSocket;

  // send nickname to make sure
  strncpy(user_config.nickname, user->getUsername().c_str(), 0x20 - 1);

  netconfig.local_communication_id = -1;
  netconfig.participant_max = 2;      // Adjust as needed.

  sec_config.type = 1;
  sec_config.data_size = sizeof(sec_data);
  memcpy(sec_config.data, sec_data, sizeof(sec_data));

  rc = ldnInitialize(LdnServiceType_User);
  if (R_FAILED(rc)) {
    goto out;
  }

  rc = ldnGetState(&state);
  if (!R_SUCCEEDED(rc) || state != LdnState_Initialized) {
    goto ldn_out;
  }

  rc = create_network(&sec_config, &user_config, &netconfig, advertStr.c_str(),
                      advertStr.length());
  if (R_FAILED(rc)) {
    goto ldn_out;
  }

  return rc;

ldn_out:
  ldnExit();
out:
  return rc;
}

Result LDN::createLDNClient(LDNCommunicate *comm) {
  Result rc;
  LdnUserConfig user_config = {0};
  LdnSecurityConfig sec_config = {0};
  LdnNetworkConfig netconfig = {0};
  LdnScanFilter filter = {0};
  LdnState state;
  data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
  /*
   * Use Title ID to the advert, so if advert not match
   * LDN connect between client and server will bind fail
   */
  std::string advertStr = std::to_string(utinfo->tid);
  data::user *user = data::getCurrentUser();

  strncpy(user_config.nickname, user->getUsername().c_str(), 0x20 - 1);

  rc = ldnInitialize(LdnServiceType_User);
  if (R_FAILED(rc)) {
    goto out;
  }

  netconfig.local_communication_id = -1;
  netconfig.participant_max = 2;      // Adjust as needed.

  sec_config.type = 1;
  sec_config.data_size = sizeof(sec_data);
  memcpy(sec_config.data, sec_data, sizeof(sec_data));

  filter.local_communication_id = -1;
  filter.userdata_filter = netconfig.userdata_filter;
  filter.flags =
      LdnScanFilterFlags_LocalCommunicationId | LdnScanFilterFlags_UserData;

  rc = ldnGetState(&state);
  if (!R_SUCCEEDED(rc) || state != LdnState_Initialized) {
    goto ldnOut;
  }

  rc = connect_network(&filter, &sec_config, &user_config, advertStr.c_str(),
                       advertStr.length());

out:
  return rc;
ldnOut:
  ldnExit();
  return rc;
}

int LDN::bindClient(int serverFD) {
  struct sockaddr_in clientAddr;
  socklen_t length = sizeof(clientAddr);
  int cSocket =
      accept(serverFD, (struct sockaddr *)&clientAddr, &length);
  if (cSocket < 0) {
    return -1;
  }

  return cSocket;
}

int LDN::bindServer(struct sockaddr_in *serverAddr) {
  /// sockfd
  int sock_cli = socket(AF_INET, SOCK_STREAM, 0);
  int ret;
  if ((ret = connect(sock_cli, (struct sockaddr *)serverAddr,
                     sizeof(*serverAddr))) < 0) {
    close(sock_cli);
    return -1;
  }

  return sock_cli;
}

static void reciveBuf(char *buf, ssize_t size, int socketfd) {
  ssize_t offset = 0, ssize;
  while (offset < size) {
    ssize =
        size - offset < SOCKET_BUFFER_SIZE ? size - offset : SOCKET_BUFFER_SIZE;
    ssize_t done = recv(socketfd, buf + offset, ssize, 0);
    if (done == -1) {
      break;
    }
    offset += done;
  }
}

static void sendBuf(const char *buf, ssize_t size, int socketfd) {
  ssize_t offset = 0, ssize;
  while (offset < size) {
    ssize =
        size - offset < SOCKET_BUFFER_SIZE ? size - offset : SOCKET_BUFFER_SIZE;
    ssize_t done = send(socketfd, buf + offset, ssize, 0);
    if (done == -1) {
      break;
    }
    offset += done;
  }
}

bool LDN::waitForOK(int socketfd) {
  commMeta meta;
  reciveBuf((char *)&meta, sizeof(meta), socketfd);
  if (meta.type == UPDATE_OK)
    return true;
  return false;
}

bool LDN::waitForDONE(int socketfd) {
  commMeta meta;
  reciveBuf((char *)&meta, sizeof(meta), socketfd);
  if (meta.type == UPDATE_DONE)
    return true;
  return false;
}

void LDN::sendOK(int socket_fd) {
  commMeta meta;
  meta.type = UPDATE_OK;

  sendBuf((char *)&meta, sizeof(meta), socket_fd);
}

void LDN::sendDONE(int socket_fd) {
  commMeta meta;
  meta.type = UPDATE_DONE;

  sendBuf((char *)&meta, sizeof(meta), socket_fd);
}

void LDN::sendAbort(int socket_fd) {
  commMeta meta;
  meta.type = UPDATE_ABORT;

  sendBuf((char *)&meta, sizeof(meta), socket_fd);
}

void LDN::reciveMeta(commMeta *meta, int socketfd) {
  bzero(meta, sizeof(commMeta));
  reciveBuf((char *)meta, sizeof(commMeta), socketfd);
  sendOK(socketfd);
}

void LDN::sendMeta(commMeta *meta, int socketfd) {
  sendBuf((char *)meta, sizeof(commMeta), socketfd);
  waitForOK(socketfd);
}

static void copySaveFileToRemote_t(void *a) {
  LDN::LDNfcopyArgs *in = (LDN::LDNfcopyArgs *)a;
  LDN::LDNCommunicate *comm = in->comm;
  size_t written = 0;
  std::vector<uint8_t> localBuffer;

  int fSocket = LDN::bindClient(comm->serverFD);
  if (fSocket < 0)
    return;

  while (written < in->filesize) {
    std::unique_lock<std::mutex> buffLock(in->bufferLock);
    in->cond.wait(buffLock, [in] { return in->bufferIsFull; });
    localBuffer.clear();
    localBuffer.assign(in->sharedBuffer.begin(), in->sharedBuffer.end());
    in->sharedBuffer.clear();
    in->bufferIsFull = false;
    buffLock.unlock();
    in->cond.notify_one();

    written += send(fSocket, localBuffer.data(), localBuffer.size(), 0);
  }

  close(fSocket);
}

void LDN::copySaveFileToRemote(const std::string &local, threadInfo *t) {
  fs::copyArgs *c = NULL;
  c = (fs::copyArgs *)t->argPtr;
  LDN::LDNCommunicate *comm = c->comm;
  commMeta cm;
  std::string zipPath = fs::getWorkDir() + "tempSave.zip";

  //Trim this to unzip direct in direct sv:/
  int zipTrim = util::getTotalPlacesInPath("sv:/");

  t->status->setStatus(ui::getUICString("LDNStatus", 7));


  zipFile zip = zipOpen64(zipPath.c_str(), 0);
  fs::copyDirToZip(local, zip, true, zipTrim, t);
  zipClose(zip, NULL);


  size_t filesize = fs::fsize(zipPath);
  t->status->setStatus(ui::getUICString("LDNStatus", 8));
  c->offset = 0;
  c->prog->setMax(filesize);
  c->prog->update(0);

  FILE *fsrc = fopen(zipPath.c_str(), "rb");
  if(!fsrc)
  {
    sendAbort(comm->commFD);
    fclose(fsrc);
    return;
  }

  cm.type = UPDATE_FILE;
  cm.fsz = filesize;
  sendMeta(&cm, comm->commFD);

  LDNfcopyArgs thrdArgs;
  thrdArgs.comm = comm;
  thrdArgs.filesize = filesize;

  uint8_t *buff = new uint8_t[BUFF_SIZE];
  std::vector<uint8_t> transferBuffer;
  Thread writeThread;
  threadCreate(&writeThread, copySaveFileToRemote_t, &thrdArgs, NULL,
               0x40000, 0x2E, 2);
  threadStart(&writeThread);
  size_t readIn = 0;
  uint64_t readCount = 0;

  while ((readIn = fread(buff, 1, BUFF_SIZE, fsrc)) > 0) {
    transferBuffer.insert(transferBuffer.end(), buff, buff + readIn);
    readCount += readIn;

    if (c)
      c->offset = readCount;

    if (transferBuffer.size() >= TRANSFER_BUFFER_LIMIT ||
        readCount == filesize) {
      std::unique_lock<std::mutex> buffLock(thrdArgs.bufferLock);
      thrdArgs.cond.wait(
          buffLock, [&thrdArgs] { return thrdArgs.bufferIsFull == false; });
      thrdArgs.sharedBuffer.assign(transferBuffer.begin(),
                                   transferBuffer.end());
      transferBuffer.clear();
      thrdArgs.bufferIsFull = true;
      buffLock.unlock();
      thrdArgs.cond.notify_one();
    }
  }
  threadWaitForExit(&writeThread);
  threadClose(&writeThread);
  fclose(fsrc);
  fs::delfile(zipPath);
  delete[] buff;

  t->status->setStatus(ui::getUICString("LDNStatus", 11));
  // wait for client recived sure
  // otherwise, may lost package
  LDN::sendDONE(comm->commFD);
  LDN::waitForDONE(comm->commFD);
}

static void copyRemoteFileWrite_t(void *a) {
  LDN::LDNfcopyArgs *in = (LDN::LDNfcopyArgs *)a;
  size_t written = 0;
  std::vector<uint8_t> localBuffer;
  FILE *out = fopen(in->dst.c_str(), "wb");

  while (written < in->filesize) {
    std::unique_lock<std::mutex> buffLock(in->bufferLock);
    in->cond.wait(buffLock, [in] { return in->bufferIsFull; });
    localBuffer.clear();
    localBuffer.assign(in->sharedBuffer.begin(), in->sharedBuffer.end());
    in->sharedBuffer.clear();
    in->bufferIsFull = false;
    buffLock.unlock();
    in->cond.notify_one();
    written += fwrite(localBuffer.data(), 1, localBuffer.size(), out);
  }
  fclose(out);
}

static void copyRemoteSaveFileCommit(LDN::commMeta *meta, threadInfo *t) {
  fs::copyArgs *c = (fs::copyArgs *)t->argPtr;
  LDN::LDNCommunicate *comm = c->comm;

  std::string fullPath = fs::getWorkDir() + "tempSave.zip";
  size_t filesize = meta->fsz;

  c->offset = 0;
  c->prog->setMax(filesize);
  c->prog->update(0);

  int fSocket = LDN::bindServer(&comm->serverAddr);
  if (fSocket < 0)
    return;

  t->status->setStatus(ui::getUICString("LDNStatus", 9));
  LDN::LDNfcopyArgs thrdArgs;
  thrdArgs.dst = fullPath;
  thrdArgs.filesize = filesize;

  uint8_t *buff = new uint8_t[BUFF_SIZE];
  std::vector<uint8_t> transferBuffer;
  Thread writeThread;
  threadCreate(&writeThread, copyRemoteFileWrite_t, &thrdArgs, NULL, 0x40000,
               0x2E, 2);
  threadStart(&writeThread);
  size_t readIn = 0;
  uint64_t readCount = 0;
  while ((readIn = recv(fSocket, buff, BUFF_SIZE, 0)) > 0) {
    transferBuffer.insert(transferBuffer.end(), buff, buff + readIn);
    readCount += readIn;

    if (c)
      c->offset = readCount;

    if (transferBuffer.size() >= TRANSFER_BUFFER_LIMIT ||
        readCount == filesize) {
      std::unique_lock<std::mutex> buffLock(thrdArgs.bufferLock);
      thrdArgs.cond.wait(
          buffLock, [&thrdArgs] { return thrdArgs.bufferIsFull == false; });
      thrdArgs.sharedBuffer.assign(transferBuffer.begin(),
                                   transferBuffer.end());
      transferBuffer.clear();
      thrdArgs.bufferIsFull = true;
      buffLock.unlock();
      thrdArgs.cond.notify_one();
    }
  }
  threadWaitForExit(&writeThread);
  threadClose(&writeThread);

  t->status->setStatus(ui::getUICString("LDNStatus", 10));

  unzFile unz = unzOpen64(fullPath.c_str());
  if(unz && fs::zipNotEmpty(unz)) {
    t->status->setStatus(
        ui::getUICString("threadStatusCalculatingSaveSize", 0));
    uint64_t saveSize = fs::getZipTotalSize(unz);
    int64_t availSize = 0;
    fsFsGetTotalSpace(fsdevGetDeviceFileSystem("sv"), "/", &availSize);
    if ((int)saveSize > availSize) {
      data::userTitleInfo *utinfo = data::getCurrentUserTitleInfo();
      fs::unmountSave();
      fs::extendSaveData(utinfo, saveSize + 0x500000, t);
      fs::mountSave(utinfo->saveInfo);
    }

    fs::delDir("sv:/");
    fs::commitToDevice("sv");
    fs::copyZipToDir(unz, "sv:/", "sv", t);
  }

  if (unz)
    unzClose(unz);
  close(fSocket);

  delete[] buff;
  fs::delfile(fullPath);

  LDN::waitForDONE(comm->commFD);
  LDN::sendDONE(comm->commFD);
}

void LDN::copyRemoteSaveFile(threadInfo *t) {
  fs::copyArgs *c = (fs::copyArgs *)t->argPtr;
  LDN::LDNCommunicate *comm = (LDN::LDNCommunicate *)c->comm;
  LDN::commMeta meta;

  t->status->setStatus(ui::getUICString("LDNStatus", 12));
  reciveMeta(&meta, comm->commFD);
  if (meta.type == UPDATE_ABORT)
    return;

  if (meta.type == UPDATE_FILE) {
    copyRemoteSaveFileCommit(&meta, t);
  }
}