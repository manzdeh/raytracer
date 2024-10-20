#pragma once

#include "common.h"
#include "vec.h"

#include <limits>

namespace ae {
    struct ray_hit_info {
        ae::vec4f point_;
        ae::vec4f normal_;
        f32 t_ = std::numeric_limits<f32>::max();
    };

    class ray {
    public:
        ray() = default;
        ray(const ae::vec4f &origin, const ae::vec4f &dir)
            : origin_(origin)
            , dir_(dir.get_normalized()) {}

        const ae::vec4f & origin() const { return origin_; }
        const ae::vec4f & direction() const { return dir_; }

        ae::vec4f get_point(f32 t) const { return origin_ + (dir_ * t); }

    private:
        ae::vec4f origin_;
        ae::vec4f dir_;
    };
}
