#pragma once

#include "raytracer.h"

#include <vulkan/vulkan.h>

namespace ae {
    class vulkan_raytracer final : public raytracer {
    public:
        static constexpr u32 req_major_version = 1;
        static constexpr u32 req_minor_version = 0;

        static bool init();
        static void terminate();

        vulkan_raytracer(u32 *buffer);
        ~vulkan_raytracer() override;

        bool setup() override;
        void trace() override;

    private:
        static constexpr VkFormat image_format = VK_FORMAT_B8G8R8A8_UNORM;

        bool create_instance();
        bool create_device();
        bool create_pipeline();
        bool create_command_handles();
        [[nodiscard]] VkImage create_image(u32 width, u32 height) const;
        [[nodiscard]] VkImageView create_image_view(VkImage image) const;
        bool load_functions();

        static void * lib_;
        VkInstance instance_ = VK_NULL_HANDLE;
        VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;
        VkShaderModule compute_shader_module_ = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
        VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
        VkPipeline pipeline_ = VK_NULL_HANDLE;
        VkCommandPool command_pool_ = VK_NULL_HANDLE;
        VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;

        u32 queue_family_index_ = static_cast<u32>(-1);
    };
}
