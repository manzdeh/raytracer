#include "random.h"

#ifdef AE_PLATFORM_WIN32
#include "common_win32.h"
#endif
#include "system.h"

#include <intrin.h>
#include <limits>
#include <time.h>

namespace ae {

random::random(u32 seed)
    : state_((seed == 0) ? generate_seed() : seed) {}

u32 random::next_u32() {
    // 32-bit xorshift by George Marsaglia
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return state_;
}

f32 random::next_f32() {
    f64 next = static_cast<f64>(next_u32()) / static_cast<f64>(std::numeric_limits<u32>::max());
    return static_cast<f32>(next);
}

u32 random::generate_seed() const {
    u32 seed;

    if(ae::system_has_feature(ae::cpu_feature::rdrand) && _rdrand32_step(&seed)) {
        return seed;
    }

    return static_cast<u32>(time(nullptr))
        ^ 0x9e3779b9
#ifdef AE_PLATFORM_WIN32
        ^ static_cast<u32>(GetCurrentProcessId())
        ^ static_cast<u32>(GetCurrentThreadId())
#endif
        ;
}

}
