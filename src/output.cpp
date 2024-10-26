#include "output.h"

#include "raytracer.h"

#include <cstring>

namespace ae {

size_t output::file_size() {
    auto [w, h] = raytracer::get_resolution();
    return sizeof(tga_file_header) * (w * h * sizeof(u32));
}

void output::write_default_header(void *buffer) {
    auto [w, h] = raytracer::get_resolution();

    tga_file_header header = {
        .image_type_ = 2,
        .width_ = static_cast<u16>(w),
        .height_ = static_cast<u16>(h),
        .pixel_depth_ = 32
    };

    std::memcpy(buffer, &header, sizeof(header));
}

}
