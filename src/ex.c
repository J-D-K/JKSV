#include <switch.h>

#include "ex.h"

Result fsOpenDataFileSystemByCurrentProcess(FsFileSystem *out)
{
    Result ret = 0;

    IpcCommand c;
    ipcInitialize(&c);
    struct
    {
        uint64_t mag;
        uint64_t cmd;
    } *raw = serviceIpcPrepareHeader(fsGetServiceSession(), &c, sizeof(*raw));
    raw->mag = SFCI_MAGIC;
    raw->cmd = 2;

    ret = serviceIpcDispatch(fsGetServiceSession());
    if(R_SUCCEEDED(ret))
    {
        IpcParsedCommand p;
        struct
        {
            uint64_t mag;
            uint64_t res;
        } *resp;

        serviceIpcParse(fsGetServiceSession(), &p, sizeof(*resp));
        resp = p.Raw;

        ret = resp->res;
        if(R_SUCCEEDED(ret))
            serviceCreateSubservice(&out->s, fsGetServiceSession(), &p, 0);
    }

    return ret;
}
