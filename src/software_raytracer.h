#pragma once

#include "raytracer.h"

namespace ae {
    class software_raytracer final : public raytracer {
    public:
        software_raytracer() = default;
        software_raytracer(u32 *buffer);

        bool setup() override;
        void trace() override;
        std::span<u32> get_result() const override;

    private:
        u32 *framebuffer_ = nullptr;
    };
}
