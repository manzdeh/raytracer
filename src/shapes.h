#pragma once

#include "common.h"
#include "vec.h"

namespace ae {
    struct ray_hit_info;
    class ray;

    class sphere {
    public:
        sphere() = default;
        sphere(const ae::vec4f &center, f32 radius);

        bool intersects(const ae::ray &ray, ae::ray_hit_info &out_hit_info) const;
        u32 intersects(const ae::ray (&rays)[4], ae::ray_hit_info (&out_hit_infos)[4]) const;

    private:
        ae::vec4f center_;
        f32 radius_ = 0.0f;
    };
}
