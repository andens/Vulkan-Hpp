// Copyright (c) 2015-2017 The Khronos Group Inc.
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Dispatch tables for Vulkan 1.0.57, generated from the Khronos Vulkan API XML Registry.
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

#undef VK_KHR_surface
class VK_KHR_surface {
public:
  VK_KHR_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkGetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported) const;
  VkResult vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilitiesKHR* pSurfaceCapabilities) const;
  VkResult vkGetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pSurfaceFormatCount, VkSurfaceFormatKHR* pSurfaceFormats) const;
  VkResult vkGetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pPresentModeCount, VkPresentModeKHR* pPresentModes) const;

private:
  PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR_ = nullptr;
};

#undef VK_KHR_swapchain
class VK_KHR_swapchain {
public:
  VK_KHR_swapchain(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkCreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) const;
  void vkDestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkGetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages) const;
  VkResult vkAcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex) const;
  VkResult vkQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) const;

private:
  PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR_ = nullptr;
  PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR_ = nullptr;
  PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR_ = nullptr;
  PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR_ = nullptr;
  PFN_vkQueuePresentKHR vkQueuePresentKHR_ = nullptr;
};

#undef VK_KHR_display
class VK_KHR_display {
public:
  VK_KHR_display(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkGetPhysicalDeviceDisplayPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkDisplayPropertiesKHR* pProperties) const;
  VkResult vkGetPhysicalDeviceDisplayPlanePropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkDisplayPlanePropertiesKHR* pProperties) const;
  VkResult vkGetDisplayPlaneSupportedDisplaysKHR(VkPhysicalDevice physicalDevice, uint32_t planeIndex, uint32_t* pDisplayCount, VkDisplayKHR* pDisplays) const;
  VkResult vkGetDisplayModePropertiesKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, uint32_t* pPropertyCount, VkDisplayModePropertiesKHR* pProperties) const;
  VkResult vkCreateDisplayModeKHR(VkPhysicalDevice physicalDevice, VkDisplayKHR display, const VkDisplayModeCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDisplayModeKHR* pMode) const;
  VkResult vkGetDisplayPlaneCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkDisplayModeKHR mode, uint32_t planeIndex, VkDisplayPlaneCapabilitiesKHR* pCapabilities) const;
  VkResult vkCreateDisplayPlaneSurfaceKHR(VkInstance instance, const VkDisplaySurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;

private:
  PFN_vkGetPhysicalDeviceDisplayPropertiesKHR vkGetPhysicalDeviceDisplayPropertiesKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR vkGetPhysicalDeviceDisplayPlanePropertiesKHR_ = nullptr;
  PFN_vkGetDisplayPlaneSupportedDisplaysKHR vkGetDisplayPlaneSupportedDisplaysKHR_ = nullptr;
  PFN_vkGetDisplayModePropertiesKHR vkGetDisplayModePropertiesKHR_ = nullptr;
  PFN_vkCreateDisplayModeKHR vkCreateDisplayModeKHR_ = nullptr;
  PFN_vkGetDisplayPlaneCapabilitiesKHR vkGetDisplayPlaneCapabilitiesKHR_ = nullptr;
  PFN_vkCreateDisplayPlaneSurfaceKHR vkCreateDisplayPlaneSurfaceKHR_ = nullptr;
};

#undef VK_KHR_display_swapchain
class VK_KHR_display_swapchain {
public:
  VK_KHR_display_swapchain(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkCreateSharedSwapchainsKHR(VkDevice device, uint32_t swapchainCount, const VkSwapchainCreateInfoKHR* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchains) const;

private:
  PFN_vkCreateSharedSwapchainsKHR vkCreateSharedSwapchainsKHR_ = nullptr;
};

#if defined(VK_USE_PLATFORM_XLIB_KHR)
#undef VK_KHR_xlib_surface
class VK_KHR_xlib_surface {
public:
  VK_KHR_xlib_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;
  VkBool32 vkGetPhysicalDeviceXlibPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, ?* dpy, VisualID visualID) const;

private:
  PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR_ = nullptr;
};
#endif // VK_USE_PLATFORM_XLIB_KHR

#if defined(VK_USE_PLATFORM_XCB_KHR)
#undef VK_KHR_xcb_surface
class VK_KHR_xcb_surface {
public:
  VK_KHR_xcb_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;
  VkBool32 vkGetPhysicalDeviceXcbPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, ?* connection, xcb_visualid_t visual_id) const;

private:
  PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR_ = nullptr;
};
#endif // VK_USE_PLATFORM_XCB_KHR

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
#undef VK_KHR_wayland_surface
class VK_KHR_wayland_surface {
public:
  VK_KHR_wayland_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;
  VkBool32 vkGetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, ?* display) const;

private:
  PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR vkGetPhysicalDeviceWaylandPresentationSupportKHR_ = nullptr;
};
#endif // VK_USE_PLATFORM_WAYLAND_KHR

#if defined(VK_USE_PLATFORM_MIR_KHR)
#undef VK_KHR_mir_surface
class VK_KHR_mir_surface {
public:
  VK_KHR_mir_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateMirSurfaceKHR(VkInstance instance, const VkMirSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;
  VkBool32 vkGetPhysicalDeviceMirPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, ?* connection) const;

private:
  PFN_vkCreateMirSurfaceKHR vkCreateMirSurfaceKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceMirPresentationSupportKHR vkGetPhysicalDeviceMirPresentationSupportKHR_ = nullptr;
};
#endif // VK_USE_PLATFORM_MIR_KHR

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
#undef VK_KHR_android_surface
class VK_KHR_android_surface {
public:
  VK_KHR_android_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;

private:
  PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR_ = nullptr;
};
#endif // VK_USE_PLATFORM_ANDROID_KHR

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#undef VK_KHR_win32_surface
class VK_KHR_win32_surface {
public:
  VK_KHR_win32_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;
  VkBool32 vkGetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex) const;

private:
  PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR_ = nullptr;
  PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR_ = nullptr;
};
#endif // VK_USE_PLATFORM_WIN32_KHR

#undef VK_EXT_debug_report
class VK_EXT_debug_report {
public:
  VK_EXT_debug_report(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback) const;
  void vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator) const;
  void vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage) const;

private:
  PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT_ = nullptr;
  PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT_ = nullptr;
  PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT_ = nullptr;
};

#undef VK_NV_glsl_shader
class VK_NV_glsl_shader {
public:
  VK_NV_glsl_shader(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_EXT_depth_range_unrestricted
class VK_EXT_depth_range_unrestricted {
public:
  VK_EXT_depth_range_unrestricted(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_KHR_sampler_mirror_clamp_to_edge
class VK_KHR_sampler_mirror_clamp_to_edge {
public:
  VK_KHR_sampler_mirror_clamp_to_edge(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_IMG_filter_cubic
class VK_IMG_filter_cubic {
public:
  VK_IMG_filter_cubic(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_AMD_rasterization_order
class VK_AMD_rasterization_order {
public:
  VK_AMD_rasterization_order(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_AMD_shader_trinary_minmax
class VK_AMD_shader_trinary_minmax {
public:
  VK_AMD_shader_trinary_minmax(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_AMD_shader_explicit_vertex_parameter
class VK_AMD_shader_explicit_vertex_parameter {
public:
  VK_AMD_shader_explicit_vertex_parameter(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_EXT_debug_marker
class VK_EXT_debug_marker {
public:
  VK_EXT_debug_marker(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkDebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT* pTagInfo) const;
  VkResult vkDebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT* pNameInfo) const;
  void vkCmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const;
  void vkCmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer) const;
  void vkCmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo) const;

private:
  PFN_vkDebugMarkerSetObjectTagEXT vkDebugMarkerSetObjectTagEXT_ = nullptr;
  PFN_vkDebugMarkerSetObjectNameEXT vkDebugMarkerSetObjectNameEXT_ = nullptr;
  PFN_vkCmdDebugMarkerBeginEXT vkCmdDebugMarkerBeginEXT_ = nullptr;
  PFN_vkCmdDebugMarkerEndEXT vkCmdDebugMarkerEndEXT_ = nullptr;
  PFN_vkCmdDebugMarkerInsertEXT vkCmdDebugMarkerInsertEXT_ = nullptr;
};

#undef VK_AMD_gcn_shader
class VK_AMD_gcn_shader {
public:
  VK_AMD_gcn_shader(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_NV_dedicated_allocation
class VK_NV_dedicated_allocation {
public:
  VK_NV_dedicated_allocation(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_AMD_draw_indirect_count
class VK_AMD_draw_indirect_count {
public:
  VK_AMD_draw_indirect_count(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  void vkCmdDrawIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) const;
  void vkCmdDrawIndexedIndirectCountAMD(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer, VkDeviceSize countBufferOffset, uint32_t maxDrawCount, uint32_t stride) const;

private:
  PFN_vkCmdDrawIndirectCountAMD vkCmdDrawIndirectCountAMD_ = nullptr;
  PFN_vkCmdDrawIndexedIndirectCountAMD vkCmdDrawIndexedIndirectCountAMD_ = nullptr;
};

#undef VK_AMD_negative_viewport_height
class VK_AMD_negative_viewport_height {
public:
  VK_AMD_negative_viewport_height(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_AMD_gpu_shader_half_float
class VK_AMD_gpu_shader_half_float {
public:
  VK_AMD_gpu_shader_half_float(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_AMD_shader_ballot
class VK_AMD_shader_ballot {
public:
  VK_AMD_shader_ballot(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_AMD_texture_gather_bias_lod
class VK_AMD_texture_gather_bias_lod {
public:
  VK_AMD_texture_gather_bias_lod(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_KHX_multiview
class VK_KHX_multiview {
public:
  VK_KHX_multiview(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_IMG_format_pvrtc
class VK_IMG_format_pvrtc {
public:
  VK_IMG_format_pvrtc(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_NV_external_memory_capabilities
class VK_NV_external_memory_capabilities {
public:
  VK_NV_external_memory_capabilities(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkGetPhysicalDeviceExternalImageFormatPropertiesNV(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkExternalMemoryHandleTypeFlagsNV externalHandleType, VkExternalImageFormatPropertiesNV* pExternalImageFormatProperties) const;

private:
  PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV vkGetPhysicalDeviceExternalImageFormatPropertiesNV_ = nullptr;
};

#undef VK_NV_external_memory
class VK_NV_external_memory {
public:
  VK_NV_external_memory(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#undef VK_NV_external_memory_win32
class VK_NV_external_memory_win32 {
public:
  VK_NV_external_memory_win32(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkGetMemoryWin32HandleNV(VkDevice device, VkDeviceMemory memory, VkExternalMemoryHandleTypeFlagsNV handleType, HANDLE* pHandle) const;

private:
  PFN_vkGetMemoryWin32HandleNV vkGetMemoryWin32HandleNV_ = nullptr;
};
#endif // VK_USE_PLATFORM_WIN32_KHR

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#undef VK_NV_win32_keyed_mutex
class VK_NV_win32_keyed_mutex {
public:
  VK_NV_win32_keyed_mutex(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};
#endif // VK_USE_PLATFORM_WIN32_KHR

#undef VK_KHR_get_physical_device_properties2
class VK_KHR_get_physical_device_properties2 {
public:
  VK_KHR_get_physical_device_properties2(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  void vkGetPhysicalDeviceFeatures2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2KHR* pFeatures) const;
  void vkGetPhysicalDeviceProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2KHR* pProperties) const;
  void vkGetPhysicalDeviceFormatProperties2KHR(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties2KHR* pFormatProperties) const;
  VkResult vkGetPhysicalDeviceImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceImageFormatInfo2KHR* pImageFormatInfo, VkImageFormatProperties2KHR* pImageFormatProperties) const;
  void vkGetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice, uint32_t* pQueueFamilyPropertyCount, VkQueueFamilyProperties2KHR* pQueueFamilyProperties) const;
  void vkGetPhysicalDeviceMemoryProperties2KHR(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties2KHR* pMemoryProperties) const;
  void vkGetPhysicalDeviceSparseImageFormatProperties2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2KHR* pFormatInfo, uint32_t* pPropertyCount, VkSparseImageFormatProperties2KHR* pProperties) const;

private:
  PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR_ = nullptr;
  PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR_ = nullptr;
  PFN_vkGetPhysicalDeviceFormatProperties2KHR vkGetPhysicalDeviceFormatProperties2KHR_ = nullptr;
  PFN_vkGetPhysicalDeviceImageFormatProperties2KHR vkGetPhysicalDeviceImageFormatProperties2KHR_ = nullptr;
  PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR vkGetPhysicalDeviceQueueFamilyProperties2KHR_ = nullptr;
  PFN_vkGetPhysicalDeviceMemoryProperties2KHR vkGetPhysicalDeviceMemoryProperties2KHR_ = nullptr;
  PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR vkGetPhysicalDeviceSparseImageFormatProperties2KHR_ = nullptr;
};

#undef VK_KHX_device_group
class VK_KHX_device_group {
public:
  VK_KHX_device_group(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  void vkGetDeviceGroupPeerMemoryFeaturesKHX(VkDevice device, uint32_t heapIndex, uint32_t localDeviceIndex, uint32_t remoteDeviceIndex, VkPeerMemoryFeatureFlagsKHX* pPeerMemoryFeatures) const;
  VkResult vkBindBufferMemory2KHX(VkDevice device, uint32_t bindInfoCount, const VkBindBufferMemoryInfoKHX* pBindInfos) const;
  VkResult vkBindImageMemory2KHX(VkDevice device, uint32_t bindInfoCount, const VkBindImageMemoryInfoKHX* pBindInfos) const;
  void vkCmdSetDeviceMaskKHX(VkCommandBuffer commandBuffer, uint32_t deviceMask) const;
  VkResult vkGetDeviceGroupPresentCapabilitiesKHX(VkDevice device, VkDeviceGroupPresentCapabilitiesKHX* pDeviceGroupPresentCapabilities) const;
  VkResult vkGetDeviceGroupSurfacePresentModesKHX(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHX* pModes) const;
  VkResult vkAcquireNextImage2KHX(VkDevice device, const VkAcquireNextImageInfoKHX* pAcquireInfo, uint32_t* pImageIndex) const;
  void vkCmdDispatchBaseKHX(VkCommandBuffer commandBuffer, uint32_t baseGroupX, uint32_t baseGroupY, uint32_t baseGroupZ, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) const;
  VkResult vkGetPhysicalDevicePresentRectanglesKHX(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pRectCount, VkRect2D* pRects) const;

private:
  PFN_vkGetDeviceGroupPeerMemoryFeaturesKHX vkGetDeviceGroupPeerMemoryFeaturesKHX_ = nullptr;
  PFN_vkBindBufferMemory2KHX vkBindBufferMemory2KHX_ = nullptr;
  PFN_vkBindImageMemory2KHX vkBindImageMemory2KHX_ = nullptr;
  PFN_vkCmdSetDeviceMaskKHX vkCmdSetDeviceMaskKHX_ = nullptr;
  PFN_vkGetDeviceGroupPresentCapabilitiesKHX vkGetDeviceGroupPresentCapabilitiesKHX_ = nullptr;
  PFN_vkGetDeviceGroupSurfacePresentModesKHX vkGetDeviceGroupSurfacePresentModesKHX_ = nullptr;
  PFN_vkAcquireNextImage2KHX vkAcquireNextImage2KHX_ = nullptr;
  PFN_vkCmdDispatchBaseKHX vkCmdDispatchBaseKHX_ = nullptr;
  PFN_vkGetPhysicalDevicePresentRectanglesKHX vkGetPhysicalDevicePresentRectanglesKHX_ = nullptr;
};

#undef VK_EXT_validation_flags
class VK_EXT_validation_flags {
public:
  VK_EXT_validation_flags(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);

private:
};

#if defined(VK_USE_PLATFORM_VI_NN)
#undef VK_NN_vi_surface
class VK_NN_vi_surface {
public:
  VK_NN_vi_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateViSurfaceNN(VkInstance instance, const VkViSurfaceCreateInfoNN* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;

private:
  PFN_vkCreateViSurfaceNN vkCreateViSurfaceNN_ = nullptr;
};
#endif // VK_USE_PLATFORM_VI_NN

#undef VK_KHR_shader_draw_parameters
class VK_KHR_shader_draw_parameters {
public:
  VK_KHR_shader_draw_parameters(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_EXT_shader_subgroup_ballot
class VK_EXT_shader_subgroup_ballot {
public:
  VK_EXT_shader_subgroup_ballot(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_EXT_shader_subgroup_vote
class VK_EXT_shader_subgroup_vote {
public:
  VK_EXT_shader_subgroup_vote(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_KHR_maintenance1
class VK_KHR_maintenance1 {
public:
  VK_KHR_maintenance1(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  void vkTrimCommandPoolKHR(VkDevice device, VkCommandPool commandPool, VkCommandPoolTrimFlagsKHR flags) const;

private:
  PFN_vkTrimCommandPoolKHR vkTrimCommandPoolKHR_ = nullptr;
};

#undef VK_KHX_device_group_creation
class VK_KHX_device_group_creation {
public:
  VK_KHX_device_group_creation(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkEnumeratePhysicalDeviceGroupsKHX(VkInstance instance, uint32_t* pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupPropertiesKHX* pPhysicalDeviceGroupProperties) const;

private:
  PFN_vkEnumeratePhysicalDeviceGroupsKHX vkEnumeratePhysicalDeviceGroupsKHX_ = nullptr;
};

#undef VK_KHR_external_memory_capabilities
class VK_KHR_external_memory_capabilities {
public:
  VK_KHR_external_memory_capabilities(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  void vkGetPhysicalDeviceExternalBufferPropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalBufferInfoKHR* pExternalBufferInfo, VkExternalBufferPropertiesKHR* pExternalBufferProperties) const;

private:
  PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR vkGetPhysicalDeviceExternalBufferPropertiesKHR_ = nullptr;
};

#undef VK_KHR_external_memory
class VK_KHR_external_memory {
public:
  VK_KHR_external_memory(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#undef VK_KHR_external_memory_win32
class VK_KHR_external_memory_win32 {
public:
  VK_KHR_external_memory_win32(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkGetMemoryWin32HandleKHR(VkDevice device, const VkMemoryGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) const;
  VkResult vkGetMemoryWin32HandlePropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBitsKHR handleType, HANDLE handle, VkMemoryWin32HandlePropertiesKHR* pMemoryWin32HandleProperties) const;

private:
  PFN_vkGetMemoryWin32HandleKHR vkGetMemoryWin32HandleKHR_ = nullptr;
  PFN_vkGetMemoryWin32HandlePropertiesKHR vkGetMemoryWin32HandlePropertiesKHR_ = nullptr;
};
#endif // VK_USE_PLATFORM_WIN32_KHR

#undef VK_KHR_external_memory_fd
class VK_KHR_external_memory_fd {
public:
  VK_KHR_external_memory_fd(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkGetMemoryFdKHR(VkDevice device, const VkMemoryGetFdInfoKHR* pGetFdInfo, int* pFd) const;
  VkResult vkGetMemoryFdPropertiesKHR(VkDevice device, VkExternalMemoryHandleTypeFlagBitsKHR handleType, int fd, VkMemoryFdPropertiesKHR* pMemoryFdProperties) const;

private:
  PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR_ = nullptr;
  PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR_ = nullptr;
};

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#undef VK_KHR_win32_keyed_mutex
class VK_KHR_win32_keyed_mutex {
public:
  VK_KHR_win32_keyed_mutex(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};
#endif // VK_USE_PLATFORM_WIN32_KHR

#undef VK_KHR_external_semaphore_capabilities
class VK_KHR_external_semaphore_capabilities {
public:
  VK_KHR_external_semaphore_capabilities(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  void vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfoKHR* pExternalSemaphoreInfo, VkExternalSemaphorePropertiesKHR* pExternalSemaphoreProperties) const;

private:
  PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR vkGetPhysicalDeviceExternalSemaphorePropertiesKHR_ = nullptr;
};

#undef VK_KHR_external_semaphore
class VK_KHR_external_semaphore {
public:
  VK_KHR_external_semaphore(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#undef VK_KHR_external_semaphore_win32
class VK_KHR_external_semaphore_win32 {
public:
  VK_KHR_external_semaphore_win32(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkImportSemaphoreWin32HandleKHR(VkDevice device, const VkImportSemaphoreWin32HandleInfoKHR* pImportSemaphoreWin32HandleInfo) const;
  VkResult vkGetSemaphoreWin32HandleKHR(VkDevice device, const VkSemaphoreGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) const;

private:
  PFN_vkImportSemaphoreWin32HandleKHR vkImportSemaphoreWin32HandleKHR_ = nullptr;
  PFN_vkGetSemaphoreWin32HandleKHR vkGetSemaphoreWin32HandleKHR_ = nullptr;
};
#endif // VK_USE_PLATFORM_WIN32_KHR

#undef VK_KHR_external_semaphore_fd
class VK_KHR_external_semaphore_fd {
public:
  VK_KHR_external_semaphore_fd(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkImportSemaphoreFdKHR(VkDevice device, const VkImportSemaphoreFdInfoKHR* pImportSemaphoreFdInfo) const;
  VkResult vkGetSemaphoreFdKHR(VkDevice device, const VkSemaphoreGetFdInfoKHR* pGetFdInfo, int* pFd) const;

private:
  PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR_ = nullptr;
  PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR_ = nullptr;
};

#undef VK_KHR_push_descriptor
class VK_KHR_push_descriptor {
public:
  VK_KHR_push_descriptor(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  void vkCmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t set, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites) const;

private:
  PFN_vkCmdPushDescriptorSetKHR vkCmdPushDescriptorSetKHR_ = nullptr;
};

#undef VK_KHR_16bit_storage
class VK_KHR_16bit_storage {
public:
  VK_KHR_16bit_storage(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_KHR_incremental_present
class VK_KHR_incremental_present {
public:
  VK_KHR_incremental_present(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_KHR_descriptor_update_template
class VK_KHR_descriptor_update_template {
public:
  VK_KHR_descriptor_update_template(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkCreateDescriptorUpdateTemplateKHR(VkDevice device, const VkDescriptorUpdateTemplateCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorUpdateTemplateKHR* pDescriptorUpdateTemplate) const;
  void vkDestroyDescriptorUpdateTemplateKHR(VkDevice device, VkDescriptorUpdateTemplateKHR descriptorUpdateTemplate, const VkAllocationCallbacks* pAllocator) const;
  void vkUpdateDescriptorSetWithTemplateKHR(VkDevice device, VkDescriptorSet descriptorSet, VkDescriptorUpdateTemplateKHR descriptorUpdateTemplate, const void* pData) const;
  void vkCmdPushDescriptorSetWithTemplateKHR(VkCommandBuffer commandBuffer, VkDescriptorUpdateTemplateKHR descriptorUpdateTemplate, VkPipelineLayout layout, uint32_t set, const void* pData) const;

private:
  PFN_vkCreateDescriptorUpdateTemplateKHR vkCreateDescriptorUpdateTemplateKHR_ = nullptr;
  PFN_vkDestroyDescriptorUpdateTemplateKHR vkDestroyDescriptorUpdateTemplateKHR_ = nullptr;
  PFN_vkUpdateDescriptorSetWithTemplateKHR vkUpdateDescriptorSetWithTemplateKHR_ = nullptr;
  PFN_vkCmdPushDescriptorSetWithTemplateKHR vkCmdPushDescriptorSetWithTemplateKHR_ = nullptr;
};

#undef VK_NVX_device_generated_commands
class VK_NVX_device_generated_commands {
public:
  VK_NVX_device_generated_commands(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  void vkCmdProcessCommandsNVX(VkCommandBuffer commandBuffer, const VkCmdProcessCommandsInfoNVX* pProcessCommandsInfo) const;
  void vkCmdReserveSpaceForCommandsNVX(VkCommandBuffer commandBuffer, const VkCmdReserveSpaceForCommandsInfoNVX* pReserveSpaceInfo) const;
  VkResult vkCreateIndirectCommandsLayoutNVX(VkDevice device, const VkIndirectCommandsLayoutCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkIndirectCommandsLayoutNVX* pIndirectCommandsLayout) const;
  void vkDestroyIndirectCommandsLayoutNVX(VkDevice device, VkIndirectCommandsLayoutNVX indirectCommandsLayout, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkCreateObjectTableNVX(VkDevice device, const VkObjectTableCreateInfoNVX* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkObjectTableNVX* pObjectTable) const;
  void vkDestroyObjectTableNVX(VkDevice device, VkObjectTableNVX objectTable, const VkAllocationCallbacks* pAllocator) const;
  VkResult vkRegisterObjectsNVX(VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectTableEntryNVX* const* ppObjectTableEntries, const uint32_t* pObjectIndices) const;
  VkResult vkUnregisterObjectsNVX(VkDevice device, VkObjectTableNVX objectTable, uint32_t objectCount, const VkObjectEntryTypeNVX* pObjectEntryTypes, const uint32_t* pObjectIndices) const;
  void vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX(VkPhysicalDevice physicalDevice, VkDeviceGeneratedCommandsFeaturesNVX* pFeatures, VkDeviceGeneratedCommandsLimitsNVX* pLimits) const;

private:
  PFN_vkCmdProcessCommandsNVX vkCmdProcessCommandsNVX_ = nullptr;
  PFN_vkCmdReserveSpaceForCommandsNVX vkCmdReserveSpaceForCommandsNVX_ = nullptr;
  PFN_vkCreateIndirectCommandsLayoutNVX vkCreateIndirectCommandsLayoutNVX_ = nullptr;
  PFN_vkDestroyIndirectCommandsLayoutNVX vkDestroyIndirectCommandsLayoutNVX_ = nullptr;
  PFN_vkCreateObjectTableNVX vkCreateObjectTableNVX_ = nullptr;
  PFN_vkDestroyObjectTableNVX vkDestroyObjectTableNVX_ = nullptr;
  PFN_vkRegisterObjectsNVX vkRegisterObjectsNVX_ = nullptr;
  PFN_vkUnregisterObjectsNVX vkUnregisterObjectsNVX_ = nullptr;
  PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX_ = nullptr;
};

#undef VK_NV_clip_space_w_scaling
class VK_NV_clip_space_w_scaling {
public:
  VK_NV_clip_space_w_scaling(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  void vkCmdSetViewportWScalingNV(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewportWScalingNV* pViewportWScalings) const;

private:
  PFN_vkCmdSetViewportWScalingNV vkCmdSetViewportWScalingNV_ = nullptr;
};

#undef VK_EXT_direct_mode_display
class VK_EXT_direct_mode_display {
public:
  VK_EXT_direct_mode_display(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkReleaseDisplayEXT(VkPhysicalDevice physicalDevice, VkDisplayKHR display) const;

private:
  PFN_vkReleaseDisplayEXT vkReleaseDisplayEXT_ = nullptr;
};

#if defined(VK_USE_PLATFORM_XLIB_XRANDR_EXT)
#undef VK_EXT_acquire_xlib_display
class VK_EXT_acquire_xlib_display {
public:
  VK_EXT_acquire_xlib_display(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkAcquireXlibDisplayEXT(VkPhysicalDevice physicalDevice, ?* dpy, VkDisplayKHR display) const;
  VkResult vkGetRandROutputDisplayEXT(VkPhysicalDevice physicalDevice, ?* dpy, RROutput rrOutput, VkDisplayKHR* pDisplay) const;

private:
  PFN_vkAcquireXlibDisplayEXT vkAcquireXlibDisplayEXT_ = nullptr;
  PFN_vkGetRandROutputDisplayEXT vkGetRandROutputDisplayEXT_ = nullptr;
};
#endif // VK_USE_PLATFORM_XLIB_XRANDR_EXT

#undef VK_EXT_display_surface_counter
class VK_EXT_display_surface_counter {
public:
  VK_EXT_display_surface_counter(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkGetPhysicalDeviceSurfaceCapabilities2EXT(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSurfaceCapabilities2EXT* pSurfaceCapabilities) const;

private:
  PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT vkGetPhysicalDeviceSurfaceCapabilities2EXT_ = nullptr;
};

#undef VK_EXT_display_control
class VK_EXT_display_control {
public:
  VK_EXT_display_control(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkDisplayPowerControlEXT(VkDevice device, VkDisplayKHR display, const VkDisplayPowerInfoEXT* pDisplayPowerInfo) const;
  VkResult vkRegisterDeviceEventEXT(VkDevice device, const VkDeviceEventInfoEXT* pDeviceEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) const;
  VkResult vkRegisterDisplayEventEXT(VkDevice device, VkDisplayKHR display, const VkDisplayEventInfoEXT* pDisplayEventInfo, const VkAllocationCallbacks* pAllocator, VkFence* pFence) const;
  VkResult vkGetSwapchainCounterEXT(VkDevice device, VkSwapchainKHR swapchain, VkSurfaceCounterFlagBitsEXT counter, uint64_t* pCounterValue) const;

private:
  PFN_vkDisplayPowerControlEXT vkDisplayPowerControlEXT_ = nullptr;
  PFN_vkRegisterDeviceEventEXT vkRegisterDeviceEventEXT_ = nullptr;
  PFN_vkRegisterDisplayEventEXT vkRegisterDisplayEventEXT_ = nullptr;
  PFN_vkGetSwapchainCounterEXT vkGetSwapchainCounterEXT_ = nullptr;
};

#undef VK_GOOGLE_display_timing
class VK_GOOGLE_display_timing {
public:
  VK_GOOGLE_display_timing(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkGetRefreshCycleDurationGOOGLE(VkDevice device, VkSwapchainKHR swapchain, VkRefreshCycleDurationGOOGLE* pDisplayTimingProperties) const;
  VkResult vkGetPastPresentationTimingGOOGLE(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pPresentationTimingCount, VkPastPresentationTimingGOOGLE* pPresentationTimings) const;

private:
  PFN_vkGetRefreshCycleDurationGOOGLE vkGetRefreshCycleDurationGOOGLE_ = nullptr;
  PFN_vkGetPastPresentationTimingGOOGLE vkGetPastPresentationTimingGOOGLE_ = nullptr;
};

#undef VK_NV_sample_mask_override_coverage
class VK_NV_sample_mask_override_coverage {
public:
  VK_NV_sample_mask_override_coverage(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_NV_geometry_shader_passthrough
class VK_NV_geometry_shader_passthrough {
public:
  VK_NV_geometry_shader_passthrough(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_NV_viewport_array2
class VK_NV_viewport_array2 {
public:
  VK_NV_viewport_array2(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_NVX_multiview_per_view_attributes
class VK_NVX_multiview_per_view_attributes {
public:
  VK_NVX_multiview_per_view_attributes(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_NV_viewport_swizzle
class VK_NV_viewport_swizzle {
public:
  VK_NV_viewport_swizzle(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_EXT_discard_rectangles
class VK_EXT_discard_rectangles {
public:
  VK_EXT_discard_rectangles(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  void vkCmdSetDiscardRectangleEXT(VkCommandBuffer commandBuffer, uint32_t firstDiscardRectangle, uint32_t discardRectangleCount, const VkRect2D* pDiscardRectangles) const;

private:
  PFN_vkCmdSetDiscardRectangleEXT vkCmdSetDiscardRectangleEXT_ = nullptr;
};

#undef VK_EXT_swapchain_colorspace
class VK_EXT_swapchain_colorspace {
public:
  VK_EXT_swapchain_colorspace(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);

private:
};

#undef VK_EXT_hdr_metadata
class VK_EXT_hdr_metadata {
public:
  VK_EXT_hdr_metadata(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  void vkSetHdrMetadataEXT(VkDevice device, uint32_t swapchainCount, const VkSwapchainKHR* pSwapchains, const VkHdrMetadataEXT* pMetadata) const;

private:
  PFN_vkSetHdrMetadataEXT vkSetHdrMetadataEXT_ = nullptr;
};

#undef VK_KHR_shared_presentable_image
class VK_KHR_shared_presentable_image {
public:
  VK_KHR_shared_presentable_image(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkGetSwapchainStatusKHR(VkDevice device, VkSwapchainKHR swapchain) const;

private:
  PFN_vkGetSwapchainStatusKHR vkGetSwapchainStatusKHR_ = nullptr;
};

#undef VK_KHR_external_fence_capabilities
class VK_KHR_external_fence_capabilities {
public:
  VK_KHR_external_fence_capabilities(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  void vkGetPhysicalDeviceExternalFencePropertiesKHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceExternalFenceInfoKHR* pExternalFenceInfo, VkExternalFencePropertiesKHR* pExternalFenceProperties) const;

private:
  PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR vkGetPhysicalDeviceExternalFencePropertiesKHR_ = nullptr;
};

#undef VK_KHR_external_fence
class VK_KHR_external_fence {
public:
  VK_KHR_external_fence(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#if defined(VK_USE_PLATFORM_WIN32_KHR)
#undef VK_KHR_external_fence_win32
class VK_KHR_external_fence_win32 {
public:
  VK_KHR_external_fence_win32(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkImportFenceWin32HandleKHR(VkDevice device, const VkImportFenceWin32HandleInfoKHR* pImportFenceWin32HandleInfo) const;
  VkResult vkGetFenceWin32HandleKHR(VkDevice device, const VkFenceGetWin32HandleInfoKHR* pGetWin32HandleInfo, HANDLE* pHandle) const;

private:
  PFN_vkImportFenceWin32HandleKHR vkImportFenceWin32HandleKHR_ = nullptr;
  PFN_vkGetFenceWin32HandleKHR vkGetFenceWin32HandleKHR_ = nullptr;
};
#endif // VK_USE_PLATFORM_WIN32_KHR

#undef VK_KHR_external_fence_fd
class VK_KHR_external_fence_fd {
public:
  VK_KHR_external_fence_fd(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  VkResult vkImportFenceFdKHR(VkDevice device, const VkImportFenceFdInfoKHR* pImportFenceFdInfo) const;
  VkResult vkGetFenceFdKHR(VkDevice device, const VkFenceGetFdInfoKHR* pGetFdInfo, int* pFd) const;

private:
  PFN_vkImportFenceFdKHR vkImportFenceFdKHR_ = nullptr;
  PFN_vkGetFenceFdKHR vkGetFenceFdKHR_ = nullptr;
};

#undef VK_KHR_get_surface_capabilities2
class VK_KHR_get_surface_capabilities2 {
public:
  VK_KHR_get_surface_capabilities2(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkGetPhysicalDeviceSurfaceCapabilities2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, VkSurfaceCapabilities2KHR* pSurfaceCapabilities) const;
  VkResult vkGetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR* pSurfaceInfo, uint32_t* pSurfaceFormatCount, VkSurfaceFormat2KHR* pSurfaceFormats) const;

private:
  PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR vkGetPhysicalDeviceSurfaceCapabilities2KHR_ = nullptr;
  PFN_vkGetPhysicalDeviceSurfaceFormats2KHR vkGetPhysicalDeviceSurfaceFormats2KHR_ = nullptr;
};

#undef VK_KHR_variable_pointers
class VK_KHR_variable_pointers {
public:
  VK_KHR_variable_pointers(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#if defined(VK_USE_PLATFORM_IOS_MVK)
#undef VK_MVK_ios_surface
class VK_MVK_ios_surface {
public:
  VK_MVK_ios_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateIOSSurfaceMVK(VkInstance instance, const VkIOSSurfaceCreateInfoMVK* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;

private:
  PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK_ = nullptr;
};
#endif // VK_USE_PLATFORM_IOS_MVK

#if defined(VK_USE_PLATFORM_MACOS_MVK)
#undef VK_MVK_macos_surface
class VK_MVK_macos_surface {
public:
  VK_MVK_macos_surface(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance);
  VkResult vkCreateMacOSSurfaceMVK(VkInstance instance, const VkMacOSSurfaceCreateInfoMVK* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface) const;

private:
  PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK_ = nullptr;
};
#endif // VK_USE_PLATFORM_MACOS_MVK

#undef VK_KHR_dedicated_allocation
class VK_KHR_dedicated_allocation {
public:
  VK_KHR_dedicated_allocation(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_EXT_sampler_filter_minmax
class VK_EXT_sampler_filter_minmax {
public:
  VK_EXT_sampler_filter_minmax(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_KHR_storage_buffer_storage_class
class VK_KHR_storage_buffer_storage_class {
public:
  VK_KHR_storage_buffer_storage_class(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_AMD_gpu_shader_int16
class VK_AMD_gpu_shader_int16 {
public:
  VK_AMD_gpu_shader_int16(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_AMD_mixed_attachment_samples
class VK_AMD_mixed_attachment_samples {
public:
  VK_AMD_mixed_attachment_samples(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_KHR_relaxed_block_layout
class VK_KHR_relaxed_block_layout {
public:
  VK_KHR_relaxed_block_layout(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_KHR_get_memory_requirements2
class VK_KHR_get_memory_requirements2 {
public:
  VK_KHR_get_memory_requirements2(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);
  void vkGetImageMemoryRequirements2KHR(VkDevice device, const VkImageMemoryRequirementsInfo2KHR* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements) const;
  void vkGetBufferMemoryRequirements2KHR(VkDevice device, const VkBufferMemoryRequirementsInfo2KHR* pInfo, VkMemoryRequirements2KHR* pMemoryRequirements) const;
  void vkGetImageSparseMemoryRequirements2KHR(VkDevice device, const VkImageSparseMemoryRequirementsInfo2KHR* pInfo, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements2KHR* pSparseMemoryRequirements) const;

private:
  PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR_ = nullptr;
  PFN_vkGetBufferMemoryRequirements2KHR vkGetBufferMemoryRequirements2KHR_ = nullptr;
  PFN_vkGetImageSparseMemoryRequirements2KHR vkGetImageSparseMemoryRequirements2KHR_ = nullptr;
};

#undef VK_EXT_blend_operation_advanced
class VK_EXT_blend_operation_advanced {
public:
  VK_EXT_blend_operation_advanced(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_NV_fragment_coverage_to_color
class VK_NV_fragment_coverage_to_color {
public:
  VK_NV_fragment_coverage_to_color(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_NV_framebuffer_mixed_samples
class VK_NV_framebuffer_mixed_samples {
public:
  VK_NV_framebuffer_mixed_samples(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_NV_fill_rectangle
class VK_NV_fill_rectangle {
public:
  VK_NV_fill_rectangle(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

#undef VK_EXT_post_depth_coverage
class VK_EXT_post_depth_coverage {
public:
  VK_EXT_post_depth_coverage(VulkanGlobalTable const* globals, VulkanInstanceTable const* instance, VulkanDeviceTable const* device);

private:
};

} // vkgen

#endif // VK_DISPATCH_TABLES_INCLUDE

