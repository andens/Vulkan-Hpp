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

} // vkgen
