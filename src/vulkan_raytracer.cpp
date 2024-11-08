#include "vulkan_raytracer.h"

#ifdef AE_PLATFORM_WIN32
#include "common_win32.h"
#elif defined(AE_PLATFORM_LINUX)
#include "common_linux.h"
#endif

#include "vulkan_funcs.h"

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

vulkan_raytracer::~vulkan_raytracer() {
#define AE_VULKAN_SAFE_DELETE(delete_func, handle, ...) \
    do { \
        if(handle != VK_NULL_HANDLE) { \
            delete_func(handle, ##__VA_ARGS__); \
            handle = VK_NULL_HANDLE; \
        } \
    } while(0)

    vkDeviceWaitIdle(device_);

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
    vkCmdDispatch(command_buffer_, ae::raytracer::tile_size, ae::raytracer::tile_size, 1);
}

bool vulkan_raytracer::create_instance() {
#if 0
#ifdef AE_DEBUG
    const char *layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    const char *extensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    // TODO: Verify that we support the layers and extensions
#endif
#endif

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
        .pApplicationInfo = &application_info,
#if 0
#ifdef AE_DEBUG
        .enabledLayerCount = AE_ARRAY_COUNT(layers),
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = AE_ARRAY_COUNT(extensions),
        .ppEnabledExtensionNames = extensions
#endif
#endif
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
    const VkShaderModuleCreateInfo shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        // TODO: Create and load a shader from disk
    };

    if(vkCreateShaderModule(device_, &shader_module_create_info, nullptr, &compute_shader_module_) != VK_SUCCESS) {
        return false;
    }

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        // TODO: Fill out the layout
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

bool vulkan_raytracer::load_functions() {
    if(!lib_) {
        return false;
    }

    if(!vkGetInstanceProcAddr) {
        vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
#ifdef AE_PLATFORM_WIN32
            GetProcAddress(lib_, "vkGetInstanceProcAddr")
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
