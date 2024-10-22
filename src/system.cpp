#include "system.h"

#include <intrin.h>

#define X(item) bool item : 1;
static struct {
    bool initialized : 1;
    CPU_FEATURE_LIST
} system_supported_feature_bits;
#undef X

namespace ae {

void system_init() {
    if(system_supported_feature_bits.initialized) {
        return;
    }

    int cpu_info[4];
    __cpuid(cpu_info, 1);

    system_supported_feature_bits.sse2 = cpu_info[3] & (1 << 26);
    system_supported_feature_bits.rdrand = cpu_info[2] & (1 << 30);

    system_supported_feature_bits.initialized = true;
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
