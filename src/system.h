#pragma once

#include "common.h"

#define CPU_FEATURE_LIST \
    X(sse2) \
    X(rdrand)

namespace ae {
#define X(item) item,
    enum class cpu_feature : u32 {
        CPU_FEATURE_LIST
    };
#undef X

    void system_init();
    bool system_has_feature(cpu_feature feature);
}
