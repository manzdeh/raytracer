#pragma once

#include "color.h"
#include "common.h"
#include "shapes.h"

#include <span>
#include <utility>

namespace ae {
    class raytracer {
    public:
        static constexpr u32 tile_size = 4;

        // Test scene
        static ae::color background0;
        static ae::color background1;
        static ae::vec4f camera_pos;
        static ae::sphere sphere;

        static std::pair<u32, u32> get_resolution();

        virtual ~raytracer() = default;

        virtual bool setup() = 0;
        virtual void trace() = 0;

    protected:
        raytracer() = default;
        raytracer(u32 *buffer);

        u32 *framebuffer_ = nullptr;
    };

    static_assert(raytracer::tile_size > 0 && ((raytracer::tile_size & (raytracer::tile_size - 1)) == 0),
        "tile size needs to be a power of 2");
}
