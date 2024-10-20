#include "shapes.h"

#include "ray.h"

#include <cmath>

namespace ae {

sphere::sphere(const ae::vec4f &center, f32 radius)
    : center_(center)
    , radius_(std::fabs(radius)) {}

bool sphere::intersects(const ae::ray &/*ray*/, ray_hit_info &/*out_hit_info*/) const {
    return false;
}

}
