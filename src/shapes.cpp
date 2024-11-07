#include "shapes.h"

#include "ray.h"

#include <cmath>
#include <emmintrin.h>

namespace ae {

sphere::sphere(const ae::vec4f &center, f32 radius)
    : center_(center)
    , radius_(std::fabs(radius)) {}

bool sphere::intersects(const ae::ray &ray, ae::ray_hit_info &out_hit_info) const {
    const ae::vec4f oc = center_ - ray.origin();
    const f32 a = ray.direction().dot3(ray.direction());
    const f32 b = -2.0f * ray.direction().dot3(oc);
    const f32 c = oc.dot3(oc) - radius_ * radius_;

    const f32 discriminant = b * b - 4 * a * c;

    if(discriminant >= 0) {
        out_hit_info.t_ = (-b - std::sqrtf(discriminant)) / (2.0f * a);
        out_hit_info.point_ = ray.get_point(out_hit_info.t_);
        out_hit_info.normal_ = (out_hit_info.point_ - center_).get_normalized();
        return true;
    }

    return false;
}

}
