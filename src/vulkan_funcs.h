#pragma once

#include <vulkan/vulkan.h>

#define AE_VULKAN_GLOBAL_FUNCS \
    X(vkCreateInstance) \
    X(vkEnumerateInstanceLayerProperties) \
    X(vkEnumerateInstanceVersion)

#define AE_VULKAN_INSTANCE_FUNCS \
    X(vkGetDeviceProcAddr) \
    X(vkDestroyInstance) \
    X(vkEnumeratePhysicalDevices) \
    X(vkGetPhysicalDeviceQueueFamilyProperties) \
    X(vkCreateDevice)

#define AE_VULKAN_DEVICE_FUNCS \
    X(vkDestroyDevice) \
    X(vkGetDeviceQueue) \
    X(vkDeviceWaitIdle) \
    X(vkCreateShaderModule) \
    X(vkDestroyShaderModule) \
    X(vkCreatePipelineLayout) \
    X(vkDestroyPipelineLayout) \
    X(vkCreateComputePipelines) \
    X(vkDestroyPipeline) \
    X(vkCreateCommandPool) \
    X(vkDestroyCommandPool) \
    X(vkAllocateCommandBuffers) \
    X(vkFreeCommandBuffers) \
    X(vkCmdDispatch)

inline PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

#define X(item) inline PFN_##item item;
AE_VULKAN_GLOBAL_FUNCS
AE_VULKAN_INSTANCE_FUNCS
AE_VULKAN_DEVICE_FUNCS
#undef X
