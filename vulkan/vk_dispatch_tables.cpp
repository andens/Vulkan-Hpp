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

} // vkgen
