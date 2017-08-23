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

} // vkgen

#endif // VK_DISPATCH_TABLES_INCLUDE

