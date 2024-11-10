#include "vulkan_raytracer.h"

#ifdef AE_PLATFORM_WIN32
#include "common_win32.h"
#elif defined(AE_PLATFORM_LINUX)
#include "common_linux.h"
#endif

#include "vulkan_funcs.h"

#include <cassert>
#include <cstdio>
#include <vector>
#include <vulkan/vulkan.h>

namespace ae {

void * vulkan_raytracer::lib_ = nullptr;

bool vulkan_raytracer::init() {
    if(!lib_) {
        lib_ =
#ifdef AE_PLATFORM_WIN32
            LoadLibraryW(L"vulkan-1.dll")
#elif defined(AE_PLATFORM_LINUX)
            dlopen("libvulkan.so.1", RTLD_NOW)
#endif
        ;
    }

    return lib_ != nullptr;
}

void vulkan_raytracer::terminate() {
    if(lib_) {
#ifdef AE_PLATFORM_WIN32
        FreeLibrary(reinterpret_cast<HMODULE>(lib_));
#elif defined(AE_PLATFORM_LINUX)
        dlclose(lib_);
#endif

        lib_ = nullptr;
    }
}

vulkan_raytracer::vulkan_raytracer(u32 *buffer)
    : raytracer(buffer) {}

vulkan_raytracer::~vulkan_raytracer() {
#define AE_VULKAN_SAFE_DELETE(delete_func, handle, ...) \
    do { \
        if(handle != VK_NULL_HANDLE) { \
            delete_func(handle, ##__VA_ARGS__); \
            handle = VK_NULL_HANDLE; \
        } \
    } while(0)

    AE_VULKAN_SAFE_DELETE(vkFreeCommandBuffers, device_, command_pool_, 1, &command_buffer_);
    AE_VULKAN_SAFE_DELETE(vkDestroyCommandPool, device_, command_pool_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyShaderModule, device_, compute_shader_module_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyPipelineLayout, device_, pipeline_layout_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyPipeline, device_, pipeline_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyDevice, device_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyInstance, instance_, nullptr);

#undef AE_VULKAN_SAFE_DELETE
}

bool vulkan_raytracer::setup() {
    if(!lib_) {
        return false;
    }

    if(!load_functions()) {
        return false;
    }

    if(!create_pipeline()) {
        return false;
    }

    if(!create_command_handles()) {
        return false;
    }

    return true;
}

void vulkan_raytracer::trace() {
    // TODO: Disabled for now, since this is still a WIP
#if 0
    auto [w, h] = raytracer::get_resolution();

    // Image and memory allocation
    // TODO: The lifetime management of these resources can be done a bit more cleanly with RAII-wrappers instead
    VkImage image = create_image(w, h);
    if(image == VK_NULL_HANDLE) {
        return;
    }

    VkImageView image_view = create_image_view(image);
    if(image_view == VK_NULL_HANDLE) {
        vkDestroyImage(device_, image, nullptr);
        return;
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device_, image, &memory_requirements);

    const VkMemoryAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = [this](const VkMemoryRequirements &requirements) {
            VkPhysicalDeviceMemoryProperties memory_properties;
            vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties);

            for(u32 i = 0; i < memory_properties.memoryTypeCount; i++) {
                if((requirements.memoryTypeBits & (1 << i))
                   && (memory_properties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))) {
                    return i;
                }
            }

            return static_cast<u32>(-1);
        }(memory_requirements)
    };

    VkDeviceMemory device_memory;
    if(allocate_info.memoryTypeIndex == static_cast<u32>(-1)
        || vkAllocateMemory(device_, &allocate_info, nullptr, &device_memory) != VK_SUCCESS) {
        vkDestroyImageView(device_, image_view, nullptr);
        vkDestroyImage(device_, image, nullptr);
        return;
    }

    // Command buffer recoding
    const VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    };
    vkBeginCommandBuffer(command_buffer_, &command_buffer_begin_info);

    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);
    vkCmdDispatch(command_buffer_, w / 16, h / 16, 1);

    vkEndCommandBuffer(command_buffer_);

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer_
    };

    VkQueue queue;
    vkGetDeviceQueue(device_, queue_family_index_, 0, &queue);

    // Queue submission
    vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkMapMemory(device_,
                device_memory,
                0,
                w * h * 4,
                0,
                reinterpret_cast<void **>(&framebuffer_));

    vkDestroyImageView(device_, image_view, nullptr);
    vkDestroyImage(device_, image, nullptr);
    vkFreeMemory(device_, device_memory, nullptr);
#endif
}

bool vulkan_raytracer::create_instance() {
    u32 api_version;
    vkEnumerateInstanceVersion(&api_version);

    const u32 major = VK_API_VERSION_MAJOR(api_version);
    const u32 minor = VK_API_VERSION_MINOR(api_version);

    if(!(major > req_major_version || (major == req_major_version && minor >= req_minor_version))) {
        return false;
    }

    const VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .applicationVersion = VK_MAKE_API_VERSION(0, req_major_version, req_minor_version, 0),
        .apiVersion = VK_MAKE_API_VERSION(0, req_major_version, req_minor_version, 0)
    };

    const VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info
    };

    const VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);
    return result == VK_SUCCESS;
}

bool vulkan_raytracer::create_device() {
    u32 device_count = 0;
    if(vkEnumeratePhysicalDevices(instance_, &device_count, nullptr) == VK_SUCCESS) {
        std::vector<VkPhysicalDevice> physical_devices(device_count);

        if(vkEnumeratePhysicalDevices(instance_, &device_count, physical_devices.data()) != VK_SUCCESS) {
            return false;
        }

        i32 selected_index = -1;
        i32 best_queue_count = -1;

        for(u32 i = 0; i < physical_devices.size(); i++) {
            VkPhysicalDevice physical_device = physical_devices[i];

            u32 queue_family_properties_count;
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_properties_count, nullptr);

            std::vector<VkQueueFamilyProperties> queue_families(queue_family_properties_count);
            vkGetPhysicalDeviceQueueFamilyProperties(physical_device,
                                                     &queue_family_properties_count,
                                                     queue_families.data());

            u32 family_index = 0;

            for(const VkQueueFamilyProperties &queue_family_properties : queue_families) {
                if(queue_family_properties.queueFlags & VK_QUEUE_COMPUTE_BIT
                   && static_cast<i32>(queue_family_properties.queueCount) > best_queue_count) {

                    selected_index = i;
                    best_queue_count = queue_family_properties.queueCount;
                    queue_family_index_ = family_index;
                }

                family_index++;
            }
        }

        if(selected_index == -1 || queue_family_index_ == static_cast<u32>(-1)) {
            return false;
        }

        physical_device_ = physical_devices[selected_index];

    } else {
        return false;
    }

    const f32 queue_priorities[] = {
        1.0f
    };

    const VkDeviceQueueCreateInfo queue_create_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queue_family_index_,
            .queueCount = AE_ARRAY_COUNT(queue_priorities),
            .pQueuePriorities = queue_priorities
        }
    };

    const VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = AE_ARRAY_COUNT(queue_create_infos),
        .pQueueCreateInfos = queue_create_infos
    };

    const VkResult result = vkCreateDevice(physical_device_, &create_info, nullptr, &device_);
    return result == VK_SUCCESS;
}

bool vulkan_raytracer::create_pipeline() {
    std::FILE *shader_file = std::fopen("compute.spv", "rb");
    if(shader_file) {
        std::fseek(shader_file, 0, SEEK_END);
        const size_t shader_buffer_size = std::ftell(shader_file);
        std::rewind(shader_file);

        std::vector<char> shader_buffer(shader_buffer_size);
        std::fread(shader_buffer.data(),
                   sizeof(decltype(shader_buffer)::value_type),
                   shader_buffer_size,
                   shader_file);

        const VkShaderModuleCreateInfo shader_module_create_info = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shader_buffer_size,
            .pCode = reinterpret_cast<const u32 *>(shader_buffer.data())
        };

        if(vkCreateShaderModule(device_, &shader_module_create_info, nullptr, &compute_shader_module_) != VK_SUCCESS) {
            return false;
        }

        std::fclose(shader_file);
    } else {
        return false;
    }

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        // TODO: Fill out rest of the layout
    };

    if(vkCreatePipelineLayout(device_, &pipeline_layout_create_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        return false;
    }

    const VkComputePipelineCreateInfo create_infos[] = {
        {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .stage = {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                .module = compute_shader_module_,
                .pName = "main",
                .pSpecializationInfo = nullptr
            },
            .layout = pipeline_layout_,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0
        }
    };

    const VkResult result = vkCreateComputePipelines(device_,
                                                     VK_NULL_HANDLE,
                                                     AE_ARRAY_COUNT(create_infos),
                                                     create_infos,
                                                     nullptr,
                                                     &pipeline_);
    return result == VK_SUCCESS;
}

bool vulkan_raytracer::create_command_handles() {
    const VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queue_family_index_
    };

    if(vkCreateCommandPool(device_, &command_pool_create_info, nullptr, &command_pool_) != VK_SUCCESS) {
        return false;
    }

    const VkCommandBufferAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool_,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    const VkResult result = vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer_);
    return result == VK_SUCCESS;
}

VkImage vulkan_raytracer::create_image(u32 width, u32 height) const {
    const VkImageCreateInfo image_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vulkan_raytracer::image_format,
        .extent = {
            .width = width,
            .height = height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = &queue_family_index_,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage image;

    const VkResult result = vkCreateImage(device_,
                                          &image_create_info,
                                          nullptr,
                                          &image);

    return (result == VK_SUCCESS) ? image : VK_NULL_HANDLE;
}

VkImageView vulkan_raytracer::create_image_view(VkImage image) const {
    assert(image != VK_NULL_HANDLE);

    const VkImageViewCreateInfo image_view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vulkan_raytracer::image_format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkImageView image_view;

    const VkResult result = vkCreateImageView(device_,
                                              &image_view_create_info,
                                              nullptr,
                                              &image_view);

    return (result == VK_SUCCESS) ? image_view : VK_NULL_HANDLE;
}

bool vulkan_raytracer::load_functions() {
    if(!lib_) {
        return false;
    }

    if(!vkGetInstanceProcAddr) {
        vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
#ifdef AE_PLATFORM_WIN32
            GetProcAddress(reinterpret_cast<HMODULE>(lib_), "vkGetInstanceProcAddr")
#elif defined(AE_PLATFORM_LINUX)
            dlsym(lib_, "vkGetInstanceProcAddr")
#endif
        );

        if(!vkGetInstanceProcAddr) {
            return false;
        }
    }

#define X(item) \
    if(!item) { \
        if(item = reinterpret_cast<PFN_##item>(AE_VULKAN_GET_PROC_ADDR(item)); !item) { \
            return false; \
        } \
    }

#define AE_VULKAN_GET_PROC_ADDR(item) vkGetInstanceProcAddr(VK_NULL_HANDLE, #item)
    AE_VULKAN_GLOBAL_FUNCS
    if(!create_instance()) {
        return false;
    }
#undef AE_VULKAN_GET_PROC_ADDR

#define AE_VULKAN_GET_PROC_ADDR(item) vkGetInstanceProcAddr(instance_, #item)
    AE_VULKAN_INSTANCE_FUNCS
    if(!create_device()) {
        return false;
    }
#undef AE_VULKAN_GET_PROC_ADDR

#define AE_VULKAN_GET_PROC_ADDR(item) vkGetDeviceProcAddr(device_, #item)
    AE_VULKAN_DEVICE_FUNCS
#undef AE_VULKAN_GET_PROC_ADDR

#undef X

    return true;
}

}
