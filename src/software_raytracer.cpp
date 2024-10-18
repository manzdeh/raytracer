#include "software_raytracer.h"

namespace ae {

software_raytracer::~software_raytracer() {
    delete[] framebuffer_;
}

bool software_raytracer::setup(u32 width, u32 height) {
    width_ = width;
    height_ = height;

    framebuffer_ = new u32[width_ * height_]{0};

    return true;
}

void software_raytracer::trace() {
}

std::span<u32> software_raytracer::get_result() const {
    return std::span(framebuffer_, width_ * height_);
}

}
