#include "software_raytracer.h"

#include "color.h"
#include "ray.h"
#include "shapes.h"
#include "vec.h"

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
    const ae::hsv background0 = ae::color(AE_RGB(0x4d, 0xa6, 0xf0)).to_hsv();
    const ae::hsv background1 = ae::color(1.0f, 1.0f, 1.0f).to_hsv();

    const ae::vec4f viewport_size(static_cast<f32>(width_) / static_cast<f32>(height_), 1.0f, 0.0f);
    const ae::vec4f pixel_size(viewport_size.x() / static_cast<f32>(width_),
                               viewport_size.y() / static_cast<f32>(height_),
                               0.0f);

    const ae::vec4f camera_pos = ae::vec4f(0.0f, 0.0f, 1.0f);
    const ae::sphere sphere(ae::vec4f(0.0f, 0.0f, -2.0f), 1.0f);

    for(u32 y = 0; y < height_; y++) {
        const f32 yf = static_cast<f32>(y);
        const f32 t = yf / static_cast<f32>(height_);

        const ae::hsv background = background1.lerp(background0, t);
        const u32 background_argb = (0xff << 24) | ae::color(background).get_argb32();

        for(u32 x = 0; x < width_; x++) {
            const ae::vec4f uv = (ae::vec4f(static_cast<f32>(x) + 0.5f, yf + 0.5f, 0.0f) * pixel_size) - (viewport_size * 0.5f);

            const ae::ray ray(camera_pos, uv - camera_pos);
            ae::ray_hit_info hit_info;

            framebuffer_[y * width_ + x] = sphere.intersects(ray, hit_info) ? 0xffff0000 : background_argb;
        }
    }
}

std::span<u32> software_raytracer::get_result() const {
    return std::span(framebuffer_, width_ * height_);
}

}
