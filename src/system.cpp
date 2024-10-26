#include "system.h"

#ifdef AE_PLATFORM_WIN32
#include "common_win32.h"
#elif defined(AE_PLATFORM_LINUX)
#include "common_linux.h"
#endif

#define X(item) bool item : 1;
static struct {
    CPU_FEATURE_LIST
} system_supported_feature_bits;
#undef X

namespace ae {

void system_init() {
    static volatile bool initialized = false;

    if(initialized) {
        return;
    }

    const int leaf = 1;
    int cpu_info[4];

#ifdef AE_PLATFORM_WIN32
    __cpuid(cpu_info, leaf);
#elif defined(AE_PLATFORM_LINUX)
    asm volatile(
        "cpuid;"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "0"(leaf)
    );
#endif

    system_supported_feature_bits.sse2 = cpu_info[3] & (1 << 26);
    system_supported_feature_bits.rdrand = cpu_info[2] & (1 << 30);

    initialized = true;
}

bool system_has_feature(cpu_feature feature) {
#define X(item) case cpu_feature::item: \
    return system_supported_feature_bits.item;

    switch(feature) {
        CPU_FEATURE_LIST
        default:
            break;
    }

#undef X

    return false;
}

}
