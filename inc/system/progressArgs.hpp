#pragma once
#include <cstdint>
#include "system/taskArgs.hpp"

namespace sys
{
    // Just so we have some way of keeping track of progress for these tasks
    struct progressArgs : taskArgs
    {
        uint64_t progress;
    };
}