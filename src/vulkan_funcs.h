#pragma once

#include <vulkan/vulkan.h>

#define AE_VULKAN_GLOBAL_FUNCS \
    X(vkCreateInstance) \
    X(vkEnumerateInstanceLayerProperties) \
    X(vkEnumerateInstanceVersion)

#define AE_VULKAN_INSTANCE_FUNCS \
    X(vkGetDeviceProcAddr) \
    X(vkDestroyInstance)

#define AE_VULKAN_DEVICE_FUNCS \
    X(vkDestroyDevice) \
    X(vkGetDeviceQueue) \
    X(vkDeviceWaitIdle)

inline PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

#define X(item) inline PFN_##item item;
AE_VULKAN_GLOBAL_FUNCS
AE_VULKAN_INSTANCE_FUNCS
AE_VULKAN_DEVICE_FUNCS
#undef X
