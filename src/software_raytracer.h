#pragma once

#include "raytracer.h"
#include "vec.h"

#include <queue>

namespace ae {
    class sphere;

    struct tile_data {
        u32 row;
        u32 col;
        u32 pixels[ae::raytracer::tile_size * ae::raytracer::tile_size];
    };

    class software_raytracer final : public raytracer {
    public:
        software_raytracer() = default;
        software_raytracer(u32 *buffer);

        bool setup() override;
        void trace() override;

    private:
        void trace_tile(tile_data &tile);
        bool get_next_tile(tile_data &tile);
        static void * thread_func(void *data);

        std::queue<tile_data> tile_queue_;

        u32 *framebuffer_ = nullptr;

        ae::vec4f viewport_size_;
        ae::vec4f pixel_size_;

        u32 width_ = 0;
        u32 height_ = 0;
        u32 row_count_ = 0;
        u32 col_count_ = 0;
        u32 current_row_ = 0;
        u32 current_col_ = 0;

        bool finished_ : 1 = false;
    };
}
