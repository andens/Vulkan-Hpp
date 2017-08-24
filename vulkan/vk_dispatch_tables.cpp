#include "vk_dispatch_tables.h"

#include <stdexcept>

namespace vkgen {

/*
 * ------------------------------------------------------
 * VulkanGlobalTable
 * ------------------------------------------------------
*/

VulkanGlobalTable::VulkanGlobalTable(std::string const& vulkan_library) {
#if defined(_WIN32)
  library_ = LoadLibraryA(vulkan_library.c_str());
#elif defined(__linux__)
  library_ = dlopen(vulkan_library.c_str(), RTLD_NOW);
#else
#error "Unsupported OS"
#endif

  if (!library_) {
    throw std::runtime_error("Could not load Vulkan loader.");
  }

#if defined(_WIN32)
  vkGetInstanceProcAddr_ = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
      GetProcAddress(library_, "vkGetInstanceProcAddr"));
#elif defined(__linux__)
  vkGetInstanceProcAddr_ = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
      dlsym(library_, "vkGetInstanceProcAddr"));
#else
#error "Unsupported OS"
#endif

  if (!vkGetInstanceProcAddr_) {
    throw VulkanProcNotFound("vkGetInstanceProcAddr");
  }

  // With vkGetInstanceProcAddr loaded, we can load global functions, which
  // are the ones not making use of the instance parameter.
#define LOAD_GLOBAL_FUNC(fun)\
  this->##fun##_ = reinterpret_cast<PFN_##fun>(this->vkGetInstanceProcAddr_(\
      nullptr, #fun));\
  if (!this->##fun##_) {\
    throw VulkanProcNotFound(#fun);\
  }

  LOAD_GLOBAL_FUNC(vkCreateInstance);
  LOAD_GLOBAL_FUNC(vkEnumerateInstanceExtensionProperties);
  LOAD_GLOBAL_FUNC(vkEnumerateInstanceLayerProperties);
}

PFN_vkVoidFunction VulkanGlobalTable::vkGetInstanceProcAddr(VkInstance instance, const char* pName) const {
  return this->vkGetInstanceProcAddr_(instance, pName);
}

VkResult VulkanGlobalTable::vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) const {
  return this->vkCreateInstance_(pCreateInfo, pAllocator, pInstance);
}

VkResult VulkanGlobalTable::vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) const {
  return this->vkEnumerateInstanceExtensionProperties_(pLayerName, pPropertyCount, pProperties);
}

VkResult VulkanGlobalTable::vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) const {
  return this->vkEnumerateInstanceLayerProperties_(pPropertyCount, pProperties);
}

/*
 * ------------------------------------------------------
 * VulkanInstanceTable
 * ------------------------------------------------------
*/

VulkanInstanceTable::VulkanInstanceTable(VulkanGlobalTable const* globals, VkInstance instance) : instance_(instance) {
#define LOAD_INSTANCE_FUNC(fun)\
  this->##fun##_ = reinterpret_cast<PFN_##fun>(globals->vkGetInstanceProcAddr(\
      instance, #fun));\
  if (!this->##fun##_) {\
    throw VulkanProcNotFound(#fun);\
  }

  LOAD_INSTANCE_FUNC(vkDestroyInstance);
  LOAD_INSTANCE_FUNC(vkEnumeratePhysicalDevices);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFeatures);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFormatProperties);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceImageFormatProperties);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceProperties);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties);
  LOAD_INSTANCE_FUNC(vkGetDeviceProcAddr);
  LOAD_INSTANCE_FUNC(vkCreateDevice);
  LOAD_INSTANCE_FUNC(vkEnumerateDeviceExtensionProperties);
  LOAD_INSTANCE_FUNC(vkEnumerateDeviceLayerProperties);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSparseImageFormatProperties);
}

void VulkanInstanceTable::vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyInstance_(instance, pAllocator);
}

VkResult VulkanInstanceTable::vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) const {
  return this->vkEnumeratePhysicalDevices_(instance, pPhysicalDeviceCount, pPhysicalDevices);
}

void VulkanInstanceTable::vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures) const {
  return this->vkGetPhysicalDeviceFeatures_(physicalDevice, pFeatures);
}

void VulkanInstanceTable::vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties) const {
  return this->vkGetPhysicalDeviceFormatProperties_(physicalDevice, format, pFormatProperties);
}

VkResult VulkanInstanceTable::vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties) const {
  return this->vkGetPhysicalDeviceImageFormatProperties_(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

void VulkanInstanceTable::vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) const {
  return this->vkGetPhysicalDeviceProperties_(physicalDevice, pProperties);
}

void VulkanInstanceTable::vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties) const {
  return this->vkGetPhysicalDeviceQueueFamilyProperties_(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void VulkanInstanceTable::vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) const {
  return this->vkGetPhysicalDeviceMemoryProperties_(physicalDevice, pMemoryProperties);
}

PFN_vkVoidFunction VulkanInstanceTable::vkGetDeviceProcAddr(VkDevice device, const char* pName) const {
  return this->vkGetDeviceProcAddr_(device, pName);
}

VkResult VulkanInstanceTable::vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) const {
  return this->vkCreateDevice_(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

VkResult VulkanInstanceTable::vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) const {
  return this->vkEnumerateDeviceExtensionProperties_(physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VkResult VulkanInstanceTable::vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkLayerProperties* pProperties) const {
  return this->vkEnumerateDeviceLayerProperties_(physicalDevice, pPropertyCount, pProperties);
}

void VulkanInstanceTable::vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pPropertyCount, VkSparseImageFormatProperties* pProperties) const {
  return this->vkGetPhysicalDeviceSparseImageFormatProperties_(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

/*
 * ------------------------------------------------------
 * VulkanDeviceTable
 * ------------------------------------------------------
*/

VulkanDeviceTable::VulkanDeviceTable(VulkanInstanceTable const* instance, VkDevice device) : device_(device) {
#define LOAD_DEVICE_FUNC(fun)\
  this->##fun##_ = reinterpret_cast<PFN_##fun>(instance->vkGetDeviceProcAddr(\
      device, #fun));\
  if (!this->##fun##_) {\
    throw VulkanProcNotFound(#fun);\
  }

  LOAD_DEVICE_FUNC(vkDestroyDevice);
  LOAD_DEVICE_FUNC(vkGetDeviceQueue);
  LOAD_DEVICE_FUNC(vkQueueSubmit);
  LOAD_DEVICE_FUNC(vkQueueWaitIdle);
  LOAD_DEVICE_FUNC(vkDeviceWaitIdle);
  LOAD_DEVICE_FUNC(vkAllocateMemory);
  LOAD_DEVICE_FUNC(vkFreeMemory);
  LOAD_DEVICE_FUNC(vkMapMemory);
  LOAD_DEVICE_FUNC(vkUnmapMemory);
  LOAD_DEVICE_FUNC(vkFlushMappedMemoryRanges);
  LOAD_DEVICE_FUNC(vkInvalidateMappedMemoryRanges);
  LOAD_DEVICE_FUNC(vkGetDeviceMemoryCommitment);
  LOAD_DEVICE_FUNC(vkBindBufferMemory);
  LOAD_DEVICE_FUNC(vkBindImageMemory);
  LOAD_DEVICE_FUNC(vkGetBufferMemoryRequirements);
  LOAD_DEVICE_FUNC(vkGetImageMemoryRequirements);
  LOAD_DEVICE_FUNC(vkGetImageSparseMemoryRequirements);
  LOAD_DEVICE_FUNC(vkQueueBindSparse);
  LOAD_DEVICE_FUNC(vkCreateFence);
  LOAD_DEVICE_FUNC(vkDestroyFence);
  LOAD_DEVICE_FUNC(vkResetFences);
  LOAD_DEVICE_FUNC(vkGetFenceStatus);
  LOAD_DEVICE_FUNC(vkWaitForFences);
  LOAD_DEVICE_FUNC(vkCreateSemaphore);
  LOAD_DEVICE_FUNC(vkDestroySemaphore);
  LOAD_DEVICE_FUNC(vkCreateEvent);
  LOAD_DEVICE_FUNC(vkDestroyEvent);
  LOAD_DEVICE_FUNC(vkGetEventStatus);
  LOAD_DEVICE_FUNC(vkSetEvent);
  LOAD_DEVICE_FUNC(vkResetEvent);
  LOAD_DEVICE_FUNC(vkCreateQueryPool);
  LOAD_DEVICE_FUNC(vkDestroyQueryPool);
  LOAD_DEVICE_FUNC(vkGetQueryPoolResults);
  LOAD_DEVICE_FUNC(vkCreateBuffer);
  LOAD_DEVICE_FUNC(vkDestroyBuffer);
  LOAD_DEVICE_FUNC(vkCreateBufferView);
  LOAD_DEVICE_FUNC(vkDestroyBufferView);
  LOAD_DEVICE_FUNC(vkCreateImage);
  LOAD_DEVICE_FUNC(vkDestroyImage);
  LOAD_DEVICE_FUNC(vkGetImageSubresourceLayout);
  LOAD_DEVICE_FUNC(vkCreateImageView);
  LOAD_DEVICE_FUNC(vkDestroyImageView);
  LOAD_DEVICE_FUNC(vkCreateShaderModule);
  LOAD_DEVICE_FUNC(vkDestroyShaderModule);
  LOAD_DEVICE_FUNC(vkCreatePipelineCache);
  LOAD_DEVICE_FUNC(vkDestroyPipelineCache);
  LOAD_DEVICE_FUNC(vkGetPipelineCacheData);
  LOAD_DEVICE_FUNC(vkMergePipelineCaches);
  LOAD_DEVICE_FUNC(vkCreateGraphicsPipelines);
  LOAD_DEVICE_FUNC(vkCreateComputePipelines);
  LOAD_DEVICE_FUNC(vkDestroyPipeline);
  LOAD_DEVICE_FUNC(vkCreatePipelineLayout);
  LOAD_DEVICE_FUNC(vkDestroyPipelineLayout);
  LOAD_DEVICE_FUNC(vkCreateSampler);
  LOAD_DEVICE_FUNC(vkDestroySampler);
  LOAD_DEVICE_FUNC(vkCreateDescriptorSetLayout);
  LOAD_DEVICE_FUNC(vkDestroyDescriptorSetLayout);
  LOAD_DEVICE_FUNC(vkCreateDescriptorPool);
  LOAD_DEVICE_FUNC(vkDestroyDescriptorPool);
  LOAD_DEVICE_FUNC(vkResetDescriptorPool);
  LOAD_DEVICE_FUNC(vkAllocateDescriptorSets);
  LOAD_DEVICE_FUNC(vkFreeDescriptorSets);
  LOAD_DEVICE_FUNC(vkUpdateDescriptorSets);
  LOAD_DEVICE_FUNC(vkCreateFramebuffer);
  LOAD_DEVICE_FUNC(vkDestroyFramebuffer);
  LOAD_DEVICE_FUNC(vkCreateRenderPass);
  LOAD_DEVICE_FUNC(vkDestroyRenderPass);
  LOAD_DEVICE_FUNC(vkGetRenderAreaGranularity);
  LOAD_DEVICE_FUNC(vkCreateCommandPool);
  LOAD_DEVICE_FUNC(vkDestroyCommandPool);
  LOAD_DEVICE_FUNC(vkResetCommandPool);
  LOAD_DEVICE_FUNC(vkAllocateCommandBuffers);
  LOAD_DEVICE_FUNC(vkFreeCommandBuffers);
  LOAD_DEVICE_FUNC(vkBeginCommandBuffer);
  LOAD_DEVICE_FUNC(vkEndCommandBuffer);
  LOAD_DEVICE_FUNC(vkResetCommandBuffer);
  LOAD_DEVICE_FUNC(vkCmdBindPipeline);
  LOAD_DEVICE_FUNC(vkCmdSetViewport);
  LOAD_DEVICE_FUNC(vkCmdSetScissor);
  LOAD_DEVICE_FUNC(vkCmdSetLineWidth);
  LOAD_DEVICE_FUNC(vkCmdSetDepthBias);
  LOAD_DEVICE_FUNC(vkCmdSetBlendConstants);
  LOAD_DEVICE_FUNC(vkCmdSetDepthBounds);
  LOAD_DEVICE_FUNC(vkCmdSetStencilCompareMask);
  LOAD_DEVICE_FUNC(vkCmdSetStencilWriteMask);
  LOAD_DEVICE_FUNC(vkCmdSetStencilReference);
  LOAD_DEVICE_FUNC(vkCmdBindDescriptorSets);
  LOAD_DEVICE_FUNC(vkCmdBindIndexBuffer);
  LOAD_DEVICE_FUNC(vkCmdBindVertexBuffers);
  LOAD_DEVICE_FUNC(vkCmdDraw);
  LOAD_DEVICE_FUNC(vkCmdDrawIndexed);
  LOAD_DEVICE_FUNC(vkCmdDrawIndirect);
  LOAD_DEVICE_FUNC(vkCmdDrawIndexedIndirect);
  LOAD_DEVICE_FUNC(vkCmdDispatch);
  LOAD_DEVICE_FUNC(vkCmdDispatchIndirect);
  LOAD_DEVICE_FUNC(vkCmdCopyBuffer);
  LOAD_DEVICE_FUNC(vkCmdCopyImage);
  LOAD_DEVICE_FUNC(vkCmdBlitImage);
  LOAD_DEVICE_FUNC(vkCmdCopyBufferToImage);
  LOAD_DEVICE_FUNC(vkCmdCopyImageToBuffer);
  LOAD_DEVICE_FUNC(vkCmdUpdateBuffer);
  LOAD_DEVICE_FUNC(vkCmdFillBuffer);
  LOAD_DEVICE_FUNC(vkCmdClearColorImage);
  LOAD_DEVICE_FUNC(vkCmdClearDepthStencilImage);
  LOAD_DEVICE_FUNC(vkCmdClearAttachments);
  LOAD_DEVICE_FUNC(vkCmdResolveImage);
  LOAD_DEVICE_FUNC(vkCmdSetEvent);
  LOAD_DEVICE_FUNC(vkCmdResetEvent);
  LOAD_DEVICE_FUNC(vkCmdWaitEvents);
  LOAD_DEVICE_FUNC(vkCmdPipelineBarrier);
  LOAD_DEVICE_FUNC(vkCmdBeginQuery);
  LOAD_DEVICE_FUNC(vkCmdEndQuery);
  LOAD_DEVICE_FUNC(vkCmdResetQueryPool);
  LOAD_DEVICE_FUNC(vkCmdWriteTimestamp);
  LOAD_DEVICE_FUNC(vkCmdCopyQueryPoolResults);
  LOAD_DEVICE_FUNC(vkCmdPushConstants);
  LOAD_DEVICE_FUNC(vkCmdBeginRenderPass);
  LOAD_DEVICE_FUNC(vkCmdNextSubpass);
  LOAD_DEVICE_FUNC(vkCmdEndRenderPass);
  LOAD_DEVICE_FUNC(vkCmdExecuteCommands);
}

void VulkanDeviceTable::vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyDevice_(device, pAllocator);
}

void VulkanDeviceTable::vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) const {
  return this->vkGetDeviceQueue_(device, queueFamilyIndex, queueIndex, pQueue);
}

VkResult VulkanDeviceTable::vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) const {
  return this->vkQueueSubmit_(queue, submitCount, pSubmits, fence);
}

VkResult VulkanDeviceTable::vkQueueWaitIdle(VkQueue queue) const {
  return this->vkQueueWaitIdle_(queue);
}

VkResult VulkanDeviceTable::vkDeviceWaitIdle(VkDevice device) const {
  return this->vkDeviceWaitIdle_(device);
}

VkResult VulkanDeviceTable::vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory) const {
  return this->vkAllocateMemory_(device, pAllocateInfo, pAllocator, pMemory);
}

void VulkanDeviceTable::vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator) const {
  return this->vkFreeMemory_(device, memory, pAllocator);
}

VkResult VulkanDeviceTable::vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) const {
  return this->vkMapMemory_(device, memory, offset, size, flags, ppData);
}

void VulkanDeviceTable::vkUnmapMemory(VkDevice device, VkDeviceMemory memory) const {
  return this->vkUnmapMemory_(device, memory);
}

VkResult VulkanDeviceTable::vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const {
  return this->vkFlushMappedMemoryRanges_(device, memoryRangeCount, pMemoryRanges);
}

VkResult VulkanDeviceTable::vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const {
  return this->vkInvalidateMappedMemoryRanges_(device, memoryRangeCount, pMemoryRanges);
}

void VulkanDeviceTable::vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes) const {
  return this->vkGetDeviceMemoryCommitment_(device, memory, pCommittedMemoryInBytes);
}

VkResult VulkanDeviceTable::vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) const {
  return this->vkBindBufferMemory_(device, buffer, memory, memoryOffset);
}

VkResult VulkanDeviceTable::vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) const {
  return this->vkBindImageMemory_(device, image, memory, memoryOffset);
}

void VulkanDeviceTable::vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements) const {
  return this->vkGetBufferMemoryRequirements_(device, buffer, pMemoryRequirements);
}

void VulkanDeviceTable::vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements) const {
  return this->vkGetImageMemoryRequirements_(device, image, pMemoryRequirements);
}

void VulkanDeviceTable::vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements) const {
  return this->vkGetImageSparseMemoryRequirements_(device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

VkResult VulkanDeviceTable::vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence) const {
  return this->vkQueueBindSparse_(queue, bindInfoCount, pBindInfo, fence);
}

VkResult VulkanDeviceTable::vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) const {
  return this->vkCreateFence_(device, pCreateInfo, pAllocator, pFence);
}

void VulkanDeviceTable::vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyFence_(device, fence, pAllocator);
}

VkResult VulkanDeviceTable::vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences) const {
  return this->vkResetFences_(device, fenceCount, pFences);
}

VkResult VulkanDeviceTable::vkGetFenceStatus(VkDevice device, VkFence fence) const {
  return this->vkGetFenceStatus_(device, fence);
}

VkResult VulkanDeviceTable::vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout) const {
  return this->vkWaitForFences_(device, fenceCount, pFences, waitAll, timeout);
}

VkResult VulkanDeviceTable::vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore) const {
  return this->vkCreateSemaphore_(device, pCreateInfo, pAllocator, pSemaphore);
}

void VulkanDeviceTable::vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroySemaphore_(device, semaphore, pAllocator);
}

VkResult VulkanDeviceTable::vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent) const {
  return this->vkCreateEvent_(device, pCreateInfo, pAllocator, pEvent);
}

void VulkanDeviceTable::vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyEvent_(device, event, pAllocator);
}

VkResult VulkanDeviceTable::vkGetEventStatus(VkDevice device, VkEvent event) const {
  return this->vkGetEventStatus_(device, event);
}

VkResult VulkanDeviceTable::vkSetEvent(VkDevice device, VkEvent event) const {
  return this->vkSetEvent_(device, event);
}

VkResult VulkanDeviceTable::vkResetEvent(VkDevice device, VkEvent event) const {
  return this->vkResetEvent_(device, event);
}

VkResult VulkanDeviceTable::vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool) const {
  return this->vkCreateQueryPool_(device, pCreateInfo, pAllocator, pQueryPool);
}

void VulkanDeviceTable::vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyQueryPool_(device, queryPool, pAllocator);
}

VkResult VulkanDeviceTable::vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags) const {
  return this->vkGetQueryPoolResults_(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

VkResult VulkanDeviceTable::vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer) const {
  return this->vkCreateBuffer_(device, pCreateInfo, pAllocator, pBuffer);
}

void VulkanDeviceTable::vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyBuffer_(device, buffer, pAllocator);
}

VkResult VulkanDeviceTable::vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView) const {
  return this->vkCreateBufferView_(device, pCreateInfo, pAllocator, pView);
}

void VulkanDeviceTable::vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyBufferView_(device, bufferView, pAllocator);
}

VkResult VulkanDeviceTable::vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) const {
  return this->vkCreateImage_(device, pCreateInfo, pAllocator, pImage);
}

void VulkanDeviceTable::vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyImage_(device, image, pAllocator);
}

void VulkanDeviceTable::vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout) const {
  return this->vkGetImageSubresourceLayout_(device, image, pSubresource, pLayout);
}

VkResult VulkanDeviceTable::vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) const {
  return this->vkCreateImageView_(device, pCreateInfo, pAllocator, pView);
}

void VulkanDeviceTable::vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyImageView_(device, imageView, pAllocator);
}

VkResult VulkanDeviceTable::vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) const {
  return this->vkCreateShaderModule_(device, pCreateInfo, pAllocator, pShaderModule);
}

void VulkanDeviceTable::vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyShaderModule_(device, shaderModule, pAllocator);
}

VkResult VulkanDeviceTable::vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache) const {
  return this->vkCreatePipelineCache_(device, pCreateInfo, pAllocator, pPipelineCache);
}

void VulkanDeviceTable::vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyPipelineCache_(device, pipelineCache, pAllocator);
}

VkResult VulkanDeviceTable::vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t* pDataSize, void* pData) const {
  return this->vkGetPipelineCacheData_(device, pipelineCache, pDataSize, pData);
}

VkResult VulkanDeviceTable::vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches) const {
  return this->vkMergePipelineCaches_(device, dstCache, srcCacheCount, pSrcCaches);
}

VkResult VulkanDeviceTable::vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) const {
  return this->vkCreateGraphicsPipelines_(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VkResult VulkanDeviceTable::vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) const {
  return this->vkCreateComputePipelines_(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

void VulkanDeviceTable::vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyPipeline_(device, pipeline, pAllocator);
}

VkResult VulkanDeviceTable::vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout) const {
  return this->vkCreatePipelineLayout_(device, pCreateInfo, pAllocator, pPipelineLayout);
}

void VulkanDeviceTable::vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyPipelineLayout_(device, pipelineLayout, pAllocator);
}

VkResult VulkanDeviceTable::vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler) const {
  return this->vkCreateSampler_(device, pCreateInfo, pAllocator, pSampler);
}

void VulkanDeviceTable::vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroySampler_(device, sampler, pAllocator);
}

VkResult VulkanDeviceTable::vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout) const {
  return this->vkCreateDescriptorSetLayout_(device, pCreateInfo, pAllocator, pSetLayout);
}

void VulkanDeviceTable::vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyDescriptorSetLayout_(device, descriptorSetLayout, pAllocator);
}

VkResult VulkanDeviceTable::vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool) const {
  return this->vkCreateDescriptorPool_(device, pCreateInfo, pAllocator, pDescriptorPool);
}

void VulkanDeviceTable::vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyDescriptorPool_(device, descriptorPool, pAllocator);
}

VkResult VulkanDeviceTable::vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) const {
  return this->vkResetDescriptorPool_(device, descriptorPool, flags);
}

VkResult VulkanDeviceTable::vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) const {
  return this->vkAllocateDescriptorSets_(device, pAllocateInfo, pDescriptorSets);
}

VkResult VulkanDeviceTable::vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) const {
  return this->vkFreeDescriptorSets_(device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

void VulkanDeviceTable::vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) const {
  return this->vkUpdateDescriptorSets_(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VkResult VulkanDeviceTable::vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer) const {
  return this->vkCreateFramebuffer_(device, pCreateInfo, pAllocator, pFramebuffer);
}

void VulkanDeviceTable::vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyFramebuffer_(device, framebuffer, pAllocator);
}

VkResult VulkanDeviceTable::vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) const {
  return this->vkCreateRenderPass_(device, pCreateInfo, pAllocator, pRenderPass);
}

void VulkanDeviceTable::vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyRenderPass_(device, renderPass, pAllocator);
}

void VulkanDeviceTable::vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity) const {
  return this->vkGetRenderAreaGranularity_(device, renderPass, pGranularity);
}

VkResult VulkanDeviceTable::vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool) const {
  return this->vkCreateCommandPool_(device, pCreateInfo, pAllocator, pCommandPool);
}

void VulkanDeviceTable::vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyCommandPool_(device, commandPool, pAllocator);
}

VkResult VulkanDeviceTable::vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) const {
  return this->vkResetCommandPool_(device, commandPool, flags);
}

VkResult VulkanDeviceTable::vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) const {
  return this->vkAllocateCommandBuffers_(device, pAllocateInfo, pCommandBuffers);
}

void VulkanDeviceTable::vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const {
  return this->vkFreeCommandBuffers_(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult VulkanDeviceTable::vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) const {
  return this->vkBeginCommandBuffer_(commandBuffer, pBeginInfo);
}

VkResult VulkanDeviceTable::vkEndCommandBuffer(VkCommandBuffer commandBuffer) const {
  return this->vkEndCommandBuffer_(commandBuffer);
}

VkResult VulkanDeviceTable::vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) const {
  return this->vkResetCommandBuffer_(commandBuffer, flags);
}

void VulkanDeviceTable::vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const {
  return this->vkCmdBindPipeline_(commandBuffer, pipelineBindPoint, pipeline);
}

void VulkanDeviceTable::vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) const {
  return this->vkCmdSetViewport_(commandBuffer, firstViewport, viewportCount, pViewports);
}

void VulkanDeviceTable::vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) const {
  return this->vkCmdSetScissor_(commandBuffer, firstScissor, scissorCount, pScissors);
}

void VulkanDeviceTable::vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) const {
  return this->vkCmdSetLineWidth_(commandBuffer, lineWidth);
}

void VulkanDeviceTable::vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) const {
  return this->vkCmdSetDepthBias_(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

void VulkanDeviceTable::vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) const {
  return this->vkCmdSetBlendConstants_(commandBuffer, blendConstants);
}

void VulkanDeviceTable::vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) const {
  return this->vkCmdSetDepthBounds_(commandBuffer, minDepthBounds, maxDepthBounds);
}

void VulkanDeviceTable::vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask) const {
  return this->vkCmdSetStencilCompareMask_(commandBuffer, faceMask, compareMask);
}

void VulkanDeviceTable::vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) const {
  return this->vkCmdSetStencilWriteMask_(commandBuffer, faceMask, writeMask);
}

void VulkanDeviceTable::vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) const {
  return this->vkCmdSetStencilReference_(commandBuffer, faceMask, reference);
}

void VulkanDeviceTable::vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) const {
  return this->vkCmdBindDescriptorSets_(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

void VulkanDeviceTable::vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) const {
  return this->vkCmdBindIndexBuffer_(commandBuffer, buffer, offset, indexType);
}

void VulkanDeviceTable::vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) const {
  return this->vkCmdBindVertexBuffers_(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

void VulkanDeviceTable::vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const {
  return this->vkCmdDraw_(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanDeviceTable::vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const {
  return this->vkCmdDrawIndexed_(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanDeviceTable::vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) const {
  return this->vkCmdDrawIndirect_(commandBuffer, buffer, offset, drawCount, stride);
}

void VulkanDeviceTable::vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) const {
  return this->vkCmdDrawIndexedIndirect_(commandBuffer, buffer, offset, drawCount, stride);
}

void VulkanDeviceTable::vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const {
  return this->vkCmdDispatch_(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VulkanDeviceTable::vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) const {
  return this->vkCmdDispatchIndirect_(commandBuffer, buffer, offset);
}

void VulkanDeviceTable::vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const {
  return this->vkCmdCopyBuffer_(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

void VulkanDeviceTable::vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions) const {
  return this->vkCmdCopyImage_(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void VulkanDeviceTable::vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) const {
  return this->vkCmdBlitImage_(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

void VulkanDeviceTable::vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const {
  return this->vkCmdCopyBufferToImage_(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

void VulkanDeviceTable::vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions) const {
  return this->vkCmdCopyImageToBuffer_(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

void VulkanDeviceTable::vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData) const {
  return this->vkCmdUpdateBuffer_(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

void VulkanDeviceTable::vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) const {
  return this->vkCmdFillBuffer_(commandBuffer, dstBuffer, dstOffset, size, data);
}

void VulkanDeviceTable::vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) const {
  return this->vkCmdClearColorImage_(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

void VulkanDeviceTable::vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) const {
  return this->vkCmdClearDepthStencilImage_(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

void VulkanDeviceTable::vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects) const {
  return this->vkCmdClearAttachments_(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

void VulkanDeviceTable::vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) const {
  return this->vkCmdResolveImage_(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

void VulkanDeviceTable::vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) const {
  return this->vkCmdSetEvent_(commandBuffer, event, stageMask);
}

void VulkanDeviceTable::vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) const {
  return this->vkCmdResetEvent_(commandBuffer, event, stageMask);
}

void VulkanDeviceTable::vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) const {
  return this->vkCmdWaitEvents_(commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void VulkanDeviceTable::vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) const {
  return this->vkCmdPipelineBarrier_(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

void VulkanDeviceTable::vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) const {
  return this->vkCmdBeginQuery_(commandBuffer, queryPool, query, flags);
}

void VulkanDeviceTable::vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query) const {
  return this->vkCmdEndQuery_(commandBuffer, queryPool, query);
}

void VulkanDeviceTable::vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) const {
  return this->vkCmdResetQueryPool_(commandBuffer, queryPool, firstQuery, queryCount);
}

void VulkanDeviceTable::vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query) const {
  return this->vkCmdWriteTimestamp_(commandBuffer, pipelineStage, queryPool, query);
}

void VulkanDeviceTable::vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) const {
  return this->vkCmdCopyQueryPoolResults_(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

void VulkanDeviceTable::vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues) const {
  return this->vkCmdPushConstants_(commandBuffer, layout, stageFlags, offset, size, pValues);
}

void VulkanDeviceTable::vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) const {
  return this->vkCmdBeginRenderPass_(commandBuffer, pRenderPassBegin, contents);
}

void VulkanDeviceTable::vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) const {
  return this->vkCmdNextSubpass_(commandBuffer, contents);
}

void VulkanDeviceTable::vkCmdEndRenderPass(VkCommandBuffer commandBuffer) const {
  return this->vkCmdEndRenderPass_(commandBuffer);
}

void VulkanDeviceTable::vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const {
  return this->vkCmdExecuteCommands_(commandBuffer, commandBufferCount, pCommandBuffers);
}

#undef LOAD_INSTANCE_FUNC
#define LOAD_INSTANCE_FUNC(fun)\
  this->##fun##_ = reinterpret_cast<PFN_##fun>(globals->vkGetInstanceProcAddr(\
      instance->instance(), #fun));\
  if (!this->##fun##_) {\
    throw VulkanProcNotFound(#fun);\
  }

#undef LOAD_DEVICE_FUNC
#define LOAD_DEVICE_FUNC(fun)\
  this->##fun##_ = reinterpret_cast<PFN_##fun>(instance->vkGetDeviceProcAddr(\
      device->device(), #fun));\
  if (!this->##fun##_) {\
    throw VulkanProcNotFound(#fun);\
  }

/*
 * ------------------------------------------------------
 * VK_KHR_surface
 * ------------------------------------------------------
*/

VK_KHR_surface::VK_KHR_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkDestroySurfaceKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
}

void VK_KHR_surface::vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroySurfaceKHR_(instance, surface, pAllocator);
}

VkResult VK_KHR_surface::vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported) const {
  return this->vkGetPhysicalDeviceSurfaceSupportKHR_(physicalDevice, queueFamilyIndex, surface, pSupported);
}

VkResult VK_KHR_surface::vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) const {
  return this->vkGetPhysicalDeviceSurfaceCapabilitiesKHR_(physicalDevice, surface, pSurfaceCapabilities);
}

VkResult VK_KHR_surface::vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats) const {
  return this->vkGetPhysicalDeviceSurfaceFormatsKHR_(physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VkResult VK_KHR_surface::vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) const {
  return this->vkGetPhysicalDeviceSurfacePresentModesKHR_(physicalDevice, surface, pPresentModeCount, pPresentModes);
}

/*
 * ------------------------------------------------------
 * VK_KHR_swapchain
 * ------------------------------------------------------
*/

VK_KHR_swapchain::VK_KHR_swapchain(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkCreateSwapchainKHR);
  LOAD_DEVICE_FUNC(vkDestroySwapchainKHR);
  LOAD_DEVICE_FUNC(vkGetSwapchainImagesKHR);
  LOAD_DEVICE_FUNC(vkAcquireNextImageKHR);
  LOAD_DEVICE_FUNC(vkQueuePresentKHR);
}

VkResult VK_KHR_swapchain::vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) const {
  return this->vkCreateSwapchainKHR_(device, pCreateInfo, pAllocator, pSwapchain);
}

void VK_KHR_swapchain::vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroySwapchainKHR_(device, swapchain, pAllocator);
}

VkResult VK_KHR_swapchain::vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages) const {
  return this->vkGetSwapchainImagesKHR_(device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult VK_KHR_swapchain::vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) const {
  return this->vkAcquireNextImageKHR_(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult VK_KHR_swapchain::vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) const {
  return this->vkQueuePresentKHR_(queue, pPresentInfo);
}

/*
 * ------------------------------------------------------
 * VK_KHR_display
 * ------------------------------------------------------
*/

VK_KHR_display::VK_KHR_display(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceDisplayPropertiesKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceDisplayPlanePropertiesKHR);
  LOAD_INSTANCE_FUNC(vkGetDisplayPlaneSupportedDisplaysKHR);
  LOAD_INSTANCE_FUNC(vkGetDisplayModePropertiesKHR);
  LOAD_INSTANCE_FUNC(vkCreateDisplayModeKHR);
  LOAD_INSTANCE_FUNC(vkGetDisplayPlaneCapabilitiesKHR);
  LOAD_INSTANCE_FUNC(vkCreateDisplayPlaneSurfaceKHR);
}

VkResult VK_KHR_display::vkGetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkDisplayPropertiesKHR* pProperties) const {
  return this->vkGetPhysicalDeviceDisplayPropertiesKHR_(physicalDevice, pPropertyCount, pProperties);
}

VkResult VK_KHR_display::vkGetPhysicalDeviceDisplayPlanePropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkDisplayPlanePropertiesKHR* pProperties) const {
  return this->vkGetPhysicalDeviceDisplayPlanePropertiesKHR_(physicalDevice, pPropertyCount, pProperties);
}

VkResult VK_KHR_display::vkGetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice, uint32_t planeIndex, uint32_t* pDisplayCount, VkDisplayKHR* pDisplays) const {
  return this->vkGetDisplayPlaneSupportedDisplaysKHR_(physicalDevice, planeIndex, pDisplayCount, pDisplays);
}

VkResult VK_KHR_display::vkGetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t* pPropertyCount, VkDisplayModePropertiesKHR* pProperties) const {
  return this->vkGetDisplayModePropertiesKHR_(physicalDevice, display, pPropertyCount, pProperties);
}

VkResult VK_KHR_display::vkCreateDisplayModeKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, const VkDisplayModeCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDisplayModeKHR* pMode) const {
  return this->vkCreateDisplayModeKHR_(physicalDevice, display, pCreateInfo, pAllocator, pMode);
}

VkResult VK_KHR_display::vkGetDisplayPlaneCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkDisplayModeKHR mode, uint32_t planeIndex, VkDisplayPlaneCapabilitiesKHR* pCapabilities) const {
  return this->vkGetDisplayPlaneCapabilitiesKHR_(physicalDevice, mode, planeIndex, pCapabilities);
}

VkResult VK_KHR_display::vkCreateDisplayPlaneSurfaceKHR(VkInstance instance, const VkDisplaySurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateDisplayPlaneSurfaceKHR_(instance, pCreateInfo, pAllocator, pSurface);
}

/*
 * ------------------------------------------------------
 * VK_KHR_display_swapchain
 * ------------------------------------------------------
*/

VK_KHR_display_swapchain::VK_KHR_display_swapchain(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkCreateSharedSwapchainsKHR);
}

VkResult VK_KHR_display_swapchain::vkCreateSharedSwapchainsKHR(VkDevice device, uint32_t swapchainCount, const VkSwapchainCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchains) const {
  return this->vkCreateSharedSwapchainsKHR_(device, swapchainCount, pCreateInfos, pAllocator, pSwapchains);
}

/*
 * ------------------------------------------------------
 * VK_KHR_xlib_surface
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_XLIB_KHR)

VK_KHR_xlib_surface::VK_KHR_xlib_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateXlibSurfaceKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceXlibPresentationSupportKHR);
}

VkResult VK_KHR_xlib_surface::vkCreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateXlibSurfaceKHR_(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 VK_KHR_xlib_surface::vkGetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, ?* dpy, VisualID visualID) const {
  return this->vkGetPhysicalDeviceXlibPresentationSupportKHR_(physicalDevice, queueFamilyIndex, dpy, visualID);
}

#endif // VK_USE_PLATFORM_XLIB_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_xcb_surface
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_XCB_KHR)

VK_KHR_xcb_surface::VK_KHR_xcb_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateXcbSurfaceKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceXcbPresentationSupportKHR);
}

VkResult VK_KHR_xcb_surface::vkCreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateXcbSurfaceKHR_(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 VK_KHR_xcb_surface::vkGetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, ?* connection, xcb_visualid_t visual_id) const {
  return this->vkGetPhysicalDeviceXcbPresentationSupportKHR_(physicalDevice, queueFamilyIndex, connection, visual_id);
}

#endif // VK_USE_PLATFORM_XCB_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_wayland_surface
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)

VK_KHR_wayland_surface::VK_KHR_wayland_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateWaylandSurfaceKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceWaylandPresentationSupportKHR);
}

VkResult VK_KHR_wayland_surface::vkCreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateWaylandSurfaceKHR_(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 VK_KHR_wayland_surface::vkGetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, ?* display) const {
  return this->vkGetPhysicalDeviceWaylandPresentationSupportKHR_(physicalDevice, queueFamilyIndex, display);
}

#endif // VK_USE_PLATFORM_WAYLAND_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_mir_surface
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_MIR_KHR)

VK_KHR_mir_surface::VK_KHR_mir_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateMirSurfaceKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceMirPresentationSupportKHR);
}

VkResult VK_KHR_mir_surface::vkCreateMirSurfaceKHR(VkInstance instance, const VkMirSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateMirSurfaceKHR_(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 VK_KHR_mir_surface::vkGetPhysicalDeviceMirPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, ?* connection) const {
  return this->vkGetPhysicalDeviceMirPresentationSupportKHR_(physicalDevice, queueFamilyIndex, connection);
}

#endif // VK_USE_PLATFORM_MIR_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_android_surface
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_ANDROID_KHR)

VK_KHR_android_surface::VK_KHR_android_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateAndroidSurfaceKHR);
}

VkResult VK_KHR_android_surface::vkCreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateAndroidSurfaceKHR_(instance, pCreateInfo, pAllocator, pSurface);
}

#endif // VK_USE_PLATFORM_ANDROID_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_win32_surface
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_WIN32_KHR)

VK_KHR_win32_surface::VK_KHR_win32_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateWin32SurfaceKHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceWin32PresentationSupportKHR);
}

VkResult VK_KHR_win32_surface::vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateWin32SurfaceKHR_(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 VK_KHR_win32_surface::vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) const {
  return this->vkGetPhysicalDeviceWin32PresentationSupportKHR_(physicalDevice, queueFamilyIndex);
}

#endif // VK_USE_PLATFORM_WIN32_KHR

/*
 * ------------------------------------------------------
 * VK_EXT_debug_report
 * ------------------------------------------------------
*/

VK_EXT_debug_report::VK_EXT_debug_report(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateDebugReportCallbackEXT);
  LOAD_INSTANCE_FUNC(vkDestroyDebugReportCallbackEXT);
  LOAD_INSTANCE_FUNC(vkDebugReportMessageEXT);
}

VkResult VK_EXT_debug_report::vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) const {
  return this->vkCreateDebugReportCallbackEXT_(instance, pCreateInfo, pAllocator, pCallback);
}

void VK_EXT_debug_report::vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyDebugReportCallbackEXT_(instance, callback, pAllocator);
}

void VK_EXT_debug_report::vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage) const {
  return this->vkDebugReportMessageEXT_(instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
}

/*
 * ------------------------------------------------------
 * VK_NV_glsl_shader
 * ------------------------------------------------------
*/

VK_NV_glsl_shader::VK_NV_glsl_shader(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_EXT_depth_range_unrestricted
 * ------------------------------------------------------
*/

VK_EXT_depth_range_unrestricted::VK_EXT_depth_range_unrestricted(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_sampler_mirror_clamp_to_edge
 * ------------------------------------------------------
*/

VK_KHR_sampler_mirror_clamp_to_edge::VK_KHR_sampler_mirror_clamp_to_edge(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_IMG_filter_cubic
 * ------------------------------------------------------
*/

VK_IMG_filter_cubic::VK_IMG_filter_cubic(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_AMD_rasterization_order
 * ------------------------------------------------------
*/

VK_AMD_rasterization_order::VK_AMD_rasterization_order(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_AMD_shader_trinary_minmax
 * ------------------------------------------------------
*/

VK_AMD_shader_trinary_minmax::VK_AMD_shader_trinary_minmax(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_AMD_shader_explicit_vertex_parameter
 * ------------------------------------------------------
*/

VK_AMD_shader_explicit_vertex_parameter::VK_AMD_shader_explicit_vertex_parameter(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_EXT_debug_marker
 * ------------------------------------------------------
*/

VK_EXT_debug_marker::VK_EXT_debug_marker(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkDebugMarkerSetObjectTagEXT);
  LOAD_DEVICE_FUNC(vkDebugMarkerSetObjectNameEXT);
  LOAD_DEVICE_FUNC(vkCmdDebugMarkerBeginEXT);
  LOAD_DEVICE_FUNC(vkCmdDebugMarkerEndEXT);
  LOAD_DEVICE_FUNC(vkCmdDebugMarkerInsertEXT);
}

VkResult VK_EXT_debug_marker::vkDebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT* pTagInfo) const {
  return this->vkDebugMarkerSetObjectTagEXT_(device, pTagInfo);
}

VkResult VK_EXT_debug_marker::vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT* pNameInfo) const {
  return this->vkDebugMarkerSetObjectNameEXT_(device, pNameInfo);
}

void VK_EXT_debug_marker::vkCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const {
  return this->vkCmdDebugMarkerBeginEXT_(commandBuffer, pMarkerInfo);
}

void VK_EXT_debug_marker::vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) const {
  return this->vkCmdDebugMarkerEndEXT_(commandBuffer);
}

void VK_EXT_debug_marker::vkCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const {
  return this->vkCmdDebugMarkerInsertEXT_(commandBuffer, pMarkerInfo);
}

/*
 * ------------------------------------------------------
 * VK_AMD_gcn_shader
 * ------------------------------------------------------
*/

VK_AMD_gcn_shader::VK_AMD_gcn_shader(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NV_dedicated_allocation
 * ------------------------------------------------------
*/

VK_NV_dedicated_allocation::VK_NV_dedicated_allocation(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_AMD_draw_indirect_count
 * ------------------------------------------------------
*/

VK_AMD_draw_indirect_count::VK_AMD_draw_indirect_count(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkCmdDrawIndirectCountAMD);
  LOAD_DEVICE_FUNC(vkCmdDrawIndexedIndirectCountAMD);
}

void VK_AMD_draw_indirect_count::vkCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) const {
  return this->vkCmdDrawIndirectCountAMD_(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void VK_AMD_draw_indirect_count::vkCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) const {
  return this->vkCmdDrawIndexedIndirectCountAMD_(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

/*
 * ------------------------------------------------------
 * VK_AMD_negative_viewport_height
 * ------------------------------------------------------
*/

VK_AMD_negative_viewport_height::VK_AMD_negative_viewport_height(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_AMD_gpu_shader_half_float
 * ------------------------------------------------------
*/

VK_AMD_gpu_shader_half_float::VK_AMD_gpu_shader_half_float(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_AMD_shader_ballot
 * ------------------------------------------------------
*/

VK_AMD_shader_ballot::VK_AMD_shader_ballot(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_AMD_texture_gather_bias_lod
 * ------------------------------------------------------
*/

VK_AMD_texture_gather_bias_lod::VK_AMD_texture_gather_bias_lod(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHX_multiview
 * ------------------------------------------------------
*/

VK_KHX_multiview::VK_KHX_multiview(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_IMG_format_pvrtc
 * ------------------------------------------------------
*/

VK_IMG_format_pvrtc::VK_IMG_format_pvrtc(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NV_external_memory_capabilities
 * ------------------------------------------------------
*/

VK_NV_external_memory_capabilities::VK_NV_external_memory_capabilities(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceExternalImageFormatPropertiesNV);
}

VkResult VK_NV_external_memory_capabilities::vkGetPhysicalDeviceExternalImageFormatPropertiesNV(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkExternalMemoryHandleTypeFlagsNV externalHandleType, VkExternalImageFormatPropertiesNV* pExternalImageFormatProperties) const {
  return this->vkGetPhysicalDeviceExternalImageFormatPropertiesNV_(physicalDevice, format, type, tiling, usage, flags, externalHandleType, pExternalImageFormatProperties);
}

/*
 * ------------------------------------------------------
 * VK_NV_external_memory
 * ------------------------------------------------------
*/

VK_NV_external_memory::VK_NV_external_memory(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NV_external_memory_win32
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_WIN32_KHR)

VK_NV_external_memory_win32::VK_NV_external_memory_win32(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkGetMemoryWin32HandleNV);
}

VkResult VK_NV_external_memory_win32::vkGetMemoryWin32HandleNV(VkDevice device, VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE* pHandle) const {
  return this->vkGetMemoryWin32HandleNV_(device, memory, handleType, pHandle);
}

#endif // VK_USE_PLATFORM_WIN32_KHR

/*
 * ------------------------------------------------------
 * VK_NV_win32_keyed_mutex
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_WIN32_KHR)

VK_NV_win32_keyed_mutex::VK_NV_win32_keyed_mutex(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

#endif // VK_USE_PLATFORM_WIN32_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_get_physical_device_properties2
 * ------------------------------------------------------
*/

VK_KHR_get_physical_device_properties2::VK_KHR_get_physical_device_properties2(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFeatures2KHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceProperties2KHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFormatProperties2KHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceImageFormatProperties2KHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceQueueFamilyProperties2KHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties2KHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSparseImageFormatProperties2KHR);
}

void VK_KHR_get_physical_device_properties2::vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2KHR* pFeatures) const {
  return this->vkGetPhysicalDeviceFeatures2KHR_(physicalDevice, pFeatures);
}

void VK_KHR_get_physical_device_properties2::vkGetPhysicalDeviceProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2KHR* pProperties) const {
  return this->vkGetPhysicalDeviceProperties2KHR_(physicalDevice, pProperties);
}

void VK_KHR_get_physical_device_properties2::vkGetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2KHR* pFormatProperties) const {
  return this->vkGetPhysicalDeviceFormatProperties2KHR_(physicalDevice, format, pFormatProperties);
}

VkResult VK_KHR_get_physical_device_properties2::vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2KHR* pImageFormatInfo, VkImageFormatProperties2KHR* pImageFormatProperties) const {
  return this->vkGetPhysicalDeviceImageFormatProperties2KHR_(physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

void VK_KHR_get_physical_device_properties2::vkGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties2KHR* pQueueFamilyProperties) const {
  return this->vkGetPhysicalDeviceQueueFamilyProperties2KHR_(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

void VK_KHR_get_physical_device_properties2::vkGetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2KHR* pMemoryProperties) const {
  return this->vkGetPhysicalDeviceMemoryProperties2KHR_(physicalDevice, pMemoryProperties);
}

void VK_KHR_get_physical_device_properties2::vkGetPhysicalDeviceSparseImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2KHR* pFormatInfo, uint32_t* pPropertyCount, VkSparseImageFormatProperties2KHR* pProperties) const {
  return this->vkGetPhysicalDeviceSparseImageFormatProperties2KHR_(physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}

/*
 * ------------------------------------------------------
 * VK_KHX_device_group
 * ------------------------------------------------------
*/

VK_KHX_device_group::VK_KHX_device_group(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkGetDeviceGroupPeerMemoryFeaturesKHX);
  LOAD_DEVICE_FUNC(vkBindBufferMemory2KHX);
  LOAD_DEVICE_FUNC(vkBindImageMemory2KHX);
  LOAD_DEVICE_FUNC(vkCmdSetDeviceMaskKHX);
  LOAD_DEVICE_FUNC(vkGetDeviceGroupPresentCapabilitiesKHX);
  LOAD_DEVICE_FUNC(vkGetDeviceGroupSurfacePresentModesKHX);
  LOAD_DEVICE_FUNC(vkAcquireNextImage2KHX);
  LOAD_DEVICE_FUNC(vkCmdDispatchBaseKHX);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDevicePresentRectanglesKHX);
}

void VK_KHX_device_group::vkGetDeviceGroupPeerMemoryFeaturesKHX(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlagsKHX* pPeerMemoryFeatures) const {
  return this->vkGetDeviceGroupPeerMemoryFeaturesKHX_(device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

VkResult VK_KHX_device_group::vkBindBufferMemory2KHX(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfoKHX* pBindInfos) const {
  return this->vkBindBufferMemory2KHX_(device, bindInfoCount, pBindInfos);
}

VkResult VK_KHX_device_group::vkBindImageMemory2KHX(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfoKHX* pBindInfos) const {
  return this->vkBindImageMemory2KHX_(device, bindInfoCount, pBindInfos);
}

void VK_KHX_device_group::vkCmdSetDeviceMaskKHX(VkCommandBuffer commandBuffer, uint32_t deviceMask) const {
  return this->vkCmdSetDeviceMaskKHX_(commandBuffer, deviceMask);
}

VkResult VK_KHX_device_group::vkGetDeviceGroupPresentCapabilitiesKHX(VkDevice device, VkDeviceGroupPresentCapabilitiesKHX* pDeviceGroupPresentCapabilities) const {
  return this->vkGetDeviceGroupPresentCapabilitiesKHX_(device, pDeviceGroupPresentCapabilities);
}

VkResult VK_KHX_device_group::vkGetDeviceGroupSurfacePresentModesKHX(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHX* pModes) const {
  return this->vkGetDeviceGroupSurfacePresentModesKHX_(device, surface, pModes);
}

VkResult VK_KHX_device_group::vkAcquireNextImage2KHX(VkDevice device, const VkAcquireNextImageInfoKHX* pAcquireInfo, uint32_t* pImageIndex) const {
  return this->vkAcquireNextImage2KHX_(device, pAcquireInfo, pImageIndex);
}

void VK_KHX_device_group::vkCmdDispatchBaseKHX(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const {
  return this->vkCmdDispatchBaseKHX_(commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

VkResult VK_KHX_device_group::vkGetPhysicalDevicePresentRectanglesKHX(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pRectCount, VkRect2D* pRects) const {
  return this->vkGetPhysicalDevicePresentRectanglesKHX_(physicalDevice, surface, pRectCount, pRects);
}

/*
 * ------------------------------------------------------
 * VK_EXT_validation_flags
 * ------------------------------------------------------
*/

VK_EXT_validation_flags::VK_EXT_validation_flags(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
}

/*
 * ------------------------------------------------------
 * VK_NN_vi_surface
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_VI_NN)

VK_NN_vi_surface::VK_NN_vi_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateViSurfaceNN);
}

VkResult VK_NN_vi_surface::vkCreateViSurfaceNN(VkInstance instance, const VkViSurfaceCreateInfoNN* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateViSurfaceNN_(instance, pCreateInfo, pAllocator, pSurface);
}

#endif // VK_USE_PLATFORM_VI_NN

/*
 * ------------------------------------------------------
 * VK_KHR_shader_draw_parameters
 * ------------------------------------------------------
*/

VK_KHR_shader_draw_parameters::VK_KHR_shader_draw_parameters(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_EXT_shader_subgroup_ballot
 * ------------------------------------------------------
*/

VK_EXT_shader_subgroup_ballot::VK_EXT_shader_subgroup_ballot(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_EXT_shader_subgroup_vote
 * ------------------------------------------------------
*/

VK_EXT_shader_subgroup_vote::VK_EXT_shader_subgroup_vote(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_maintenance1
 * ------------------------------------------------------
*/

VK_KHR_maintenance1::VK_KHR_maintenance1(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkTrimCommandPoolKHR);
}

void VK_KHR_maintenance1::vkTrimCommandPoolKHR(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlagsKHR flags) const {
  return this->vkTrimCommandPoolKHR_(device, commandPool, flags);
}

/*
 * ------------------------------------------------------
 * VK_KHX_device_group_creation
 * ------------------------------------------------------
*/

VK_KHX_device_group_creation::VK_KHX_device_group_creation(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkEnumeratePhysicalDeviceGroupsKHX);
}

VkResult VK_KHX_device_group_creation::vkEnumeratePhysicalDeviceGroupsKHX(VkInstance instance, uint32_t* pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupPropertiesKHX* pPhysicalDeviceGroupProperties) const {
  return this->vkEnumeratePhysicalDeviceGroupsKHX_(instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
}

/*
 * ------------------------------------------------------
 * VK_KHR_external_memory_capabilities
 * ------------------------------------------------------
*/

VK_KHR_external_memory_capabilities::VK_KHR_external_memory_capabilities(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceExternalBufferPropertiesKHR);
}

void VK_KHR_external_memory_capabilities::vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfoKHR* pExternalBufferInfo, VkExternalBufferPropertiesKHR* pExternalBufferProperties) const {
  return this->vkGetPhysicalDeviceExternalBufferPropertiesKHR_(physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

/*
 * ------------------------------------------------------
 * VK_KHR_external_memory
 * ------------------------------------------------------
*/

VK_KHR_external_memory::VK_KHR_external_memory(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_external_memory_win32
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_WIN32_KHR)

VK_KHR_external_memory_win32::VK_KHR_external_memory_win32(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkGetMemoryWin32HandleKHR);
  LOAD_DEVICE_FUNC(vkGetMemoryWin32HandlePropertiesKHR);
}

VkResult VK_KHR_external_memory_win32::vkGetMemoryWin32HandleKHR(VkDevice device, const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) const {
  return this->vkGetMemoryWin32HandleKHR_(device, pGetWin32HandleInfo, pHandle);
}

VkResult VK_KHR_external_memory_win32::vkGetMemoryWin32HandlePropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBitsKHR handleType, HANDLE handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties) const {
  return this->vkGetMemoryWin32HandlePropertiesKHR_(device, handleType, handle, pMemoryWin32HandleProperties);
}

#endif // VK_USE_PLATFORM_WIN32_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_external_memory_fd
 * ------------------------------------------------------
*/

VK_KHR_external_memory_fd::VK_KHR_external_memory_fd(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkGetMemoryFdKHR);
  LOAD_DEVICE_FUNC(vkGetMemoryFdPropertiesKHR);
}

VkResult VK_KHR_external_memory_fd::vkGetMemoryFdKHR(VkDevice device, const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd) const {
  return this->vkGetMemoryFdKHR_(device, pGetFdInfo, pFd);
}

VkResult VK_KHR_external_memory_fd::vkGetMemoryFdPropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBitsKHR handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties) const {
  return this->vkGetMemoryFdPropertiesKHR_(device, handleType, fd, pMemoryFdProperties);
}

/*
 * ------------------------------------------------------
 * VK_KHR_win32_keyed_mutex
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_WIN32_KHR)

VK_KHR_win32_keyed_mutex::VK_KHR_win32_keyed_mutex(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

#endif // VK_USE_PLATFORM_WIN32_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_external_semaphore_capabilities
 * ------------------------------------------------------
*/

VK_KHR_external_semaphore_capabilities::VK_KHR_external_semaphore_capabilities(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceExternalSemaphorePropertiesKHR);
}

void VK_KHR_external_semaphore_capabilities::vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfoKHR* pExternalSemaphoreInfo, VkExternalSemaphorePropertiesKHR* pExternalSemaphoreProperties) const {
  return this->vkGetPhysicalDeviceExternalSemaphorePropertiesKHR_(physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}

/*
 * ------------------------------------------------------
 * VK_KHR_external_semaphore
 * ------------------------------------------------------
*/

VK_KHR_external_semaphore::VK_KHR_external_semaphore(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_external_semaphore_win32
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_WIN32_KHR)

VK_KHR_external_semaphore_win32::VK_KHR_external_semaphore_win32(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkImportSemaphoreWin32HandleKHR);
  LOAD_DEVICE_FUNC(vkGetSemaphoreWin32HandleKHR);
}

VkResult VK_KHR_external_semaphore_win32::vkImportSemaphoreWin32HandleKHR(VkDevice device, const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo) const {
  return this->vkImportSemaphoreWin32HandleKHR_(device, pImportSemaphoreWin32HandleInfo);
}

VkResult VK_KHR_external_semaphore_win32::vkGetSemaphoreWin32HandleKHR(VkDevice device, const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) const {
  return this->vkGetSemaphoreWin32HandleKHR_(device, pGetWin32HandleInfo, pHandle);
}

#endif // VK_USE_PLATFORM_WIN32_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_external_semaphore_fd
 * ------------------------------------------------------
*/

VK_KHR_external_semaphore_fd::VK_KHR_external_semaphore_fd(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkImportSemaphoreFdKHR);
  LOAD_DEVICE_FUNC(vkGetSemaphoreFdKHR);
}

VkResult VK_KHR_external_semaphore_fd::vkImportSemaphoreFdKHR(VkDevice device, const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) const {
  return this->vkImportSemaphoreFdKHR_(device, pImportSemaphoreFdInfo);
}

VkResult VK_KHR_external_semaphore_fd::vkGetSemaphoreFdKHR(VkDevice device, const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd) const {
  return this->vkGetSemaphoreFdKHR_(device, pGetFdInfo, pFd);
}

/*
 * ------------------------------------------------------
 * VK_KHR_push_descriptor
 * ------------------------------------------------------
*/

VK_KHR_push_descriptor::VK_KHR_push_descriptor(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkCmdPushDescriptorSetKHR);
}

void VK_KHR_push_descriptor::vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites) const {
  return this->vkCmdPushDescriptorSetKHR_(commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
}

/*
 * ------------------------------------------------------
 * VK_KHR_16bit_storage
 * ------------------------------------------------------
*/

VK_KHR_16bit_storage::VK_KHR_16bit_storage(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_incremental_present
 * ------------------------------------------------------
*/

VK_KHR_incremental_present::VK_KHR_incremental_present(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_descriptor_update_template
 * ------------------------------------------------------
*/

VK_KHR_descriptor_update_template::VK_KHR_descriptor_update_template(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkCreateDescriptorUpdateTemplateKHR);
  LOAD_DEVICE_FUNC(vkDestroyDescriptorUpdateTemplateKHR);
  LOAD_DEVICE_FUNC(vkUpdateDescriptorSetWithTemplateKHR);
  LOAD_DEVICE_FUNC(vkCmdPushDescriptorSetWithTemplateKHR);
}

VkResult VK_KHR_descriptor_update_template::vkCreateDescriptorUpdateTemplateKHR(VkDevice device, const VkDescriptorUpdateTemplateCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplateKHR* pDescriptorUpdateTemplate) const {
  return this->vkCreateDescriptorUpdateTemplateKHR_(device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

void VK_KHR_descriptor_update_template::vkDestroyDescriptorUpdateTemplateKHR(VkDevice device, VkDescriptorUpdateTemplateKHR descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyDescriptorUpdateTemplateKHR_(device, descriptorUpdateTemplate, pAllocator);
}

void VK_KHR_descriptor_update_template::vkUpdateDescriptorSetWithTemplateKHR(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplateKHR descriptorUpdateTemplate, const void* pData) const {
  return this->vkUpdateDescriptorSetWithTemplateKHR_(device, descriptorSet, descriptorUpdateTemplate, pData);
}

void VK_KHR_descriptor_update_template::vkCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplateKHR descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void* pData) const {
  return this->vkCmdPushDescriptorSetWithTemplateKHR_(commandBuffer, descriptorUpdateTemplate, layout, set, pData);
}

/*
 * ------------------------------------------------------
 * VK_NVX_device_generated_commands
 * ------------------------------------------------------
*/

VK_NVX_device_generated_commands::VK_NVX_device_generated_commands(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkCmdProcessCommandsNVX);
  LOAD_DEVICE_FUNC(vkCmdReserveSpaceForCommandsNVX);
  LOAD_DEVICE_FUNC(vkCreateIndirectCommandsLayoutNVX);
  LOAD_DEVICE_FUNC(vkDestroyIndirectCommandsLayoutNVX);
  LOAD_DEVICE_FUNC(vkCreateObjectTableNVX);
  LOAD_DEVICE_FUNC(vkDestroyObjectTableNVX);
  LOAD_DEVICE_FUNC(vkRegisterObjectsNVX);
  LOAD_DEVICE_FUNC(vkUnregisterObjectsNVX);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX);
}

void VK_NVX_device_generated_commands::vkCmdProcessCommandsNVX(VkCommandBuffer commandBuffer, const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) const {
  return this->vkCmdProcessCommandsNVX_(commandBuffer, pProcessCommandsInfo);
}

void VK_NVX_device_generated_commands::vkCmdReserveSpaceForCommandsNVX(VkCommandBuffer commandBuffer, const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) const {
  return this->vkCmdReserveSpaceForCommandsNVX_(commandBuffer, pReserveSpaceInfo);
}

VkResult VK_NVX_device_generated_commands::vkCreateIndirectCommandsLayoutNVX(VkDevice device, const VkIndirectCommandsLayoutCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkIndirectCommandsLayoutNVX* pIndirectCommandsLayout) const {
  return this->vkCreateIndirectCommandsLayoutNVX_(device, pCreateInfo, pAllocator, pIndirectCommandsLayout);
}

void VK_NVX_device_generated_commands::vkDestroyIndirectCommandsLayoutNVX(VkDevice device, VkIndirectCommandsLayoutNVX indirectCommandsLayout, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyIndirectCommandsLayoutNVX_(device, indirectCommandsLayout, pAllocator);
}

VkResult VK_NVX_device_generated_commands::vkCreateObjectTableNVX(VkDevice device, const VkObjectTableCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkObjectTableNVX* pObjectTable) const {
  return this->vkCreateObjectTableNVX_(device, pCreateInfo, pAllocator, pObjectTable);
}

void VK_NVX_device_generated_commands::vkDestroyObjectTableNVX(VkDevice device, VkObjectTableNVX objectTable, const VkAllocationCallbacks* pAllocator) const {
  return this->vkDestroyObjectTableNVX_(device, objectTable, pAllocator);
}

VkResult VK_NVX_device_generated_commands::vkRegisterObjectsNVX(VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectTableEntryNVX* const* ppObjectTableEntries, const uint32_t* pObjectIndices) const {
  return this->vkRegisterObjectsNVX_(device, objectTable, objectCount, ppObjectTableEntries, pObjectIndices);
}

VkResult VK_NVX_device_generated_commands::vkUnregisterObjectsNVX(VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectEntryTypeNVX* pObjectEntryTypes, const uint32_t* pObjectIndices) const {
  return this->vkUnregisterObjectsNVX_(device, objectTable, objectCount, pObjectEntryTypes, pObjectIndices);
}

void VK_NVX_device_generated_commands::vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX(VkPhysicalDevice physicalDevice, VkDeviceGeneratedCommandsFeaturesNVX* pFeatures, VkDeviceGeneratedCommandsLimitsNVX* pLimits) const {
  return this->vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX_(physicalDevice, pFeatures, pLimits);
}

/*
 * ------------------------------------------------------
 * VK_NV_clip_space_w_scaling
 * ------------------------------------------------------
*/

VK_NV_clip_space_w_scaling::VK_NV_clip_space_w_scaling(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkCmdSetViewportWScalingNV);
}

void VK_NV_clip_space_w_scaling::vkCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV* pViewportWScalings) const {
  return this->vkCmdSetViewportWScalingNV_(commandBuffer, firstViewport, viewportCount, pViewportWScalings);
}

/*
 * ------------------------------------------------------
 * VK_EXT_direct_mode_display
 * ------------------------------------------------------
*/

VK_EXT_direct_mode_display::VK_EXT_direct_mode_display(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkReleaseDisplayEXT);
}

VkResult VK_EXT_direct_mode_display::vkReleaseDisplayEXT(VkPhysicalDevice physicalDevice, VkDisplayKHR display) const {
  return this->vkReleaseDisplayEXT_(physicalDevice, display);
}

/*
 * ------------------------------------------------------
 * VK_EXT_acquire_xlib_display
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT)

VK_EXT_acquire_xlib_display::VK_EXT_acquire_xlib_display(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkAcquireXlibDisplayEXT);
  LOAD_INSTANCE_FUNC(vkGetRandROutputDisplayEXT);
}

VkResult VK_EXT_acquire_xlib_display::vkAcquireXlibDisplayEXT(VkPhysicalDevice physicalDevice, ?* dpy, VkDisplayKHR display) const {
  return this->vkAcquireXlibDisplayEXT_(physicalDevice, dpy, display);
}

VkResult VK_EXT_acquire_xlib_display::vkGetRandROutputDisplayEXT(VkPhysicalDevice physicalDevice, ?* dpy, RROutput rrOutput, VkDisplayKHR* pDisplay) const {
  return this->vkGetRandROutputDisplayEXT_(physicalDevice, dpy, rrOutput, pDisplay);
}

#endif // VK_USE_PLATFORM_XLIB_XRANDR_EXT

/*
 * ------------------------------------------------------
 * VK_EXT_display_surface_counter
 * ------------------------------------------------------
*/

VK_EXT_display_surface_counter::VK_EXT_display_surface_counter(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceCapabilities2EXT);
}

VkResult VK_EXT_display_surface_counter::vkGetPhysicalDeviceSurfaceCapabilities2EXT(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilities2EXT* pSurfaceCapabilities) const {
  return this->vkGetPhysicalDeviceSurfaceCapabilities2EXT_(physicalDevice, surface, pSurfaceCapabilities);
}

/*
 * ------------------------------------------------------
 * VK_EXT_display_control
 * ------------------------------------------------------
*/

VK_EXT_display_control::VK_EXT_display_control(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkDisplayPowerControlEXT);
  LOAD_DEVICE_FUNC(vkRegisterDeviceEventEXT);
  LOAD_DEVICE_FUNC(vkRegisterDisplayEventEXT);
  LOAD_DEVICE_FUNC(vkGetSwapchainCounterEXT);
}

VkResult VK_EXT_display_control::vkDisplayPowerControlEXT(VkDevice device, VkDisplayKHR display, const VkDisplayPowerInfoEXT* pDisplayPowerInfo) const {
  return this->vkDisplayPowerControlEXT_(device, display, pDisplayPowerInfo);
}

VkResult VK_EXT_display_control::vkRegisterDeviceEventEXT(VkDevice device, const VkDeviceEventInfoEXT* pDeviceEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) const {
  return this->vkRegisterDeviceEventEXT_(device, pDeviceEventInfo, pAllocator, pFence);
}

VkResult VK_EXT_display_control::vkRegisterDisplayEventEXT(VkDevice device, VkDisplayKHR display, const VkDisplayEventInfoEXT* pDisplayEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) const {
  return this->vkRegisterDisplayEventEXT_(device, display, pDisplayEventInfo, pAllocator, pFence);
}

VkResult VK_EXT_display_control::vkGetSwapchainCounterEXT(VkDevice device, VkSwapchainKHR swapchain, VkSurfaceCounterFlagBitsEXT counter, uint64_t* pCounterValue) const {
  return this->vkGetSwapchainCounterEXT_(device, swapchain, counter, pCounterValue);
}

/*
 * ------------------------------------------------------
 * VK_GOOGLE_display_timing
 * ------------------------------------------------------
*/

VK_GOOGLE_display_timing::VK_GOOGLE_display_timing(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkGetRefreshCycleDurationGOOGLE);
  LOAD_DEVICE_FUNC(vkGetPastPresentationTimingGOOGLE);
}

VkResult VK_GOOGLE_display_timing::vkGetRefreshCycleDurationGOOGLE(VkDevice device, VkSwapchainKHR swapchain, VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties) const {
  return this->vkGetRefreshCycleDurationGOOGLE_(device, swapchain, pDisplayTimingProperties);
}

VkResult VK_GOOGLE_display_timing::vkGetPastPresentationTimingGOOGLE(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pPresentationTimingCount, VkPastPresentationTimingGOOGLE* pPresentationTimings) const {
  return this->vkGetPastPresentationTimingGOOGLE_(device, swapchain, pPresentationTimingCount, pPresentationTimings);
}

/*
 * ------------------------------------------------------
 * VK_NV_sample_mask_override_coverage
 * ------------------------------------------------------
*/

VK_NV_sample_mask_override_coverage::VK_NV_sample_mask_override_coverage(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NV_geometry_shader_passthrough
 * ------------------------------------------------------
*/

VK_NV_geometry_shader_passthrough::VK_NV_geometry_shader_passthrough(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NV_viewport_array2
 * ------------------------------------------------------
*/

VK_NV_viewport_array2::VK_NV_viewport_array2(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NVX_multiview_per_view_attributes
 * ------------------------------------------------------
*/

VK_NVX_multiview_per_view_attributes::VK_NVX_multiview_per_view_attributes(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NV_viewport_swizzle
 * ------------------------------------------------------
*/

VK_NV_viewport_swizzle::VK_NV_viewport_swizzle(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_EXT_discard_rectangles
 * ------------------------------------------------------
*/

VK_EXT_discard_rectangles::VK_EXT_discard_rectangles(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkCmdSetDiscardRectangleEXT);
}

void VK_EXT_discard_rectangles::vkCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles) const {
  return this->vkCmdSetDiscardRectangleEXT_(commandBuffer, firstDiscardRectangle, discardRectangleCount, pDiscardRectangles);
}

/*
 * ------------------------------------------------------
 * VK_EXT_swapchain_colorspace
 * ------------------------------------------------------
*/

VK_EXT_swapchain_colorspace::VK_EXT_swapchain_colorspace(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
}

/*
 * ------------------------------------------------------
 * VK_EXT_hdr_metadata
 * ------------------------------------------------------
*/

VK_EXT_hdr_metadata::VK_EXT_hdr_metadata(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkSetHdrMetadataEXT);
}

void VK_EXT_hdr_metadata::vkSetHdrMetadataEXT(VkDevice device, uint32_t swapchainCount, const VkSwapchainKHR* pSwapchains, const VkHdrMetadataEXT* pMetadata) const {
  return this->vkSetHdrMetadataEXT_(device, swapchainCount, pSwapchains, pMetadata);
}

/*
 * ------------------------------------------------------
 * VK_KHR_shared_presentable_image
 * ------------------------------------------------------
*/

VK_KHR_shared_presentable_image::VK_KHR_shared_presentable_image(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkGetSwapchainStatusKHR);
}

VkResult VK_KHR_shared_presentable_image::vkGetSwapchainStatusKHR(VkDevice device, VkSwapchainKHR swapchain) const {
  return this->vkGetSwapchainStatusKHR_(device, swapchain);
}

/*
 * ------------------------------------------------------
 * VK_KHR_external_fence_capabilities
 * ------------------------------------------------------
*/

VK_KHR_external_fence_capabilities::VK_KHR_external_fence_capabilities(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceExternalFencePropertiesKHR);
}

void VK_KHR_external_fence_capabilities::vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfoKHR* pExternalFenceInfo, VkExternalFencePropertiesKHR* pExternalFenceProperties) const {
  return this->vkGetPhysicalDeviceExternalFencePropertiesKHR_(physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
}

/*
 * ------------------------------------------------------
 * VK_KHR_external_fence
 * ------------------------------------------------------
*/

VK_KHR_external_fence::VK_KHR_external_fence(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_external_fence_win32
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_WIN32_KHR)

VK_KHR_external_fence_win32::VK_KHR_external_fence_win32(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkImportFenceWin32HandleKHR);
  LOAD_DEVICE_FUNC(vkGetFenceWin32HandleKHR);
}

VkResult VK_KHR_external_fence_win32::vkImportFenceWin32HandleKHR(VkDevice device, const VkImportFenceWin32HandleInfoKHR* pImportFenceWin32HandleInfo) const {
  return this->vkImportFenceWin32HandleKHR_(device, pImportFenceWin32HandleInfo);
}

VkResult VK_KHR_external_fence_win32::vkGetFenceWin32HandleKHR(VkDevice device, const VkFenceGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) const {
  return this->vkGetFenceWin32HandleKHR_(device, pGetWin32HandleInfo, pHandle);
}

#endif // VK_USE_PLATFORM_WIN32_KHR

/*
 * ------------------------------------------------------
 * VK_KHR_external_fence_fd
 * ------------------------------------------------------
*/

VK_KHR_external_fence_fd::VK_KHR_external_fence_fd(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkImportFenceFdKHR);
  LOAD_DEVICE_FUNC(vkGetFenceFdKHR);
}

VkResult VK_KHR_external_fence_fd::vkImportFenceFdKHR(VkDevice device, const VkImportFenceFdInfoKHR* pImportFenceFdInfo) const {
  return this->vkImportFenceFdKHR_(device, pImportFenceFdInfo);
}

VkResult VK_KHR_external_fence_fd::vkGetFenceFdKHR(VkDevice device, const VkFenceGetFdInfoKHR* pGetFdInfo, int* pFd) const {
  return this->vkGetFenceFdKHR_(device, pGetFdInfo, pFd);
}

/*
 * ------------------------------------------------------
 * VK_KHR_get_surface_capabilities2
 * ------------------------------------------------------
*/

VK_KHR_get_surface_capabilities2::VK_KHR_get_surface_capabilities2(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceCapabilities2KHR);
  LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceFormats2KHR);
}

VkResult VK_KHR_get_surface_capabilities2::vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkSurfaceCapabilities2KHR* pSurfaceCapabilities) const {
  return this->vkGetPhysicalDeviceSurfaceCapabilities2KHR_(physicalDevice, pSurfaceInfo, pSurfaceCapabilities);
}

VkResult VK_KHR_get_surface_capabilities2::vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, uint32_t* pSurfaceFormatCount, VkSurfaceFormat2KHR* pSurfaceFormats) const {
  return this->vkGetPhysicalDeviceSurfaceFormats2KHR_(physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
}

/*
 * ------------------------------------------------------
 * VK_KHR_variable_pointers
 * ------------------------------------------------------
*/

VK_KHR_variable_pointers::VK_KHR_variable_pointers(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_MVK_ios_surface
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_IOS_MVK)

VK_MVK_ios_surface::VK_MVK_ios_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateIOSSurfaceMVK);
}

VkResult VK_MVK_ios_surface::vkCreateIOSSurfaceMVK(VkInstance instance, const VkIOSSurfaceCreateInfoMVK* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateIOSSurfaceMVK_(instance, pCreateInfo, pAllocator, pSurface);
}

#endif // VK_USE_PLATFORM_IOS_MVK

/*
 * ------------------------------------------------------
 * VK_MVK_macos_surface
 * ------------------------------------------------------
*/

#if defined(VK_USE_PLATFORM_MACOS_MVK)

VK_MVK_macos_surface::VK_MVK_macos_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance) {
  LOAD_INSTANCE_FUNC(vkCreateMacOSSurfaceMVK);
}

VkResult VK_MVK_macos_surface::vkCreateMacOSSurfaceMVK(VkInstance instance, const VkMacOSSurfaceCreateInfoMVK* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const {
  return this->vkCreateMacOSSurfaceMVK_(instance, pCreateInfo, pAllocator, pSurface);
}

#endif // VK_USE_PLATFORM_MACOS_MVK

/*
 * ------------------------------------------------------
 * VK_KHR_dedicated_allocation
 * ------------------------------------------------------
*/

VK_KHR_dedicated_allocation::VK_KHR_dedicated_allocation(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_EXT_sampler_filter_minmax
 * ------------------------------------------------------
*/

VK_EXT_sampler_filter_minmax::VK_EXT_sampler_filter_minmax(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_storage_buffer_storage_class
 * ------------------------------------------------------
*/

VK_KHR_storage_buffer_storage_class::VK_KHR_storage_buffer_storage_class(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_AMD_gpu_shader_int16
 * ------------------------------------------------------
*/

VK_AMD_gpu_shader_int16::VK_AMD_gpu_shader_int16(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_AMD_mixed_attachment_samples
 * ------------------------------------------------------
*/

VK_AMD_mixed_attachment_samples::VK_AMD_mixed_attachment_samples(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_relaxed_block_layout
 * ------------------------------------------------------
*/

VK_KHR_relaxed_block_layout::VK_KHR_relaxed_block_layout(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_KHR_get_memory_requirements2
 * ------------------------------------------------------
*/

VK_KHR_get_memory_requirements2::VK_KHR_get_memory_requirements2(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
  LOAD_DEVICE_FUNC(vkGetImageMemoryRequirements2KHR);
  LOAD_DEVICE_FUNC(vkGetBufferMemoryRequirements2KHR);
  LOAD_DEVICE_FUNC(vkGetImageSparseMemoryRequirements2KHR);
}

void VK_KHR_get_memory_requirements2::vkGetImageMemoryRequirements2KHR(VkDevice device, const VkImageMemoryRequirementsInfo2KHR* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements) const {
  return this->vkGetImageMemoryRequirements2KHR_(device, pInfo, pMemoryRequirements);
}

void VK_KHR_get_memory_requirements2::vkGetBufferMemoryRequirements2KHR(VkDevice device, const VkBufferMemoryRequirementsInfo2KHR* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements) const {
  return this->vkGetBufferMemoryRequirements2KHR_(device, pInfo, pMemoryRequirements);
}

void VK_KHR_get_memory_requirements2::vkGetImageSparseMemoryRequirements2KHR(VkDevice device, const VkImageSparseMemoryRequirementsInfo2KHR* pInfo, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2KHR* pSparseMemoryRequirements) const {
  return this->vkGetImageSparseMemoryRequirements2KHR_(device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

/*
 * ------------------------------------------------------
 * VK_EXT_blend_operation_advanced
 * ------------------------------------------------------
*/

VK_EXT_blend_operation_advanced::VK_EXT_blend_operation_advanced(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NV_fragment_coverage_to_color
 * ------------------------------------------------------
*/

VK_NV_fragment_coverage_to_color::VK_NV_fragment_coverage_to_color(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NV_framebuffer_mixed_samples
 * ------------------------------------------------------
*/

VK_NV_framebuffer_mixed_samples::VK_NV_framebuffer_mixed_samples(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_NV_fill_rectangle
 * ------------------------------------------------------
*/

VK_NV_fill_rectangle::VK_NV_fill_rectangle(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

/*
 * ------------------------------------------------------
 * VK_EXT_post_depth_coverage
 * ------------------------------------------------------
*/

VK_EXT_post_depth_coverage::VK_EXT_post_depth_coverage(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device) {
}

} // vkgen
