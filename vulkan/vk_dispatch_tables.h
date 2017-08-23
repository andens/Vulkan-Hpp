// Copyright (c) 2015-2017 The Khronos Group Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
// 
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

// Dispatch tables for Vulkan 1.0.48, generated from the Khronos Vulkan API XML Registry.
// See https://github.com/andens/Vulkan-Hpp for generator details.

#ifndef VK_DISPATCH_TABLES_INCLUDE
#define VK_DISPATCH_TABLES_INCLUDE

#include "vulkan_include.inl"
#include <stdexcept>
#include <string>
#if defined(_WIN32)
#include <Windows.h>
#endif

namespace vkgen {

class VulkanProcNotFound: public std::exception {
public:
  VulkanProcNotFound(std::string const& proc) : proc_(proc) {}
  virtual const char* what() const throw() {
    return "Write better error message here";
  }

private:
  std::string proc_;
};

class VulkanGlobalTable {
#if defined(_WIN32)
  typedef HMODULE library_handle;
#elif defined(__linux__)
  typedef void* library_handle;
#else
#error "Unsupported OS"
#endif

public:
  VulkanGlobalTable(std::string const& vulkan_library);
  PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName) const;
  VkResult vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) const;
  VkResult vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) const;
  VkResult vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) const;

private:
  library_handle library_ = nullptr;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_ = nullptr;
  PFN_vkCreateInstance vkCreateInstance_ = nullptr;
  PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties_ = nullptr;
  PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties_ = nullptr;
};

class VulkanInstanceTable {
public:
  VulkanInstanceTable(VulkanGlobalTable const* globals, VkInstance instance);
  VkInstance instance() const { return instance_; }
  void vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices) const;
  void vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures) const;
  void vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties) const;
  VkResult vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties) const;
  void vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties) const;
  void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties* pQueueFamilyProperties) const;
  void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties) const;
  PFN_vkVoidFunction vkGetDeviceProcAddr(VkDevice device, const char* pName) const;
  VkResult vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice) const;
  VkResult vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) const;
  VkResult vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkLayerProperties* pProperties) const;
  void vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pPropertyCount, VkSparseImageFormatProperties* pProperties) const;

private:
  VkInstance instance_ = VK_NULL_HANDLE;
  PFN_vkDestroyInstance vkDestroyInstance_ = nullptr;
  PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices_ = nullptr;
  PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures_ = nullptr;
  PFN_vkGetPhysicalDeviceFormatProperties vkGetPhysicalDeviceFormatProperties_ = nullptr;
  PFN_vkGetPhysicalDeviceImageFormatProperties vkGetPhysicalDeviceImageFormatProperties_ = nullptr;
  PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties_ = nullptr;
  PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties_ = nullptr;
  PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties_ = nullptr;
  PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr_ = nullptr;
  PFN_vkCreateDevice vkCreateDevice_ = nullptr;
  PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties_ = nullptr;
  PFN_vkEnumerateDeviceLayerProperties vkEnumerateDeviceLayerProperties_ = nullptr;
  PFN_vkGetPhysicalDeviceSparseImageFormatProperties vkGetPhysicalDeviceSparseImageFormatProperties_ = nullptr;
};

class VulkanDeviceTable {
public:
  VulkanDeviceTable(VulkanInstanceTable const* instance, VkDevice device);
  VkDevice device() const { return device_; }
  void vkDestroyDevice(VkDevice device, const VkAllocationCallbacks* pAllocator) const;
  void vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue) const;
  VkResult vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence) const;
  VkResult vkQueueWaitIdle(VkQueue queue) const;
  VkResult vkDeviceWaitIdle(VkDevice device) const;
  VkResult vkAllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory) const;
  void vkFreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkMapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData) const;
  void vkUnmapMemory(VkDevice device, VkDeviceMemory memory) const;
  VkResult vkFlushMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const;
  VkResult vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange* pMemoryRanges) const;
  void vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes) const;
  VkResult vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) const;
  VkResult vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) const;
  void vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements) const;
  void vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements) const;
  void vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements) const;
  VkResult vkQueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence) const;
  VkResult vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) const;
  void vkDestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences) const;
  VkResult vkGetFenceStatus(VkDevice device, VkFence fence) const;
  VkResult vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout) const;
  VkResult vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSemaphore* pSemaphore) const;
  void vkDestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkEvent* pEvent) const;
  void vkDestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkGetEventStatus(VkDevice device, VkEvent event) const;
  VkResult vkSetEvent(VkDevice device, VkEvent event) const;
  VkResult vkResetEvent(VkDevice device, VkEvent event) const;
  VkResult vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool) const;
  void vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags) const;
  VkResult vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBuffer* pBuffer) const;
  void vkDestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView) const;
  void vkDestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImage* pImage) const;
  void vkDestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks* pAllocator) const;
  void vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout) const;
  VkResult vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkImageView* pView) const;
  void vkDestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkShaderModule* pShaderModule) const;
  void vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineCache* pPipelineCache) const;
  void vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t* pDataSize, void* pData) const;
  VkResult vkMergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches) const;
  VkResult vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) const;
  VkResult vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines) const;
  void vkDestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkPipelineLayout* pPipelineLayout) const;
  void vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSampler* pSampler) const;
  void vkDestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorSetLayout* pSetLayout) const;
  void vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool) const;
  void vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags) const;
  VkResult vkAllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets) const;
  VkResult vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets) const;
  void vkUpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies) const;
  VkResult vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkFramebuffer* pFramebuffer) const;
  void vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkRenderPass* pRenderPass) const;
  void vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator) const;
  void vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity) const;
  VkResult vkCreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkCommandPool* pCommandPool) const;
  void vkDestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) const;
  VkResult vkAllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers) const;
  void vkFreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const;
  VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo) const;
  VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer) const;
  VkResult vkResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) const;
  void vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) const;
  void vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports) const;
  void vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors) const;
  void vkCmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) const;
  void vkCmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) const;
  void vkCmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) const;
  void vkCmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) const;
  void vkCmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask) const;
  void vkCmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) const;
  void vkCmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) const;
  void vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets) const;
  void vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) const;
  void vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets) const;
  void vkCmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const;
  void vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) const;
  void vkCmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) const;
  void vkCmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride) const;
  void vkCmdDispatch(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const;
  void vkCmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) const;
  void vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const;
  void vkCmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageCopy* pRegions) const;
  void vkCmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkFilter filter) const;
  void vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const;
  void vkCmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions) const;
  void vkCmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* pData) const;
  void vkCmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data) const;
  void vkCmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) const;
  void vkCmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges) const;
  void vkCmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects) const;
  void vkCmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve* pRegions) const;
  void vkCmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) const;
  void vkCmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) const;
  void vkCmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) const;
  void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers) const;
  void vkCmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags) const;
  void vkCmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query) const;
  void vkCmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount) const;
  void vkCmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query) const;
  void vkCmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride, VkQueryResultFlags flags) const;
  void vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues) const;
  void vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents) const;
  void vkCmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) const;
  void vkCmdEndRenderPass(VkCommandBuffer commandBuffer) const;
  void vkCmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers) const;

private:
  VkDevice device_ = VK_NULL_HANDLE;
  PFN_vkDestroyDevice vkDestroyDevice_ = nullptr;
  PFN_vkGetDeviceQueue vkGetDeviceQueue_ = nullptr;
  PFN_vkQueueSubmit vkQueueSubmit_ = nullptr;
  PFN_vkQueueWaitIdle vkQueueWaitIdle_ = nullptr;
  PFN_vkDeviceWaitIdle vkDeviceWaitIdle_ = nullptr;
  PFN_vkAllocateMemory vkAllocateMemory_ = nullptr;
  PFN_vkFreeMemory vkFreeMemory_ = nullptr;
  PFN_vkMapMemory vkMapMemory_ = nullptr;
  PFN_vkUnmapMemory vkUnmapMemory_ = nullptr;
  PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges_ = nullptr;
  PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges_ = nullptr;
  PFN_vkGetDeviceMemoryCommitment vkGetDeviceMemoryCommitment_ = nullptr;
  PFN_vkBindBufferMemory vkBindBufferMemory_ = nullptr;
  PFN_vkBindImageMemory vkBindImageMemory_ = nullptr;
  PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements_ = nullptr;
  PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements_ = nullptr;
  PFN_vkGetImageSparseMemoryRequirements vkGetImageSparseMemoryRequirements_ = nullptr;
  PFN_vkQueueBindSparse vkQueueBindSparse_ = nullptr;
  PFN_vkCreateFence vkCreateFence_ = nullptr;
  PFN_vkDestroyFence vkDestroyFence_ = nullptr;
  PFN_vkResetFences vkResetFences_ = nullptr;
  PFN_vkGetFenceStatus vkGetFenceStatus_ = nullptr;
  PFN_vkWaitForFences vkWaitForFences_ = nullptr;
  PFN_vkCreateSemaphore vkCreateSemaphore_ = nullptr;
  PFN_vkDestroySemaphore vkDestroySemaphore_ = nullptr;
  PFN_vkCreateEvent vkCreateEvent_ = nullptr;
  PFN_vkDestroyEvent vkDestroyEvent_ = nullptr;
  PFN_vkGetEventStatus vkGetEventStatus_ = nullptr;
  PFN_vkSetEvent vkSetEvent_ = nullptr;
  PFN_vkResetEvent vkResetEvent_ = nullptr;
  PFN_vkCreateQueryPool vkCreateQueryPool_ = nullptr;
  PFN_vkDestroyQueryPool vkDestroyQueryPool_ = nullptr;
  PFN_vkGetQueryPoolResults vkGetQueryPoolResults_ = nullptr;
  PFN_vkCreateBuffer vkCreateBuffer_ = nullptr;
  PFN_vkDestroyBuffer vkDestroyBuffer_ = nullptr;
  PFN_vkCreateBufferView vkCreateBufferView_ = nullptr;
  PFN_vkDestroyBufferView vkDestroyBufferView_ = nullptr;
  PFN_vkCreateImage vkCreateImage_ = nullptr;
  PFN_vkDestroyImage vkDestroyImage_ = nullptr;
  PFN_vkGetImageSubresourceLayout vkGetImageSubresourceLayout_ = nullptr;
  PFN_vkCreateImageView vkCreateImageView_ = nullptr;
  PFN_vkDestroyImageView vkDestroyImageView_ = nullptr;
  PFN_vkCreateShaderModule vkCreateShaderModule_ = nullptr;
  PFN_vkDestroyShaderModule vkDestroyShaderModule_ = nullptr;
  PFN_vkCreatePipelineCache vkCreatePipelineCache_ = nullptr;
  PFN_vkDestroyPipelineCache vkDestroyPipelineCache_ = nullptr;
  PFN_vkGetPipelineCacheData vkGetPipelineCacheData_ = nullptr;
  PFN_vkMergePipelineCaches vkMergePipelineCaches_ = nullptr;
  PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines_ = nullptr;
  PFN_vkCreateComputePipelines vkCreateComputePipelines_ = nullptr;
  PFN_vkDestroyPipeline vkDestroyPipeline_ = nullptr;
  PFN_vkCreatePipelineLayout vkCreatePipelineLayout_ = nullptr;
  PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout_ = nullptr;
  PFN_vkCreateSampler vkCreateSampler_ = nullptr;
  PFN_vkDestroySampler vkDestroySampler_ = nullptr;
  PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout_ = nullptr;
  PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout_ = nullptr;
  PFN_vkCreateDescriptorPool vkCreateDescriptorPool_ = nullptr;
  PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool_ = nullptr;
  PFN_vkResetDescriptorPool vkResetDescriptorPool_ = nullptr;
  PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets_ = nullptr;
  PFN_vkFreeDescriptorSets vkFreeDescriptorSets_ = nullptr;
  PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets_ = nullptr;
  PFN_vkCreateFramebuffer vkCreateFramebuffer_ = nullptr;
  PFN_vkDestroyFramebuffer vkDestroyFramebuffer_ = nullptr;
  PFN_vkCreateRenderPass vkCreateRenderPass_ = nullptr;
  PFN_vkDestroyRenderPass vkDestroyRenderPass_ = nullptr;
  PFN_vkGetRenderAreaGranularity vkGetRenderAreaGranularity_ = nullptr;
  PFN_vkCreateCommandPool vkCreateCommandPool_ = nullptr;
  PFN_vkDestroyCommandPool vkDestroyCommandPool_ = nullptr;
  PFN_vkResetCommandPool vkResetCommandPool_ = nullptr;
  PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers_ = nullptr;
  PFN_vkFreeCommandBuffers vkFreeCommandBuffers_ = nullptr;
  PFN_vkBeginCommandBuffer vkBeginCommandBuffer_ = nullptr;
  PFN_vkEndCommandBuffer vkEndCommandBuffer_ = nullptr;
  PFN_vkResetCommandBuffer vkResetCommandBuffer_ = nullptr;
  PFN_vkCmdBindPipeline vkCmdBindPipeline_ = nullptr;
  PFN_vkCmdSetViewport vkCmdSetViewport_ = nullptr;
  PFN_vkCmdSetScissor vkCmdSetScissor_ = nullptr;
  PFN_vkCmdSetLineWidth vkCmdSetLineWidth_ = nullptr;
  PFN_vkCmdSetDepthBias vkCmdSetDepthBias_ = nullptr;
  PFN_vkCmdSetBlendConstants vkCmdSetBlendConstants_ = nullptr;
  PFN_vkCmdSetDepthBounds vkCmdSetDepthBounds_ = nullptr;
  PFN_vkCmdSetStencilCompareMask vkCmdSetStencilCompareMask_ = nullptr;
  PFN_vkCmdSetStencilWriteMask vkCmdSetStencilWriteMask_ = nullptr;
  PFN_vkCmdSetStencilReference vkCmdSetStencilReference_ = nullptr;
  PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets_ = nullptr;
  PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer_ = nullptr;
  PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers_ = nullptr;
  PFN_vkCmdDraw vkCmdDraw_ = nullptr;
  PFN_vkCmdDrawIndexed vkCmdDrawIndexed_ = nullptr;
  PFN_vkCmdDrawIndirect vkCmdDrawIndirect_ = nullptr;
  PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect_ = nullptr;
  PFN_vkCmdDispatch vkCmdDispatch_ = nullptr;
  PFN_vkCmdDispatchIndirect vkCmdDispatchIndirect_ = nullptr;
  PFN_vkCmdCopyBuffer vkCmdCopyBuffer_ = nullptr;
  PFN_vkCmdCopyImage vkCmdCopyImage_ = nullptr;
  PFN_vkCmdBlitImage vkCmdBlitImage_ = nullptr;
  PFN_vkCmdCopyBufferToImage vkCmdCopyBufferToImage_ = nullptr;
  PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer_ = nullptr;
  PFN_vkCmdUpdateBuffer vkCmdUpdateBuffer_ = nullptr;
  PFN_vkCmdFillBuffer vkCmdFillBuffer_ = nullptr;
  PFN_vkCmdClearColorImage vkCmdClearColorImage_ = nullptr;
  PFN_vkCmdClearDepthStencilImage vkCmdClearDepthStencilImage_ = nullptr;
  PFN_vkCmdClearAttachments vkCmdClearAttachments_ = nullptr;
  PFN_vkCmdResolveImage vkCmdResolveImage_ = nullptr;
  PFN_vkCmdSetEvent vkCmdSetEvent_ = nullptr;
  PFN_vkCmdResetEvent vkCmdResetEvent_ = nullptr;
  PFN_vkCmdWaitEvents vkCmdWaitEvents_ = nullptr;
  PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier_ = nullptr;
  PFN_vkCmdBeginQuery vkCmdBeginQuery_ = nullptr;
  PFN_vkCmdEndQuery vkCmdEndQuery_ = nullptr;
  PFN_vkCmdResetQueryPool vkCmdResetQueryPool_ = nullptr;
  PFN_vkCmdWriteTimestamp vkCmdWriteTimestamp_ = nullptr;
  PFN_vkCmdCopyQueryPoolResults vkCmdCopyQueryPoolResults_ = nullptr;
  PFN_vkCmdPushConstants vkCmdPushConstants_ = nullptr;
  PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass_ = nullptr;
  PFN_vkCmdNextSubpass vkCmdNextSubpass_ = nullptr;
  PFN_vkCmdEndRenderPass vkCmdEndRenderPass_ = nullptr;
  PFN_vkCmdExecuteCommands vkCmdExecuteCommands_ = nullptr;
};

} // vkgen

#endif // VK_DISPATCH_TABLES_INCLUDE

