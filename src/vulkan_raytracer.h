#pragma once

#include "raytracer.h"

// TODO: Stubbed out for now

namespace ae {
    class vulkan_raytracer final : public raytracer {
    public:
        vulkan_raytracer() = default;
        ~vulkan_raytracer() override {}

        bool setup() override { return false; }
        void trace() override {}
    };
}
