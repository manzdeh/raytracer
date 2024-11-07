#pragma once

#include "raytracer.h"

#include <vulkan/vulkan.h>

namespace ae {
    class vulkan_raytracer final : public raytracer {
    public:
        static constexpr u32 req_major_version = 1;
        static constexpr u32 req_minor_version = 0;

        vulkan_raytracer();
        ~vulkan_raytracer() override;

        bool setup() override;
        void trace() override;

    private:
        bool create_instance();
        bool create_device();
        bool load_functions();

        static void * lib_;
        VkInstance instance_ = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;
    };
}
