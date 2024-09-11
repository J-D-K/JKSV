#pragma once
#include <memory>

namespace sys
{
    // This is just for inheritance to pass stuff to task threads
    struct taskData { }; 
    // Makes stuff easier to type
    using sharedTaskData = std::shared_ptr<sys::taskData>;
}
