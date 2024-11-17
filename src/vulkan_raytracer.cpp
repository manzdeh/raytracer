#include "vulkan_raytracer.h"

#ifdef AE_PLATFORM_WIN32
#include "common_win32.h"
#elif defined(AE_PLATFORM_LINUX)
#include "common_linux.h"
#endif

#include "vulkan_funcs.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

struct push_constants {
    ae::color bg0;
    ae::color bg1;
};
static_assert(sizeof(push_constants) <= 128, "128 Bytes is the guaranteed min size for push constants");

#ifdef AE_DEBUG
static const char * get_property_name(const VkLayerProperties &layer) { return layer.layerName; }
static const char * get_property_name(const VkExtensionProperties &extension) { return extension.extensionName; }

template<typename TType, typename TEnumerateFunc, typename... TArgs>
static void fill_supported_property_requests(const std::initializer_list<const char *> &requested,
                                             std::vector<const char *> &supported,
                                             TEnumerateFunc enumerate_func,
                                             TArgs&&... args) {
    if(requested.size() == 0) {
        return;
    }

    supported.clear();

    u32 count;
    if(enumerate_func(std::forward<TArgs>(args)..., &count, nullptr) == VK_SUCCESS) {
        std::vector<TType> properties(count);

        if(enumerate_func(std::forward<TArgs>(args)..., &count, properties.data()) == VK_SUCCESS) {
            for(const char *request : requested) {
                for(size_t i = 0; i < properties.size(); i++) {
                    if(std::strcmp(request, get_property_name(properties[i])) == 0) {
                        supported.push_back(request);
                    }
                }
            }
        }
    }
}
#endif

template<typename THandle, typename TParentObject, typename TDestroyFunc>
class vulkan_handle {
public:
    vulkan_handle(TParentObject parent, TDestroyFunc destroy_func)
        : parent_(parent)
        , destroy_(destroy_func) {}

    vulkan_handle(THandle handle, TParentObject parent, TDestroyFunc destroy_func)
        : handle_(handle)
        , parent_(parent)
        , destroy_(destroy_func) {}

    ~vulkan_handle() {
        if(handle_ != VK_NULL_HANDLE && destroy_) {
            destroy_(parent_, handle_, nullptr);
            handle_ = VK_NULL_HANDLE;
        }
    }

    template<typename... TArgs>
    bool create(auto create_func, TArgs&&... args) {
        return create_func(parent_, std::forward<TArgs>(args)..., nullptr, &handle_)
            == VK_SUCCESS;
    }

    operator bool() const { return handle_ != VK_NULL_HANDLE; }
    explicit operator THandle() { return handle_; }
    THandle operator*() { return handle_; }

private:
    THandle handle_ = VK_NULL_HANDLE;
    TParentObject parent_ = VK_NULL_HANDLE;
    TDestroyFunc destroy_ = nullptr;
};

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
            delete_func(__VA_ARGS__); \
        } \
    } while(0)

    AE_VULKAN_SAFE_DELETE(vkFreeCommandBuffers, command_buffer_,
                          device_, command_pool_, 1, &command_buffer_);
    AE_VULKAN_SAFE_DELETE(vkDestroyCommandPool, command_pool_,
                          device_, command_pool_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyShaderModule, compute_shader_module_,
                          device_, compute_shader_module_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyDescriptorSetLayout, descriptor_set_layout_,
                          device_, descriptor_set_layout_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyDescriptorPool, descriptor_pool_,
                          device_, descriptor_pool_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyPipelineLayout, pipeline_layout_,
                          device_, pipeline_layout_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyPipeline, pipeline_,
                          device_, pipeline_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyDevice, device_,
                          device_, nullptr);
    AE_VULKAN_SAFE_DELETE(vkDestroyInstance, instance_,
                          instance_, nullptr);

#undef AE_VULKAN_SAFE_DELETE
}

bool vulkan_raytracer::setup() {
    return lib_
        && load_functions()
        && create_pipeline()
        && create_command_handles();
}

void vulkan_raytracer::trace() {
    auto [w, h] = raytracer::get_resolution();

    vulkan_handle<VkImage, VkDevice, decltype(vkDestroyImage)>
        image(create_image(w, h), device_, vkDestroyImage);

    if(!image) {
        return;
    }

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device_, *image, &memory_requirements);

    const VkMemoryAllocateInfo allocate_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = [this](const VkMemoryRequirements &requirements) {
            VkPhysicalDeviceMemoryProperties memory_properties;
            vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties);

            constexpr u32 required_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

            for(u32 i = 0; i < memory_properties.memoryTypeCount; i++) {
                if((requirements.memoryTypeBits & (1 << i))
                   && ((memory_properties.memoryTypes[i].propertyFlags & required_flags) == required_flags)) {
                    return i;
                }
            }

            return static_cast<u32>(-1);
        }(memory_requirements)
    };

    vulkan_handle<VkDeviceMemory, VkDevice, decltype(vkFreeMemory)>
        device_memory(device_, vkFreeMemory);

    if(allocate_info.memoryTypeIndex == static_cast<u32>(-1)
       || !device_memory.create(vkAllocateMemory, &allocate_info)
       || (vkBindImageMemory(device_, *image, *device_memory, 0) != VK_SUCCESS)) {
        return;
    }

    vulkan_handle<VkImageView, VkDevice, decltype(vkDestroyImageView)>
        image_view(create_image_view(*image), device_, vkDestroyImageView);

    if(!image_view) {
        return;
    }

    const VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout_
    };

    VkDescriptorSet descriptor_set;
    if(vkAllocateDescriptorSets(device_, &descriptor_set_allocate_info, &descriptor_set) != VK_SUCCESS) {
        return;
    }

    const VkDescriptorImageInfo descriptor_image_info = {
        .sampler = VK_NULL_HANDLE,
        .imageView = *image_view,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    const VkWriteDescriptorSet write_descriptor_sets[] = {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &descriptor_image_info
        }
    };

    vkUpdateDescriptorSets(device_,
                           AE_ARRAY_COUNT(write_descriptor_sets),
                           write_descriptor_sets,
                           0,
                           nullptr);

    // Command buffer recording
    const VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(command_buffer_, &command_buffer_begin_info);

    vkCmdBindPipeline(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);

    const VkImageMemoryBarrier image_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = *image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    vkCmdPipelineBarrier(command_buffer_,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &image_barrier);

    push_constants pc = {
        .bg0 = raytracer::background0,
        .bg1 = raytracer::background1
    };

    vkCmdPushConstants(command_buffer_, pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pc), &pc);
    vkCmdBindDescriptorSets(command_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, 1, &descriptor_set, 0, nullptr);
    vkCmdDispatch(command_buffer_, w / 16, h / 16, 1);

    vkEndCommandBuffer(command_buffer_);

    // Queue submission
    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer_
    };

    vulkan_handle<VkFence, VkDevice, decltype(vkDestroyFence)>
        fence(device_, vkDestroyFence);

    const VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    };

    if(!fence.create(vkCreateFence, &fence_create_info)) {
        return;
    }

    VkQueue queue;
    vkGetDeviceQueue(device_, queue_family_index_, 0, &queue);

    vkQueueSubmit(queue, 1, &submit_info, *fence);

    VkFence fence_handle = static_cast<VkFence>(fence);
    vkWaitForFences(device_, 1, &fence_handle, VK_TRUE, static_cast<u32>(-1));

    assert(allocate_info.allocationSize == (w * h * 4));

    void *data;
    vkMapMemory(device_,
                *device_memory,
                0,
                allocate_info.allocationSize,
                0,
                &data);

    std::memcpy(framebuffer_, data, allocate_info.allocationSize);
    vkUnmapMemory(device_, *device_memory);
}

bool vulkan_raytracer::create_instance() {
    u32 api_version;
    vkEnumerateInstanceVersion(&api_version);

#ifdef AE_DEBUG
    std::vector<const char *> layers;
    fill_supported_property_requests<VkLayerProperties, decltype(vkEnumerateInstanceLayerProperties)>
        ({ "VK_LAYER_KHRONOS_validation" }, layers, vkEnumerateInstanceLayerProperties);

    std::vector<const char *>extensions;
    fill_supported_property_requests<VkExtensionProperties, decltype(vkEnumerateInstanceExtensionProperties)>
        ({ VK_EXT_DEBUG_UTILS_EXTENSION_NAME }, extensions, vkEnumerateInstanceExtensionProperties, nullptr);
#endif

    const u32 major = VK_API_VERSION_MAJOR(api_version);
    const u32 minor = VK_API_VERSION_MINOR(api_version);

    if(!(major > req_major_version || (major == req_major_version && minor >= req_minor_version))) {
        return false;
    }

    const VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = VK_MAKE_API_VERSION(0, req_major_version, req_minor_version, 0)
    };

    const VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info,
#ifdef AE_DEBUG
        .enabledLayerCount = static_cast<u32>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<u32>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
#endif
    };

    return vkCreateInstance(&create_info, nullptr, &instance_) == VK_SUCCESS;
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

    return vkCreateDevice(physical_device_, &create_info, nullptr, &device_) == VK_SUCCESS;
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

        std::fclose(shader_file);

        if(vkCreateShaderModule(device_, &shader_module_create_info, nullptr, &compute_shader_module_) != VK_SUCCESS) {
            return false;
        }
    } else {
        return false;
    }

    const VkDescriptorSetLayoutBinding descriptor_set_layout_bindings[] = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        }
    };

    const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = AE_ARRAY_COUNT(descriptor_set_layout_bindings),
        .pBindings = descriptor_set_layout_bindings
    };

    if(vkCreateDescriptorSetLayout(device_, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
        return false;
    }

    const VkDescriptorPoolSize descriptor_pool_sizes[] = {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1
        }
    };

    const VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = AE_ARRAY_COUNT(descriptor_pool_sizes),
        .pPoolSizes = descriptor_pool_sizes
    };

    if(vkCreateDescriptorPool(device_, &descriptor_pool_create_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        return false;
    }

    const VkPushConstantRange push_constant_ranges[] = {
        {
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .offset = 0,
            .size = sizeof(push_constants)
        }
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout_,
        .pushConstantRangeCount = AE_ARRAY_COUNT(push_constant_ranges),
        .pPushConstantRanges = push_constant_ranges
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

    return vkCreateComputePipelines(device_,
                                    VK_NULL_HANDLE,
                                    AE_ARRAY_COUNT(create_infos),
                                    create_infos,
                                    nullptr,
                                    &pipeline_) == VK_SUCCESS;
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

    return vkAllocateCommandBuffers(device_, &allocate_info, &command_buffer_) == VK_SUCCESS;
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
        .tiling = VK_IMAGE_TILING_LINEAR,
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
