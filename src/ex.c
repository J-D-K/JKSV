#include <switch.h>

#include "ex.h"

Result fsOpenDataFileSystemByCurrentProcess(FsFileSystem *out)
{
    return serviceDispatch(fsGetServiceSession(), 2, 0, .out_num_objects = 1, .out_objects = &out->s);
}
