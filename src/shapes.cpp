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

#if 0
struct alignas(16) rayvec {
    f32 ox_[4];
    f32 oy_[4];
    f32 oz_[4];
    f32 dx_[4];
    f32 dy_[4];
    f32 dz_[4];
};

u32 sphere::intersects(const ae::ray (&rays)[4], ae::ray_hit_info (&out_hit_infos)[4]) const {
    // TODO: Just a test. This is quite inefficient
    rayvec rayv = {
        .ox_ = { rays[0].origin().x_, rays[1].origin().x_, rays[2].origin().x_, rays[3].origin().x_ },
        .oy_ = { rays[0].origin().y_, rays[1].origin().y_, rays[2].origin().y_, rays[3].origin().y_ },
        .oz_ = { rays[0].origin().z_, rays[1].origin().z_, rays[2].origin().z_, rays[3].origin().z_ },
        .dx_ = { rays[0].direction().x_, rays[1].direction().x_, rays[2].direction().x_, rays[3].direction().x_ },
        .dy_ = { rays[0].direction().y_, rays[1].direction().y_, rays[2].direction().y_, rays[3].direction().y_ },
        .dz_ = { rays[0].direction().z_, rays[1].direction().z_, rays[2].direction().z_, rays[3].direction().z_ }
    };

    __m128 ox = _mm_load_ps(&rayv.ox_[0]);
    __m128 oy = _mm_load_ps(&rayv.oy_[0]);
    __m128 oz = _mm_load_ps(&rayv.oz_[0]);
    __m128 dx = _mm_load_ps(&rayv.dx_[0]);
    __m128 dy = _mm_load_ps(&rayv.dy_[0]);
    __m128 dz = _mm_load_ps(&rayv.dz_[0]);

    __m128 r2 = _mm_set_ps1(radius_ * radius_);

    __m128 a = _mm_add_ps(_mm_add_ps(_mm_mul_ps(dx, dx), _mm_mul_ps(dy, dy)), _mm_mul_ps(dz, dz));
    __m128 b = _mm_mul_ps(_mm_set_ps1(-2.0f),
                          _mm_add_ps(_mm_add_ps(_mm_mul_ps(ox, dx), _mm_mul_ps(oy, dy)), _mm_mul_ps(oz, dz)));
    __m128 c = _mm_sub_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(ox, ox), _mm_mul_ps(oy, oy)), _mm_mul_ps(oz, oz)),
                          r2);

    __m128 discriminant = _mm_sub_ps(_mm_mul_ps(b, b),
                                     _mm_mul_ps(_mm_mul_ps(_mm_set_ps1(4.0f), a), c));

    // TODO: Continue
    (void)discriminant;
    (void)out_hit_infos;
    return 0;
}
#endif

}
