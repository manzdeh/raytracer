#include "software_raytracer.h"

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
    const f32 heightf = static_cast<f32>(height_);

    for(u32 y = 0; y < height_; y++) {
        const f32 yfloat = static_cast<f32>(y) / heightf;
        const u32 color = (static_cast<u32>(255.0f * yfloat) << 16) | (0xff << 24);

        for(u32 x = 0; x < width_; x++) {
            framebuffer_[y * width_ + x] = color;
        }
    }
}

std::span<u32> software_raytracer::get_result() const {
    return std::span(framebuffer_, width_ * height_);
}

}
