#include "shapes.h"

#include "ray.h"

#include <cmath>

namespace ae {

sphere::sphere(const ae::vec4f &center, f32 radius)
    : center_(center)
    , radius_(std::fabs(radius)) {}

bool sphere::intersects(const ae::ray &ray, ae::ray_hit_info &out_hit_info) const {
    const f32 t = (center_ - ray.origin()).dot(ray.direction());
    const ae::vec4f closest_point = ray.get_point(t);

    const f32 dist = (center_ - closest_point).magnitude();

    if(dist < radius_) {
        out_hit_info.point_ = ray.get_point(dist);
        out_hit_info.normal_ = (out_hit_info.point_ - center_).get_normalized();
        out_hit_info.t_ = dist;
        return true;
    }

    return false;
}

}
