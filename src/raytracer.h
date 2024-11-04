#pragma once

#include "common.h"

#include <span>
#include <utility>

namespace ae {
    class raytracer {
    public:
        static constexpr u32 tile_size = 4;

        static std::pair<u32, u32> get_resolution();

        virtual ~raytracer() = default;

        virtual bool setup() = 0;
        virtual void trace() = 0;
        virtual std::span<u32> get_result() const = 0;

    protected:
        u32 width_ = 0;
        u32 height_ = 0;
    };

    static_assert(raytracer::tile_size > 0 && ((raytracer::tile_size & (raytracer::tile_size - 1)) == 0),
        "tile size needs to be a power of 2");
}
