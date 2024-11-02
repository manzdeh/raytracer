#include "software_raytracer.h"

#include "color.h"
#include "math.h"
#include "ray.h"
#include "shapes.h"
#include "vec.h"

#include <utility>

namespace ae {

software_raytracer::software_raytracer(u32 *buffer)
    : framebuffer_(buffer) {
}

bool software_raytracer::setup() {
    auto [w, h] = raytracer::get_resolution();

    width_ = w;
    height_ = h;

    return true;
}

void software_raytracer::trace() {
    // Test scene setup
    const ae::color background0(1.0f, 1.0f, 1.0f);
    const ae::color background1(AE_RGB(0x4d, 0xa6, 0xf0));

    const ae::vec4f viewport_size(static_cast<f32>(width_) / static_cast<f32>(height_), 1.0f, 0.0f);
    const ae::vec4f pixel_size(viewport_size.x() / static_cast<f32>(width_),
                               viewport_size.y() / static_cast<f32>(height_),
                               0.0f);

    const ae::vec4f camera_pos = ae::vec4f(0.0f, 0.0f, 1.0f);
    const ae::sphere sphere(ae::vec4f(0.0f, 0.0f, -2.0f), 1.0f);

    for(u32 y = 0; y < height_; y++) {
        const f32 yf = static_cast<f32>(y);
        const f32 t = yf / static_cast<f32>(height_);

        const u32 background = ae::color(ae::lerp(t, background0.r_, background1.r_),
                                         ae::lerp(t, background0.g_, background1.g_),
                                         ae::lerp(t, background0.b_, background1.b_)).get_argb32();

        for(u32 x = 0; x < width_; x++) {
            const ae::vec4f uv = (ae::vec4f(static_cast<f32>(x) + 0.5f, yf + 0.5f, 0.0f) * pixel_size) - (viewport_size * 0.5f);

            const ae::ray ray(camera_pos, uv - camera_pos);
            ae::ray_hit_info hit_info;

            u32 *pixel = &framebuffer_[y * width_ + x];

            if(sphere.intersects(ray, hit_info)) {
                const std::pair<f32, f32> input{-1.0f, 1.0f};
                const std::pair<f32, f32> output{0.0f, 1.0f};

                const color c(ae::remap(hit_info.normal_.x(), input, output),
                        ae::remap(hit_info.normal_.y(), input, output),
                        ae::remap(hit_info.normal_.z(), input, output));

                *pixel = c.get_argb32();
            } else {
                *pixel = background;
            }
        }
    }
}

std::span<u32> software_raytracer::get_result() const {
    return std::span(framebuffer_, width_ * height_);
}

}
