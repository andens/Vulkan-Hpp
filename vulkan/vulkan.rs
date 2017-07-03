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

// Rust bindings for Vulkan 1.0.48, generated from the Khronos Vulkan API XML Registry.
// See https://github.com/andens/Vulkan-Hpp for generator details.

#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

#[macro_use]
mod macros {
    pub use ::std::ffi::CString;
    pub use ::std::ops::{BitOr, BitAnd};
    pub use ::std::{fmt, mem};

    /*
    For regular enums, a repr(C) enum is used, which seems to be the way to go.
    Things become a bit more difficult for flags because Rust requires enum values
    to be valid variants, which is not the case when oring them together. Some
    tests by transmuting worked, but sometimes Rust would cast to some actual
    variant and this was not always obvious (matching for example). Instead, the
    flag enums use the newtype pattern to build a struct that wraps an integer.
    This struct enables bitwise operations, provides type safety, and scopes so
    that only particular values can be created, just like an enum. The problem now
    is that since we are working with a struct using a single member and not the
    underlying type, the ABI may not be the same as when working with the wrapped
    type directly. This could cause problems when passing this new type to C. A
    suggestion about transparency attribute has been proposed that would solve this
    problem, but it's slow going: https://github.com/rust-lang/rfcs/pull/1758. As
    long as it works for me I'll leave it like this. Note that the type keyword
    does not help here as it is just an alias and not an actual new type. When it
    comes to the variants, for now I can create them using functions. It's
    expected that associated constants (const values inside a struct) will land in
    the 1.20 version of the compiler which could replace the functions.
    */
    macro_rules! flag_definitions {
        ($bit_definitions:ident, { $($flag:ident = $flag_val:expr,)* }) => (
            #[repr(C)]
            pub enum $bit_definitions {
                $(
                    $flag = $flag_val,
                )*
            }
        )
    }

    macro_rules! bitmask {
        ($bitmask:ident) => (
            #[repr(C)]
            #[derive(Debug, Copy, Clone, PartialEq)]
            pub struct $bitmask {
                flags: VkFlags,
            }

            impl $bitmask {
                #[allow(dead_code)] // Don't know why this one warns... it's public
                pub fn none() -> $bitmask {
                    $bitmask { flags: 0 }
                }
            }
        )
    }

    macro_rules! flag_traits {
        ($bitmask:ident) => (
            impl fmt::Display for $bitmask {
                fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
                    write!(f, concat!(stringify!($bitmask), " {{\n}}"))
                }
            }
        );
        ($bitmask:ident, $bit_definitions:ident, { $($flag:ident = $flag_val:expr,)* }) => (
            impl BitOr for $bitmask {
                type Output = Self;

                fn bitor(self, rhs: Self) -> Self {
                    $bitmask { flags: self.flags | rhs.flags }
                }
            }

            impl BitOr<$bit_definitions> for $bitmask {
                type Output = Self;

                fn bitor(self, rhs: $bit_definitions) -> Self {
                    $bitmask { flags: self.flags | (rhs as VkFlags) }
                }
            }

            impl BitAnd for $bitmask {
                type Output = Self;

                fn bitand(self, rhs: Self) -> Self {
                    $bitmask { flags: self.flags & rhs.flags }
                }
            }

            impl BitAnd<$bit_definitions> for $bitmask {
                type Output = Self;

                fn bitand(self, rhs: $bit_definitions) -> Self {
                    $bitmask { flags: self.flags & (rhs as VkFlags) }
                }
            }

            impl fmt::Display for $bitmask {
                fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
                    write!(f, concat!(
                        stringify!($bitmask), " {{\n",
                            $(
                                "    [{}] ", stringify!($flag), "\n",
                            )* "}}"
                        )
                        //$(, if *self & $type_name::$flag() == $type_name::$flag() { if self.flags != 0 && $type_name::$flag().flags == 0 { " " } else { "x" } } else { " " } )*
                        //$(, if *self & $type_name::$flag() == $type_name::$flag() { "x" } else { " " } )*
                        $(, if self.flags & ($bit_definitions::$flag as VkFlags) == ($bit_definitions::$flag as VkFlags) { "x" } else { " " } )*
                    )
                }
            }
        )
    }

    macro_rules! vulkan_flags {
        ($bitmask:ident) => (
            bitmask!($bitmask);
            flag_traits!($bitmask);
        );
        ($bitmask:ident, $bit_definitions:ident, { $($flag:ident = $flag_val:expr,)* }) => (
            flag_definitions!($bit_definitions, {$($flag = $flag_val,)*});
            bitmask!($bitmask);
            flag_traits!($bitmask, $bit_definitions, {$($flag = $flag_val,)*});
        );
    }

    // I don't think I can use "system" as that translates into "C" for
    // 64 bit Windows, but Vulkan always uses "stdcall" on Windows.
    #[cfg(windows)]
    macro_rules! vk_fun {
        (($($param_id:ident: $param_type:ty),*) -> $return_type:ty) => (
            unsafe extern "stdcall" fn($($param_id: $param_type),*) -> $return_type
        );
    }

    #[cfg(not(windows))]
    macro_rules! vk_fun {
        (($($param_id:ident: $param_type:ty),*) -> $return_type:ty) => (
            unsafe extern "C" fn($($param_id: $param_type),*) -> $return_type
        );
    }

    // Generates a global dispatch table consisting of the provided member
    // functions. This table uses the entry function to load commands not
    // depending on an instance. For each function pointer an inline method
    // is generated to hide function pointer syntax.
    macro_rules! global_dispatch_table {
        { $($fun:ident => ($($param_id:ident: $param_type:ty),*) -> $return_type:ty,)* } => (
            // Define member function pointers
            pub struct GlobalDispatchTable {
                $(
                    $fun: vk_fun!(($($param_id: $param_type),*) -> $return_type),
                )*
            }

            impl GlobalDispatchTable {
                pub fn new(vulkan_entry: &VulkanEntry) -> Result<GlobalDispatchTable, String> {
                    unsafe {
                        Ok(GlobalDispatchTable {
                            // Attempt to load provided function pointers into
                            // their corresponding variables. Early exits in case
                            // of failure assures that success means that all
                            // pointers are valid to call.
                            $(
                                $fun: match vulkan_entry.vkGetInstanceProcAddr(0, CString::new(stringify!($fun)).unwrap().as_ptr()) {
                                    Some(f) => mem::transmute(f),
                                    None => return Err(String::from(concat!("Could not load ", stringify!($fun)))),
                                },
                            )*
                        })
                    }
                }

                // Generate unsafe methods that simply wraps an internal function
                // pointer. Note that creation of this struct ensures pointers
                // are valid, but unsafe is used here to indicate to the caller
                // that the method is a raw C-function behind the scenes.
                $(
                    #[inline]
                    pub unsafe fn $fun(&self $(, $param_id: $param_type)*) -> $return_type {
                        (self.$fun)($($param_id),*)
                    }
                )*
            }
        )
    }

    // Similar to the global_dispatch_table! macro, but creating the table additionally
    // requires a VkInstance to pass as parameter to vkGetInstanceProcAddr.
    macro_rules! instance_dispatch_table {
        { $($fun:ident => ($($param_id:ident: $param_type:ty),*) -> $return_type:ty,)* } => (
            pub struct InstanceDispatchTable {
                $(
                    $fun: vk_fun!(($($param_id: $param_type),*) -> $return_type),
                )*
            }

            impl InstanceDispatchTable {
                pub fn new(vulkan_entry: &VulkanEntry, instance: VkInstance) -> Result<InstanceDispatchTable, String> {
                    unsafe {
                        Ok(InstanceDispatchTable {
                            $(
                                $fun: match vulkan_entry.vkGetInstanceProcAddr(instance, CString::new(stringify!($fun)).unwrap().as_ptr()) {
                                    Some(f) => mem::transmute(f),
                                    None => return Err(String::from(concat!("Could not load ", stringify!($fun)))),
                                },
                            )*
                        })
                    }
                }

                $(
                    #[inline]
                    pub unsafe fn $fun(&self $(, $param_id: $param_type)*) -> $return_type {
                        (self.$fun)($($param_id),*)
                    }
                )*
            }
        )
    }

    // Similar to the other dispatch table macros, but this time we need the
    // instance dispatch table for vkGetDeviceProcAddr and a Device object to
    // generate the table for.
    macro_rules! device_dispatch_table {
        { $($fun:ident => ($($param_id:ident: $param_type:ty),*) -> $return_type:ty,)* } => (
            pub struct DeviceDispatchTable {
                $(
                    $fun: vk_fun!(($($param_id: $param_type),*) -> $return_type),
                )*
            }

            impl DeviceDispatchTable {
                pub fn new(instance_table: &InstanceDispatchTable, device: VkDevice) -> Result<DeviceDispatchTable, String> {
                    unsafe {
                        Ok(DeviceDispatchTable {
                            $(
                                $fun: match instance_table.vkGetDeviceProcAddr(device, CString::new(stringify!($fun)).unwrap().as_ptr()) {
                                    Some(f) => mem::transmute(f),
                                    None => return Err(String::from(concat!("Could not load ", stringify!($fun)))),
                                },
                            )*
                        })
                    }
                }

                $(
                    #[inline]
                    pub unsafe fn $fun(&self $(, $param_id: $param_type)*) -> $return_type {
                        (self.$fun)($($param_id),*)
                    }
                )*
            }
        )
    }

    macro_rules! load_function {
        (instance, $fun:ident, $vulkan_entry:ident, $instance:ident) => (
            match $vulkan_entry.vkGetInstanceProcAddr($instance, CString::new(stringify!($fun)).unwrap().as_ptr()) {
                Some(f) => mem::transmute(f),
                None => return Err(String::from(concat!("Could not load ", stringify!($fun)))),
            }
        );
        (instance, $fun:ident, $vulkan_entry:ident, $instance:ident, $instance_table:ident, $device:ident) => (
            match $vulkan_entry.vkGetInstanceProcAddr($instance, CString::new(stringify!($fun)).unwrap().as_ptr()) {
                Some(f) => mem::transmute(f),
                None => return Err(String::from(concat!("Could not load ", stringify!($fun)))),
            }
        );
        (device, $fun:ident, $vulkan_entry:ident, $instance:ident, $instance_table:ident, $device:ident) => (
            match $instance_table.vkGetDeviceProcAddr($device, CString::new(stringify!($fun)).unwrap().as_ptr()) {
                Some(f) => mem::transmute(f),
                None => return Err(String::from(concat!("Could not load ", stringify!($fun)))),
            }
        );
    }

    macro_rules! table_ctor {
        (instance, $table_name:ident $(, $fun_type:ident, $fun:ident)*) => (
            #[allow(unused_variables)] // Yes, vulkan_entry and instance are used
            pub fn new(vulkan_entry: &VulkanEntry, instance: VkInstance) -> Result<$table_name, String> {
                #[allow(unused_unsafe)]            
                unsafe {
                    Ok($table_name {
                        $(
                            $fun: load_function!($fun_type, $fun, vulkan_entry, instance),
                        )*
                    })
                }
            }
        );
        (device, $table_name:ident $(, $fun_type:ident, $fun:ident)*) => (
            #[allow(unused_variables)] // For device extensions, instance and device functions use different parameters
            pub fn new(vulkan_entry: &VulkanEntry, instance: VkInstance, instance_table: &InstanceDispatchTable, device: VkDevice) -> Result<$table_name, String> {
                #[allow(unused_unsafe)] // Yes, it is necessary. Don't know why it says it isn't
                unsafe {
                    Ok($table_name {
                        $(
                            $fun: load_function!($fun_type, $fun, vulkan_entry, instance, instance_table, device),
                        )*
                    })
                }
            }
        );
    }

    // Generates a dispatch table in a similar fashion as before. Slightly more
    // complex because we invoke it for multiple tables, and commands can be
    // either instance or device commands, which are loaded differently.
    macro_rules! extension_dispatch_table {
        { $table_name:ident | $ext_type:ident, { $([$fun_type:ident] $fun:ident => ($($param_id:ident: $param_type:ty),*) -> $return_type:ty,)* } } => (
            pub struct $table_name {
                $(
                    $fun: vk_fun!(($($param_id: $param_type),*) -> $return_type),
                )*
            }

            impl $table_name {
                table_ctor!($ext_type, $table_name $(,$fun_type, $fun)*);

                $(
                    #[inline]
                    pub unsafe fn $fun(&self $(, $param_id: $param_type)*) -> $return_type {
                        (self.$fun)($($param_id),*)
                    }
                )*
            }
        )
    }
} // mod macros

pub mod core {
    use super::macros::*;
    extern crate libloading;
    pub use ::std::os::raw::{c_void, c_char, c_int, c_ulong};

    pub fn VK_MAKE_VERSION(major: u32, minor: u32, patch: u32) -> u32 {
        (major << 22) | (minor << 12) | patch
    }

    pub type VkFlags = u32;
    pub type VkBool32 = u32;
    pub type VkDeviceSize = u64;
    pub type VkSampleMask = u32;

    pub type VkInstance = usize;
    pub type VkPhysicalDevice = usize;
    pub type VkDevice = usize;
    pub type VkQueue = usize;
    pub type VkSemaphore = u64;
    pub type VkCommandBuffer = usize;
    pub type VkFence = u64;
    pub type VkDeviceMemory = u64;
    pub type VkBuffer = u64;
    pub type VkImage = u64;
    pub type VkEvent = u64;
    pub type VkQueryPool = u64;
    pub type VkBufferView = u64;
    pub type VkImageView = u64;
    pub type VkShaderModule = u64;
    pub type VkPipelineCache = u64;
    pub type VkPipelineLayout = u64;
    pub type VkRenderPass = u64;
    pub type VkPipeline = u64;
    pub type VkDescriptorSetLayout = u64;
    pub type VkSampler = u64;
    pub type VkDescriptorPool = u64;
    pub type VkDescriptorSet = u64;
    pub type VkFramebuffer = u64;
    pub type VkCommandPool = u64;

    pub const VK_LOD_CLAMP_NONE: f32 = 1000.0;
    pub const VK_REMAINING_MIP_LEVELS: u32 = !0;
    pub const VK_REMAINING_ARRAY_LAYERS: u32 = !0;
    pub const VK_WHOLE_SIZE: u64 = !0;
    pub const VK_ATTACHMENT_UNUSED: u32 = !0;
    pub const VK_TRUE: u32 = 1;
    pub const VK_FALSE: u32 = 0;
    pub const VK_QUEUE_FAMILY_IGNORED: u32 = !0;
    pub const VK_SUBPASS_EXTERNAL: u32 = !0;
    pub const VK_MAX_PHYSICAL_DEVICE_NAME_SIZE: u32 = 256;
    pub const VK_UUID_SIZE: u32 = 16;
    pub const VK_MAX_MEMORY_TYPES: u32 = 32;
    pub const VK_MAX_MEMORY_HEAPS: u32 = 16;
    pub const VK_MAX_EXTENSION_NAME_SIZE: u32 = 256;
    pub const VK_MAX_DESCRIPTION_SIZE: u32 = 256;

    #[repr(C)]
    pub enum VkPipelineCacheHeaderVersion {
        VK_PIPELINE_CACHE_HEADER_VERSION_ONE = 1,
    }

    #[repr(C)]
    pub enum VkResult {
        VK_SUCCESS = 0,
        VK_NOT_READY = 1,
        VK_TIMEOUT = 2,
        VK_EVENT_SET = 3,
        VK_EVENT_RESET = 4,
        VK_INCOMPLETE = 5,
        VK_ERROR_OUT_OF_HOST_MEMORY = -1,
        VK_ERROR_OUT_OF_DEVICE_MEMORY = -2,
        VK_ERROR_INITIALIZATION_FAILED = -3,
        VK_ERROR_DEVICE_LOST = -4,
        VK_ERROR_MEMORY_MAP_FAILED = -5,
        VK_ERROR_LAYER_NOT_PRESENT = -6,
        VK_ERROR_EXTENSION_NOT_PRESENT = -7,
        VK_ERROR_FEATURE_NOT_PRESENT = -8,
        VK_ERROR_INCOMPATIBLE_DRIVER = -9,
        VK_ERROR_TOO_MANY_OBJECTS = -10,
        VK_ERROR_FORMAT_NOT_SUPPORTED = -11,
        VK_ERROR_FRAGMENTED_POOL = -12,
        VK_ERROR_SURFACE_LOST_KHR = -1000000000,
        VK_ERROR_NATIVE_WINDOW_IN_USE_KHR = -1000000001,
        VK_SUBOPTIMAL_KHR = 1000001003,
        VK_ERROR_OUT_OF_DATE_KHR = -1000001004,
        VK_ERROR_INCOMPATIBLE_DISPLAY_KHR = -1000003001,
        VK_ERROR_VALIDATION_FAILED_EXT = -1000011001,
        VK_ERROR_INVALID_SHADER_NV = -1000012000,
        VK_ERROR_OUT_OF_POOL_MEMORY_KHR = -1000069000,
        VK_ERROR_INVALID_EXTERNAL_HANDLE_KHX = -1000072003,
    }

    #[repr(C)]
    pub enum VkStructureType {
        VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1,
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2,
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3,
        VK_STRUCTURE_TYPE_SUBMIT_INFO = 4,
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5,
        VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE = 6,
        VK_STRUCTURE_TYPE_BIND_SPARSE_INFO = 7,
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO = 8,
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO = 9,
        VK_STRUCTURE_TYPE_EVENT_CREATE_INFO = 10,
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO = 11,
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12,
        VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO = 13,
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO = 14,
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO = 15,
        VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO = 16,
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO = 17,
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO = 18,
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO = 19,
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO = 20,
        VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO = 21,
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO = 22,
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO = 23,
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO = 24,
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO = 25,
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO = 26,
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO = 27,
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO = 28,
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO = 29,
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO = 30,
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO = 31,
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO = 32,
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO = 33,
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO = 34,
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET = 35,
        VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET = 36,
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO = 37,
        VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO = 38,
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO = 39,
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO = 40,
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO = 41,
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO = 42,
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO = 43,
        VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER = 44,
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER = 45,
        VK_STRUCTURE_TYPE_MEMORY_BARRIER = 46,
        VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO = 47,
        VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO = 48,
        VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR = 1000001000,
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR = 1000001001,
        VK_STRUCTURE_TYPE_DISPLAY_MODE_CREATE_INFO_KHR = 1000002000,
        VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR = 1000002001,
        VK_STRUCTURE_TYPE_DISPLAY_PRESENT_INFO_KHR = 1000003000,
        VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR = 1000004000,
        VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR = 1000005000,
        VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR = 1000006000,
        VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR = 1000007000,
        VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR = 1000008000,
        VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR = 1000009000,
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT = 1000011000,
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD = 1000018000,
        VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT = 1000022000,
        VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_TAG_INFO_EXT = 1000022001,
        VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT = 1000022002,
        VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV = 1000026000,
        VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_BUFFER_CREATE_INFO_NV = 1000026001,
        VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV = 1000026002,
        VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO_KHX = 1000053000,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHX = 1000053001,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHX = 1000053002,
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_NV = 1000056000,
        VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_NV = 1000056001,
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_NV = 1000057000,
        VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_NV = 1000057001,
        VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_NV = 1000058000,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR = 1000059000,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR = 1000059001,
        VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2_KHR = 1000059002,
        VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2_KHR = 1000059003,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2_KHR = 1000059004,
        VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR = 1000059005,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR = 1000059006,
        VK_STRUCTURE_TYPE_SPARSE_IMAGE_FORMAT_PROPERTIES_2_KHR = 1000059007,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SPARSE_IMAGE_FORMAT_INFO_2_KHR = 1000059008,
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHX = 1000060000,
        VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO_KHX = 1000060001,
        VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO_KHX = 1000060002,
        VK_STRUCTURE_TYPE_DEVICE_GROUP_RENDER_PASS_BEGIN_INFO_KHX = 1000060003,
        VK_STRUCTURE_TYPE_DEVICE_GROUP_COMMAND_BUFFER_BEGIN_INFO_KHX = 1000060004,
        VK_STRUCTURE_TYPE_DEVICE_GROUP_SUBMIT_INFO_KHX = 1000060005,
        VK_STRUCTURE_TYPE_DEVICE_GROUP_BIND_SPARSE_INFO_KHX = 1000060006,
        VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_CAPABILITIES_KHX = 1000060007,
        VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHX = 1000060008,
        VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHX = 1000060009,
        VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHX = 1000060010,
        VK_STRUCTURE_TYPE_DEVICE_GROUP_PRESENT_INFO_KHX = 1000060011,
        VK_STRUCTURE_TYPE_DEVICE_GROUP_SWAPCHAIN_CREATE_INFO_KHX = 1000060012,
        VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT = 1000061000,
        VK_STRUCTURE_TYPE_VI_SURFACE_CREATE_INFO_NN = 1000062000,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES_KHX = 1000070000,
        VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO_KHX = 1000070001,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO_KHX = 1000071000,
        VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES_KHX = 1000071001,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO_KHX = 1000071002,
        VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES_KHX = 1000071003,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES_KHX = 1000071004,
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO_KHX = 1000072000,
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHX = 1000072001,
        VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHX = 1000072002,
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHX = 1000073000,
        VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHX = 1000073001,
        VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHX = 1000073002,
        VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHX = 1000074000,
        VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHX = 1000074001,
        VK_STRUCTURE_TYPE_WIN32_KEYED_MUTEX_ACQUIRE_RELEASE_INFO_KHX = 1000075000,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO_KHX = 1000076000,
        VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES_KHX = 1000076001,
        VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO_KHX = 1000077000,
        VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHX = 1000078000,
        VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHX = 1000078001,
        VK_STRUCTURE_TYPE_D3D12_FENCE_SUBMIT_INFO_KHX = 1000078002,
        VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHX = 1000079000,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR = 1000080000,
        VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR = 1000084000,
        VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO_KHR = 1000085000,
        VK_STRUCTURE_TYPE_OBJECT_TABLE_CREATE_INFO_NVX = 1000086000,
        VK_STRUCTURE_TYPE_INDIRECT_COMMANDS_LAYOUT_CREATE_INFO_NVX = 1000086001,
        VK_STRUCTURE_TYPE_CMD_PROCESS_COMMANDS_INFO_NVX = 1000086002,
        VK_STRUCTURE_TYPE_CMD_RESERVE_SPACE_FOR_COMMANDS_INFO_NVX = 1000086003,
        VK_STRUCTURE_TYPE_DEVICE_GENERATED_COMMANDS_LIMITS_NVX = 1000086004,
        VK_STRUCTURE_TYPE_DEVICE_GENERATED_COMMANDS_FEATURES_NVX = 1000086005,
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_W_SCALING_STATE_CREATE_INFO_NV = 1000087000,
        VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES2_EXT = 1000090000,
        VK_STRUCTURE_TYPE_DISPLAY_POWER_INFO_EXT = 1000091000,
        VK_STRUCTURE_TYPE_DEVICE_EVENT_INFO_EXT = 1000091001,
        VK_STRUCTURE_TYPE_DISPLAY_EVENT_INFO_EXT = 1000091002,
        VK_STRUCTURE_TYPE_SWAPCHAIN_COUNTER_CREATE_INFO_EXT = 1000091003,
        VK_STRUCTURE_TYPE_PRESENT_TIMES_INFO_GOOGLE = 1000092000,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PER_VIEW_ATTRIBUTES_PROPERTIES_NVX = 1000097000,
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_SWIZZLE_STATE_CREATE_INFO_NV = 1000098000,
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DISCARD_RECTANGLE_PROPERTIES_EXT = 1000099000,
        VK_STRUCTURE_TYPE_PIPELINE_DISCARD_RECTANGLE_STATE_CREATE_INFO_EXT = 1000099001,
        VK_STRUCTURE_TYPE_HDR_METADATA_EXT = 1000105000,
        VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK = 1000122000,
        VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK = 1000123000,
    }

    #[repr(C)]
    pub enum VkSystemAllocationScope {
        VK_SYSTEM_ALLOCATION_SCOPE_COMMAND = 0,
        VK_SYSTEM_ALLOCATION_SCOPE_OBJECT = 1,
        VK_SYSTEM_ALLOCATION_SCOPE_CACHE = 2,
        VK_SYSTEM_ALLOCATION_SCOPE_DEVICE = 3,
        VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE = 4,
    }

    #[repr(C)]
    pub enum VkInternalAllocationType {
        VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE = 0,
    }

    #[repr(C)]
    pub enum VkFormat {
        VK_FORMAT_UNDEFINED = 0,
        VK_FORMAT_R4G4_UNORM_PACK8 = 1,
        VK_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
        VK_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
        VK_FORMAT_R5G6B5_UNORM_PACK16 = 4,
        VK_FORMAT_B5G6R5_UNORM_PACK16 = 5,
        VK_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
        VK_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
        VK_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
        VK_FORMAT_R8_UNORM = 9,
        VK_FORMAT_R8_SNORM = 10,
        VK_FORMAT_R8_USCALED = 11,
        VK_FORMAT_R8_SSCALED = 12,
        VK_FORMAT_R8_UINT = 13,
        VK_FORMAT_R8_SINT = 14,
        VK_FORMAT_R8_SRGB = 15,
        VK_FORMAT_R8G8_UNORM = 16,
        VK_FORMAT_R8G8_SNORM = 17,
        VK_FORMAT_R8G8_USCALED = 18,
        VK_FORMAT_R8G8_SSCALED = 19,
        VK_FORMAT_R8G8_UINT = 20,
        VK_FORMAT_R8G8_SINT = 21,
        VK_FORMAT_R8G8_SRGB = 22,
        VK_FORMAT_R8G8B8_UNORM = 23,
        VK_FORMAT_R8G8B8_SNORM = 24,
        VK_FORMAT_R8G8B8_USCALED = 25,
        VK_FORMAT_R8G8B8_SSCALED = 26,
        VK_FORMAT_R8G8B8_UINT = 27,
        VK_FORMAT_R8G8B8_SINT = 28,
        VK_FORMAT_R8G8B8_SRGB = 29,
        VK_FORMAT_B8G8R8_UNORM = 30,
        VK_FORMAT_B8G8R8_SNORM = 31,
        VK_FORMAT_B8G8R8_USCALED = 32,
        VK_FORMAT_B8G8R8_SSCALED = 33,
        VK_FORMAT_B8G8R8_UINT = 34,
        VK_FORMAT_B8G8R8_SINT = 35,
        VK_FORMAT_B8G8R8_SRGB = 36,
        VK_FORMAT_R8G8B8A8_UNORM = 37,
        VK_FORMAT_R8G8B8A8_SNORM = 38,
        VK_FORMAT_R8G8B8A8_USCALED = 39,
        VK_FORMAT_R8G8B8A8_SSCALED = 40,
        VK_FORMAT_R8G8B8A8_UINT = 41,
        VK_FORMAT_R8G8B8A8_SINT = 42,
        VK_FORMAT_R8G8B8A8_SRGB = 43,
        VK_FORMAT_B8G8R8A8_UNORM = 44,
        VK_FORMAT_B8G8R8A8_SNORM = 45,
        VK_FORMAT_B8G8R8A8_USCALED = 46,
        VK_FORMAT_B8G8R8A8_SSCALED = 47,
        VK_FORMAT_B8G8R8A8_UINT = 48,
        VK_FORMAT_B8G8R8A8_SINT = 49,
        VK_FORMAT_B8G8R8A8_SRGB = 50,
        VK_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
        VK_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
        VK_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
        VK_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
        VK_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
        VK_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
        VK_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
        VK_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
        VK_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
        VK_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
        VK_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
        VK_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
        VK_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
        VK_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
        VK_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
        VK_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
        VK_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
        VK_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
        VK_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
        VK_FORMAT_R16_UNORM = 70,
        VK_FORMAT_R16_SNORM = 71,
        VK_FORMAT_R16_USCALED = 72,
        VK_FORMAT_R16_SSCALED = 73,
        VK_FORMAT_R16_UINT = 74,
        VK_FORMAT_R16_SINT = 75,
        VK_FORMAT_R16_SFLOAT = 76,
        VK_FORMAT_R16G16_UNORM = 77,
        VK_FORMAT_R16G16_SNORM = 78,
        VK_FORMAT_R16G16_USCALED = 79,
        VK_FORMAT_R16G16_SSCALED = 80,
        VK_FORMAT_R16G16_UINT = 81,
        VK_FORMAT_R16G16_SINT = 82,
        VK_FORMAT_R16G16_SFLOAT = 83,
        VK_FORMAT_R16G16B16_UNORM = 84,
        VK_FORMAT_R16G16B16_SNORM = 85,
        VK_FORMAT_R16G16B16_USCALED = 86,
        VK_FORMAT_R16G16B16_SSCALED = 87,
        VK_FORMAT_R16G16B16_UINT = 88,
        VK_FORMAT_R16G16B16_SINT = 89,
        VK_FORMAT_R16G16B16_SFLOAT = 90,
        VK_FORMAT_R16G16B16A16_UNORM = 91,
        VK_FORMAT_R16G16B16A16_SNORM = 92,
        VK_FORMAT_R16G16B16A16_USCALED = 93,
        VK_FORMAT_R16G16B16A16_SSCALED = 94,
        VK_FORMAT_R16G16B16A16_UINT = 95,
        VK_FORMAT_R16G16B16A16_SINT = 96,
        VK_FORMAT_R16G16B16A16_SFLOAT = 97,
        VK_FORMAT_R32_UINT = 98,
        VK_FORMAT_R32_SINT = 99,
        VK_FORMAT_R32_SFLOAT = 100,
        VK_FORMAT_R32G32_UINT = 101,
        VK_FORMAT_R32G32_SINT = 102,
        VK_FORMAT_R32G32_SFLOAT = 103,
        VK_FORMAT_R32G32B32_UINT = 104,
        VK_FORMAT_R32G32B32_SINT = 105,
        VK_FORMAT_R32G32B32_SFLOAT = 106,
        VK_FORMAT_R32G32B32A32_UINT = 107,
        VK_FORMAT_R32G32B32A32_SINT = 108,
        VK_FORMAT_R32G32B32A32_SFLOAT = 109,
        VK_FORMAT_R64_UINT = 110,
        VK_FORMAT_R64_SINT = 111,
        VK_FORMAT_R64_SFLOAT = 112,
        VK_FORMAT_R64G64_UINT = 113,
        VK_FORMAT_R64G64_SINT = 114,
        VK_FORMAT_R64G64_SFLOAT = 115,
        VK_FORMAT_R64G64B64_UINT = 116,
        VK_FORMAT_R64G64B64_SINT = 117,
        VK_FORMAT_R64G64B64_SFLOAT = 118,
        VK_FORMAT_R64G64B64A64_UINT = 119,
        VK_FORMAT_R64G64B64A64_SINT = 120,
        VK_FORMAT_R64G64B64A64_SFLOAT = 121,
        VK_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
        VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,
        VK_FORMAT_D16_UNORM = 124,
        VK_FORMAT_X8_D24_UNORM_PACK32 = 125,
        VK_FORMAT_D32_SFLOAT = 126,
        VK_FORMAT_S8_UINT = 127,
        VK_FORMAT_D16_UNORM_S8_UINT = 128,
        VK_FORMAT_D24_UNORM_S8_UINT = 129,
        VK_FORMAT_D32_SFLOAT_S8_UINT = 130,
        VK_FORMAT_BC1_RGB_UNORM_BLOCK = 131,
        VK_FORMAT_BC1_RGB_SRGB_BLOCK = 132,
        VK_FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
        VK_FORMAT_BC1_RGBA_SRGB_BLOCK = 134,
        VK_FORMAT_BC2_UNORM_BLOCK = 135,
        VK_FORMAT_BC2_SRGB_BLOCK = 136,
        VK_FORMAT_BC3_UNORM_BLOCK = 137,
        VK_FORMAT_BC3_SRGB_BLOCK = 138,
        VK_FORMAT_BC4_UNORM_BLOCK = 139,
        VK_FORMAT_BC4_SNORM_BLOCK = 140,
        VK_FORMAT_BC5_UNORM_BLOCK = 141,
        VK_FORMAT_BC5_SNORM_BLOCK = 142,
        VK_FORMAT_BC6H_UFLOAT_BLOCK = 143,
        VK_FORMAT_BC6H_SFLOAT_BLOCK = 144,
        VK_FORMAT_BC7_UNORM_BLOCK = 145,
        VK_FORMAT_BC7_SRGB_BLOCK = 146,
        VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147,
        VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148,
        VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
        VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
        VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
        VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152,
        VK_FORMAT_EAC_R11_UNORM_BLOCK = 153,
        VK_FORMAT_EAC_R11_SNORM_BLOCK = 154,
        VK_FORMAT_EAC_R11G11_UNORM_BLOCK = 155,
        VK_FORMAT_EAC_R11G11_SNORM_BLOCK = 156,
        VK_FORMAT_ASTC_4x4_UNORM_BLOCK = 157,
        VK_FORMAT_ASTC_4x4_SRGB_BLOCK = 158,
        VK_FORMAT_ASTC_5x4_UNORM_BLOCK = 159,
        VK_FORMAT_ASTC_5x4_SRGB_BLOCK = 160,
        VK_FORMAT_ASTC_5x5_UNORM_BLOCK = 161,
        VK_FORMAT_ASTC_5x5_SRGB_BLOCK = 162,
        VK_FORMAT_ASTC_6x5_UNORM_BLOCK = 163,
        VK_FORMAT_ASTC_6x5_SRGB_BLOCK = 164,
        VK_FORMAT_ASTC_6x6_UNORM_BLOCK = 165,
        VK_FORMAT_ASTC_6x6_SRGB_BLOCK = 166,
        VK_FORMAT_ASTC_8x5_UNORM_BLOCK = 167,
        VK_FORMAT_ASTC_8x5_SRGB_BLOCK = 168,
        VK_FORMAT_ASTC_8x6_UNORM_BLOCK = 169,
        VK_FORMAT_ASTC_8x6_SRGB_BLOCK = 170,
        VK_FORMAT_ASTC_8x8_UNORM_BLOCK = 171,
        VK_FORMAT_ASTC_8x8_SRGB_BLOCK = 172,
        VK_FORMAT_ASTC_10x5_UNORM_BLOCK = 173,
        VK_FORMAT_ASTC_10x5_SRGB_BLOCK = 174,
        VK_FORMAT_ASTC_10x6_UNORM_BLOCK = 175,
        VK_FORMAT_ASTC_10x6_SRGB_BLOCK = 176,
        VK_FORMAT_ASTC_10x8_UNORM_BLOCK = 177,
        VK_FORMAT_ASTC_10x8_SRGB_BLOCK = 178,
        VK_FORMAT_ASTC_10x10_UNORM_BLOCK = 179,
        VK_FORMAT_ASTC_10x10_SRGB_BLOCK = 180,
        VK_FORMAT_ASTC_12x10_UNORM_BLOCK = 181,
        VK_FORMAT_ASTC_12x10_SRGB_BLOCK = 182,
        VK_FORMAT_ASTC_12x12_UNORM_BLOCK = 183,
        VK_FORMAT_ASTC_12x12_SRGB_BLOCK = 184,
        VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000,
        VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001,
        VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002,
        VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003,
        VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004,
        VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005,
        VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006,
        VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007,
    }

    #[repr(C)]
    pub enum VkImageType {
        VK_IMAGE_TYPE_1D = 0,
        VK_IMAGE_TYPE_2D = 1,
        VK_IMAGE_TYPE_3D = 2,
    }

    #[repr(C)]
    pub enum VkImageTiling {
        VK_IMAGE_TILING_OPTIMAL = 0,
        VK_IMAGE_TILING_LINEAR = 1,
    }

    #[repr(C)]
    pub enum VkPhysicalDeviceType {
        VK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
        VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
        VK_PHYSICAL_DEVICE_TYPE_CPU = 4,
    }

    #[repr(C)]
    pub enum VkQueryType {
        VK_QUERY_TYPE_OCCLUSION = 0,
        VK_QUERY_TYPE_PIPELINE_STATISTICS = 1,
        VK_QUERY_TYPE_TIMESTAMP = 2,
    }

    #[repr(C)]
    pub enum VkSharingMode {
        VK_SHARING_MODE_EXCLUSIVE = 0,
        VK_SHARING_MODE_CONCURRENT = 1,
    }

    #[repr(C)]
    pub enum VkImageLayout {
        VK_IMAGE_LAYOUT_UNDEFINED = 0,
        VK_IMAGE_LAYOUT_GENERAL = 1,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL = 2,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL = 5,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL = 6,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL = 7,
        VK_IMAGE_LAYOUT_PREINITIALIZED = 8,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR = 1000001002,
    }

    #[repr(C)]
    pub enum VkImageViewType {
        VK_IMAGE_VIEW_TYPE_1D = 0,
        VK_IMAGE_VIEW_TYPE_2D = 1,
        VK_IMAGE_VIEW_TYPE_3D = 2,
        VK_IMAGE_VIEW_TYPE_CUBE = 3,
        VK_IMAGE_VIEW_TYPE_1D_ARRAY = 4,
        VK_IMAGE_VIEW_TYPE_2D_ARRAY = 5,
        VK_IMAGE_VIEW_TYPE_CUBE_ARRAY = 6,
    }

    #[repr(C)]
    pub enum VkComponentSwizzle {
        VK_COMPONENT_SWIZZLE_IDENTITY = 0,
        VK_COMPONENT_SWIZZLE_ZERO = 1,
        VK_COMPONENT_SWIZZLE_ONE = 2,
        VK_COMPONENT_SWIZZLE_R = 3,
        VK_COMPONENT_SWIZZLE_G = 4,
        VK_COMPONENT_SWIZZLE_B = 5,
        VK_COMPONENT_SWIZZLE_A = 6,
    }

    #[repr(C)]
    pub enum VkVertexInputRate {
        VK_VERTEX_INPUT_RATE_VERTEX = 0,
        VK_VERTEX_INPUT_RATE_INSTANCE = 1,
    }

    #[repr(C)]
    pub enum VkPrimitiveTopology {
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST = 0,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST = 1,
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP = 2,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST = 3,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP = 4,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN = 5,
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY = 6,
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY = 7,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY = 8,
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY = 9,
        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST = 10,
    }

    #[repr(C)]
    pub enum VkPolygonMode {
        VK_POLYGON_MODE_FILL = 0,
        VK_POLYGON_MODE_LINE = 1,
        VK_POLYGON_MODE_POINT = 2,
    }

    #[repr(C)]
    pub enum VkFrontFace {
        VK_FRONT_FACE_COUNTER_CLOCKWISE = 0,
        VK_FRONT_FACE_CLOCKWISE = 1,
    }

    #[repr(C)]
    pub enum VkCompareOp {
        VK_COMPARE_OP_NEVER = 0,
        VK_COMPARE_OP_LESS = 1,
        VK_COMPARE_OP_EQUAL = 2,
        VK_COMPARE_OP_LESS_OR_EQUAL = 3,
        VK_COMPARE_OP_GREATER = 4,
        VK_COMPARE_OP_NOT_EQUAL = 5,
        VK_COMPARE_OP_GREATER_OR_EQUAL = 6,
        VK_COMPARE_OP_ALWAYS = 7,
    }

    #[repr(C)]
    pub enum VkStencilOp {
        VK_STENCIL_OP_KEEP = 0,
        VK_STENCIL_OP_ZERO = 1,
        VK_STENCIL_OP_REPLACE = 2,
        VK_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
        VK_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
        VK_STENCIL_OP_INVERT = 5,
        VK_STENCIL_OP_INCREMENT_AND_WRAP = 6,
        VK_STENCIL_OP_DECREMENT_AND_WRAP = 7,
    }

    #[repr(C)]
    pub enum VkLogicOp {
        VK_LOGIC_OP_CLEAR = 0,
        VK_LOGIC_OP_AND = 1,
        VK_LOGIC_OP_AND_REVERSE = 2,
        VK_LOGIC_OP_COPY = 3,
        VK_LOGIC_OP_AND_INVERTED = 4,
        VK_LOGIC_OP_NO_OP = 5,
        VK_LOGIC_OP_XOR = 6,
        VK_LOGIC_OP_OR = 7,
        VK_LOGIC_OP_NOR = 8,
        VK_LOGIC_OP_EQUIVALENT = 9,
        VK_LOGIC_OP_INVERT = 10,
        VK_LOGIC_OP_OR_REVERSE = 11,
        VK_LOGIC_OP_COPY_INVERTED = 12,
        VK_LOGIC_OP_OR_INVERTED = 13,
        VK_LOGIC_OP_NAND = 14,
        VK_LOGIC_OP_SET = 15,
    }

    #[repr(C)]
    pub enum VkBlendFactor {
        VK_BLEND_FACTOR_ZERO = 0,
        VK_BLEND_FACTOR_ONE = 1,
        VK_BLEND_FACTOR_SRC_COLOR = 2,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
        VK_BLEND_FACTOR_DST_COLOR = 4,
        VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
        VK_BLEND_FACTOR_SRC_ALPHA = 6,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
        VK_BLEND_FACTOR_DST_ALPHA = 8,
        VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
        VK_BLEND_FACTOR_CONSTANT_COLOR = 10,
        VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
        VK_BLEND_FACTOR_CONSTANT_ALPHA = 12,
        VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
        VK_BLEND_FACTOR_SRC_ALPHA_SATURATE = 14,
        VK_BLEND_FACTOR_SRC1_COLOR = 15,
        VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR = 16,
        VK_BLEND_FACTOR_SRC1_ALPHA = 17,
        VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA = 18,
    }

    #[repr(C)]
    pub enum VkBlendOp {
        VK_BLEND_OP_ADD = 0,
        VK_BLEND_OP_SUBTRACT = 1,
        VK_BLEND_OP_REVERSE_SUBTRACT = 2,
        VK_BLEND_OP_MIN = 3,
        VK_BLEND_OP_MAX = 4,
    }

    #[repr(C)]
    pub enum VkDynamicState {
        VK_DYNAMIC_STATE_VIEWPORT = 0,
        VK_DYNAMIC_STATE_SCISSOR = 1,
        VK_DYNAMIC_STATE_LINE_WIDTH = 2,
        VK_DYNAMIC_STATE_DEPTH_BIAS = 3,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS = 4,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS = 5,
        VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK = 6,
        VK_DYNAMIC_STATE_STENCIL_WRITE_MASK = 7,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE = 8,
        VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV = 1000087000,
        VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT = 1000099000,
    }

    #[repr(C)]
    pub enum VkFilter {
        VK_FILTER_NEAREST = 0,
        VK_FILTER_LINEAR = 1,
        VK_FILTER_CUBIC_IMG = 1000015000,
    }

    #[repr(C)]
    pub enum VkSamplerMipmapMode {
        VK_SAMPLER_MIPMAP_MODE_NEAREST = 0,
        VK_SAMPLER_MIPMAP_MODE_LINEAR = 1,
    }

    #[repr(C)]
    pub enum VkSamplerAddressMode {
        VK_SAMPLER_ADDRESS_MODE_REPEAT = 0,
        VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT = 1,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,
        VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE = 4,
    }

    #[repr(C)]
    pub enum VkBorderColor {
        VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK = 0,
        VK_BORDER_COLOR_INT_TRANSPARENT_BLACK = 1,
        VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK = 2,
        VK_BORDER_COLOR_INT_OPAQUE_BLACK = 3,
        VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE = 4,
        VK_BORDER_COLOR_INT_OPAQUE_WHITE = 5,
    }

    #[repr(C)]
    pub enum VkDescriptorType {
        VK_DESCRIPTOR_TYPE_SAMPLER = 0,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
        VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
    }

    #[repr(C)]
    pub enum VkAttachmentLoadOp {
        VK_ATTACHMENT_LOAD_OP_LOAD = 0,
        VK_ATTACHMENT_LOAD_OP_CLEAR = 1,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE = 2,
    }

    #[repr(C)]
    pub enum VkAttachmentStoreOp {
        VK_ATTACHMENT_STORE_OP_STORE = 0,
        VK_ATTACHMENT_STORE_OP_DONT_CARE = 1,
    }

    #[repr(C)]
    pub enum VkPipelineBindPoint {
        VK_PIPELINE_BIND_POINT_GRAPHICS = 0,
        VK_PIPELINE_BIND_POINT_COMPUTE = 1,
    }

    #[repr(C)]
    pub enum VkCommandBufferLevel {
        VK_COMMAND_BUFFER_LEVEL_PRIMARY = 0,
        VK_COMMAND_BUFFER_LEVEL_SECONDARY = 1,
    }

    #[repr(C)]
    pub enum VkIndexType {
        VK_INDEX_TYPE_UINT16 = 0,
        VK_INDEX_TYPE_UINT32 = 1,
    }

    #[repr(C)]
    pub enum VkSubpassContents {
        VK_SUBPASS_CONTENTS_INLINE = 0,
        VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS = 1,
    }

    vulkan_flags!(VkInstanceCreateFlags);
    vulkan_flags!(VkFormatFeatureFlags, VkFormatFeatureFlagBits, {
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT = 0x00000001,
        VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT = 0x00000002,
        VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT = 0x00000004,
        VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT = 0x00000008,
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT = 0x00000010,
        VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT = 0x00000020,
        VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT = 0x00000040,
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT = 0x00000080,
        VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT = 0x00000100,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000200,
        VK_FORMAT_FEATURE_BLIT_SRC_BIT = 0x00000400,
        VK_FORMAT_FEATURE_BLIT_DST_BIT = 0x00000800,
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT = 0x00001000,
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG = 0x00002000,
        VK_FORMAT_FEATURE_TRANSFER_SRC_BIT_KHR = 0x00004000,
        VK_FORMAT_FEATURE_TRANSFER_DST_BIT_KHR = 0x00008000,
    });
    vulkan_flags!(VkImageUsageFlags, VkImageUsageFlagBits, {
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT = 0x00000001,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT = 0x00000002,
        VK_IMAGE_USAGE_SAMPLED_BIT = 0x00000004,
        VK_IMAGE_USAGE_STORAGE_BIT = 0x00000008,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT = 0x00000010,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = 0x00000020,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT = 0x00000040,
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT = 0x00000080,
    });
    vulkan_flags!(VkImageCreateFlags, VkImageCreateFlagBits, {
        VK_IMAGE_CREATE_SPARSE_BINDING_BIT = 0x00000001,
        VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT = 0x00000002,
        VK_IMAGE_CREATE_SPARSE_ALIASED_BIT = 0x00000004,
        VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT = 0x00000008,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT = 0x00000010,
        VK_IMAGE_CREATE_BIND_SFR_BIT_KHX = 0x00000040,
        VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR = 0x00000020,
    });
    vulkan_flags!(VkSampleCountFlags, VkSampleCountFlagBits, {
        VK_SAMPLE_COUNT_1_BIT = 0x00000001,
        VK_SAMPLE_COUNT_2_BIT = 0x00000002,
        VK_SAMPLE_COUNT_4_BIT = 0x00000004,
        VK_SAMPLE_COUNT_8_BIT = 0x00000008,
        VK_SAMPLE_COUNT_16_BIT = 0x00000010,
        VK_SAMPLE_COUNT_32_BIT = 0x00000020,
        VK_SAMPLE_COUNT_64_BIT = 0x00000040,
    });
    vulkan_flags!(VkQueueFlags, VkQueueFlagBits, {
        VK_QUEUE_GRAPHICS_BIT = 0x00000001,
        VK_QUEUE_COMPUTE_BIT = 0x00000002,
        VK_QUEUE_TRANSFER_BIT = 0x00000004,
        VK_QUEUE_SPARSE_BINDING_BIT = 0x00000008,
    });
    vulkan_flags!(VkMemoryPropertyFlags, VkMemoryPropertyFlagBits, {
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT = 0x00000001,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT = 0x00000002,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT = 0x00000004,
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT = 0x00000008,
        VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT = 0x00000010,
    });
    vulkan_flags!(VkMemoryHeapFlags, VkMemoryHeapFlagBits, {
        VK_MEMORY_HEAP_DEVICE_LOCAL_BIT = 0x00000001,
        VK_MEMORY_HEAP_MULTI_INSTANCE_BIT_KHX = 0x00000002,
    });
    vulkan_flags!(VkDeviceCreateFlags);
    vulkan_flags!(VkDeviceQueueCreateFlags);
    vulkan_flags!(VkPipelineStageFlags, VkPipelineStageFlagBits, {
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT = 0x00000001,
        VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT = 0x00000002,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT = 0x00000004,
        VK_PIPELINE_STAGE_VERTEX_SHADER_BIT = 0x00000008,
        VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT = 0x00000010,
        VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = 0x00000020,
        VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT = 0x00000040,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT = 0x00000080,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = 0x00000100,
        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = 0x00000200,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT = 0x00000800,
        VK_PIPELINE_STAGE_TRANSFER_BIT = 0x00001000,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = 0x00002000,
        VK_PIPELINE_STAGE_HOST_BIT = 0x00004000,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT = 0x00008000,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT = 0x00010000,
        VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX = 0x00020000,
    });
    vulkan_flags!(VkMemoryMapFlags);
    vulkan_flags!(VkImageAspectFlags, VkImageAspectFlagBits, {
        VK_IMAGE_ASPECT_COLOR_BIT = 0x00000001,
        VK_IMAGE_ASPECT_DEPTH_BIT = 0x00000002,
        VK_IMAGE_ASPECT_STENCIL_BIT = 0x00000004,
        VK_IMAGE_ASPECT_METADATA_BIT = 0x00000008,
    });
    vulkan_flags!(VkSparseImageFormatFlags, VkSparseImageFormatFlagBits, {
        VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT = 0x00000001,
        VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT = 0x00000002,
        VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT = 0x00000004,
    });
    vulkan_flags!(VkSparseMemoryBindFlags, VkSparseMemoryBindFlagBits, {
        VK_SPARSE_MEMORY_BIND_METADATA_BIT = 0x00000001,
    });
    vulkan_flags!(VkFenceCreateFlags, VkFenceCreateFlagBits, {
        VK_FENCE_CREATE_SIGNALED_BIT = 0x00000001,
    });
    vulkan_flags!(VkSemaphoreCreateFlags);
    vulkan_flags!(VkEventCreateFlags);
    vulkan_flags!(VkQueryPoolCreateFlags);
    vulkan_flags!(VkQueryPipelineStatisticFlags, VkQueryPipelineStatisticFlagBits, {
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT = 0x00000001,
        VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT = 0x00000002,
        VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT = 0x00000004,
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT = 0x00000008,
        VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT = 0x00000010,
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT = 0x00000020,
        VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT = 0x00000040,
        VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT = 0x00000080,
        VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT = 0x00000100,
        VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT = 0x00000200,
        VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT = 0x00000400,
    });
    vulkan_flags!(VkQueryResultFlags, VkQueryResultFlagBits, {
        VK_QUERY_RESULT_64_BIT = 0x00000001,
        VK_QUERY_RESULT_WAIT_BIT = 0x00000002,
        VK_QUERY_RESULT_WITH_AVAILABILITY_BIT = 0x00000004,
        VK_QUERY_RESULT_PARTIAL_BIT = 0x00000008,
    });
    vulkan_flags!(VkBufferCreateFlags, VkBufferCreateFlagBits, {
        VK_BUFFER_CREATE_SPARSE_BINDING_BIT = 0x00000001,
        VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT = 0x00000002,
        VK_BUFFER_CREATE_SPARSE_ALIASED_BIT = 0x00000004,
    });
    vulkan_flags!(VkBufferUsageFlags, VkBufferUsageFlagBits, {
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT = 0x00000001,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT = 0x00000002,
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = 0x00000004,
        VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = 0x00000008,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT = 0x00000010,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT = 0x00000020,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x00000040,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x00000080,
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT = 0x00000100,
    });
    vulkan_flags!(VkBufferViewCreateFlags);
    vulkan_flags!(VkImageViewCreateFlags);
    vulkan_flags!(VkShaderModuleCreateFlags);
    vulkan_flags!(VkPipelineCacheCreateFlags);
    vulkan_flags!(VkPipelineCreateFlags, VkPipelineCreateFlagBits, {
        VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT = 0x00000001,
        VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT = 0x00000002,
        VK_PIPELINE_CREATE_DERIVATIVE_BIT = 0x00000004,
        VK_PIPELINE_CREATE_VIEW_INDEX_FROM_DEVICE_INDEX_BIT_KHX = 0x00000008,
        VK_PIPELINE_CREATE_DISPATCH_BASE_KHX = 0x00000010,
    });
    vulkan_flags!(VkPipelineShaderStageCreateFlags);
    vulkan_flags!(VkPipelineVertexInputStateCreateFlags);
    vulkan_flags!(VkPipelineInputAssemblyStateCreateFlags);
    vulkan_flags!(VkPipelineTessellationStateCreateFlags);
    vulkan_flags!(VkPipelineViewportStateCreateFlags);
    vulkan_flags!(VkPipelineRasterizationStateCreateFlags);
    vulkan_flags!(VkCullModeFlags, VkCullModeFlagBits, {
        VK_CULL_MODE_NONE = 0,
        VK_CULL_MODE_FRONT_BIT = 0x00000001,
        VK_CULL_MODE_BACK_BIT = 0x00000002,
        VK_CULL_MODE_FRONT_AND_BACK = 0x00000003,
    });
    vulkan_flags!(VkPipelineMultisampleStateCreateFlags);
    vulkan_flags!(VkPipelineDepthStencilStateCreateFlags);
    vulkan_flags!(VkPipelineColorBlendStateCreateFlags);
    vulkan_flags!(VkColorComponentFlags, VkColorComponentFlagBits, {
        VK_COLOR_COMPONENT_R_BIT = 0x00000001,
        VK_COLOR_COMPONENT_G_BIT = 0x00000002,
        VK_COLOR_COMPONENT_B_BIT = 0x00000004,
        VK_COLOR_COMPONENT_A_BIT = 0x00000008,
    });
    vulkan_flags!(VkPipelineDynamicStateCreateFlags);
    vulkan_flags!(VkPipelineLayoutCreateFlags);
    vulkan_flags!(VkShaderStageFlags, VkShaderStageFlagBits, {
        VK_SHADER_STAGE_VERTEX_BIT = 0x00000001,
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT = 0x00000002,
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 0x00000004,
        VK_SHADER_STAGE_GEOMETRY_BIT = 0x00000008,
        VK_SHADER_STAGE_FRAGMENT_BIT = 0x00000010,
        VK_SHADER_STAGE_COMPUTE_BIT = 0x00000020,
        VK_SHADER_STAGE_ALL_GRAPHICS = 0x0000001F,
        VK_SHADER_STAGE_ALL = 0x7FFFFFFF,
    });
    vulkan_flags!(VkSamplerCreateFlags);
    vulkan_flags!(VkDescriptorSetLayoutCreateFlags, VkDescriptorSetLayoutCreateFlagBits, {
        VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR = 0x00000001,
    });
    vulkan_flags!(VkDescriptorPoolCreateFlags, VkDescriptorPoolCreateFlagBits, {
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT = 0x00000001,
    });
    vulkan_flags!(VkDescriptorPoolResetFlags);
    vulkan_flags!(VkFramebufferCreateFlags);
    vulkan_flags!(VkRenderPassCreateFlags);
    vulkan_flags!(VkAttachmentDescriptionFlags, VkAttachmentDescriptionFlagBits, {
        VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT = 0x00000001,
    });
    vulkan_flags!(VkSubpassDescriptionFlags, VkSubpassDescriptionFlagBits, {
        VK_SUBPASS_DESCRIPTION_PER_VIEW_ATTRIBUTES_BIT_NVX = 0x00000001,
        VK_SUBPASS_DESCRIPTION_PER_VIEW_POSITION_X_ONLY_BIT_NVX = 0x00000002,
    });
    vulkan_flags!(VkAccessFlags, VkAccessFlagBits, {
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT = 0x00000001,
        VK_ACCESS_INDEX_READ_BIT = 0x00000002,
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT = 0x00000004,
        VK_ACCESS_UNIFORM_READ_BIT = 0x00000008,
        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT = 0x00000010,
        VK_ACCESS_SHADER_READ_BIT = 0x00000020,
        VK_ACCESS_SHADER_WRITE_BIT = 0x00000040,
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT = 0x00000080,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = 0x00000100,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = 0x00000200,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400,
        VK_ACCESS_TRANSFER_READ_BIT = 0x00000800,
        VK_ACCESS_TRANSFER_WRITE_BIT = 0x00001000,
        VK_ACCESS_HOST_READ_BIT = 0x00002000,
        VK_ACCESS_HOST_WRITE_BIT = 0x00004000,
        VK_ACCESS_MEMORY_READ_BIT = 0x00008000,
        VK_ACCESS_MEMORY_WRITE_BIT = 0x00010000,
        VK_ACCESS_COMMAND_PROCESS_READ_BIT_NVX = 0x00020000,
        VK_ACCESS_COMMAND_PROCESS_WRITE_BIT_NVX = 0x00040000,
    });
    vulkan_flags!(VkDependencyFlags, VkDependencyFlagBits, {
        VK_DEPENDENCY_BY_REGION_BIT = 0x00000001,
        VK_DEPENDENCY_VIEW_LOCAL_BIT_KHX = 0x00000002,
        VK_DEPENDENCY_DEVICE_GROUP_BIT_KHX = 0x00000004,
    });
    vulkan_flags!(VkCommandPoolCreateFlags, VkCommandPoolCreateFlagBits, {
        VK_COMMAND_POOL_CREATE_TRANSIENT_BIT = 0x00000001,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT = 0x00000002,
    });
    vulkan_flags!(VkCommandPoolResetFlags, VkCommandPoolResetFlagBits, {
        VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT = 0x00000001,
    });
    vulkan_flags!(VkCommandBufferUsageFlags, VkCommandBufferUsageFlagBits, {
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 0x00000001,
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT = 0x00000002,
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT = 0x00000004,
    });
    vulkan_flags!(VkQueryControlFlags, VkQueryControlFlagBits, {
        VK_QUERY_CONTROL_PRECISE_BIT = 0x00000001,
    });
    vulkan_flags!(VkCommandBufferResetFlags, VkCommandBufferResetFlagBits, {
        VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT = 0x00000001,
    });
    vulkan_flags!(VkStencilFaceFlags, VkStencilFaceFlagBits, {
        VK_STENCIL_FACE_FRONT_BIT = 0x00000001,
        VK_STENCIL_FACE_BACK_BIT = 0x00000002,
        VK_STENCIL_FRONT_AND_BACK = 0x00000003,
    });

    pub type PFN_vkAllocationFunction = vk_fun!((pUserData: *mut c_void, size: usize, alignment: usize, allocationScope: VkSystemAllocationScope) -> *mut c_void);
    pub type PFN_vkReallocationFunction = vk_fun!((pUserData: *mut c_void, pOriginal: *mut c_void, size: usize, alignment: usize, allocationScope: VkSystemAllocationScope) -> *mut c_void);
    pub type PFN_vkFreeFunction = vk_fun!((pUserData: *mut c_void, pMemory: *mut c_void) -> ());
    pub type PFN_vkInternalAllocationNotification = vk_fun!((pUserData: *mut c_void, size: usize, allocationType: VkInternalAllocationType, allocationScope: VkSystemAllocationScope) -> ());
    pub type PFN_vkInternalFreeNotification = vk_fun!((pUserData: *mut c_void, size: usize, allocationType: VkInternalAllocationType, allocationScope: VkSystemAllocationScope) -> ());
    pub type PFN_vkVoidFunction = vk_fun!(() -> ());

    #[repr(C)]
    pub struct VkApplicationInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub pApplicationName: *const c_char,
        pub applicationVersion: u32,
        pub pEngineName: *const c_char,
        pub engineVersion: u32,
        pub apiVersion: u32,
    }

    #[repr(C)]
    pub struct VkInstanceCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkInstanceCreateFlags,
        pub pApplicationInfo: *const VkApplicationInfo,
        pub enabledLayerCount: u32,
        pub ppEnabledLayerNames: *const *const c_char,
        pub enabledExtensionCount: u32,
        pub ppEnabledExtensionNames: *const *const c_char,
    }

    #[repr(C)]
    pub struct VkAllocationCallbacks {
        pub pUserData: *mut c_void,
        pub pfnAllocation: Option<PFN_vkAllocationFunction>,
        pub pfnReallocation: Option<PFN_vkReallocationFunction>,
        pub pfnFree: Option<PFN_vkFreeFunction>,
        pub pfnInternalAllocation: Option<PFN_vkInternalAllocationNotification>,
        pub pfnInternalFree: Option<PFN_vkInternalFreeNotification>,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceFeatures {
        pub robustBufferAccess: VkBool32,
        pub fullDrawIndexUint32: VkBool32,
        pub imageCubeArray: VkBool32,
        pub independentBlend: VkBool32,
        pub geometryShader: VkBool32,
        pub tessellationShader: VkBool32,
        pub sampleRateShading: VkBool32,
        pub dualSrcBlend: VkBool32,
        pub logicOp: VkBool32,
        pub multiDrawIndirect: VkBool32,
        pub drawIndirectFirstInstance: VkBool32,
        pub depthClamp: VkBool32,
        pub depthBiasClamp: VkBool32,
        pub fillModeNonSolid: VkBool32,
        pub depthBounds: VkBool32,
        pub wideLines: VkBool32,
        pub largePoints: VkBool32,
        pub alphaToOne: VkBool32,
        pub multiViewport: VkBool32,
        pub samplerAnisotropy: VkBool32,
        pub textureCompressionETC2: VkBool32,
        pub textureCompressionASTC_LDR: VkBool32,
        pub textureCompressionBC: VkBool32,
        pub occlusionQueryPrecise: VkBool32,
        pub pipelineStatisticsQuery: VkBool32,
        pub vertexPipelineStoresAndAtomics: VkBool32,
        pub fragmentStoresAndAtomics: VkBool32,
        pub shaderTessellationAndGeometryPointSize: VkBool32,
        pub shaderImageGatherExtended: VkBool32,
        pub shaderStorageImageExtendedFormats: VkBool32,
        pub shaderStorageImageMultisample: VkBool32,
        pub shaderStorageImageReadWithoutFormat: VkBool32,
        pub shaderStorageImageWriteWithoutFormat: VkBool32,
        pub shaderUniformBufferArrayDynamicIndexing: VkBool32,
        pub shaderSampledImageArrayDynamicIndexing: VkBool32,
        pub shaderStorageBufferArrayDynamicIndexing: VkBool32,
        pub shaderStorageImageArrayDynamicIndexing: VkBool32,
        pub shaderClipDistance: VkBool32,
        pub shaderCullDistance: VkBool32,
        pub shaderFloat64: VkBool32,
        pub shaderInt64: VkBool32,
        pub shaderInt16: VkBool32,
        pub shaderResourceResidency: VkBool32,
        pub shaderResourceMinLod: VkBool32,
        pub sparseBinding: VkBool32,
        pub sparseResidencyBuffer: VkBool32,
        pub sparseResidencyImage2D: VkBool32,
        pub sparseResidencyImage3D: VkBool32,
        pub sparseResidency2Samples: VkBool32,
        pub sparseResidency4Samples: VkBool32,
        pub sparseResidency8Samples: VkBool32,
        pub sparseResidency16Samples: VkBool32,
        pub sparseResidencyAliased: VkBool32,
        pub variableMultisampleRate: VkBool32,
        pub inheritedQueries: VkBool32,
    }

    #[repr(C)]
    pub struct VkFormatProperties {
        pub linearTilingFeatures: VkFormatFeatureFlags,
        pub optimalTilingFeatures: VkFormatFeatureFlags,
        pub bufferFeatures: VkFormatFeatureFlags,
    }

    #[repr(C)]
    pub struct VkExtent3D {
        pub width: u32,
        pub height: u32,
        pub depth: u32,
    }

    #[repr(C)]
    pub struct VkImageFormatProperties {
        pub maxExtent: VkExtent3D,
        pub maxMipLevels: u32,
        pub maxArrayLayers: u32,
        pub sampleCounts: VkSampleCountFlags,
        pub maxResourceSize: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceLimits {
        pub maxImageDimension1D: u32,
        pub maxImageDimension2D: u32,
        pub maxImageDimension3D: u32,
        pub maxImageDimensionCube: u32,
        pub maxImageArrayLayers: u32,
        pub maxTexelBufferElements: u32,
        pub maxUniformBufferRange: u32,
        pub maxStorageBufferRange: u32,
        pub maxPushConstantsSize: u32,
        pub maxMemoryAllocationCount: u32,
        pub maxSamplerAllocationCount: u32,
        pub bufferImageGranularity: VkDeviceSize,
        pub sparseAddressSpaceSize: VkDeviceSize,
        pub maxBoundDescriptorSets: u32,
        pub maxPerStageDescriptorSamplers: u32,
        pub maxPerStageDescriptorUniformBuffers: u32,
        pub maxPerStageDescriptorStorageBuffers: u32,
        pub maxPerStageDescriptorSampledImages: u32,
        pub maxPerStageDescriptorStorageImages: u32,
        pub maxPerStageDescriptorInputAttachments: u32,
        pub maxPerStageResources: u32,
        pub maxDescriptorSetSamplers: u32,
        pub maxDescriptorSetUniformBuffers: u32,
        pub maxDescriptorSetUniformBuffersDynamic: u32,
        pub maxDescriptorSetStorageBuffers: u32,
        pub maxDescriptorSetStorageBuffersDynamic: u32,
        pub maxDescriptorSetSampledImages: u32,
        pub maxDescriptorSetStorageImages: u32,
        pub maxDescriptorSetInputAttachments: u32,
        pub maxVertexInputAttributes: u32,
        pub maxVertexInputBindings: u32,
        pub maxVertexInputAttributeOffset: u32,
        pub maxVertexInputBindingStride: u32,
        pub maxVertexOutputComponents: u32,
        pub maxTessellationGenerationLevel: u32,
        pub maxTessellationPatchSize: u32,
        pub maxTessellationControlPerVertexInputComponents: u32,
        pub maxTessellationControlPerVertexOutputComponents: u32,
        pub maxTessellationControlPerPatchOutputComponents: u32,
        pub maxTessellationControlTotalOutputComponents: u32,
        pub maxTessellationEvaluationInputComponents: u32,
        pub maxTessellationEvaluationOutputComponents: u32,
        pub maxGeometryShaderInvocations: u32,
        pub maxGeometryInputComponents: u32,
        pub maxGeometryOutputComponents: u32,
        pub maxGeometryOutputVertices: u32,
        pub maxGeometryTotalOutputComponents: u32,
        pub maxFragmentInputComponents: u32,
        pub maxFragmentOutputAttachments: u32,
        pub maxFragmentDualSrcAttachments: u32,
        pub maxFragmentCombinedOutputResources: u32,
        pub maxComputeSharedMemorySize: u32,
        pub maxComputeWorkGroupCount: [u32; 3 as usize],
        pub maxComputeWorkGroupInvocations: u32,
        pub maxComputeWorkGroupSize: [u32; 3 as usize],
        pub subPixelPrecisionBits: u32,
        pub subTexelPrecisionBits: u32,
        pub mipmapPrecisionBits: u32,
        pub maxDrawIndexedIndexValue: u32,
        pub maxDrawIndirectCount: u32,
        pub maxSamplerLodBias: f32,
        pub maxSamplerAnisotropy: f32,
        pub maxViewports: u32,
        pub maxViewportDimensions: [u32; 2 as usize],
        pub viewportBoundsRange: [f32; 2 as usize],
        pub viewportSubPixelBits: u32,
        pub minMemoryMapAlignment: usize,
        pub minTexelBufferOffsetAlignment: VkDeviceSize,
        pub minUniformBufferOffsetAlignment: VkDeviceSize,
        pub minStorageBufferOffsetAlignment: VkDeviceSize,
        pub minTexelOffset: i32,
        pub maxTexelOffset: u32,
        pub minTexelGatherOffset: i32,
        pub maxTexelGatherOffset: u32,
        pub minInterpolationOffset: f32,
        pub maxInterpolationOffset: f32,
        pub subPixelInterpolationOffsetBits: u32,
        pub maxFramebufferWidth: u32,
        pub maxFramebufferHeight: u32,
        pub maxFramebufferLayers: u32,
        pub framebufferColorSampleCounts: VkSampleCountFlags,
        pub framebufferDepthSampleCounts: VkSampleCountFlags,
        pub framebufferStencilSampleCounts: VkSampleCountFlags,
        pub framebufferNoAttachmentsSampleCounts: VkSampleCountFlags,
        pub maxColorAttachments: u32,
        pub sampledImageColorSampleCounts: VkSampleCountFlags,
        pub sampledImageIntegerSampleCounts: VkSampleCountFlags,
        pub sampledImageDepthSampleCounts: VkSampleCountFlags,
        pub sampledImageStencilSampleCounts: VkSampleCountFlags,
        pub storageImageSampleCounts: VkSampleCountFlags,
        pub maxSampleMaskWords: u32,
        pub timestampComputeAndGraphics: VkBool32,
        pub timestampPeriod: f32,
        pub maxClipDistances: u32,
        pub maxCullDistances: u32,
        pub maxCombinedClipAndCullDistances: u32,
        pub discreteQueuePriorities: u32,
        pub pointSizeRange: [f32; 2 as usize],
        pub lineWidthRange: [f32; 2 as usize],
        pub pointSizeGranularity: f32,
        pub lineWidthGranularity: f32,
        pub strictLines: VkBool32,
        pub standardSampleLocations: VkBool32,
        pub optimalBufferCopyOffsetAlignment: VkDeviceSize,
        pub optimalBufferCopyRowPitchAlignment: VkDeviceSize,
        pub nonCoherentAtomSize: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceSparseProperties {
        pub residencyStandard2DBlockShape: VkBool32,
        pub residencyStandard2DMultisampleBlockShape: VkBool32,
        pub residencyStandard3DBlockShape: VkBool32,
        pub residencyAlignedMipSize: VkBool32,
        pub residencyNonResidentStrict: VkBool32,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceProperties {
        pub apiVersion: u32,
        pub driverVersion: u32,
        pub vendorID: u32,
        pub deviceID: u32,
        pub deviceType: VkPhysicalDeviceType,
        pub deviceName: [c_char; VK_MAX_PHYSICAL_DEVICE_NAME_SIZE as usize],
        pub pipelineCacheUUID: [u8; VK_UUID_SIZE as usize],
        pub limits: VkPhysicalDeviceLimits,
        pub sparseProperties: VkPhysicalDeviceSparseProperties,
    }

    #[repr(C)]
    pub struct VkQueueFamilyProperties {
        pub queueFlags: VkQueueFlags,
        pub queueCount: u32,
        pub timestampValidBits: u32,
        pub minImageTransferGranularity: VkExtent3D,
    }

    #[repr(C)]
    pub struct VkMemoryType {
        pub propertyFlags: VkMemoryPropertyFlags,
        pub heapIndex: u32,
    }

    #[repr(C)]
    pub struct VkMemoryHeap {
        pub size: VkDeviceSize,
        pub flags: VkMemoryHeapFlags,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceMemoryProperties {
        pub memoryTypeCount: u32,
        pub memoryTypes: [VkMemoryType; VK_MAX_MEMORY_TYPES as usize],
        pub memoryHeapCount: u32,
        pub memoryHeaps: [VkMemoryHeap; VK_MAX_MEMORY_HEAPS as usize],
    }

    #[repr(C)]
    pub struct VkDeviceQueueCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkDeviceQueueCreateFlags,
        pub queueFamilyIndex: u32,
        pub queueCount: u32,
        pub pQueuePriorities: *const f32,
    }

    #[repr(C)]
    pub struct VkDeviceCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkDeviceCreateFlags,
        pub queueCreateInfoCount: u32,
        pub pQueueCreateInfos: *const VkDeviceQueueCreateInfo,
        pub enabledLayerCount: u32,
        pub ppEnabledLayerNames: *const *const c_char,
        pub enabledExtensionCount: u32,
        pub ppEnabledExtensionNames: *const *const c_char,
        pub pEnabledFeatures: *const VkPhysicalDeviceFeatures,
    }

    #[repr(C)]
    pub struct VkExtensionProperties {
        pub extensionName: [c_char; VK_MAX_EXTENSION_NAME_SIZE as usize],
        pub specVersion: u32,
    }

    #[repr(C)]
    pub struct VkLayerProperties {
        pub layerName: [c_char; VK_MAX_EXTENSION_NAME_SIZE as usize],
        pub specVersion: u32,
        pub implementationVersion: u32,
        pub description: [c_char; VK_MAX_DESCRIPTION_SIZE as usize],
    }

    #[repr(C)]
    pub struct VkSubmitInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub waitSemaphoreCount: u32,
        pub pWaitSemaphores: *const VkSemaphore,
        pub pWaitDstStageMask: *const VkPipelineStageFlags,
        pub commandBufferCount: u32,
        pub pCommandBuffers: *const VkCommandBuffer,
        pub signalSemaphoreCount: u32,
        pub pSignalSemaphores: *const VkSemaphore,
    }

    #[repr(C)]
    pub struct VkMemoryAllocateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub allocationSize: VkDeviceSize,
        pub memoryTypeIndex: u32,
    }

    #[repr(C)]
    pub struct VkMappedMemoryRange {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub memory: VkDeviceMemory,
        pub offset: VkDeviceSize,
        pub size: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkMemoryRequirements {
        pub size: VkDeviceSize,
        pub alignment: VkDeviceSize,
        pub memoryTypeBits: u32,
    }

    #[repr(C)]
    pub struct VkSparseImageFormatProperties {
        pub aspectMask: VkImageAspectFlags,
        pub imageGranularity: VkExtent3D,
        pub flags: VkSparseImageFormatFlags,
    }

    #[repr(C)]
    pub struct VkSparseImageMemoryRequirements {
        pub formatProperties: VkSparseImageFormatProperties,
        pub imageMipTailFirstLod: u32,
        pub imageMipTailSize: VkDeviceSize,
        pub imageMipTailOffset: VkDeviceSize,
        pub imageMipTailStride: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkSparseMemoryBind {
        pub resourceOffset: VkDeviceSize,
        pub size: VkDeviceSize,
        pub memory: VkDeviceMemory,
        pub memoryOffset: VkDeviceSize,
        pub flags: VkSparseMemoryBindFlags,
    }

    #[repr(C)]
    pub struct VkSparseBufferMemoryBindInfo {
        pub buffer: VkBuffer,
        pub bindCount: u32,
        pub pBinds: *const VkSparseMemoryBind,
    }

    #[repr(C)]
    pub struct VkSparseImageOpaqueMemoryBindInfo {
        pub image: VkImage,
        pub bindCount: u32,
        pub pBinds: *const VkSparseMemoryBind,
    }

    #[repr(C)]
    pub struct VkImageSubresource {
        pub aspectMask: VkImageAspectFlags,
        pub mipLevel: u32,
        pub arrayLayer: u32,
    }

    #[repr(C)]
    pub struct VkOffset3D {
        pub x: i32,
        pub y: i32,
        pub z: i32,
    }

    #[repr(C)]
    pub struct VkSparseImageMemoryBind {
        pub subresource: VkImageSubresource,
        pub offset: VkOffset3D,
        pub extent: VkExtent3D,
        pub memory: VkDeviceMemory,
        pub memoryOffset: VkDeviceSize,
        pub flags: VkSparseMemoryBindFlags,
    }

    #[repr(C)]
    pub struct VkSparseImageMemoryBindInfo {
        pub image: VkImage,
        pub bindCount: u32,
        pub pBinds: *const VkSparseImageMemoryBind,
    }

    #[repr(C)]
    pub struct VkBindSparseInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub waitSemaphoreCount: u32,
        pub pWaitSemaphores: *const VkSemaphore,
        pub bufferBindCount: u32,
        pub pBufferBinds: *const VkSparseBufferMemoryBindInfo,
        pub imageOpaqueBindCount: u32,
        pub pImageOpaqueBinds: *const VkSparseImageOpaqueMemoryBindInfo,
        pub imageBindCount: u32,
        pub pImageBinds: *const VkSparseImageMemoryBindInfo,
        pub signalSemaphoreCount: u32,
        pub pSignalSemaphores: *const VkSemaphore,
    }

    #[repr(C)]
    pub struct VkFenceCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkFenceCreateFlags,
    }

    #[repr(C)]
    pub struct VkSemaphoreCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkSemaphoreCreateFlags,
    }

    #[repr(C)]
    pub struct VkEventCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkEventCreateFlags,
    }

    #[repr(C)]
    pub struct VkQueryPoolCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkQueryPoolCreateFlags,
        pub queryType: VkQueryType,
        pub queryCount: u32,
        pub pipelineStatistics: VkQueryPipelineStatisticFlags,
    }

    #[repr(C)]
    pub struct VkBufferCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkBufferCreateFlags,
        pub size: VkDeviceSize,
        pub usage: VkBufferUsageFlags,
        pub sharingMode: VkSharingMode,
        pub queueFamilyIndexCount: u32,
        pub pQueueFamilyIndices: *const u32,
    }

    #[repr(C)]
    pub struct VkBufferViewCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkBufferViewCreateFlags,
        pub buffer: VkBuffer,
        pub format: VkFormat,
        pub offset: VkDeviceSize,
        pub range: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkImageCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkImageCreateFlags,
        pub imageType: VkImageType,
        pub format: VkFormat,
        pub extent: VkExtent3D,
        pub mipLevels: u32,
        pub arrayLayers: u32,
        pub samples: VkSampleCountFlagBits,
        pub tiling: VkImageTiling,
        pub usage: VkImageUsageFlags,
        pub sharingMode: VkSharingMode,
        pub queueFamilyIndexCount: u32,
        pub pQueueFamilyIndices: *const u32,
        pub initialLayout: VkImageLayout,
    }

    #[repr(C)]
    pub struct VkSubresourceLayout {
        pub offset: VkDeviceSize,
        pub size: VkDeviceSize,
        pub rowPitch: VkDeviceSize,
        pub arrayPitch: VkDeviceSize,
        pub depthPitch: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkComponentMapping {
        pub r: VkComponentSwizzle,
        pub g: VkComponentSwizzle,
        pub b: VkComponentSwizzle,
        pub a: VkComponentSwizzle,
    }

    #[repr(C)]
    pub struct VkImageSubresourceRange {
        pub aspectMask: VkImageAspectFlags,
        pub baseMipLevel: u32,
        pub levelCount: u32,
        pub baseArrayLayer: u32,
        pub layerCount: u32,
    }

    #[repr(C)]
    pub struct VkImageViewCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkImageViewCreateFlags,
        pub image: VkImage,
        pub viewType: VkImageViewType,
        pub format: VkFormat,
        pub components: VkComponentMapping,
        pub subresourceRange: VkImageSubresourceRange,
    }

    #[repr(C)]
    pub struct VkShaderModuleCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkShaderModuleCreateFlags,
        pub codeSize: usize,
        pub pCode: *const u32,
    }

    #[repr(C)]
    pub struct VkPipelineCacheCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineCacheCreateFlags,
        pub initialDataSize: usize,
        pub pInitialData: *const c_void,
    }

    #[repr(C)]
    pub struct VkSpecializationMapEntry {
        pub constantID: u32,
        pub offset: u32,
        pub size: usize,
    }

    #[repr(C)]
    pub struct VkSpecializationInfo {
        pub mapEntryCount: u32,
        pub pMapEntries: *const VkSpecializationMapEntry,
        pub dataSize: usize,
        pub pData: *const c_void,
    }

    #[repr(C)]
    pub struct VkPipelineShaderStageCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineShaderStageCreateFlags,
        pub stage: VkShaderStageFlagBits,
        pub module: VkShaderModule,
        pub pName: *const c_char,
        pub pSpecializationInfo: *const VkSpecializationInfo,
    }

    #[repr(C)]
    pub struct VkVertexInputBindingDescription {
        pub binding: u32,
        pub stride: u32,
        pub inputRate: VkVertexInputRate,
    }

    #[repr(C)]
    pub struct VkVertexInputAttributeDescription {
        pub location: u32,
        pub binding: u32,
        pub format: VkFormat,
        pub offset: u32,
    }

    #[repr(C)]
    pub struct VkPipelineVertexInputStateCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineVertexInputStateCreateFlags,
        pub vertexBindingDescriptionCount: u32,
        pub pVertexBindingDescriptions: *const VkVertexInputBindingDescription,
        pub vertexAttributeDescriptionCount: u32,
        pub pVertexAttributeDescriptions: *const VkVertexInputAttributeDescription,
    }

    #[repr(C)]
    pub struct VkPipelineInputAssemblyStateCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineInputAssemblyStateCreateFlags,
        pub topology: VkPrimitiveTopology,
        pub primitiveRestartEnable: VkBool32,
    }

    #[repr(C)]
    pub struct VkPipelineTessellationStateCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineTessellationStateCreateFlags,
        pub patchControlPoints: u32,
    }

    #[repr(C)]
    pub struct VkViewport {
        pub x: f32,
        pub y: f32,
        pub width: f32,
        pub height: f32,
        pub minDepth: f32,
        pub maxDepth: f32,
    }

    #[repr(C)]
    pub struct VkOffset2D {
        pub x: i32,
        pub y: i32,
    }

    #[repr(C)]
    pub struct VkExtent2D {
        pub width: u32,
        pub height: u32,
    }

    #[repr(C)]
    pub struct VkRect2D {
        pub offset: VkOffset2D,
        pub extent: VkExtent2D,
    }

    #[repr(C)]
    pub struct VkPipelineViewportStateCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineViewportStateCreateFlags,
        pub viewportCount: u32,
        pub pViewports: *const VkViewport,
        pub scissorCount: u32,
        pub pScissors: *const VkRect2D,
    }

    #[repr(C)]
    pub struct VkPipelineRasterizationStateCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineRasterizationStateCreateFlags,
        pub depthClampEnable: VkBool32,
        pub rasterizerDiscardEnable: VkBool32,
        pub polygonMode: VkPolygonMode,
        pub cullMode: VkCullModeFlags,
        pub frontFace: VkFrontFace,
        pub depthBiasEnable: VkBool32,
        pub depthBiasConstantFactor: f32,
        pub depthBiasClamp: f32,
        pub depthBiasSlopeFactor: f32,
        pub lineWidth: f32,
    }

    #[repr(C)]
    pub struct VkPipelineMultisampleStateCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineMultisampleStateCreateFlags,
        pub rasterizationSamples: VkSampleCountFlagBits,
        pub sampleShadingEnable: VkBool32,
        pub minSampleShading: f32,
        pub pSampleMask: *const VkSampleMask,
        pub alphaToCoverageEnable: VkBool32,
        pub alphaToOneEnable: VkBool32,
    }

    #[repr(C)]
    pub struct VkStencilOpState {
        pub failOp: VkStencilOp,
        pub passOp: VkStencilOp,
        pub depthFailOp: VkStencilOp,
        pub compareOp: VkCompareOp,
        pub compareMask: u32,
        pub writeMask: u32,
        pub reference: u32,
    }

    #[repr(C)]
    pub struct VkPipelineDepthStencilStateCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineDepthStencilStateCreateFlags,
        pub depthTestEnable: VkBool32,
        pub depthWriteEnable: VkBool32,
        pub depthCompareOp: VkCompareOp,
        pub depthBoundsTestEnable: VkBool32,
        pub stencilTestEnable: VkBool32,
        pub front: VkStencilOpState,
        pub back: VkStencilOpState,
        pub minDepthBounds: f32,
        pub maxDepthBounds: f32,
    }

    #[repr(C)]
    pub struct VkPipelineColorBlendAttachmentState {
        pub blendEnable: VkBool32,
        pub srcColorBlendFactor: VkBlendFactor,
        pub dstColorBlendFactor: VkBlendFactor,
        pub colorBlendOp: VkBlendOp,
        pub srcAlphaBlendFactor: VkBlendFactor,
        pub dstAlphaBlendFactor: VkBlendFactor,
        pub alphaBlendOp: VkBlendOp,
        pub colorWriteMask: VkColorComponentFlags,
    }

    #[repr(C)]
    pub struct VkPipelineColorBlendStateCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineColorBlendStateCreateFlags,
        pub logicOpEnable: VkBool32,
        pub logicOp: VkLogicOp,
        pub attachmentCount: u32,
        pub pAttachments: *const VkPipelineColorBlendAttachmentState,
        pub blendConstants: [f32; 4 as usize],
    }

    #[repr(C)]
    pub struct VkPipelineDynamicStateCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineDynamicStateCreateFlags,
        pub dynamicStateCount: u32,
        pub pDynamicStates: *const VkDynamicState,
    }

    #[repr(C)]
    pub struct VkGraphicsPipelineCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineCreateFlags,
        pub stageCount: u32,
        pub pStages: *const VkPipelineShaderStageCreateInfo,
        pub pVertexInputState: *const VkPipelineVertexInputStateCreateInfo,
        pub pInputAssemblyState: *const VkPipelineInputAssemblyStateCreateInfo,
        pub pTessellationState: *const VkPipelineTessellationStateCreateInfo,
        pub pViewportState: *const VkPipelineViewportStateCreateInfo,
        pub pRasterizationState: *const VkPipelineRasterizationStateCreateInfo,
        pub pMultisampleState: *const VkPipelineMultisampleStateCreateInfo,
        pub pDepthStencilState: *const VkPipelineDepthStencilStateCreateInfo,
        pub pColorBlendState: *const VkPipelineColorBlendStateCreateInfo,
        pub pDynamicState: *const VkPipelineDynamicStateCreateInfo,
        pub layout: VkPipelineLayout,
        pub renderPass: VkRenderPass,
        pub subpass: u32,
        pub basePipelineHandle: VkPipeline,
        pub basePipelineIndex: i32,
    }

    #[repr(C)]
    pub struct VkComputePipelineCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineCreateFlags,
        pub stage: VkPipelineShaderStageCreateInfo,
        pub layout: VkPipelineLayout,
        pub basePipelineHandle: VkPipeline,
        pub basePipelineIndex: i32,
    }

    #[repr(C)]
    pub struct VkPushConstantRange {
        pub stageFlags: VkShaderStageFlags,
        pub offset: u32,
        pub size: u32,
    }

    #[repr(C)]
    pub struct VkPipelineLayoutCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineLayoutCreateFlags,
        pub setLayoutCount: u32,
        pub pSetLayouts: *const VkDescriptorSetLayout,
        pub pushConstantRangeCount: u32,
        pub pPushConstantRanges: *const VkPushConstantRange,
    }

    #[repr(C)]
    pub struct VkSamplerCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkSamplerCreateFlags,
        pub magFilter: VkFilter,
        pub minFilter: VkFilter,
        pub mipmapMode: VkSamplerMipmapMode,
        pub addressModeU: VkSamplerAddressMode,
        pub addressModeV: VkSamplerAddressMode,
        pub addressModeW: VkSamplerAddressMode,
        pub mipLodBias: f32,
        pub anisotropyEnable: VkBool32,
        pub maxAnisotropy: f32,
        pub compareEnable: VkBool32,
        pub compareOp: VkCompareOp,
        pub minLod: f32,
        pub maxLod: f32,
        pub borderColor: VkBorderColor,
        pub unnormalizedCoordinates: VkBool32,
    }

    #[repr(C)]
    pub struct VkDescriptorSetLayoutBinding {
        pub binding: u32,
        pub descriptorType: VkDescriptorType,
        pub descriptorCount: u32,
        pub stageFlags: VkShaderStageFlags,
        pub pImmutableSamplers: *const VkSampler,
    }

    #[repr(C)]
    pub struct VkDescriptorSetLayoutCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkDescriptorSetLayoutCreateFlags,
        pub bindingCount: u32,
        pub pBindings: *const VkDescriptorSetLayoutBinding,
    }

    #[repr(C)]
    pub struct VkDescriptorPoolSize {
        pub type_: VkDescriptorType,
        pub descriptorCount: u32,
    }

    #[repr(C)]
    pub struct VkDescriptorPoolCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkDescriptorPoolCreateFlags,
        pub maxSets: u32,
        pub poolSizeCount: u32,
        pub pPoolSizes: *const VkDescriptorPoolSize,
    }

    #[repr(C)]
    pub struct VkDescriptorSetAllocateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub descriptorPool: VkDescriptorPool,
        pub descriptorSetCount: u32,
        pub pSetLayouts: *const VkDescriptorSetLayout,
    }

    #[repr(C)]
    pub struct VkDescriptorImageInfo {
        pub sampler: VkSampler,
        pub imageView: VkImageView,
        pub imageLayout: VkImageLayout,
    }

    #[repr(C)]
    pub struct VkDescriptorBufferInfo {
        pub buffer: VkBuffer,
        pub offset: VkDeviceSize,
        pub range: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkWriteDescriptorSet {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub dstSet: VkDescriptorSet,
        pub dstBinding: u32,
        pub dstArrayElement: u32,
        pub descriptorCount: u32,
        pub descriptorType: VkDescriptorType,
        pub pImageInfo: *const VkDescriptorImageInfo,
        pub pBufferInfo: *const VkDescriptorBufferInfo,
        pub pTexelBufferView: *const VkBufferView,
    }

    #[repr(C)]
    pub struct VkCopyDescriptorSet {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub srcSet: VkDescriptorSet,
        pub srcBinding: u32,
        pub srcArrayElement: u32,
        pub dstSet: VkDescriptorSet,
        pub dstBinding: u32,
        pub dstArrayElement: u32,
        pub descriptorCount: u32,
    }

    #[repr(C)]
    pub struct VkFramebufferCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkFramebufferCreateFlags,
        pub renderPass: VkRenderPass,
        pub attachmentCount: u32,
        pub pAttachments: *const VkImageView,
        pub width: u32,
        pub height: u32,
        pub layers: u32,
    }

    #[repr(C)]
    pub struct VkAttachmentDescription {
        pub flags: VkAttachmentDescriptionFlags,
        pub format: VkFormat,
        pub samples: VkSampleCountFlagBits,
        pub loadOp: VkAttachmentLoadOp,
        pub storeOp: VkAttachmentStoreOp,
        pub stencilLoadOp: VkAttachmentLoadOp,
        pub stencilStoreOp: VkAttachmentStoreOp,
        pub initialLayout: VkImageLayout,
        pub finalLayout: VkImageLayout,
    }

    #[repr(C)]
    pub struct VkAttachmentReference {
        pub attachment: u32,
        pub layout: VkImageLayout,
    }

    #[repr(C)]
    pub struct VkSubpassDescription {
        pub flags: VkSubpassDescriptionFlags,
        pub pipelineBindPoint: VkPipelineBindPoint,
        pub inputAttachmentCount: u32,
        pub pInputAttachments: *const VkAttachmentReference,
        pub colorAttachmentCount: u32,
        pub pColorAttachments: *const VkAttachmentReference,
        pub pResolveAttachments: *const VkAttachmentReference,
        pub pDepthStencilAttachment: *const VkAttachmentReference,
        pub preserveAttachmentCount: u32,
        pub pPreserveAttachments: *const u32,
    }

    #[repr(C)]
    pub struct VkSubpassDependency {
        pub srcSubpass: u32,
        pub dstSubpass: u32,
        pub srcStageMask: VkPipelineStageFlags,
        pub dstStageMask: VkPipelineStageFlags,
        pub srcAccessMask: VkAccessFlags,
        pub dstAccessMask: VkAccessFlags,
        pub dependencyFlags: VkDependencyFlags,
    }

    #[repr(C)]
    pub struct VkRenderPassCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkRenderPassCreateFlags,
        pub attachmentCount: u32,
        pub pAttachments: *const VkAttachmentDescription,
        pub subpassCount: u32,
        pub pSubpasses: *const VkSubpassDescription,
        pub dependencyCount: u32,
        pub pDependencies: *const VkSubpassDependency,
    }

    #[repr(C)]
    pub struct VkCommandPoolCreateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkCommandPoolCreateFlags,
        pub queueFamilyIndex: u32,
    }

    #[repr(C)]
    pub struct VkCommandBufferAllocateInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub commandPool: VkCommandPool,
        pub level: VkCommandBufferLevel,
        pub commandBufferCount: u32,
    }

    #[repr(C)]
    pub struct VkCommandBufferInheritanceInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub renderPass: VkRenderPass,
        pub subpass: u32,
        pub framebuffer: VkFramebuffer,
        pub occlusionQueryEnable: VkBool32,
        pub queryFlags: VkQueryControlFlags,
        pub pipelineStatistics: VkQueryPipelineStatisticFlags,
    }

    #[repr(C)]
    pub struct VkCommandBufferBeginInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkCommandBufferUsageFlags,
        pub pInheritanceInfo: *const VkCommandBufferInheritanceInfo,
    }

    #[repr(C)]
    pub struct VkBufferCopy {
        pub srcOffset: VkDeviceSize,
        pub dstOffset: VkDeviceSize,
        pub size: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkImageSubresourceLayers {
        pub aspectMask: VkImageAspectFlags,
        pub mipLevel: u32,
        pub baseArrayLayer: u32,
        pub layerCount: u32,
    }

    #[repr(C)]
    pub struct VkImageCopy {
        pub srcSubresource: VkImageSubresourceLayers,
        pub srcOffset: VkOffset3D,
        pub dstSubresource: VkImageSubresourceLayers,
        pub dstOffset: VkOffset3D,
        pub extent: VkExtent3D,
    }

    #[repr(C)]
    pub struct VkImageBlit {
        pub srcSubresource: VkImageSubresourceLayers,
        pub srcOffsets: [VkOffset3D; 2 as usize],
        pub dstSubresource: VkImageSubresourceLayers,
        pub dstOffsets: [VkOffset3D; 2 as usize],
    }

    #[repr(C)]
    pub struct VkBufferImageCopy {
        pub bufferOffset: VkDeviceSize,
        pub bufferRowLength: u32,
        pub bufferImageHeight: u32,
        pub imageSubresource: VkImageSubresourceLayers,
        pub imageOffset: VkOffset3D,
        pub imageExtent: VkExtent3D,
    }

    #[repr(C)]
    pub struct VkClearColorValue {
        data: [u32; 4],
    }
    impl VkClearColorValue {
        #[inline]
        pub unsafe fn float32(&self) -> &[f32; 4 as usize] {
            ::std::mem::transmute(&self.data)
        }
        #[inline]
        pub unsafe fn float32_mut(&mut self) -> &mut [f32; 4 as usize] {
            ::std::mem::transmute(&mut self.data)
        }
        #[inline]
        pub unsafe fn int32(&self) -> &[i32; 4 as usize] {
            ::std::mem::transmute(&self.data)
        }
        #[inline]
        pub unsafe fn int32_mut(&mut self) -> &mut [i32; 4 as usize] {
            ::std::mem::transmute(&mut self.data)
        }
        #[inline]
        pub unsafe fn uint32(&self) -> &[u32; 4 as usize] {
            ::std::mem::transmute(&self.data)
        }
        #[inline]
        pub unsafe fn uint32_mut(&mut self) -> &mut [u32; 4 as usize] {
            ::std::mem::transmute(&mut self.data)
        }
    }

    #[repr(C)]
    pub struct VkClearDepthStencilValue {
        pub depth: f32,
        pub stencil: u32,
    }

    #[repr(C)]
    pub struct VkClearValue {
        data: VkClearColorValue,
    }
    impl VkClearValue {
        #[inline]
        pub unsafe fn color(&self) -> &VkClearColorValue {
            ::std::mem::transmute(&self.data)
        }
        #[inline]
        pub unsafe fn color_mut(&mut self) -> &mut VkClearColorValue {
            ::std::mem::transmute(&mut self.data)
        }
        #[inline]
        pub unsafe fn depthStencil(&self) -> &VkClearDepthStencilValue {
            ::std::mem::transmute(&self.data)
        }
        #[inline]
        pub unsafe fn depthStencil_mut(&mut self) -> &mut VkClearDepthStencilValue {
            ::std::mem::transmute(&mut self.data)
        }
    }

    #[repr(C)]
    pub struct VkClearAttachment {
        pub aspectMask: VkImageAspectFlags,
        pub colorAttachment: u32,
        pub clearValue: VkClearValue,
    }

    #[repr(C)]
    pub struct VkClearRect {
        pub rect: VkRect2D,
        pub baseArrayLayer: u32,
        pub layerCount: u32,
    }

    #[repr(C)]
    pub struct VkImageResolve {
        pub srcSubresource: VkImageSubresourceLayers,
        pub srcOffset: VkOffset3D,
        pub dstSubresource: VkImageSubresourceLayers,
        pub dstOffset: VkOffset3D,
        pub extent: VkExtent3D,
    }

    #[repr(C)]
    pub struct VkMemoryBarrier {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub srcAccessMask: VkAccessFlags,
        pub dstAccessMask: VkAccessFlags,
    }

    #[repr(C)]
    pub struct VkBufferMemoryBarrier {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub srcAccessMask: VkAccessFlags,
        pub dstAccessMask: VkAccessFlags,
        pub srcQueueFamilyIndex: u32,
        pub dstQueueFamilyIndex: u32,
        pub buffer: VkBuffer,
        pub offset: VkDeviceSize,
        pub size: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkImageMemoryBarrier {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub srcAccessMask: VkAccessFlags,
        pub dstAccessMask: VkAccessFlags,
        pub oldLayout: VkImageLayout,
        pub newLayout: VkImageLayout,
        pub srcQueueFamilyIndex: u32,
        pub dstQueueFamilyIndex: u32,
        pub image: VkImage,
        pub subresourceRange: VkImageSubresourceRange,
    }

    #[repr(C)]
    pub struct VkRenderPassBeginInfo {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub renderPass: VkRenderPass,
        pub framebuffer: VkFramebuffer,
        pub renderArea: VkRect2D,
        pub clearValueCount: u32,
        pub pClearValues: *const VkClearValue,
    }

    #[repr(C)]
    pub struct VkDispatchIndirectCommand {
        pub x: u32,
        pub y: u32,
        pub z: u32,
    }

    #[repr(C)]
    pub struct VkDrawIndexedIndirectCommand {
        pub indexCount: u32,
        pub instanceCount: u32,
        pub firstIndex: u32,
        pub vertexOffset: i32,
        pub firstInstance: u32,
    }

    #[repr(C)]
    pub struct VkDrawIndirectCommand {
        pub vertexCount: u32,
        pub instanceCount: u32,
        pub firstVertex: u32,
        pub firstInstance: u32,
    }

    /*
     * ------------------------------------------------------------------------
     * Entry dispatch table. Represents the Vulkan entry point that can be used
     * to get other Vulkan functions. Holds the library handle so that it does
     * not get unloaded. This is very similar to the macros used for generating
     * global-, instance-, and dispatch tables, except explicit since the entry
     * is just a single function.
     * ------------------------------------------------------------------------
    */
    type PFN_vkGetInstanceProcAddr = vk_fun!((instance: VkInstance, pName: *const c_char) -> Option<PFN_vkVoidFunction>);
    pub struct VulkanEntry {
        #[allow(dead_code)]
        vulkan_lib: libloading::Library,
        vkGetInstanceProcAddr: PFN_vkGetInstanceProcAddr,
    }

    impl VulkanEntry {
        pub fn new(loader_path: &str) -> Result<VulkanEntry, String> {
            let lib = match libloading::Library::new(loader_path) {
                Ok(lib) => lib,
                Err(_) => return Err(String::from("Failed to open Vulkan loader")),
            };

            let vkGetInstanceProcAddr: PFN_vkGetInstanceProcAddr = unsafe {
                match lib.get::<PFN_vkGetInstanceProcAddr>(b"vkGetInstanceProcAddr\0") {
                    Ok(symbol) => *symbol, // Deref Symbol, not function pointer
                    Err(_) => return Err(String::from("Could not load vkGetInstanceProcAddr")),
                }
            };

            // Since I can't keep the library and the loaded function in the
            // same struct (Rust would then not be able to drop it because of
            // the symbol references into itself via the library) I have opted
            // to just storing the raw loaded function. This should be fine as
            // long as the library is also saved to prevent unloading it. Since
            // I return a Result, Rust makes sure that the struct can only be
            // used if properly initialized.
            Ok(VulkanEntry {
                vulkan_lib: lib, // Save this so that the library is not freed
                vkGetInstanceProcAddr: vkGetInstanceProcAddr,
            })
        }

        #[inline]
        pub unsafe fn vkGetInstanceProcAddr(&self, instance: VkInstance, pName: *const c_char) -> Option<PFN_vkVoidFunction> {
            (self.vkGetInstanceProcAddr)(instance, pName)
        }
    }

    global_dispatch_table!{
        vkCreateInstance => (pCreateInfo: *const VkInstanceCreateInfo, pAllocator: *const VkAllocationCallbacks, pInstance: *mut VkInstance) -> VkResult,
        vkEnumerateInstanceExtensionProperties => (pLayerName: *const c_char, pPropertyCount: *mut u32, pProperties: *mut VkExtensionProperties) -> VkResult,
        vkEnumerateInstanceLayerProperties => (pPropertyCount: *mut u32, pProperties: *mut VkLayerProperties) -> VkResult,
    }

    instance_dispatch_table!{
        vkDestroyInstance => (instance: VkInstance, pAllocator: *const VkAllocationCallbacks) -> (),
        vkEnumeratePhysicalDevices => (instance: VkInstance, pPhysicalDeviceCount: *mut u32, pPhysicalDevices: *mut VkPhysicalDevice) -> VkResult,
        vkGetPhysicalDeviceFeatures => (physicalDevice: VkPhysicalDevice, pFeatures: *mut VkPhysicalDeviceFeatures) -> (),
        vkGetPhysicalDeviceFormatProperties => (physicalDevice: VkPhysicalDevice, format: VkFormat, pFormatProperties: *mut VkFormatProperties) -> (),
        vkGetPhysicalDeviceImageFormatProperties => (physicalDevice: VkPhysicalDevice, format: VkFormat, type_: VkImageType, tiling: VkImageTiling, usage: VkImageUsageFlags, flags: VkImageCreateFlags, pImageFormatProperties: *mut VkImageFormatProperties) -> VkResult,
        vkGetPhysicalDeviceProperties => (physicalDevice: VkPhysicalDevice, pProperties: *mut VkPhysicalDeviceProperties) -> (),
        vkGetPhysicalDeviceQueueFamilyProperties => (physicalDevice: VkPhysicalDevice, pQueueFamilyPropertyCount: *mut u32, pQueueFamilyProperties: *mut VkQueueFamilyProperties) -> (),
        vkGetPhysicalDeviceMemoryProperties => (physicalDevice: VkPhysicalDevice, pMemoryProperties: *mut VkPhysicalDeviceMemoryProperties) -> (),
        vkGetDeviceProcAddr => (device: VkDevice, pName: *const c_char) -> Option<PFN_vkVoidFunction>,
        vkCreateDevice => (physicalDevice: VkPhysicalDevice, pCreateInfo: *const VkDeviceCreateInfo, pAllocator: *const VkAllocationCallbacks, pDevice: *mut VkDevice) -> VkResult,
        vkEnumerateDeviceExtensionProperties => (physicalDevice: VkPhysicalDevice, pLayerName: *const c_char, pPropertyCount: *mut u32, pProperties: *mut VkExtensionProperties) -> VkResult,
        vkEnumerateDeviceLayerProperties => (physicalDevice: VkPhysicalDevice, pPropertyCount: *mut u32, pProperties: *mut VkLayerProperties) -> VkResult,
        vkGetPhysicalDeviceSparseImageFormatProperties => (physicalDevice: VkPhysicalDevice, format: VkFormat, type_: VkImageType, samples: VkSampleCountFlagBits, usage: VkImageUsageFlags, tiling: VkImageTiling, pPropertyCount: *mut u32, pProperties: *mut VkSparseImageFormatProperties) -> (),
    }

    device_dispatch_table!{
        vkDestroyDevice => (device: VkDevice, pAllocator: *const VkAllocationCallbacks) -> (),
        vkGetDeviceQueue => (device: VkDevice, queueFamilyIndex: u32, queueIndex: u32, pQueue: *mut VkQueue) -> (),
        vkQueueSubmit => (queue: VkQueue, submitCount: u32, pSubmits: *const VkSubmitInfo, fence: VkFence) -> VkResult,
        vkQueueWaitIdle => (queue: VkQueue) -> VkResult,
        vkDeviceWaitIdle => (device: VkDevice) -> VkResult,
        vkAllocateMemory => (device: VkDevice, pAllocateInfo: *const VkMemoryAllocateInfo, pAllocator: *const VkAllocationCallbacks, pMemory: *mut VkDeviceMemory) -> VkResult,
        vkFreeMemory => (device: VkDevice, memory: VkDeviceMemory, pAllocator: *const VkAllocationCallbacks) -> (),
        vkMapMemory => (device: VkDevice, memory: VkDeviceMemory, offset: VkDeviceSize, size: VkDeviceSize, flags: VkMemoryMapFlags, ppData: *mut *mut c_void) -> VkResult,
        vkUnmapMemory => (device: VkDevice, memory: VkDeviceMemory) -> (),
        vkFlushMappedMemoryRanges => (device: VkDevice, memoryRangeCount: u32, pMemoryRanges: *const VkMappedMemoryRange) -> VkResult,
        vkInvalidateMappedMemoryRanges => (device: VkDevice, memoryRangeCount: u32, pMemoryRanges: *const VkMappedMemoryRange) -> VkResult,
        vkGetDeviceMemoryCommitment => (device: VkDevice, memory: VkDeviceMemory, pCommittedMemoryInBytes: *mut VkDeviceSize) -> (),
        vkBindBufferMemory => (device: VkDevice, buffer: VkBuffer, memory: VkDeviceMemory, memoryOffset: VkDeviceSize) -> VkResult,
        vkBindImageMemory => (device: VkDevice, image: VkImage, memory: VkDeviceMemory, memoryOffset: VkDeviceSize) -> VkResult,
        vkGetBufferMemoryRequirements => (device: VkDevice, buffer: VkBuffer, pMemoryRequirements: *mut VkMemoryRequirements) -> (),
        vkGetImageMemoryRequirements => (device: VkDevice, image: VkImage, pMemoryRequirements: *mut VkMemoryRequirements) -> (),
        vkGetImageSparseMemoryRequirements => (device: VkDevice, image: VkImage, pSparseMemoryRequirementCount: *mut u32, pSparseMemoryRequirements: *mut VkSparseImageMemoryRequirements) -> (),
        vkQueueBindSparse => (queue: VkQueue, bindInfoCount: u32, pBindInfo: *const VkBindSparseInfo, fence: VkFence) -> VkResult,
        vkCreateFence => (device: VkDevice, pCreateInfo: *const VkFenceCreateInfo, pAllocator: *const VkAllocationCallbacks, pFence: *mut VkFence) -> VkResult,
        vkDestroyFence => (device: VkDevice, fence: VkFence, pAllocator: *const VkAllocationCallbacks) -> (),
        vkResetFences => (device: VkDevice, fenceCount: u32, pFences: *const VkFence) -> VkResult,
        vkGetFenceStatus => (device: VkDevice, fence: VkFence) -> VkResult,
        vkWaitForFences => (device: VkDevice, fenceCount: u32, pFences: *const VkFence, waitAll: VkBool32, timeout: u64) -> VkResult,
        vkCreateSemaphore => (device: VkDevice, pCreateInfo: *const VkSemaphoreCreateInfo, pAllocator: *const VkAllocationCallbacks, pSemaphore: *mut VkSemaphore) -> VkResult,
        vkDestroySemaphore => (device: VkDevice, semaphore: VkSemaphore, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreateEvent => (device: VkDevice, pCreateInfo: *const VkEventCreateInfo, pAllocator: *const VkAllocationCallbacks, pEvent: *mut VkEvent) -> VkResult,
        vkDestroyEvent => (device: VkDevice, event: VkEvent, pAllocator: *const VkAllocationCallbacks) -> (),
        vkGetEventStatus => (device: VkDevice, event: VkEvent) -> VkResult,
        vkSetEvent => (device: VkDevice, event: VkEvent) -> VkResult,
        vkResetEvent => (device: VkDevice, event: VkEvent) -> VkResult,
        vkCreateQueryPool => (device: VkDevice, pCreateInfo: *const VkQueryPoolCreateInfo, pAllocator: *const VkAllocationCallbacks, pQueryPool: *mut VkQueryPool) -> VkResult,
        vkDestroyQueryPool => (device: VkDevice, queryPool: VkQueryPool, pAllocator: *const VkAllocationCallbacks) -> (),
        vkGetQueryPoolResults => (device: VkDevice, queryPool: VkQueryPool, firstQuery: u32, queryCount: u32, dataSize: usize, pData: *mut c_void, stride: VkDeviceSize, flags: VkQueryResultFlags) -> VkResult,
        vkCreateBuffer => (device: VkDevice, pCreateInfo: *const VkBufferCreateInfo, pAllocator: *const VkAllocationCallbacks, pBuffer: *mut VkBuffer) -> VkResult,
        vkDestroyBuffer => (device: VkDevice, buffer: VkBuffer, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreateBufferView => (device: VkDevice, pCreateInfo: *const VkBufferViewCreateInfo, pAllocator: *const VkAllocationCallbacks, pView: *mut VkBufferView) -> VkResult,
        vkDestroyBufferView => (device: VkDevice, bufferView: VkBufferView, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreateImage => (device: VkDevice, pCreateInfo: *const VkImageCreateInfo, pAllocator: *const VkAllocationCallbacks, pImage: *mut VkImage) -> VkResult,
        vkDestroyImage => (device: VkDevice, image: VkImage, pAllocator: *const VkAllocationCallbacks) -> (),
        vkGetImageSubresourceLayout => (device: VkDevice, image: VkImage, pSubresource: *const VkImageSubresource, pLayout: *mut VkSubresourceLayout) -> (),
        vkCreateImageView => (device: VkDevice, pCreateInfo: *const VkImageViewCreateInfo, pAllocator: *const VkAllocationCallbacks, pView: *mut VkImageView) -> VkResult,
        vkDestroyImageView => (device: VkDevice, imageView: VkImageView, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreateShaderModule => (device: VkDevice, pCreateInfo: *const VkShaderModuleCreateInfo, pAllocator: *const VkAllocationCallbacks, pShaderModule: *mut VkShaderModule) -> VkResult,
        vkDestroyShaderModule => (device: VkDevice, shaderModule: VkShaderModule, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreatePipelineCache => (device: VkDevice, pCreateInfo: *const VkPipelineCacheCreateInfo, pAllocator: *const VkAllocationCallbacks, pPipelineCache: *mut VkPipelineCache) -> VkResult,
        vkDestroyPipelineCache => (device: VkDevice, pipelineCache: VkPipelineCache, pAllocator: *const VkAllocationCallbacks) -> (),
        vkGetPipelineCacheData => (device: VkDevice, pipelineCache: VkPipelineCache, pDataSize: *mut usize, pData: *mut c_void) -> VkResult,
        vkMergePipelineCaches => (device: VkDevice, dstCache: VkPipelineCache, srcCacheCount: u32, pSrcCaches: *const VkPipelineCache) -> VkResult,
        vkCreateGraphicsPipelines => (device: VkDevice, pipelineCache: VkPipelineCache, createInfoCount: u32, pCreateInfos: *const VkGraphicsPipelineCreateInfo, pAllocator: *const VkAllocationCallbacks, pPipelines: *mut VkPipeline) -> VkResult,
        vkCreateComputePipelines => (device: VkDevice, pipelineCache: VkPipelineCache, createInfoCount: u32, pCreateInfos: *const VkComputePipelineCreateInfo, pAllocator: *const VkAllocationCallbacks, pPipelines: *mut VkPipeline) -> VkResult,
        vkDestroyPipeline => (device: VkDevice, pipeline: VkPipeline, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreatePipelineLayout => (device: VkDevice, pCreateInfo: *const VkPipelineLayoutCreateInfo, pAllocator: *const VkAllocationCallbacks, pPipelineLayout: *mut VkPipelineLayout) -> VkResult,
        vkDestroyPipelineLayout => (device: VkDevice, pipelineLayout: VkPipelineLayout, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreateSampler => (device: VkDevice, pCreateInfo: *const VkSamplerCreateInfo, pAllocator: *const VkAllocationCallbacks, pSampler: *mut VkSampler) -> VkResult,
        vkDestroySampler => (device: VkDevice, sampler: VkSampler, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreateDescriptorSetLayout => (device: VkDevice, pCreateInfo: *const VkDescriptorSetLayoutCreateInfo, pAllocator: *const VkAllocationCallbacks, pSetLayout: *mut VkDescriptorSetLayout) -> VkResult,
        vkDestroyDescriptorSetLayout => (device: VkDevice, descriptorSetLayout: VkDescriptorSetLayout, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreateDescriptorPool => (device: VkDevice, pCreateInfo: *const VkDescriptorPoolCreateInfo, pAllocator: *const VkAllocationCallbacks, pDescriptorPool: *mut VkDescriptorPool) -> VkResult,
        vkDestroyDescriptorPool => (device: VkDevice, descriptorPool: VkDescriptorPool, pAllocator: *const VkAllocationCallbacks) -> (),
        vkResetDescriptorPool => (device: VkDevice, descriptorPool: VkDescriptorPool, flags: VkDescriptorPoolResetFlags) -> VkResult,
        vkAllocateDescriptorSets => (device: VkDevice, pAllocateInfo: *const VkDescriptorSetAllocateInfo, pDescriptorSets: *mut VkDescriptorSet) -> VkResult,
        vkFreeDescriptorSets => (device: VkDevice, descriptorPool: VkDescriptorPool, descriptorSetCount: u32, pDescriptorSets: *const VkDescriptorSet) -> VkResult,
        vkUpdateDescriptorSets => (device: VkDevice, descriptorWriteCount: u32, pDescriptorWrites: *const VkWriteDescriptorSet, descriptorCopyCount: u32, pDescriptorCopies: *const VkCopyDescriptorSet) -> (),
        vkCreateFramebuffer => (device: VkDevice, pCreateInfo: *const VkFramebufferCreateInfo, pAllocator: *const VkAllocationCallbacks, pFramebuffer: *mut VkFramebuffer) -> VkResult,
        vkDestroyFramebuffer => (device: VkDevice, framebuffer: VkFramebuffer, pAllocator: *const VkAllocationCallbacks) -> (),
        vkCreateRenderPass => (device: VkDevice, pCreateInfo: *const VkRenderPassCreateInfo, pAllocator: *const VkAllocationCallbacks, pRenderPass: *mut VkRenderPass) -> VkResult,
        vkDestroyRenderPass => (device: VkDevice, renderPass: VkRenderPass, pAllocator: *const VkAllocationCallbacks) -> (),
        vkGetRenderAreaGranularity => (device: VkDevice, renderPass: VkRenderPass, pGranularity: *mut VkExtent2D) -> (),
        vkCreateCommandPool => (device: VkDevice, pCreateInfo: *const VkCommandPoolCreateInfo, pAllocator: *const VkAllocationCallbacks, pCommandPool: *mut VkCommandPool) -> VkResult,
        vkDestroyCommandPool => (device: VkDevice, commandPool: VkCommandPool, pAllocator: *const VkAllocationCallbacks) -> (),
        vkResetCommandPool => (device: VkDevice, commandPool: VkCommandPool, flags: VkCommandPoolResetFlags) -> VkResult,
        vkAllocateCommandBuffers => (device: VkDevice, pAllocateInfo: *const VkCommandBufferAllocateInfo, pCommandBuffers: *mut VkCommandBuffer) -> VkResult,
        vkFreeCommandBuffers => (device: VkDevice, commandPool: VkCommandPool, commandBufferCount: u32, pCommandBuffers: *const VkCommandBuffer) -> (),
        vkBeginCommandBuffer => (commandBuffer: VkCommandBuffer, pBeginInfo: *const VkCommandBufferBeginInfo) -> VkResult,
        vkEndCommandBuffer => (commandBuffer: VkCommandBuffer) -> VkResult,
        vkResetCommandBuffer => (commandBuffer: VkCommandBuffer, flags: VkCommandBufferResetFlags) -> VkResult,
        vkCmdBindPipeline => (commandBuffer: VkCommandBuffer, pipelineBindPoint: VkPipelineBindPoint, pipeline: VkPipeline) -> (),
        vkCmdSetViewport => (commandBuffer: VkCommandBuffer, firstViewport: u32, viewportCount: u32, pViewports: *const VkViewport) -> (),
        vkCmdSetScissor => (commandBuffer: VkCommandBuffer, firstScissor: u32, scissorCount: u32, pScissors: *const VkRect2D) -> (),
        vkCmdSetLineWidth => (commandBuffer: VkCommandBuffer, lineWidth: f32) -> (),
        vkCmdSetDepthBias => (commandBuffer: VkCommandBuffer, depthBiasConstantFactor: f32, depthBiasClamp: f32, depthBiasSlopeFactor: f32) -> (),
        vkCmdSetBlendConstants => (commandBuffer: VkCommandBuffer, blendConstants: &[f32; 4]) -> (),
        vkCmdSetDepthBounds => (commandBuffer: VkCommandBuffer, minDepthBounds: f32, maxDepthBounds: f32) -> (),
        vkCmdSetStencilCompareMask => (commandBuffer: VkCommandBuffer, faceMask: VkStencilFaceFlags, compareMask: u32) -> (),
        vkCmdSetStencilWriteMask => (commandBuffer: VkCommandBuffer, faceMask: VkStencilFaceFlags, writeMask: u32) -> (),
        vkCmdSetStencilReference => (commandBuffer: VkCommandBuffer, faceMask: VkStencilFaceFlags, reference: u32) -> (),
        vkCmdBindDescriptorSets => (commandBuffer: VkCommandBuffer, pipelineBindPoint: VkPipelineBindPoint, layout: VkPipelineLayout, firstSet: u32, descriptorSetCount: u32, pDescriptorSets: *const VkDescriptorSet, dynamicOffsetCount: u32, pDynamicOffsets: *const u32) -> (),
        vkCmdBindIndexBuffer => (commandBuffer: VkCommandBuffer, buffer: VkBuffer, offset: VkDeviceSize, indexType: VkIndexType) -> (),
        vkCmdBindVertexBuffers => (commandBuffer: VkCommandBuffer, firstBinding: u32, bindingCount: u32, pBuffers: *const VkBuffer, pOffsets: *const VkDeviceSize) -> (),
        vkCmdDraw => (commandBuffer: VkCommandBuffer, vertexCount: u32, instanceCount: u32, firstVertex: u32, firstInstance: u32) -> (),
        vkCmdDrawIndexed => (commandBuffer: VkCommandBuffer, indexCount: u32, instanceCount: u32, firstIndex: u32, vertexOffset: i32, firstInstance: u32) -> (),
        vkCmdDrawIndirect => (commandBuffer: VkCommandBuffer, buffer: VkBuffer, offset: VkDeviceSize, drawCount: u32, stride: u32) -> (),
        vkCmdDrawIndexedIndirect => (commandBuffer: VkCommandBuffer, buffer: VkBuffer, offset: VkDeviceSize, drawCount: u32, stride: u32) -> (),
        vkCmdDispatch => (commandBuffer: VkCommandBuffer, groupCountX: u32, groupCountY: u32, groupCountZ: u32) -> (),
        vkCmdDispatchIndirect => (commandBuffer: VkCommandBuffer, buffer: VkBuffer, offset: VkDeviceSize) -> (),
        vkCmdCopyBuffer => (commandBuffer: VkCommandBuffer, srcBuffer: VkBuffer, dstBuffer: VkBuffer, regionCount: u32, pRegions: *const VkBufferCopy) -> (),
        vkCmdCopyImage => (commandBuffer: VkCommandBuffer, srcImage: VkImage, srcImageLayout: VkImageLayout, dstImage: VkImage, dstImageLayout: VkImageLayout, regionCount: u32, pRegions: *const VkImageCopy) -> (),
        vkCmdBlitImage => (commandBuffer: VkCommandBuffer, srcImage: VkImage, srcImageLayout: VkImageLayout, dstImage: VkImage, dstImageLayout: VkImageLayout, regionCount: u32, pRegions: *const VkImageBlit, filter: VkFilter) -> (),
        vkCmdCopyBufferToImage => (commandBuffer: VkCommandBuffer, srcBuffer: VkBuffer, dstImage: VkImage, dstImageLayout: VkImageLayout, regionCount: u32, pRegions: *const VkBufferImageCopy) -> (),
        vkCmdCopyImageToBuffer => (commandBuffer: VkCommandBuffer, srcImage: VkImage, srcImageLayout: VkImageLayout, dstBuffer: VkBuffer, regionCount: u32, pRegions: *const VkBufferImageCopy) -> (),
        vkCmdUpdateBuffer => (commandBuffer: VkCommandBuffer, dstBuffer: VkBuffer, dstOffset: VkDeviceSize, dataSize: VkDeviceSize, pData: *const c_void) -> (),
        vkCmdFillBuffer => (commandBuffer: VkCommandBuffer, dstBuffer: VkBuffer, dstOffset: VkDeviceSize, size: VkDeviceSize, data: u32) -> (),
        vkCmdClearColorImage => (commandBuffer: VkCommandBuffer, image: VkImage, imageLayout: VkImageLayout, pColor: *const VkClearColorValue, rangeCount: u32, pRanges: *const VkImageSubresourceRange) -> (),
        vkCmdClearDepthStencilImage => (commandBuffer: VkCommandBuffer, image: VkImage, imageLayout: VkImageLayout, pDepthStencil: *const VkClearDepthStencilValue, rangeCount: u32, pRanges: *const VkImageSubresourceRange) -> (),
        vkCmdClearAttachments => (commandBuffer: VkCommandBuffer, attachmentCount: u32, pAttachments: *const VkClearAttachment, rectCount: u32, pRects: *const VkClearRect) -> (),
        vkCmdResolveImage => (commandBuffer: VkCommandBuffer, srcImage: VkImage, srcImageLayout: VkImageLayout, dstImage: VkImage, dstImageLayout: VkImageLayout, regionCount: u32, pRegions: *const VkImageResolve) -> (),
        vkCmdSetEvent => (commandBuffer: VkCommandBuffer, event: VkEvent, stageMask: VkPipelineStageFlags) -> (),
        vkCmdResetEvent => (commandBuffer: VkCommandBuffer, event: VkEvent, stageMask: VkPipelineStageFlags) -> (),
        vkCmdWaitEvents => (commandBuffer: VkCommandBuffer, eventCount: u32, pEvents: *const VkEvent, srcStageMask: VkPipelineStageFlags, dstStageMask: VkPipelineStageFlags, memoryBarrierCount: u32, pMemoryBarriers: *const VkMemoryBarrier, bufferMemoryBarrierCount: u32, pBufferMemoryBarriers: *const VkBufferMemoryBarrier, imageMemoryBarrierCount: u32, pImageMemoryBarriers: *const VkImageMemoryBarrier) -> (),
        vkCmdPipelineBarrier => (commandBuffer: VkCommandBuffer, srcStageMask: VkPipelineStageFlags, dstStageMask: VkPipelineStageFlags, dependencyFlags: VkDependencyFlags, memoryBarrierCount: u32, pMemoryBarriers: *const VkMemoryBarrier, bufferMemoryBarrierCount: u32, pBufferMemoryBarriers: *const VkBufferMemoryBarrier, imageMemoryBarrierCount: u32, pImageMemoryBarriers: *const VkImageMemoryBarrier) -> (),
        vkCmdBeginQuery => (commandBuffer: VkCommandBuffer, queryPool: VkQueryPool, query: u32, flags: VkQueryControlFlags) -> (),
        vkCmdEndQuery => (commandBuffer: VkCommandBuffer, queryPool: VkQueryPool, query: u32) -> (),
        vkCmdResetQueryPool => (commandBuffer: VkCommandBuffer, queryPool: VkQueryPool, firstQuery: u32, queryCount: u32) -> (),
        vkCmdWriteTimestamp => (commandBuffer: VkCommandBuffer, pipelineStage: VkPipelineStageFlagBits, queryPool: VkQueryPool, query: u32) -> (),
        vkCmdCopyQueryPoolResults => (commandBuffer: VkCommandBuffer, queryPool: VkQueryPool, firstQuery: u32, queryCount: u32, dstBuffer: VkBuffer, dstOffset: VkDeviceSize, stride: VkDeviceSize, flags: VkQueryResultFlags) -> (),
        vkCmdPushConstants => (commandBuffer: VkCommandBuffer, layout: VkPipelineLayout, stageFlags: VkShaderStageFlags, offset: u32, size: u32, pValues: *const c_void) -> (),
        vkCmdBeginRenderPass => (commandBuffer: VkCommandBuffer, pRenderPassBegin: *const VkRenderPassBeginInfo, contents: VkSubpassContents) -> (),
        vkCmdNextSubpass => (commandBuffer: VkCommandBuffer, contents: VkSubpassContents) -> (),
        vkCmdEndRenderPass => (commandBuffer: VkCommandBuffer) -> (),
        vkCmdExecuteCommands => (commandBuffer: VkCommandBuffer, commandBufferCount: u32, pCommandBuffers: *const VkCommandBuffer) -> (),
    }
} // mod core

pub mod extensions {
    use super::macros::*;
    use super::core::*;

    /*
     * ------------------------------------------------------
     * VK_KHR_surface
     * ------------------------------------------------------
    */

    pub type VkSurfaceKHR = u64;

    #[repr(C)]
    pub enum VkColorSpaceKHR {
        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR = 0,
        VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT = 1000104001,
        VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT = 1000104002,
        VK_COLOR_SPACE_DCI_P3_LINEAR_EXT = 1000104003,
        VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT = 1000104004,
        VK_COLOR_SPACE_BT709_LINEAR_EXT = 1000104005,
        VK_COLOR_SPACE_BT709_NONLINEAR_EXT = 1000104006,
        VK_COLOR_SPACE_BT2020_LINEAR_EXT = 1000104007,
        VK_COLOR_SPACE_HDR10_ST2084_EXT = 1000104008,
        VK_COLOR_SPACE_DOLBYVISION_EXT = 1000104009,
        VK_COLOR_SPACE_HDR10_HLG_EXT = 1000104010,
        VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT = 1000104011,
        VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT = 1000104012,
        VK_COLOR_SPACE_PASS_THROUGH_EXT = 1000104013,
    }

    #[repr(C)]
    pub enum VkPresentModeKHR {
        VK_PRESENT_MODE_IMMEDIATE_KHR = 0,
        VK_PRESENT_MODE_MAILBOX_KHR = 1,
        VK_PRESENT_MODE_FIFO_KHR = 2,
        VK_PRESENT_MODE_FIFO_RELAXED_KHR = 3,
    }

    vulkan_flags!(VkSurfaceTransformFlagsKHR, VkSurfaceTransformFlagBitsKHR, {
        VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR = 0x00000001,
        VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR = 0x00000002,
        VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR = 0x00000004,
        VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR = 0x00000008,
        VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR = 0x00000010,
        VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR = 0x00000020,
        VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR = 0x00000040,
        VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR = 0x00000080,
        VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR = 0x00000100,
    });
    vulkan_flags!(VkCompositeAlphaFlagsKHR, VkCompositeAlphaFlagBitsKHR, {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR = 0x00000001,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR = 0x00000002,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR = 0x00000004,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR = 0x00000008,
    });

    #[repr(C)]
    pub struct VkSurfaceCapabilitiesKHR {
        pub minImageCount: u32,
        pub maxImageCount: u32,
        pub currentExtent: VkExtent2D,
        pub minImageExtent: VkExtent2D,
        pub maxImageExtent: VkExtent2D,
        pub maxImageArrayLayers: u32,
        pub supportedTransforms: VkSurfaceTransformFlagsKHR,
        pub currentTransform: VkSurfaceTransformFlagBitsKHR,
        pub supportedCompositeAlpha: VkCompositeAlphaFlagsKHR,
        pub supportedUsageFlags: VkImageUsageFlags,
    }

    #[repr(C)]
    pub struct VkSurfaceFormatKHR {
        pub format: VkFormat,
        pub colorSpace: VkColorSpaceKHR,
    }

    extension_dispatch_table!{VK_KHR_surface | instance, {
        [instance] vkDestroySurfaceKHR => (instance: VkInstance, surface: VkSurfaceKHR, pAllocator: *const VkAllocationCallbacks) -> (),
        [instance] vkGetPhysicalDeviceSurfaceSupportKHR => (physicalDevice: VkPhysicalDevice, queueFamilyIndex: u32, surface: VkSurfaceKHR, pSupported: *mut VkBool32) -> VkResult,
        [instance] vkGetPhysicalDeviceSurfaceCapabilitiesKHR => (physicalDevice: VkPhysicalDevice, surface: VkSurfaceKHR, pSurfaceCapabilities: *mut VkSurfaceCapabilitiesKHR) -> VkResult,
        [instance] vkGetPhysicalDeviceSurfaceFormatsKHR => (physicalDevice: VkPhysicalDevice, surface: VkSurfaceKHR, pSurfaceFormatCount: *mut u32, pSurfaceFormats: *mut VkSurfaceFormatKHR) -> VkResult,
        [instance] vkGetPhysicalDeviceSurfacePresentModesKHR => (physicalDevice: VkPhysicalDevice, surface: VkSurfaceKHR, pPresentModeCount: *mut u32, pPresentModes: *mut VkPresentModeKHR) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_swapchain
     * ------------------------------------------------------
    */

    pub type VkSwapchainKHR = u64;

    vulkan_flags!(VkSwapchainCreateFlagsKHR, VkSwapchainCreateFlagBitsKHR, {
        VK_SWAPCHAIN_CREATE_BIND_SFR_BIT_KHX = 0x00000001,
    });

    #[repr(C)]
    pub struct VkSwapchainCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkSwapchainCreateFlagsKHR,
        pub surface: VkSurfaceKHR,
        pub minImageCount: u32,
        pub imageFormat: VkFormat,
        pub imageColorSpace: VkColorSpaceKHR,
        pub imageExtent: VkExtent2D,
        pub imageArrayLayers: u32,
        pub imageUsage: VkImageUsageFlags,
        pub imageSharingMode: VkSharingMode,
        pub queueFamilyIndexCount: u32,
        pub pQueueFamilyIndices: *const u32,
        pub preTransform: VkSurfaceTransformFlagBitsKHR,
        pub compositeAlpha: VkCompositeAlphaFlagBitsKHR,
        pub presentMode: VkPresentModeKHR,
        pub clipped: VkBool32,
        pub oldSwapchain: VkSwapchainKHR,
    }

    #[repr(C)]
    pub struct VkPresentInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub waitSemaphoreCount: u32,
        pub pWaitSemaphores: *const VkSemaphore,
        pub swapchainCount: u32,
        pub pSwapchains: *const VkSwapchainKHR,
        pub pImageIndices: *const u32,
        pub pResults: *mut VkResult,
    }

    extension_dispatch_table!{VK_KHR_swapchain | device, {
        [device] vkCreateSwapchainKHR => (device: VkDevice, pCreateInfo: *const VkSwapchainCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pSwapchain: *mut VkSwapchainKHR) -> VkResult,
        [device] vkDestroySwapchainKHR => (device: VkDevice, swapchain: VkSwapchainKHR, pAllocator: *const VkAllocationCallbacks) -> (),
        [device] vkGetSwapchainImagesKHR => (device: VkDevice, swapchain: VkSwapchainKHR, pSwapchainImageCount: *mut u32, pSwapchainImages: *mut VkImage) -> VkResult,
        [device] vkAcquireNextImageKHR => (device: VkDevice, swapchain: VkSwapchainKHR, timeout: u64, semaphore: VkSemaphore, fence: VkFence, pImageIndex: *mut u32) -> VkResult,
        [device] vkQueuePresentKHR => (queue: VkQueue, pPresentInfo: *const VkPresentInfoKHR) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_display
     * ------------------------------------------------------
    */

    pub type VkDisplayKHR = u64;
    pub type VkDisplayModeKHR = u64;

    vulkan_flags!(VkDisplayModeCreateFlagsKHR);
    vulkan_flags!(VkDisplayPlaneAlphaFlagsKHR, VkDisplayPlaneAlphaFlagBitsKHR, {
        VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR = 0x00000001,
        VK_DISPLAY_PLANE_ALPHA_GLOBAL_BIT_KHR = 0x00000002,
        VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_BIT_KHR = 0x00000004,
        VK_DISPLAY_PLANE_ALPHA_PER_PIXEL_PREMULTIPLIED_BIT_KHR = 0x00000008,
    });
    vulkan_flags!(VkDisplaySurfaceCreateFlagsKHR);

    #[repr(C)]
    pub struct VkDisplayPropertiesKHR {
        pub display: VkDisplayKHR,
        pub displayName: *const c_char,
        pub physicalDimensions: VkExtent2D,
        pub physicalResolution: VkExtent2D,
        pub supportedTransforms: VkSurfaceTransformFlagsKHR,
        pub planeReorderPossible: VkBool32,
        pub persistentContent: VkBool32,
    }

    #[repr(C)]
    pub struct VkDisplayPlanePropertiesKHR {
        pub currentDisplay: VkDisplayKHR,
        pub currentStackIndex: u32,
    }

    #[repr(C)]
    pub struct VkDisplayModeParametersKHR {
        pub visibleRegion: VkExtent2D,
        pub refreshRate: u32,
    }

    #[repr(C)]
    pub struct VkDisplayModePropertiesKHR {
        pub displayMode: VkDisplayModeKHR,
        pub parameters: VkDisplayModeParametersKHR,
    }

    #[repr(C)]
    pub struct VkDisplayModeCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkDisplayModeCreateFlagsKHR,
        pub parameters: VkDisplayModeParametersKHR,
    }

    #[repr(C)]
    pub struct VkDisplayPlaneCapabilitiesKHR {
        pub supportedAlpha: VkDisplayPlaneAlphaFlagsKHR,
        pub minSrcPosition: VkOffset2D,
        pub maxSrcPosition: VkOffset2D,
        pub minSrcExtent: VkExtent2D,
        pub maxSrcExtent: VkExtent2D,
        pub minDstPosition: VkOffset2D,
        pub maxDstPosition: VkOffset2D,
        pub minDstExtent: VkExtent2D,
        pub maxDstExtent: VkExtent2D,
    }

    #[repr(C)]
    pub struct VkDisplaySurfaceCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkDisplaySurfaceCreateFlagsKHR,
        pub displayMode: VkDisplayModeKHR,
        pub planeIndex: u32,
        pub planeStackIndex: u32,
        pub transform: VkSurfaceTransformFlagBitsKHR,
        pub globalAlpha: f32,
        pub alphaMode: VkDisplayPlaneAlphaFlagBitsKHR,
        pub imageExtent: VkExtent2D,
    }

    extension_dispatch_table!{VK_KHR_display | instance, {
        [instance] vkGetPhysicalDeviceDisplayPropertiesKHR => (physicalDevice: VkPhysicalDevice, pPropertyCount: *mut u32, pProperties: *mut VkDisplayPropertiesKHR) -> VkResult,
        [instance] vkGetPhysicalDeviceDisplayPlanePropertiesKHR => (physicalDevice: VkPhysicalDevice, pPropertyCount: *mut u32, pProperties: *mut VkDisplayPlanePropertiesKHR) -> VkResult,
        [instance] vkGetDisplayPlaneSupportedDisplaysKHR => (physicalDevice: VkPhysicalDevice, planeIndex: u32, pDisplayCount: *mut u32, pDisplays: *mut VkDisplayKHR) -> VkResult,
        [instance] vkGetDisplayModePropertiesKHR => (physicalDevice: VkPhysicalDevice, display: VkDisplayKHR, pPropertyCount: *mut u32, pProperties: *mut VkDisplayModePropertiesKHR) -> VkResult,
        [instance] vkCreateDisplayModeKHR => (physicalDevice: VkPhysicalDevice, display: VkDisplayKHR, pCreateInfo: *const VkDisplayModeCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pMode: *mut VkDisplayModeKHR) -> VkResult,
        [instance] vkGetDisplayPlaneCapabilitiesKHR => (physicalDevice: VkPhysicalDevice, mode: VkDisplayModeKHR, planeIndex: u32, pCapabilities: *mut VkDisplayPlaneCapabilitiesKHR) -> VkResult,
        [instance] vkCreateDisplayPlaneSurfaceKHR => (instance: VkInstance, pCreateInfo: *const VkDisplaySurfaceCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_display_swapchain
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkDisplayPresentInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub srcRect: VkRect2D,
        pub dstRect: VkRect2D,
        pub persistent: VkBool32,
    }

    extension_dispatch_table!{VK_KHR_display_swapchain | device, {
        [device] vkCreateSharedSwapchainsKHR => (device: VkDevice, swapchainCount: u32, pCreateInfos: *const VkSwapchainCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pSwapchains: *mut VkSwapchainKHR) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_xlib_surface
     * ------------------------------------------------------
    */

    vulkan_flags!(VkXlibSurfaceCreateFlagsKHR);

    #[repr(C)]
    pub struct VkXlibSurfaceCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkXlibSurfaceCreateFlagsKHR,
        pub dpy: *mut c_void,
        pub window: c_ulong,
    }

    extension_dispatch_table!{VK_KHR_xlib_surface | instance, {
        [instance] vkCreateXlibSurfaceKHR => (instance: VkInstance, pCreateInfo: *const VkXlibSurfaceCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
        [instance] vkGetPhysicalDeviceXlibPresentationSupportKHR => (physicalDevice: VkPhysicalDevice, queueFamilyIndex: u32, dpy: *mut c_void, visualID: c_ulong) -> VkBool32,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_xcb_surface
     * ------------------------------------------------------
    */

    vulkan_flags!(VkXcbSurfaceCreateFlagsKHR);

    #[repr(C)]
    pub struct VkXcbSurfaceCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkXcbSurfaceCreateFlagsKHR,
        pub connection: *mut c_void,
        pub window: u32,
    }

    extension_dispatch_table!{VK_KHR_xcb_surface | instance, {
        [instance] vkCreateXcbSurfaceKHR => (instance: VkInstance, pCreateInfo: *const VkXcbSurfaceCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
        [instance] vkGetPhysicalDeviceXcbPresentationSupportKHR => (physicalDevice: VkPhysicalDevice, queueFamilyIndex: u32, connection: *mut c_void, visual_id: u32) -> VkBool32,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_wayland_surface
     * ------------------------------------------------------
    */

    vulkan_flags!(VkWaylandSurfaceCreateFlagsKHR);

    #[repr(C)]
    pub struct VkWaylandSurfaceCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkWaylandSurfaceCreateFlagsKHR,
        pub display: *mut c_void,
        pub surface: *mut c_void,
    }

    extension_dispatch_table!{VK_KHR_wayland_surface | instance, {
        [instance] vkCreateWaylandSurfaceKHR => (instance: VkInstance, pCreateInfo: *const VkWaylandSurfaceCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
        [instance] vkGetPhysicalDeviceWaylandPresentationSupportKHR => (physicalDevice: VkPhysicalDevice, queueFamilyIndex: u32, display: *mut c_void) -> VkBool32,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_mir_surface
     * ------------------------------------------------------
    */

    vulkan_flags!(VkMirSurfaceCreateFlagsKHR);

    #[repr(C)]
    pub struct VkMirSurfaceCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkMirSurfaceCreateFlagsKHR,
        pub connection: *mut c_void,
        pub mirSurface: *mut c_void,
    }

    extension_dispatch_table!{VK_KHR_mir_surface | instance, {
        [instance] vkCreateMirSurfaceKHR => (instance: VkInstance, pCreateInfo: *const VkMirSurfaceCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
        [instance] vkGetPhysicalDeviceMirPresentationSupportKHR => (physicalDevice: VkPhysicalDevice, queueFamilyIndex: u32, connection: *mut c_void) -> VkBool32,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_android_surface
     * ------------------------------------------------------
    */

    vulkan_flags!(VkAndroidSurfaceCreateFlagsKHR);

    #[repr(C)]
    pub struct VkAndroidSurfaceCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkAndroidSurfaceCreateFlagsKHR,
        pub window: *mut c_void,
    }

    extension_dispatch_table!{VK_KHR_android_surface | instance, {
        [instance] vkCreateAndroidSurfaceKHR => (instance: VkInstance, pCreateInfo: *const VkAndroidSurfaceCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_win32_surface
     * ------------------------------------------------------
    */

    vulkan_flags!(VkWin32SurfaceCreateFlagsKHR);

    #[repr(C)]
    pub struct VkWin32SurfaceCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkWin32SurfaceCreateFlagsKHR,
        pub hinstance: *mut c_void,
        pub hwnd: *mut c_void,
    }

    extension_dispatch_table!{VK_KHR_win32_surface | instance, {
        [instance] vkCreateWin32SurfaceKHR => (instance: VkInstance, pCreateInfo: *const VkWin32SurfaceCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
        [instance] vkGetPhysicalDeviceWin32PresentationSupportKHR => (physicalDevice: VkPhysicalDevice, queueFamilyIndex: u32) -> VkBool32,
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_debug_report
     * ------------------------------------------------------
    */

    pub type VkDebugReportCallbackEXT = u64;

    #[repr(C)]
    pub enum VkDebugReportObjectTypeEXT {
        VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT = 0,
        VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT = 1,
        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT = 2,
        VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT = 3,
        VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT = 4,
        VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT = 5,
        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT = 6,
        VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT = 7,
        VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT = 8,
        VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT = 9,
        VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT = 10,
        VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT = 11,
        VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT = 12,
        VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT = 13,
        VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT = 14,
        VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT = 15,
        VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT = 16,
        VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT = 17,
        VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT = 18,
        VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT = 19,
        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT = 20,
        VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT = 21,
        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT = 22,
        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT = 23,
        VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT = 24,
        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT = 25,
        VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT = 26,
        VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT = 27,
        VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT = 28,
        VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_KHR_EXT = 29,
        VK_DEBUG_REPORT_OBJECT_TYPE_DISPLAY_MODE_KHR_EXT = 30,
        VK_DEBUG_REPORT_OBJECT_TYPE_OBJECT_TABLE_NVX_EXT = 31,
        VK_DEBUG_REPORT_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NVX_EXT = 32,
        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_KHR_EXT = 1000085000,
    }

    #[repr(C)]
    pub enum VkDebugReportErrorEXT {
        VK_DEBUG_REPORT_ERROR_NONE_EXT = 0,
        VK_DEBUG_REPORT_ERROR_CALLBACK_REF_EXT = 1,
    }

    vulkan_flags!(VkDebugReportFlagsEXT, VkDebugReportFlagBitsEXT, {
        VK_DEBUG_REPORT_INFORMATION_BIT_EXT = 0x00000001,
        VK_DEBUG_REPORT_WARNING_BIT_EXT = 0x00000002,
        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT = 0x00000004,
        VK_DEBUG_REPORT_ERROR_BIT_EXT = 0x00000008,
        VK_DEBUG_REPORT_DEBUG_BIT_EXT = 0x00000010,
    });

    pub type PFN_vkDebugReportCallbackEXT = vk_fun!((flags: VkDebugReportFlagsEXT, objectType: VkDebugReportObjectTypeEXT, object: u64, location: usize, messageCode: i32, pLayerPrefix: *const c_char, pMessage: *const c_char, pUserData: *mut c_void) -> VkBool32);

    #[repr(C)]
    pub struct VkDebugReportCallbackCreateInfoEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkDebugReportFlagsEXT,
        pub pfnCallback: Option<PFN_vkDebugReportCallbackEXT>,
        pub pUserData: *mut c_void,
    }

    extension_dispatch_table!{VK_EXT_debug_report | instance, {
        [instance] vkCreateDebugReportCallbackEXT => (instance: VkInstance, pCreateInfo: *const VkDebugReportCallbackCreateInfoEXT, pAllocator: *const VkAllocationCallbacks, pCallback: *mut VkDebugReportCallbackEXT) -> VkResult,
        [instance] vkDestroyDebugReportCallbackEXT => (instance: VkInstance, callback: VkDebugReportCallbackEXT, pAllocator: *const VkAllocationCallbacks) -> (),
        [instance] vkDebugReportMessageEXT => (instance: VkInstance, flags: VkDebugReportFlagsEXT, objectType: VkDebugReportObjectTypeEXT, object: u64, location: usize, messageCode: i32, pLayerPrefix: *const c_char, pMessage: *const c_char) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_glsl_shader
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_NV_glsl_shader | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_sampler_mirror_clamp_to_edge
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_KHR_sampler_mirror_clamp_to_edge | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_IMG_filter_cubic
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_IMG_filter_cubic | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_AMD_rasterization_order
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub enum VkRasterizationOrderAMD {
        VK_RASTERIZATION_ORDER_STRICT_AMD = 0,
        VK_RASTERIZATION_ORDER_RELAXED_AMD = 1,
    }

    #[repr(C)]
    pub struct VkPipelineRasterizationStateRasterizationOrderAMD {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub rasterizationOrder: VkRasterizationOrderAMD,
    }

    extension_dispatch_table!{VK_AMD_rasterization_order | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_AMD_shader_trinary_minmax
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_AMD_shader_trinary_minmax | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_AMD_shader_explicit_vertex_parameter
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_AMD_shader_explicit_vertex_parameter | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_debug_marker
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkDebugMarkerObjectTagInfoEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub objectType: VkDebugReportObjectTypeEXT,
        pub object: u64,
        pub tagName: u64,
        pub tagSize: usize,
        pub pTag: *const c_void,
    }

    #[repr(C)]
    pub struct VkDebugMarkerObjectNameInfoEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub objectType: VkDebugReportObjectTypeEXT,
        pub object: u64,
        pub pObjectName: *const c_char,
    }

    #[repr(C)]
    pub struct VkDebugMarkerMarkerInfoEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub pMarkerName: *const c_char,
        pub color: [f32; 4 as usize],
    }

    extension_dispatch_table!{VK_EXT_debug_marker | device, {
        [device] vkDebugMarkerSetObjectTagEXT => (device: VkDevice, pTagInfo: *mut VkDebugMarkerObjectTagInfoEXT) -> VkResult,
        [device] vkDebugMarkerSetObjectNameEXT => (device: VkDevice, pNameInfo: *mut VkDebugMarkerObjectNameInfoEXT) -> VkResult,
        [device] vkCmdDebugMarkerBeginEXT => (commandBuffer: VkCommandBuffer, pMarkerInfo: *mut VkDebugMarkerMarkerInfoEXT) -> (),
        [device] vkCmdDebugMarkerEndEXT => (commandBuffer: VkCommandBuffer) -> (),
        [device] vkCmdDebugMarkerInsertEXT => (commandBuffer: VkCommandBuffer, pMarkerInfo: *mut VkDebugMarkerMarkerInfoEXT) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_AMD_gcn_shader
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_AMD_gcn_shader | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_dedicated_allocation
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkDedicatedAllocationImageCreateInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub dedicatedAllocation: VkBool32,
    }

    #[repr(C)]
    pub struct VkDedicatedAllocationBufferCreateInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub dedicatedAllocation: VkBool32,
    }

    #[repr(C)]
    pub struct VkDedicatedAllocationMemoryAllocateInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub image: VkImage,
        pub buffer: VkBuffer,
    }

    extension_dispatch_table!{VK_NV_dedicated_allocation | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_AMD_draw_indirect_count
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_AMD_draw_indirect_count | device, {
        [device] vkCmdDrawIndirectCountAMD => (commandBuffer: VkCommandBuffer, buffer: VkBuffer, offset: VkDeviceSize, countBuffer: VkBuffer, countBufferOffset: VkDeviceSize, maxDrawCount: u32, stride: u32) -> (),
        [device] vkCmdDrawIndexedIndirectCountAMD => (commandBuffer: VkCommandBuffer, buffer: VkBuffer, offset: VkDeviceSize, countBuffer: VkBuffer, countBufferOffset: VkDeviceSize, maxDrawCount: u32, stride: u32) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_AMD_negative_viewport_height
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_AMD_negative_viewport_height | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_AMD_gpu_shader_half_float
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_AMD_gpu_shader_half_float | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_AMD_shader_ballot
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_AMD_shader_ballot | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_multiview
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkRenderPassMultiviewCreateInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub subpassCount: u32,
        pub pViewMasks: *const u32,
        pub dependencyCount: u32,
        pub pViewOffsets: *const i32,
        pub correlationMaskCount: u32,
        pub pCorrelationMasks: *const u32,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceMultiviewFeaturesKHX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub multiview: VkBool32,
        pub multiviewGeometryShader: VkBool32,
        pub multiviewTessellationShader: VkBool32,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceMultiviewPropertiesKHX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub maxMultiviewViewCount: u32,
        pub maxMultiviewInstanceIndex: u32,
    }

    extension_dispatch_table!{VK_KHX_multiview | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_IMG_format_pvrtc
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_IMG_format_pvrtc | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_external_memory_capabilities
     * ------------------------------------------------------
    */

    vulkan_flags!(VkExternalMemoryHandleTypeFlagsNV, VkExternalMemoryHandleTypeFlagBitsNV, {
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_NV = 0x00000001,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_NV = 0x00000002,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_BIT_NV = 0x00000004,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_IMAGE_KMT_BIT_NV = 0x00000008,
    });
    vulkan_flags!(VkExternalMemoryFeatureFlagsNV, VkExternalMemoryFeatureFlagBitsNV, {
        VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_NV = 0x00000001,
        VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV = 0x00000002,
        VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_NV = 0x00000004,
    });

    #[repr(C)]
    pub struct VkExternalImageFormatPropertiesNV {
        pub imageFormatProperties: VkImageFormatProperties,
        pub externalMemoryFeatures: VkExternalMemoryFeatureFlagsNV,
        pub exportFromImportedHandleTypes: VkExternalMemoryHandleTypeFlagsNV,
        pub compatibleHandleTypes: VkExternalMemoryHandleTypeFlagsNV,
    }

    extension_dispatch_table!{VK_NV_external_memory_capabilities | instance, {
        [instance] vkGetPhysicalDeviceExternalImageFormatPropertiesNV => (physicalDevice: VkPhysicalDevice, format: VkFormat, type_: VkImageType, tiling: VkImageTiling, usage: VkImageUsageFlags, flags: VkImageCreateFlags, externalHandleType: VkExternalMemoryHandleTypeFlagsNV, pExternalImageFormatProperties: *mut VkExternalImageFormatPropertiesNV) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_external_memory
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkExternalMemoryImageCreateInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleTypes: VkExternalMemoryHandleTypeFlagsNV,
    }

    #[repr(C)]
    pub struct VkExportMemoryAllocateInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleTypes: VkExternalMemoryHandleTypeFlagsNV,
    }

    extension_dispatch_table!{VK_NV_external_memory | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_external_memory_win32
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkImportMemoryWin32HandleInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleType: VkExternalMemoryHandleTypeFlagsNV,
        pub handle: *mut c_void,
    }

    #[repr(C)]
    pub struct VkExportMemoryWin32HandleInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub pAttributes: *const c_void,
        pub dwAccess: u32,
    }

    extension_dispatch_table!{VK_NV_external_memory_win32 | device, {
        [device] vkGetMemoryWin32HandleNV => (device: VkDevice, memory: VkDeviceMemory, handleType: VkExternalMemoryHandleTypeFlagsNV, pHandle: *mut *mut c_void) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_win32_keyed_mutex
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkWin32KeyedMutexAcquireReleaseInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub acquireCount: u32,
        pub pAcquireSyncs: *const VkDeviceMemory,
        pub pAcquireKeys: *const u64,
        pub pAcquireTimeoutMilliseconds: *const u32,
        pub releaseCount: u32,
        pub pReleaseSyncs: *const VkDeviceMemory,
        pub pReleaseKeys: *const u64,
    }

    extension_dispatch_table!{VK_NV_win32_keyed_mutex | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_get_physical_device_properties2
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkPhysicalDeviceFeatures2KHR {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub features: VkPhysicalDeviceFeatures,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceProperties2KHR {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub properties: VkPhysicalDeviceProperties,
    }

    #[repr(C)]
    pub struct VkFormatProperties2KHR {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub formatProperties: VkFormatProperties,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceImageFormatInfo2KHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub format: VkFormat,
        pub type_: VkImageType,
        pub tiling: VkImageTiling,
        pub usage: VkImageUsageFlags,
        pub flags: VkImageCreateFlags,
    }

    #[repr(C)]
    pub struct VkImageFormatProperties2KHR {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub imageFormatProperties: VkImageFormatProperties,
    }

    #[repr(C)]
    pub struct VkQueueFamilyProperties2KHR {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub queueFamilyProperties: VkQueueFamilyProperties,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceMemoryProperties2KHR {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub memoryProperties: VkPhysicalDeviceMemoryProperties,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceSparseImageFormatInfo2KHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub format: VkFormat,
        pub type_: VkImageType,
        pub samples: VkSampleCountFlagBits,
        pub usage: VkImageUsageFlags,
        pub tiling: VkImageTiling,
    }

    #[repr(C)]
    pub struct VkSparseImageFormatProperties2KHR {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub properties: VkSparseImageFormatProperties,
    }

    extension_dispatch_table!{VK_KHR_get_physical_device_properties2 | instance, {
        [instance] vkGetPhysicalDeviceFeatures2KHR => (physicalDevice: VkPhysicalDevice, pFeatures: *mut VkPhysicalDeviceFeatures2KHR) -> (),
        [instance] vkGetPhysicalDeviceProperties2KHR => (physicalDevice: VkPhysicalDevice, pProperties: *mut VkPhysicalDeviceProperties2KHR) -> (),
        [instance] vkGetPhysicalDeviceFormatProperties2KHR => (physicalDevice: VkPhysicalDevice, format: VkFormat, pFormatProperties: *mut VkFormatProperties2KHR) -> (),
        [instance] vkGetPhysicalDeviceImageFormatProperties2KHR => (physicalDevice: VkPhysicalDevice, pImageFormatInfo: *const VkPhysicalDeviceImageFormatInfo2KHR, pImageFormatProperties: *mut VkImageFormatProperties2KHR) -> VkResult,
        [instance] vkGetPhysicalDeviceQueueFamilyProperties2KHR => (physicalDevice: VkPhysicalDevice, pQueueFamilyPropertyCount: *mut u32, pQueueFamilyProperties: *mut VkQueueFamilyProperties2KHR) -> (),
        [instance] vkGetPhysicalDeviceMemoryProperties2KHR => (physicalDevice: VkPhysicalDevice, pMemoryProperties: *mut VkPhysicalDeviceMemoryProperties2KHR) -> (),
        [instance] vkGetPhysicalDeviceSparseImageFormatProperties2KHR => (physicalDevice: VkPhysicalDevice, pFormatInfo: *const VkPhysicalDeviceSparseImageFormatInfo2KHR, pPropertyCount: *mut u32, pProperties: *mut VkSparseImageFormatProperties2KHR) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_device_group
     * ------------------------------------------------------
    */

    pub const VK_MAX_DEVICE_GROUP_SIZE_KHX: u32 = 32;

    vulkan_flags!(VkPeerMemoryFeatureFlagsKHX, VkPeerMemoryFeatureFlagBitsKHX, {
        VK_PEER_MEMORY_FEATURE_COPY_SRC_BIT_KHX = 0x00000001,
        VK_PEER_MEMORY_FEATURE_COPY_DST_BIT_KHX = 0x00000002,
        VK_PEER_MEMORY_FEATURE_GENERIC_SRC_BIT_KHX = 0x00000004,
        VK_PEER_MEMORY_FEATURE_GENERIC_DST_BIT_KHX = 0x00000008,
    });
    vulkan_flags!(VkDeviceGroupPresentModeFlagsKHX, VkDeviceGroupPresentModeFlagBitsKHX, {
        VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_BIT_KHX = 0x00000001,
        VK_DEVICE_GROUP_PRESENT_MODE_REMOTE_BIT_KHX = 0x00000002,
        VK_DEVICE_GROUP_PRESENT_MODE_SUM_BIT_KHX = 0x00000004,
        VK_DEVICE_GROUP_PRESENT_MODE_LOCAL_MULTI_DEVICE_BIT_KHX = 0x00000008,
    });
    vulkan_flags!(VkMemoryAllocateFlagsKHX, VkMemoryAllocateFlagBitsKHX, {
        VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT_KHX = 0x00000001,
    });

    #[repr(C)]
    pub struct VkBindBufferMemoryInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub buffer: VkBuffer,
        pub memory: VkDeviceMemory,
        pub memoryOffset: VkDeviceSize,
        pub deviceIndexCount: u32,
        pub pDeviceIndices: *const u32,
    }

    #[repr(C)]
    pub struct VkBindImageMemoryInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub image: VkImage,
        pub memory: VkDeviceMemory,
        pub memoryOffset: VkDeviceSize,
        pub deviceIndexCount: u32,
        pub pDeviceIndices: *const u32,
        pub SFRRectCount: u32,
        pub pSFRRects: *const VkRect2D,
    }

    #[repr(C)]
    pub struct VkDeviceGroupPresentCapabilitiesKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub presentMask: [u32; VK_MAX_DEVICE_GROUP_SIZE_KHX as usize],
        pub modes: VkDeviceGroupPresentModeFlagsKHX,
    }

    #[repr(C)]
    pub struct VkAcquireNextImageInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub swapchain: VkSwapchainKHR,
        pub timeout: u64,
        pub semaphore: VkSemaphore,
        pub fence: VkFence,
        pub deviceMask: u32,
    }

    #[repr(C)]
    pub struct VkMemoryAllocateFlagsInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkMemoryAllocateFlagsKHX,
        pub deviceMask: u32,
    }

    #[repr(C)]
    pub struct VkDeviceGroupRenderPassBeginInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub deviceMask: u32,
        pub deviceRenderAreaCount: u32,
        pub pDeviceRenderAreas: *const VkRect2D,
    }

    #[repr(C)]
    pub struct VkDeviceGroupCommandBufferBeginInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub deviceMask: u32,
    }

    #[repr(C)]
    pub struct VkDeviceGroupSubmitInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub waitSemaphoreCount: u32,
        pub pWaitSemaphoreDeviceIndices: *const u32,
        pub commandBufferCount: u32,
        pub pCommandBufferDeviceMasks: *const u32,
        pub signalSemaphoreCount: u32,
        pub pSignalSemaphoreDeviceIndices: *const u32,
    }

    #[repr(C)]
    pub struct VkDeviceGroupBindSparseInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub resourceDeviceIndex: u32,
        pub memoryDeviceIndex: u32,
    }

    #[repr(C)]
    pub struct VkImageSwapchainCreateInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub swapchain: VkSwapchainKHR,
    }

    #[repr(C)]
    pub struct VkBindImageMemorySwapchainInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub swapchain: VkSwapchainKHR,
        pub imageIndex: u32,
    }

    #[repr(C)]
    pub struct VkDeviceGroupPresentInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub swapchainCount: u32,
        pub pDeviceMasks: *const u32,
        pub mode: VkDeviceGroupPresentModeFlagBitsKHX,
    }

    #[repr(C)]
    pub struct VkDeviceGroupSwapchainCreateInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub modes: VkDeviceGroupPresentModeFlagsKHX,
    }

    extension_dispatch_table!{VK_KHX_device_group | device, {
        [device] vkGetDeviceGroupPeerMemoryFeaturesKHX => (device: VkDevice, heapIndex: u32, localDeviceIndex: u32, remoteDeviceIndex: u32, pPeerMemoryFeatures: *mut VkPeerMemoryFeatureFlagsKHX) -> (),
        [device] vkBindBufferMemory2KHX => (device: VkDevice, bindInfoCount: u32, pBindInfos: *const VkBindBufferMemoryInfoKHX) -> VkResult,
        [device] vkBindImageMemory2KHX => (device: VkDevice, bindInfoCount: u32, pBindInfos: *const VkBindImageMemoryInfoKHX) -> VkResult,
        [device] vkCmdSetDeviceMaskKHX => (commandBuffer: VkCommandBuffer, deviceMask: u32) -> (),
        [device] vkGetDeviceGroupPresentCapabilitiesKHX => (device: VkDevice, pDeviceGroupPresentCapabilities: *mut VkDeviceGroupPresentCapabilitiesKHX) -> VkResult,
        [device] vkGetDeviceGroupSurfacePresentModesKHX => (device: VkDevice, surface: VkSurfaceKHR, pModes: *mut VkDeviceGroupPresentModeFlagsKHX) -> VkResult,
        [device] vkAcquireNextImage2KHX => (device: VkDevice, pAcquireInfo: *const VkAcquireNextImageInfoKHX, pImageIndex: *mut u32) -> VkResult,
        [device] vkCmdDispatchBaseKHX => (commandBuffer: VkCommandBuffer, baseGroupX: u32, baseGroupY: u32, baseGroupZ: u32, groupCountX: u32, groupCountY: u32, groupCountZ: u32) -> (),
        [instance] vkGetPhysicalDevicePresentRectanglesKHX => (physicalDevice: VkPhysicalDevice, surface: VkSurfaceKHR, pRectCount: *mut u32, pRects: *mut VkRect2D) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_validation_flags
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub enum VkValidationCheckEXT {
        VK_VALIDATION_CHECK_ALL_EXT = 0,
    }

    #[repr(C)]
    pub struct VkValidationFlagsEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub disabledValidationCheckCount: u32,
        pub pDisabledValidationChecks: *mut VkValidationCheckEXT,
    }

    extension_dispatch_table!{VK_EXT_validation_flags | instance, {
    }}

    /*
     * ------------------------------------------------------
     * VK_NN_vi_surface
     * ------------------------------------------------------
    */

    vulkan_flags!(VkViSurfaceCreateFlagsNN);

    #[repr(C)]
    pub struct VkViSurfaceCreateInfoNN {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkViSurfaceCreateFlagsNN,
        pub window: *mut c_void,
    }

    extension_dispatch_table!{VK_NN_vi_surface | instance, {
        [instance] vkCreateViSurfaceNN => (instance: VkInstance, pCreateInfo: *const VkViSurfaceCreateInfoNN, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_shader_draw_parameters
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_KHR_shader_draw_parameters | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_shader_subgroup_ballot
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_EXT_shader_subgroup_ballot | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_shader_subgroup_vote
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_EXT_shader_subgroup_vote | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_maintenance1
     * ------------------------------------------------------
    */

    vulkan_flags!(VkCommandPoolTrimFlagsKHR);

    extension_dispatch_table!{VK_KHR_maintenance1 | device, {
        [device] vkTrimCommandPoolKHR => (device: VkDevice, commandPool: VkCommandPool, flags: VkCommandPoolTrimFlagsKHR) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_device_group_creation
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkPhysicalDeviceGroupPropertiesKHX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub physicalDeviceCount: u32,
        pub physicalDevices: [VkPhysicalDevice; VK_MAX_DEVICE_GROUP_SIZE_KHX as usize],
        pub subsetAllocation: VkBool32,
    }

    #[repr(C)]
    pub struct VkDeviceGroupDeviceCreateInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub physicalDeviceCount: u32,
        pub pPhysicalDevices: *const VkPhysicalDevice,
    }

    extension_dispatch_table!{VK_KHX_device_group_creation | instance, {
        [instance] vkEnumeratePhysicalDeviceGroupsKHX => (instance: VkInstance, pPhysicalDeviceGroupCount: *mut u32, pPhysicalDeviceGroupProperties: *mut VkPhysicalDeviceGroupPropertiesKHX) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_external_memory_capabilities
     * ------------------------------------------------------
    */

    pub const VK_LUID_SIZE_KHX: u32 = 8;

    vulkan_flags!(VkExternalMemoryFeatureFlagsKHX, VkExternalMemoryFeatureFlagBitsKHX, {
        VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT_KHX = 0x00000001,
        VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHX = 0x00000002,
        VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHX = 0x00000004,
    });
    vulkan_flags!(VkExternalMemoryHandleTypeFlagsKHX, VkExternalMemoryHandleTypeFlagBitsKHX, {
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHX = 0x00000001,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHX = 0x00000002,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHX = 0x00000004,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT_KHX = 0x00000008,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT_KHX = 0x00000010,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT_KHX = 0x00000020,
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT_KHX = 0x00000040,
    });

    #[repr(C)]
    pub struct VkPhysicalDeviceExternalBufferInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkBufferCreateFlags,
        pub usage: VkBufferUsageFlags,
        pub handleType: VkExternalMemoryHandleTypeFlagBitsKHX,
    }

    #[repr(C)]
    pub struct VkExternalMemoryPropertiesKHX {
        pub externalMemoryFeatures: VkExternalMemoryFeatureFlagsKHX,
        pub exportFromImportedHandleTypes: VkExternalMemoryHandleTypeFlagsKHX,
        pub compatibleHandleTypes: VkExternalMemoryHandleTypeFlagsKHX,
    }

    #[repr(C)]
    pub struct VkExternalBufferPropertiesKHX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub externalMemoryProperties: VkExternalMemoryPropertiesKHX,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceExternalImageFormatInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleType: VkExternalMemoryHandleTypeFlagBitsKHX,
    }

    #[repr(C)]
    pub struct VkExternalImageFormatPropertiesKHX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub externalMemoryProperties: VkExternalMemoryPropertiesKHX,
    }

    #[repr(C)]
    pub struct VkPhysicalDeviceIDPropertiesKHX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub deviceUUID: [u8; VK_UUID_SIZE as usize],
        pub driverUUID: [u8; VK_UUID_SIZE as usize],
        pub deviceLUID: [u8; VK_LUID_SIZE_KHX as usize],
        pub deviceLUIDValid: VkBool32,
    }

    extension_dispatch_table!{VK_KHX_external_memory_capabilities | instance, {
        [instance] vkGetPhysicalDeviceExternalBufferPropertiesKHX => (physicalDevice: VkPhysicalDevice, pExternalBufferInfo: *const VkPhysicalDeviceExternalBufferInfoKHX, pExternalBufferProperties: *mut VkExternalBufferPropertiesKHX) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_external_memory
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkExternalMemoryImageCreateInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleTypes: VkExternalMemoryHandleTypeFlagsKHX,
    }

    #[repr(C)]
    pub struct VkExternalMemoryBufferCreateInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleTypes: VkExternalMemoryHandleTypeFlagsKHX,
    }

    #[repr(C)]
    pub struct VkExportMemoryAllocateInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleTypes: VkExternalMemoryHandleTypeFlagsKHX,
    }

    extension_dispatch_table!{VK_KHX_external_memory | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_external_memory_win32
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkMemoryWin32HandlePropertiesKHX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub memoryTypeBits: u32,
    }

    #[repr(C)]
    pub struct VkImportMemoryWin32HandleInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleType: VkExternalMemoryHandleTypeFlagBitsKHX,
        pub handle: *mut c_void,
    }

    #[repr(C)]
    pub struct VkExportMemoryWin32HandleInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub pAttributes: *const c_void,
        pub dwAccess: u32,
        pub name: *const u16,
    }

    extension_dispatch_table!{VK_KHX_external_memory_win32 | device, {
        [device] vkGetMemoryWin32HandleKHX => (device: VkDevice, memory: VkDeviceMemory, handleType: VkExternalMemoryHandleTypeFlagBitsKHX, pHandle: *mut *mut c_void) -> VkResult,
        [device] vkGetMemoryWin32HandlePropertiesKHX => (device: VkDevice, handleType: VkExternalMemoryHandleTypeFlagBitsKHX, handle: *mut c_void, pMemoryWin32HandleProperties: *mut VkMemoryWin32HandlePropertiesKHX) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_external_memory_fd
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkMemoryFdPropertiesKHX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub memoryTypeBits: u32,
    }

    #[repr(C)]
    pub struct VkImportMemoryFdInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleType: VkExternalMemoryHandleTypeFlagBitsKHX,
        pub fd: c_int,
    }

    extension_dispatch_table!{VK_KHX_external_memory_fd | device, {
        [device] vkGetMemoryFdKHX => (device: VkDevice, memory: VkDeviceMemory, handleType: VkExternalMemoryHandleTypeFlagBitsKHX, pFd: *mut c_int) -> VkResult,
        [device] vkGetMemoryFdPropertiesKHX => (device: VkDevice, handleType: VkExternalMemoryHandleTypeFlagBitsKHX, fd: c_int, pMemoryFdProperties: *mut VkMemoryFdPropertiesKHX) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_win32_keyed_mutex
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkWin32KeyedMutexAcquireReleaseInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub acquireCount: u32,
        pub pAcquireSyncs: *const VkDeviceMemory,
        pub pAcquireKeys: *const u64,
        pub pAcquireTimeouts: *const u32,
        pub releaseCount: u32,
        pub pReleaseSyncs: *const VkDeviceMemory,
        pub pReleaseKeys: *const u64,
    }

    extension_dispatch_table!{VK_KHX_win32_keyed_mutex | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_external_semaphore_capabilities
     * ------------------------------------------------------
    */

    vulkan_flags!(VkExternalSemaphoreHandleTypeFlagsKHX, VkExternalSemaphoreHandleTypeFlagBitsKHX, {
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHX = 0x00000001,
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHX = 0x00000002,
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHX = 0x00000004,
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT_KHX = 0x00000008,
        VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_FENCE_FD_BIT_KHX = 0x00000010,
    });
    vulkan_flags!(VkExternalSemaphoreFeatureFlagsKHX, VkExternalSemaphoreFeatureFlagBitsKHX, {
        VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT_KHX = 0x00000001,
        VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT_KHX = 0x00000002,
    });

    #[repr(C)]
    pub struct VkPhysicalDeviceExternalSemaphoreInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleType: VkExternalSemaphoreHandleTypeFlagBitsKHX,
    }

    #[repr(C)]
    pub struct VkExternalSemaphorePropertiesKHX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub exportFromImportedHandleTypes: VkExternalSemaphoreHandleTypeFlagsKHX,
        pub compatibleHandleTypes: VkExternalSemaphoreHandleTypeFlagsKHX,
        pub externalSemaphoreFeatures: VkExternalSemaphoreFeatureFlagsKHX,
    }

    extension_dispatch_table!{VK_KHX_external_semaphore_capabilities | instance, {
        [instance] vkGetPhysicalDeviceExternalSemaphorePropertiesKHX => (physicalDevice: VkPhysicalDevice, pExternalSemaphoreInfo: *const VkPhysicalDeviceExternalSemaphoreInfoKHX, pExternalSemaphoreProperties: *mut VkExternalSemaphorePropertiesKHX) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_external_semaphore
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkExportSemaphoreCreateInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub handleTypes: VkExternalSemaphoreHandleTypeFlagsKHX,
    }

    extension_dispatch_table!{VK_KHX_external_semaphore | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_external_semaphore_win32
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkImportSemaphoreWin32HandleInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub semaphore: VkSemaphore,
        pub handleType: VkExternalSemaphoreHandleTypeFlagsKHX,
        pub handle: *mut c_void,
    }

    #[repr(C)]
    pub struct VkExportSemaphoreWin32HandleInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub pAttributes: *const c_void,
        pub dwAccess: u32,
        pub name: *const u16,
    }

    #[repr(C)]
    pub struct VkD3D12FenceSubmitInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub waitSemaphoreValuesCount: u32,
        pub pWaitSemaphoreValues: *const u64,
        pub signalSemaphoreValuesCount: u32,
        pub pSignalSemaphoreValues: *const u64,
    }

    extension_dispatch_table!{VK_KHX_external_semaphore_win32 | device, {
        [device] vkImportSemaphoreWin32HandleKHX => (device: VkDevice, pImportSemaphoreWin32HandleInfo: *const VkImportSemaphoreWin32HandleInfoKHX) -> VkResult,
        [device] vkGetSemaphoreWin32HandleKHX => (device: VkDevice, semaphore: VkSemaphore, handleType: VkExternalSemaphoreHandleTypeFlagBitsKHX, pHandle: *mut *mut c_void) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHX_external_semaphore_fd
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkImportSemaphoreFdInfoKHX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub semaphore: VkSemaphore,
        pub handleType: VkExternalSemaphoreHandleTypeFlagBitsKHX,
        pub fd: c_int,
    }

    extension_dispatch_table!{VK_KHX_external_semaphore_fd | device, {
        [device] vkImportSemaphoreFdKHX => (device: VkDevice, pImportSemaphoreFdInfo: *const VkImportSemaphoreFdInfoKHX) -> VkResult,
        [device] vkGetSemaphoreFdKHX => (device: VkDevice, semaphore: VkSemaphore, handleType: VkExternalSemaphoreHandleTypeFlagBitsKHX, pFd: *mut c_int) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_push_descriptor
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkPhysicalDevicePushDescriptorPropertiesKHR {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub maxPushDescriptors: u32,
    }

    extension_dispatch_table!{VK_KHR_push_descriptor | device, {
        [device] vkCmdPushDescriptorSetKHR => (commandBuffer: VkCommandBuffer, pipelineBindPoint: VkPipelineBindPoint, layout: VkPipelineLayout, set: u32, descriptorWriteCount: u32, pDescriptorWrites: *const VkWriteDescriptorSet) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_incremental_present
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkRectLayerKHR {
        pub offset: VkOffset2D,
        pub extent: VkExtent2D,
        pub layer: u32,
    }

    #[repr(C)]
    pub struct VkPresentRegionKHR {
        pub rectangleCount: u32,
        pub pRectangles: *const VkRectLayerKHR,
    }

    #[repr(C)]
    pub struct VkPresentRegionsKHR {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub swapchainCount: u32,
        pub pRegions: *const VkPresentRegionKHR,
    }

    extension_dispatch_table!{VK_KHR_incremental_present | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_KHR_descriptor_update_template
     * ------------------------------------------------------
    */

    pub type VkDescriptorUpdateTemplateKHR = u64;

    #[repr(C)]
    pub enum VkDescriptorUpdateTemplateTypeKHR {
        VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET_KHR = 0,
        VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR = 1,
    }

    vulkan_flags!(VkDescriptorUpdateTemplateCreateFlagsKHR);

    #[repr(C)]
    pub struct VkDescriptorUpdateTemplateEntryKHR {
        pub dstBinding: u32,
        pub dstArrayElement: u32,
        pub descriptorCount: u32,
        pub descriptorType: VkDescriptorType,
        pub offset: usize,
        pub stride: usize,
    }

    #[repr(C)]
    pub struct VkDescriptorUpdateTemplateCreateInfoKHR {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub flags: VkDescriptorUpdateTemplateCreateFlagsKHR,
        pub descriptorUpdateEntryCount: u32,
        pub pDescriptorUpdateEntries: *const VkDescriptorUpdateTemplateEntryKHR,
        pub templateType: VkDescriptorUpdateTemplateTypeKHR,
        pub descriptorSetLayout: VkDescriptorSetLayout,
        pub pipelineBindPoint: VkPipelineBindPoint,
        pub pipelineLayout: VkPipelineLayout,
        pub set: u32,
    }

    extension_dispatch_table!{VK_KHR_descriptor_update_template | device, {
        [device] vkCreateDescriptorUpdateTemplateKHR => (device: VkDevice, pCreateInfo: *const VkDescriptorUpdateTemplateCreateInfoKHR, pAllocator: *const VkAllocationCallbacks, pDescriptorUpdateTemplate: *mut VkDescriptorUpdateTemplateKHR) -> VkResult,
        [device] vkDestroyDescriptorUpdateTemplateKHR => (device: VkDevice, descriptorUpdateTemplate: VkDescriptorUpdateTemplateKHR, pAllocator: *const VkAllocationCallbacks) -> (),
        [device] vkUpdateDescriptorSetWithTemplateKHR => (device: VkDevice, descriptorSet: VkDescriptorSet, descriptorUpdateTemplate: VkDescriptorUpdateTemplateKHR, pData: *const c_void) -> (),
        [device] vkCmdPushDescriptorSetWithTemplateKHR => (commandBuffer: VkCommandBuffer, descriptorUpdateTemplate: VkDescriptorUpdateTemplateKHR, layout: VkPipelineLayout, set: u32, pData: *const c_void) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_NVX_device_generated_commands
     * ------------------------------------------------------
    */

    pub type VkObjectTableNVX = u64;
    pub type VkIndirectCommandsLayoutNVX = u64;

    #[repr(C)]
    pub enum VkIndirectCommandsTokenTypeNVX {
        VK_INDIRECT_COMMANDS_TOKEN_PIPELINE_NVX = 0,
        VK_INDIRECT_COMMANDS_TOKEN_DESCRIPTOR_SET_NVX = 1,
        VK_INDIRECT_COMMANDS_TOKEN_INDEX_BUFFER_NVX = 2,
        VK_INDIRECT_COMMANDS_TOKEN_VERTEX_BUFFER_NVX = 3,
        VK_INDIRECT_COMMANDS_TOKEN_PUSH_CONSTANT_NVX = 4,
        VK_INDIRECT_COMMANDS_TOKEN_DRAW_INDEXED_NVX = 5,
        VK_INDIRECT_COMMANDS_TOKEN_DRAW_NVX = 6,
        VK_INDIRECT_COMMANDS_TOKEN_DISPATCH_NVX = 7,
    }

    #[repr(C)]
    pub enum VkObjectEntryTypeNVX {
        VK_OBJECT_ENTRY_DESCRIPTOR_SET_NVX = 0,
        VK_OBJECT_ENTRY_PIPELINE_NVX = 1,
        VK_OBJECT_ENTRY_INDEX_BUFFER_NVX = 2,
        VK_OBJECT_ENTRY_VERTEX_BUFFER_NVX = 3,
        VK_OBJECT_ENTRY_PUSH_CONSTANT_NVX = 4,
    }

    vulkan_flags!(VkIndirectCommandsLayoutUsageFlagsNVX, VkIndirectCommandsLayoutUsageFlagBitsNVX, {
        VK_INDIRECT_COMMANDS_LAYOUT_USAGE_UNORDERED_SEQUENCES_BIT_NVX = 0x00000001,
        VK_INDIRECT_COMMANDS_LAYOUT_USAGE_SPARSE_SEQUENCES_BIT_NVX = 0x00000002,
        VK_INDIRECT_COMMANDS_LAYOUT_USAGE_EMPTY_EXECUTIONS_BIT_NVX = 0x00000004,
        VK_INDIRECT_COMMANDS_LAYOUT_USAGE_INDEXED_SEQUENCES_BIT_NVX = 0x00000008,
    });
    vulkan_flags!(VkObjectEntryUsageFlagsNVX, VkObjectEntryUsageFlagBitsNVX, {
        VK_OBJECT_ENTRY_USAGE_GRAPHICS_BIT_NVX = 0x00000001,
        VK_OBJECT_ENTRY_USAGE_COMPUTE_BIT_NVX = 0x00000002,
    });

    #[repr(C)]
    pub struct VkIndirectCommandsTokenNVX {
        pub tokenType: VkIndirectCommandsTokenTypeNVX,
        pub buffer: VkBuffer,
        pub offset: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkCmdProcessCommandsInfoNVX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub objectTable: VkObjectTableNVX,
        pub indirectCommandsLayout: VkIndirectCommandsLayoutNVX,
        pub indirectCommandsTokenCount: u32,
        pub pIndirectCommandsTokens: *const VkIndirectCommandsTokenNVX,
        pub maxSequencesCount: u32,
        pub targetCommandBuffer: VkCommandBuffer,
        pub sequencesCountBuffer: VkBuffer,
        pub sequencesCountOffset: VkDeviceSize,
        pub sequencesIndexBuffer: VkBuffer,
        pub sequencesIndexOffset: VkDeviceSize,
    }

    #[repr(C)]
    pub struct VkCmdReserveSpaceForCommandsInfoNVX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub objectTable: VkObjectTableNVX,
        pub indirectCommandsLayout: VkIndirectCommandsLayoutNVX,
        pub maxSequencesCount: u32,
    }

    #[repr(C)]
    pub struct VkIndirectCommandsLayoutTokenNVX {
        pub tokenType: VkIndirectCommandsTokenTypeNVX,
        pub bindingUnit: u32,
        pub dynamicCount: u32,
        pub divisor: u32,
    }

    #[repr(C)]
    pub struct VkIndirectCommandsLayoutCreateInfoNVX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub pipelineBindPoint: VkPipelineBindPoint,
        pub flags: VkIndirectCommandsLayoutUsageFlagsNVX,
        pub tokenCount: u32,
        pub pTokens: *const VkIndirectCommandsLayoutTokenNVX,
    }

    #[repr(C)]
    pub struct VkObjectTableCreateInfoNVX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub objectCount: u32,
        pub pObjectEntryTypes: *const VkObjectEntryTypeNVX,
        pub pObjectEntryCounts: *const u32,
        pub pObjectEntryUsageFlags: *const VkObjectEntryUsageFlagsNVX,
        pub maxUniformBuffersPerDescriptor: u32,
        pub maxStorageBuffersPerDescriptor: u32,
        pub maxStorageImagesPerDescriptor: u32,
        pub maxSampledImagesPerDescriptor: u32,
        pub maxPipelineLayouts: u32,
    }

    #[repr(C)]
    pub struct VkObjectTableEntryNVX {
        pub type_: VkObjectEntryTypeNVX,
        pub flags: VkObjectEntryUsageFlagsNVX,
    }

    #[repr(C)]
    pub struct VkDeviceGeneratedCommandsFeaturesNVX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub computeBindingPointSupport: VkBool32,
    }

    #[repr(C)]
    pub struct VkDeviceGeneratedCommandsLimitsNVX {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub maxIndirectCommandsLayoutTokenCount: u32,
        pub maxObjectEntryCounts: u32,
        pub minSequenceCountBufferOffsetAlignment: u32,
        pub minSequenceIndexBufferOffsetAlignment: u32,
        pub minCommandsTokenBufferOffsetAlignment: u32,
    }

    #[repr(C)]
    pub struct VkObjectTablePipelineEntryNVX {
        pub type_: VkObjectEntryTypeNVX,
        pub flags: VkObjectEntryUsageFlagsNVX,
        pub pipeline: VkPipeline,
    }

    #[repr(C)]
    pub struct VkObjectTableDescriptorSetEntryNVX {
        pub type_: VkObjectEntryTypeNVX,
        pub flags: VkObjectEntryUsageFlagsNVX,
        pub pipelineLayout: VkPipelineLayout,
        pub descriptorSet: VkDescriptorSet,
    }

    #[repr(C)]
    pub struct VkObjectTableVertexBufferEntryNVX {
        pub type_: VkObjectEntryTypeNVX,
        pub flags: VkObjectEntryUsageFlagsNVX,
        pub buffer: VkBuffer,
    }

    #[repr(C)]
    pub struct VkObjectTableIndexBufferEntryNVX {
        pub type_: VkObjectEntryTypeNVX,
        pub flags: VkObjectEntryUsageFlagsNVX,
        pub buffer: VkBuffer,
        pub indexType: VkIndexType,
    }

    #[repr(C)]
    pub struct VkObjectTablePushConstantEntryNVX {
        pub type_: VkObjectEntryTypeNVX,
        pub flags: VkObjectEntryUsageFlagsNVX,
        pub pipelineLayout: VkPipelineLayout,
        pub stageFlags: VkShaderStageFlags,
    }

    extension_dispatch_table!{VK_NVX_device_generated_commands | device, {
        [device] vkCmdProcessCommandsNVX => (commandBuffer: VkCommandBuffer, pProcessCommandsInfo: *const VkCmdProcessCommandsInfoNVX) -> (),
        [device] vkCmdReserveSpaceForCommandsNVX => (commandBuffer: VkCommandBuffer, pReserveSpaceInfo: *const VkCmdReserveSpaceForCommandsInfoNVX) -> (),
        [device] vkCreateIndirectCommandsLayoutNVX => (device: VkDevice, pCreateInfo: *const VkIndirectCommandsLayoutCreateInfoNVX, pAllocator: *const VkAllocationCallbacks, pIndirectCommandsLayout: *mut VkIndirectCommandsLayoutNVX) -> VkResult,
        [device] vkDestroyIndirectCommandsLayoutNVX => (device: VkDevice, indirectCommandsLayout: VkIndirectCommandsLayoutNVX, pAllocator: *const VkAllocationCallbacks) -> (),
        [device] vkCreateObjectTableNVX => (device: VkDevice, pCreateInfo: *const VkObjectTableCreateInfoNVX, pAllocator: *const VkAllocationCallbacks, pObjectTable: *mut VkObjectTableNVX) -> VkResult,
        [device] vkDestroyObjectTableNVX => (device: VkDevice, objectTable: VkObjectTableNVX, pAllocator: *const VkAllocationCallbacks) -> (),
        [device] vkRegisterObjectsNVX => (device: VkDevice, objectTable: VkObjectTableNVX, objectCount: u32, ppObjectTableEntries: *const *const VkObjectTableEntryNVX, pObjectIndices: *const u32) -> VkResult,
        [device] vkUnregisterObjectsNVX => (device: VkDevice, objectTable: VkObjectTableNVX, objectCount: u32, pObjectEntryTypes: *const VkObjectEntryTypeNVX, pObjectIndices: *const u32) -> VkResult,
        [instance] vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX => (physicalDevice: VkPhysicalDevice, pFeatures: *mut VkDeviceGeneratedCommandsFeaturesNVX, pLimits: *mut VkDeviceGeneratedCommandsLimitsNVX) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_clip_space_w_scaling
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkViewportWScalingNV {
        pub xcoeff: f32,
        pub ycoeff: f32,
    }

    #[repr(C)]
    pub struct VkPipelineViewportWScalingStateCreateInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub viewportWScalingEnable: VkBool32,
        pub viewportCount: u32,
        pub pViewportWScalings: *const VkViewportWScalingNV,
    }

    extension_dispatch_table!{VK_NV_clip_space_w_scaling | device, {
        [device] vkCmdSetViewportWScalingNV => (commandBuffer: VkCommandBuffer, firstViewport: u32, viewportCount: u32, pViewportWScalings: *const VkViewportWScalingNV) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_direct_mode_display
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_EXT_direct_mode_display | instance, {
        [instance] vkReleaseDisplayEXT => (physicalDevice: VkPhysicalDevice, display: VkDisplayKHR) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_acquire_xlib_display
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_EXT_acquire_xlib_display | instance, {
        [instance] vkAcquireXlibDisplayEXT => (physicalDevice: VkPhysicalDevice, dpy: *mut c_void, display: VkDisplayKHR) -> VkResult,
        [instance] vkGetRandROutputDisplayEXT => (physicalDevice: VkPhysicalDevice, dpy: *mut c_void, rrOutput: c_ulong, pDisplay: *mut VkDisplayKHR) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_display_surface_counter
     * ------------------------------------------------------
    */

    vulkan_flags!(VkSurfaceCounterFlagsEXT, VkSurfaceCounterFlagBitsEXT, {
        VK_SURFACE_COUNTER_VBLANK_EXT = 0x00000001,
    });

    #[repr(C)]
    pub struct VkSurfaceCapabilities2EXT {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub minImageCount: u32,
        pub maxImageCount: u32,
        pub currentExtent: VkExtent2D,
        pub minImageExtent: VkExtent2D,
        pub maxImageExtent: VkExtent2D,
        pub maxImageArrayLayers: u32,
        pub supportedTransforms: VkSurfaceTransformFlagsKHR,
        pub currentTransform: VkSurfaceTransformFlagBitsKHR,
        pub supportedCompositeAlpha: VkCompositeAlphaFlagsKHR,
        pub supportedUsageFlags: VkImageUsageFlags,
        pub supportedSurfaceCounters: VkSurfaceCounterFlagsEXT,
    }

    extension_dispatch_table!{VK_EXT_display_surface_counter | instance, {
        [instance] vkGetPhysicalDeviceSurfaceCapabilities2EXT => (physicalDevice: VkPhysicalDevice, surface: VkSurfaceKHR, pSurfaceCapabilities: *mut VkSurfaceCapabilities2EXT) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_display_control
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub enum VkDisplayPowerStateEXT {
        VK_DISPLAY_POWER_STATE_OFF_EXT = 0,
        VK_DISPLAY_POWER_STATE_SUSPEND_EXT = 1,
        VK_DISPLAY_POWER_STATE_ON_EXT = 2,
    }

    #[repr(C)]
    pub enum VkDeviceEventTypeEXT {
        VK_DEVICE_EVENT_TYPE_DISPLAY_HOTPLUG_EXT = 0,
    }

    #[repr(C)]
    pub enum VkDisplayEventTypeEXT {
        VK_DISPLAY_EVENT_TYPE_FIRST_PIXEL_OUT_EXT = 0,
    }

    #[repr(C)]
    pub struct VkDisplayPowerInfoEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub powerState: VkDisplayPowerStateEXT,
    }

    #[repr(C)]
    pub struct VkDeviceEventInfoEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub deviceEvent: VkDeviceEventTypeEXT,
    }

    #[repr(C)]
    pub struct VkDisplayEventInfoEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub displayEvent: VkDisplayEventTypeEXT,
    }

    #[repr(C)]
    pub struct VkSwapchainCounterCreateInfoEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub surfaceCounters: VkSurfaceCounterFlagsEXT,
    }

    extension_dispatch_table!{VK_EXT_display_control | device, {
        [device] vkDisplayPowerControlEXT => (device: VkDevice, display: VkDisplayKHR, pDisplayPowerInfo: *const VkDisplayPowerInfoEXT) -> VkResult,
        [device] vkRegisterDeviceEventEXT => (device: VkDevice, pDeviceEventInfo: *const VkDeviceEventInfoEXT, pAllocator: *const VkAllocationCallbacks, pFence: *mut VkFence) -> VkResult,
        [device] vkRegisterDisplayEventEXT => (device: VkDevice, display: VkDisplayKHR, pDisplayEventInfo: *const VkDisplayEventInfoEXT, pAllocator: *const VkAllocationCallbacks, pFence: *mut VkFence) -> VkResult,
        [device] vkGetSwapchainCounterEXT => (device: VkDevice, swapchain: VkSwapchainKHR, counter: VkSurfaceCounterFlagBitsEXT, pCounterValue: *mut u64) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_GOOGLE_display_timing
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkRefreshCycleDurationGOOGLE {
        pub refreshDuration: u64,
    }

    #[repr(C)]
    pub struct VkPastPresentationTimingGOOGLE {
        pub presentID: u32,
        pub desiredPresentTime: u64,
        pub actualPresentTime: u64,
        pub earliestPresentTime: u64,
        pub presentMargin: u64,
    }

    #[repr(C)]
    pub struct VkPresentTimeGOOGLE {
        pub presentID: u32,
        pub desiredPresentTime: u64,
    }

    #[repr(C)]
    pub struct VkPresentTimesInfoGOOGLE {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub swapchainCount: u32,
        pub pTimes: *const VkPresentTimeGOOGLE,
    }

    extension_dispatch_table!{VK_GOOGLE_display_timing | device, {
        [device] vkGetRefreshCycleDurationGOOGLE => (device: VkDevice, swapchain: VkSwapchainKHR, pDisplayTimingProperties: *mut VkRefreshCycleDurationGOOGLE) -> VkResult,
        [device] vkGetPastPresentationTimingGOOGLE => (device: VkDevice, swapchain: VkSwapchainKHR, pPresentationTimingCount: *mut u32, pPresentationTimings: *mut VkPastPresentationTimingGOOGLE) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_sample_mask_override_coverage
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_NV_sample_mask_override_coverage | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_geometry_shader_passthrough
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_NV_geometry_shader_passthrough | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_viewport_array2
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_NV_viewport_array2 | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_NVX_multiview_per_view_attributes
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkPhysicalDeviceMultiviewPerViewAttributesPropertiesNVX {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub perViewPositionAllComponents: VkBool32,
    }

    extension_dispatch_table!{VK_NVX_multiview_per_view_attributes | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_NV_viewport_swizzle
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub enum VkViewportCoordinateSwizzleNV {
        VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_X_NV = 0,
        VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_X_NV = 1,
        VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Y_NV = 2,
        VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Y_NV = 3,
        VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_Z_NV = 4,
        VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_Z_NV = 5,
        VK_VIEWPORT_COORDINATE_SWIZZLE_POSITIVE_W_NV = 6,
        VK_VIEWPORT_COORDINATE_SWIZZLE_NEGATIVE_W_NV = 7,
    }

    vulkan_flags!(VkPipelineViewportSwizzleStateCreateFlagsNV);

    #[repr(C)]
    pub struct VkViewportSwizzleNV {
        pub x: VkViewportCoordinateSwizzleNV,
        pub y: VkViewportCoordinateSwizzleNV,
        pub z: VkViewportCoordinateSwizzleNV,
        pub w: VkViewportCoordinateSwizzleNV,
    }

    #[repr(C)]
    pub struct VkPipelineViewportSwizzleStateCreateInfoNV {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineViewportSwizzleStateCreateFlagsNV,
        pub viewportCount: u32,
        pub pViewportSwizzles: *const VkViewportSwizzleNV,
    }

    extension_dispatch_table!{VK_NV_viewport_swizzle | device, {
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_discard_rectangles
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub enum VkDiscardRectangleModeEXT {
        VK_DISCARD_RECTANGLE_MODE_INCLUSIVE_EXT = 0,
        VK_DISCARD_RECTANGLE_MODE_EXCLUSIVE_EXT = 1,
    }

    vulkan_flags!(VkPipelineDiscardRectangleStateCreateFlagsEXT);

    #[repr(C)]
    pub struct VkPhysicalDeviceDiscardRectanglePropertiesEXT {
        pub sType: VkStructureType,
        pub pNext: *mut c_void,
        pub maxDiscardRectangles: u32,
    }

    #[repr(C)]
    pub struct VkPipelineDiscardRectangleStateCreateInfoEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkPipelineDiscardRectangleStateCreateFlagsEXT,
        pub discardRectangleMode: VkDiscardRectangleModeEXT,
        pub discardRectangleCount: u32,
        pub pDiscardRectangles: *const VkRect2D,
    }

    extension_dispatch_table!{VK_EXT_discard_rectangles | device, {
        [device] vkCmdSetDiscardRectangleEXT => (commandBuffer: VkCommandBuffer, firstDiscardRectangle: u32, discardRectangleCount: u32, pDiscardRectangles: *const VkRect2D) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_swapchain_colorspace
     * ------------------------------------------------------
    */

    extension_dispatch_table!{VK_EXT_swapchain_colorspace | instance, {
    }}

    /*
     * ------------------------------------------------------
     * VK_EXT_hdr_metadata
     * ------------------------------------------------------
    */

    #[repr(C)]
    pub struct VkXYColorEXT {
        pub x: f32,
        pub y: f32,
    }

    #[repr(C)]
    pub struct VkHdrMetadataEXT {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub displayPrimaryRed: VkXYColorEXT,
        pub displayPrimaryGreen: VkXYColorEXT,
        pub displayPrimaryBlue: VkXYColorEXT,
        pub whitePoint: VkXYColorEXT,
        pub maxLuminance: f32,
        pub minLuminance: f32,
        pub maxContentLightLevel: f32,
        pub maxFrameAverageLightLevel: f32,
    }

    extension_dispatch_table!{VK_EXT_hdr_metadata | device, {
        [device] vkSetHdrMetadataEXT => (device: VkDevice, swapchainCount: u32, pSwapchains: *const VkSwapchainKHR, pMetadata: *const VkHdrMetadataEXT) -> (),
    }}

    /*
     * ------------------------------------------------------
     * VK_MVK_ios_surface
     * ------------------------------------------------------
    */

    vulkan_flags!(VkIOSSurfaceCreateFlagsMVK);

    #[repr(C)]
    pub struct VkIOSSurfaceCreateInfoMVK {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkIOSSurfaceCreateFlagsMVK,
        pub pView: *const c_void,
    }

    extension_dispatch_table!{VK_MVK_ios_surface | instance, {
        [instance] vkCreateIOSSurfaceMVK => (instance: VkInstance, pCreateInfo: *const VkIOSSurfaceCreateInfoMVK, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
    }}

    /*
     * ------------------------------------------------------
     * VK_MVK_macos_surface
     * ------------------------------------------------------
    */

    vulkan_flags!(VkMacOSSurfaceCreateFlagsMVK);

    #[repr(C)]
    pub struct VkMacOSSurfaceCreateInfoMVK {
        pub sType: VkStructureType,
        pub pNext: *const c_void,
        pub flags: VkMacOSSurfaceCreateFlagsMVK,
        pub pView: *const c_void,
    }

    extension_dispatch_table!{VK_MVK_macos_surface | instance, {
        [instance] vkCreateMacOSSurfaceMVK => (instance: VkInstance, pCreateInfo: *const VkMacOSSurfaceCreateInfoMVK, pAllocator: *const VkAllocationCallbacks, pSurface: *mut VkSurfaceKHR) -> VkResult,
    }}
} // mod extensions
