#pragma once

#include <vulkan/vulkan.h>

#define AE_VULKAN_GLOBAL_FUNCS \
    X(vkCreateInstance) \
    X(vkEnumerateInstanceLayerProperties) \
    X(vkEnumerateInstanceExtensionProperties) \
    X(vkEnumerateInstanceVersion)

#define AE_VULKAN_INSTANCE_FUNCS \
    X(vkGetDeviceProcAddr) \
    X(vkDestroyInstance) \
    X(vkEnumeratePhysicalDevices) \
    X(vkGetPhysicalDeviceQueueFamilyProperties) \
    X(vkGetPhysicalDeviceMemoryProperties) \
    X(vkCreateDevice)

#define AE_VULKAN_DEVICE_FUNCS \
    X(vkDestroyDevice) \
    X(vkGetDeviceQueue) \
    X(vkDeviceWaitIdle) \
    X(vkCreateShaderModule) \
    X(vkDestroyShaderModule) \
    X(vkCreateDescriptorSetLayout) \
    X(vkDestroyDescriptorSetLayout) \
    X(vkCreateDescriptorPool) \
    X(vkDestroyDescriptorPool) \
    X(vkCreatePipelineLayout) \
    X(vkDestroyPipelineLayout) \
    X(vkCreateComputePipelines) \
    X(vkDestroyPipeline) \
    X(vkCreateCommandPool) \
    X(vkDestroyCommandPool) \
    X(vkAllocateCommandBuffers) \
    X(vkFreeCommandBuffers) \
    X(vkBeginCommandBuffer) \
    X(vkEndCommandBuffer) \
    X(vkCmdBindPipeline) \
    X(vkCmdPipelineBarrier) \
    X(vkCmdPushConstants) \
    X(vkCmdBindDescriptorSets) \
    X(vkCmdDispatch) \
    X(vkCreateFence) \
    X(vkDestroyFence) \
    X(vkQueueSubmit) \
    X(vkQueueWaitIdle) \
    X(vkWaitForFences) \
    X(vkMapMemory) \
    X(vkUnmapMemory) \
    X(vkCreateImage) \
    X(vkDestroyImage) \
    X(vkCreateImageView) \
    X(vkDestroyImageView) \
    X(vkGetImageMemoryRequirements) \
    X(vkAllocateMemory) \
    X(vkFreeMemory) \
    X(vkBindImageMemory) \
    X(vkAllocateDescriptorSets) \
    X(vkFreeDescriptorSets) \
    X(vkUpdateDescriptorSets)

inline PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

#define X(item) inline PFN_##item item;
AE_VULKAN_GLOBAL_FUNCS
AE_VULKAN_INSTANCE_FUNCS
AE_VULKAN_DEVICE_FUNCS
#undef X
