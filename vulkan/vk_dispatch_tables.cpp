#include "vk_dispatch_tables.h"

#include <stdexcept>

namespace vkgen {

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

PFN_vkVoidFunction VulkanGlobalTable::vkGetInstanceProcAddr(VkInstance instance, const char* pName) {
  this->vkGetInstanceProcAddr_(instance, pName);
}

VkResult VulkanGlobalTable::vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance) {
  this->vkCreateInstance_(pCreateInfo, pAllocator, pInstance);
}

VkResult VulkanGlobalTable::vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties) {
  this->vkEnumerateInstanceExtensionProperties_(pLayerName, pPropertyCount, pProperties);
}

VkResult VulkanGlobalTable::vkEnumerateInstanceLayerProperties(uint32_t* pPropertyCount, VkLayerProperties* pProperties) {
  this->vkEnumerateInstanceLayerProperties_(pPropertyCount, pProperties);
}

} // vkgen
