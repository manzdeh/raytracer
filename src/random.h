#pragma once

#include "common.h"

namespace ae {
    class random {
    public:
        // A seed of 0 will assign a random seed
        random(u32 seed = 0);

        u32 next_u32();
        f32 next_f32();

    private:
        u32 generate_seed() const;

        u32 state_;
    };
}
