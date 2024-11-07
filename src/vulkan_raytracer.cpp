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

vulkan_raytracer::vulkan_raytracer() {
    if(!lib_) {
        lib_ =
#ifdef AE_PLATFORM_WIN32
            LoadLibraryW(L"vulkan-1.dll")
#elif defined(AE_PLATFORM_LINUX)
            dlopen("libvulkan.so.1", RTLD_NOW)
#endif
        ;
    }
}

vulkan_raytracer::~vulkan_raytracer() {
#define AE_VULKAN_SAFE_DELETE(handle, delete_func, ...) \
    do { \
        if(handle != VK_NULL_HANDLE) { \
            delete_func(handle, ##__VA_ARGS__); \
            handle = VK_NULL_HANDLE; \
        } \
    } while(0)

    AE_VULKAN_SAFE_DELETE(instance_, vkDestroyInstance, nullptr);

    if(lib_) {
#ifdef AE_PLATFORM_WIN32
        FreeLibrary(reinterpret_cast<HMODULE>(lib_));
#elif defined(AE_PLATFORM_LINUX)
        dlclose(lib_);
#endif

        lib_ = nullptr;
    }

#undef AE_VULKAN_SAFE_DELETE
}

bool vulkan_raytracer::setup() {
    if(!lib_) {
        return false;
    }

    if(!load_functions()) {
        return false;
    }

    return true;
}

void vulkan_raytracer::trace() {
}

bool vulkan_raytracer::create_instance() {
#ifdef AE_DEBUG
    const char *layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    const char *extensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    // TODO: Verify that we support the layers and extensions
#endif

    u32 api_version;
    vkEnumerateInstanceVersion(&api_version);

    const u32 major = VK_API_VERSION_MAJOR(api_version);
    const u32 minor = VK_API_VERSION_MINOR(api_version);

    if(!(major > req_major_version || (major == req_major_version && minor >= req_minor_version))) {
        return false;
    }

    VkApplicationInfo application_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .applicationVersion = VK_MAKE_API_VERSION(0, req_major_version, req_minor_version, 0),
        .apiVersion = VK_MAKE_API_VERSION(0, req_major_version, req_minor_version, 0)
    };

    VkInstanceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &application_info,
#ifdef AE_DEBUG
        .enabledLayerCount = AE_ARRAY_COUNT(layers),
        .ppEnabledLayerNames = layers,
        .enabledExtensionCount = AE_ARRAY_COUNT(extensions),
        .ppEnabledExtensionNames = extensions
#endif
    };

    VkResult result = vkCreateInstance(&create_info, nullptr, &instance_);

    return result == VK_SUCCESS;
}

bool vulkan_raytracer::create_device() {
    return true;
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
