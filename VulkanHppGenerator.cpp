// Copyright(c) 2015-2016, NVIDIA CORPORATION. All rights reserved.
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

#include <cassert>
#include <fstream>
//#include <iterator>
#include <iostream>
//#include <list>
//#include <exception>

#include "vkspec.h"

class IndentingOStreambuf : public std::streambuf
{
	std::streambuf*     myDest;
	bool                myIsAtStartOfLine;
	//std::string         myIndent;
	int                 myIndent;
	std::ostream*       myOwner;
protected:
	virtual int         overflow(int ch)
	{
		if (myIsAtStartOfLine && ch != '\n') {
			//myDest->sputn(myIndent.data(), myIndent.size());
			for (int i = 0; i < myIndent; ++i) {
				myDest->sputc(' ');
			}
		}
		myIsAtStartOfLine = ch == '\n';
		return myDest->sputc(ch);
	}
public:
	explicit            IndentingOStreambuf(
		std::streambuf* dest, int indent = 4)
		: myDest(dest)
		, myIsAtStartOfLine(true)
		//, myIndent(indent, ' ')
		, myIndent(indent)
		, myOwner(NULL)
	{
	}
	explicit            IndentingOStreambuf(
		std::ostream& dest, int indent = 4)
		: myDest(dest.rdbuf())
		, myIsAtStartOfLine(true)
		//, myIndent(indent, ' ')
		, myIndent(indent)
		, myOwner(&dest)
	{
		myOwner->rdbuf(this);
	}
	virtual             ~IndentingOStreambuf()
	{
		if (myOwner != NULL) {
			myOwner->rdbuf(myDest);
		}
	}
	void increase(int indent = 4) {
		myIndent += indent;
	}
	void decrease(int indent = 4) {
		myIndent -= indent;
		if (myIndent < 0) {
			myIndent = 0;
		}
	}
} *indent;

const std::string macro_use = R"(pub use ::std::ffi::CString;
pub use ::std::ops::{BitOr, BitAnd};
pub use ::std::{fmt, mem};)";

const std::string function_macro = R"(
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
})";

const std::string use_statements = R"(use super::macros::*;
extern crate libloading;
use ::std::{ptr};
pub use ::std::os::raw::{c_void, c_char, c_int};)";

const std::string flags_macro_comment = R"(/*
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
*/)";

const std::string flags_macro = R"(
macro_rules! vulkan_flags {
    ($type_name:ident, { $($flag:ident = $flag_val:expr,)* }) => (
        #[repr(C)]
        #[derive(Debug, Copy, Clone, PartialEq)]
        pub struct $type_name {
            flags: VkFlags,
        }

        impl $type_name {
            #[allow(dead_code)] // Don't know why this one warns... it's public
            pub fn none() -> $type_name {
                $type_name { flags: 0 }
            }

            $(
                pub fn $flag() -> $type_name {
                    $type_name { flags: $flag_val }
                }
            )*
        }

        impl BitOr for $type_name {
            type Output = $type_name;

            fn bitor(self, rhs: $type_name) -> $type_name {
                $type_name { flags: self.flags | rhs.flags }
            }
        }

        impl BitAnd for $type_name {
            type Output = Self;

            fn bitand(self, rhs: Self) -> Self {
                $type_name { flags: self.flags & rhs.flags }
            }
        }

        impl fmt::Display for $type_name {
            fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
                write!(f, concat!(
                    stringify!($type_name), " {{\n",
                        $(
                            "    [{}] ", stringify!($flag), "\n",
                        )* "}}"
                    )
                    //$(, if *self & $type_name::$flag() == $type_name::$flag() { if self.flags != 0 && $type_name::$flag().flags == 0 { " " } else { "x" } } else { " " } )*
                    $(, if *self & $type_name::$flag() == $type_name::$flag() { "x" } else { " " } )*
                )
            }
        }
    );
})";

const std::string global_dispatch_table_macro = R"(
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
                            $fun: match vulkan_entry.vkGetInstanceProcAddr(ptr::null_mut(), CString::new(stringify!($fun)).unwrap().as_ptr()) {
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
})";

const std::string instance_dispatch_table_macro = R"(
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
})";

const std::string device_dispatch_table_macro = R"(
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
})";

const std::string load_function_macro = R"(
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
})";

const std::string extension_table_ctor_macro = R"(
macro_rules! table_ctor {
    (instance, $table_name:ident $(, $fun_type:ident, $fun:ident)*) => (
        pub fn new(vulkan_entry: &VulkanEntry, instance: VkInstance) -> Result<$table_name, String> {
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
        pub fn new(vulkan_entry: &VulkanEntry, instance: VkInstance, instance_table: &InstanceDispatchTable, device: VkDevice) -> Result<$table_name, String> {
            unsafe {
                Ok($table_name {
                    $(
                        $fun: load_function!($fun_type, $fun, vulkan_entry, instance, instance_table, device),
                    )*
                })
            }
        }
    );
})";

const std::string extension_dispatch_table_macro = R"(
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
})";

class RustGenerator : public vkspec::IGenerator {
public:
	RustGenerator(std::string const& out_file, std::string const& license, int major, int minor, int patch) {
		_file.open(out_file);
		if (!_file.is_open()) {
			throw std::runtime_error("Failed to open file for output");
		}

		_indent = new IndentingOStreambuf(_file, 0);

		_file << license << std::endl;
		_file << std::endl;
		_file << "// Rust bindings for Vulkan " << major << "." << minor << "." << patch << ", generated from the Khronos Vulkan API XML Registry." << std::endl;
		_file << "// See https://github.com/andens/Vulkan-Hpp for generator details." << std::endl;
		_file << std::endl;
		_file << "#![allow(non_camel_case_types)]" << std::endl;
		_file << "#![allow(non_snake_case)]" << std::endl;
		_file << std::endl;
		_write_macros();
	}

	~RustGenerator() {
		delete _indent;
		_file.close();
	}

	virtual void RustGenerator::begin_core() override final {
		_file << std::endl;
		_file << "pub mod core {" << std::endl;

		_indent->increase();

		_file << use_statements << std::endl;

		_file << std::endl;

		_file << "pub fn VK_MAKE_VERSION(major: u32, minor: u32, patch: u32) -> u32 {" << std::endl;
		_file << "    (major << 22) | (minor << 12) | patch" << std::endl;
		_file << "}" << std::endl;
	}

	virtual void RustGenerator::end_core() override final {
		_indent->decrease();
		_file << "} // mod core" << std::endl;
	}

	virtual void RustGenerator::gen_scalar_typedef(vkspec::ScalarTypedef* t) override final {
		if (_previous_type != Type::ScalarTypedef) {
			_file << std::endl;
		}

		_file << "pub type " << t->name() << " = " << t->actual_type()->name() << ";" << std::endl;
		_previous_type = Type::ScalarTypedef;
	}

	virtual void RustGenerator::gen_function_typedef(vkspec::FunctionTypedef* t) override final {
		if (_previous_type != Type::FunctionTypedef) {
			_file << std::endl;
		}

		_file << "pub type " << t->name() << " = vk_fun!((";
		if (!t->params().empty()) {
			_file << t->params()[0].name << ": " << t->params()[0].complete_type;
			for (auto it = t->params().begin() + 1; it != t->params().end(); ++it) {
				_file << ", " << it->name << ": " << it->complete_type;
			}
		}
		_file << ") -> " << t->complete_return_type() << ");" << std::endl;
		_previous_type = Type::FunctionTypedef;
	}

	virtual void RustGenerator::gen_handle_typedef(vkspec::HandleTypedef* t) override final {
		if (_previous_type != Type::HandleTypedef) {
			_file << std::endl;
		}

		_file << "pub type " << t->name() << " = " << t->actual_type()->name() << ";" << std::endl;
		_previous_type = Type::HandleTypedef;
	}

	virtual void RustGenerator::gen_struct(vkspec::Struct* t) override final {
		_file << std::endl;
		_file << "#[repr(C)]" << std::endl;
		
		if (t->is_union()) {
			_write_union(t);
		}
		else {
			_write_struct(t);
		}

		_previous_type = Type::Struct;
	}

	virtual void RustGenerator::gen_enum(vkspec::Enum* t) override final {
		_file << std::endl;
		_file << "#[repr(C)]" << std::endl;
		_file << "pub enum " << t->name() << " {" << std::endl;
		_indent->increase();
		for (auto& m : t->members()) {
			_file << m.name << " = " << m.value << "," << std::endl;
		}
		_indent->decrease();
		_file << "}" << std::endl;
		
		_previous_type = Type::Enum;
	}

	virtual void RustGenerator::gen_api_constant(vkspec::ApiConstant* t) override final {
		if (_previous_type != Type::ApiConstant) {
			_file << std::endl;
		}

		_file << "pub const " << t->name() << ": " << t->data_type()->name() << " = " << t->value() << ";" << std::endl;
		_previous_type = Type::ApiConstant;
	}

	virtual void RustGenerator::gen_bitmasks(vkspec::Bitmasks* t) override final {
		if (_previous_type != Type::Bitmasks) {
			_file << std::endl;
		}

		vkspec::Enum* flags = t->flags();
		if (!flags) {
			_file << "vulkan_flags!(" << t->name() << ", {});" << std::endl;
		}
		else {
			_file << "vulkan_flags!(" << t->name() << ", {" << std::endl;
			_indent->increase();
			for (auto& m : flags->members()) {
				_file << m.name << " = " << m.value << "," << std::endl;
			}
			_indent->decrease();
			_file << "});" << std::endl;
		}
		_previous_type = Type::Bitmasks;
	}

	virtual void RustGenerator::begin_entry() override final {

	}

	virtual void RustGenerator::gen_entry_command(vkspec::Command* c) override final {
		// There should only ever be one entry command. If not, I will need to
		// adapt the bindings accordingly.
		assert(!_entry_command);
		_entry_command = c;
	}

	virtual void RustGenerator::end_entry() override final {
		assert(_entry_command);

		_file << R"(
/*
 * ------------------------------------------------------------------------
 * Entry dispatch table. Represents the Vulkan entry point that can be used
 * to get other Vulkan functions. Holds the library handle so that it does
 * not get unloaded. This is very similar to the macros used for generating
 * global-, instance-, and dispatch tables, except explicit since the entry
 * is just a single function.
 * ------------------------------------------------------------------------
*/
)";
		_file << "type PFN_" << _entry_command->name() << " = vk_fun!((";
		if (!_entry_command->params().empty()) {
			_file << _entry_command->params()[0].name << ": " << _entry_command->params()[0].complete_type;
			for (auto it = _entry_command->params().begin() + 1; it != _entry_command->params().end(); ++it) {
				_file << ", " << it->name << ": " << it->complete_type;
			}
		}
		_file << ") -> " << _entry_command->complete_return_type() << ");" << std::endl;

		_file << "pub struct VulkanEntry {" << std::endl;
		_indent->increase();
		_file << "#[allow(dead_code)]" << std::endl;
		_file << "vulkan_lib: libloading::Library," << std::endl;
		_file << _entry_command->name() << ": PFN_" << _entry_command->name() << "," << std::endl;
		_indent->decrease();
		_file << "}" << std::endl;
		
		_file << std::endl;

		_file << "impl VulkanEntry {" << std::endl;
		_indent->increase();
		_file << "pub fn new(loader_path: &str) -> Result<VulkanEntry, String> {" << std::endl;
		_indent->increase();
		_file << R"(let lib = match libloading::Library::new(loader_path) {
    Ok(lib) => lib,
    Err(_) => return Err(String::from("Failed to open Vulkan loader")),
};)" << std::endl;

		_file << std::endl;

		_file << "let " << _entry_command->name() << ": PFN_" << _entry_command->name() << " = unsafe {" << std::endl;
		_indent->increase();
		_file << "match lib.get::<PFN_" << _entry_command->name() << ">(b\"" << _entry_command->name() << "\\0\") {" << std::endl;
		_indent->increase();
		_file << "Ok(symbol) => *symbol, // Deref Symbol, not function pointer" << std::endl;
		_file << "Err(_) => return Err(String::from(\"Could not load " << _entry_command->name() << "\"))," << std::endl;
		_indent->decrease();
		_file << "}" << std::endl;
		_indent->decrease();
		_file << "};" << std::endl;

		_file << std::endl;

		_file << R"(// Since I can't keep the library and the loaded function in the
// same struct (Rust would then not be able to drop it because of
// the symbol references into itself via the library) I have opted
// to just storing the raw loaded function. This should be fine as
// long as the library is also saved to prevent unloading it. Since
// I return a Result, Rust makes sure that the struct can only be
// used if properly initialized.)" << std::endl;
		_file << "Ok(VulkanEntry {" << std::endl;
		_indent->increase();
		_file << "vulkan_lib: lib, // Save this so that the library is not freed" << std::endl;
		_file << _entry_command->name() << ": " << _entry_command->name() << "," << std::endl;
		_indent->decrease();
		_file << "})" << std::endl;
		_indent->decrease();
		_file << "}" << std::endl;

		_file << std::endl;

		_file << "#[inline]" << std::endl;
		_file << "pub unsafe fn " << _entry_command->name() << "(&self";
		for (auto& p : _entry_command->params()) {
			_file << ", " << p.name << ": " << p.complete_type;
		}
		_file << ") -> " << _entry_command->complete_return_type() << " {" << std::endl;
		_indent->increase();
		_file << "(self." << _entry_command->name() << ")(";
		if (!_entry_command->params().empty()) {
			_file << _entry_command->params()[0].name;
			for (auto it = _entry_command->params().begin() + 1; it != _entry_command->params().end(); ++it) {
				_file << ", " << it->name;
			}
		}
		_file << ")" << std::endl;
		_indent->decrease();
		_file << "}" << std::endl;
		_indent->decrease();
		_file << "}" << std::endl;
	}

	virtual void RustGenerator::begin_global_commands() override final {

	}

	virtual void RustGenerator::gen_global_command(vkspec::Command* c) override final {
		_global_commands.push_back(c);
	}

	virtual void RustGenerator::end_global_commands() {
		_file << std::endl;
		_file << "global_dispatch_table!{" << std::endl;
		_indent->increase();
		for (auto c : _global_commands) {
			_file << c->name() << " => (";
			if (!c->params().empty()) {
				_file << c->params()[0].name << ": " << c->params()[0].complete_type;
				for (auto it = c->params().begin() + 1; it != c->params().end(); ++it) {
					_file << ", " << it->name << ": " << it->complete_type;
				}
			}
			_file << ") -> " << c->complete_return_type() << "," << std::endl;
		}
		_indent->decrease();
		_file << "}" << std::endl;
	}

	virtual void RustGenerator::begin_instance_commands() override final {

	}

	virtual void RustGenerator::gen_instance_command(vkspec::Command* c) override final {
		_instance_commands.push_back(c);
	}

	virtual void RustGenerator::end_instance_commands() override final {
		_file << std::endl;
		_file << "instance_dispatch_table!{" << std::endl;
		_indent->increase();
		for (auto c : _instance_commands) {
			_file << c->name() << " => (";
			if (!c->params().empty()) {
				std::string name = c->params()[0].name;
				if (name == "type") { name = "type_"; }
				_file << name << ": " << c->params()[0].complete_type;
				for (auto it = c->params().begin() + 1; it != c->params().end(); ++it) {
					name = it->name;
					if (name == "type") { name = "type_"; }
					_file << ", " << name << ": " << it->complete_type;
				}
			}
			_file << ") -> " << c->complete_return_type() << "," << std::endl;
		}
		_indent->decrease();
		_file << "}" << std::endl;
	}


	virtual void RustGenerator::begin_device_commands() override final {

	}

	virtual void RustGenerator::gen_device_command(vkspec::Command* c) override final {
		_device_commands.push_back(c);
	}

	virtual void RustGenerator::end_device_commands() override final {
		_file << std::endl;
		_file << "device_dispatch_table!{" << std::endl;
		_indent->increase();
		for (auto c : _device_commands) {
			_file << c->name() << " => (";
			if (!c->params().empty()) {
				_file << c->params()[0].name << ": " << c->params()[0].complete_type;
				for (auto it = c->params().begin() + 1; it != c->params().end(); ++it) {
					_file << ", " << it->name << ": " << it->complete_type;
				}
			}
			_file << ") -> " << c->complete_return_type() << "," << std::endl;
		}
		_indent->decrease();
		_file << "}" << std::endl;
	}

	virtual void RustGenerator::begin_extensions() override final {
		_file << std::endl;
		_file << "pub mod extensions {" << std::endl;
		_indent->increase();
		_file << "use super::macros::*;" << std::endl;
		_file << "use super::core::*;" << std::endl;
	}

	virtual void RustGenerator::end_extensions() override final {
		_indent->decrease();
		_file << "} // mod extensions" << std::endl;
	}

	virtual void RustGenerator::begin_extension(vkspec::Extension* e) override final {
		_file << std::endl;
		_file << "/*" << std::endl;
		_file << " * ------------------------------------------------------" << std::endl;
		_file << " * " << e->name() << std::endl;
		_file << " * ------------------------------------------------------" << std::endl;
		_file << "*/" << std::endl;
	}

	virtual void RustGenerator::end_extension(vkspec::Extension* e) override final {
		_file << std::endl;

		std::string type;
		switch (e->classification()) {
			case vkspec::ExtensionClassification::Instance:
				type = "instance"; break;
			case vkspec::ExtensionClassification::Device:
				type = "device"; break;
			default:
				throw std::runtime_error("This generator do not support extensions that are neither instance nor device kinds.");
		}

		_file << "extension_dispatch_table!{" << e->name() << " | " << type << ", {" << std::endl;
		_indent->increase();
		for (auto c : e->commands()) {
			_file << "[" << ((c->classification() == vkspec::CommandClassification::Instance) ? "instance" : "device") << "] ";
			_file << c->name() << " => (";
			if (!c->params().empty()) {
				std::string name = c->params()[0].name;
				if (name == "type") { name = "type_"; }
				_file << name << ": " << c->params()[0].complete_type;
				for (auto it = c->params().begin() + 1; it != c->params().end(); ++it) {
					name = it->name;
					if (name == "type") { name = "type_"; }
					_file << ", " << name << ": " << it->complete_type;
				}
			}
			_file << ") -> " << c->complete_return_type() << "," << std::endl;
		}
		_indent->decrease();
		_file << "}}" << std::endl;
	}

private:
	enum class Type {
		Unknown,
		ScalarTypedef,
		FunctionTypedef,
		HandleTypedef,
		Struct,
		Enum,
		ApiConstant,
		Bitmasks,
	};

private:
	void _write_macros() {
		_file << "#[macro_use]" << std::endl;
		_file << "mod macros {" << std::endl;

		_indent->increase();

		_file << macro_use << std::endl;
		_file << std::endl;
		_file << flags_macro_comment;
		_file << flags_macro << std::endl;
		_file << function_macro << std::endl;
		_file << global_dispatch_table_macro << std::endl;
		_file << instance_dispatch_table_macro << std::endl;
		_file << device_dispatch_table_macro << std::endl;
		_file << load_function_macro << std::endl;
		_file << extension_table_ctor_macro << std::endl;
		_file << extension_dispatch_table_macro << std::endl;

		_indent->decrease();

		_file << "} // mod macros" << std::endl;
	}

	void _write_union(vkspec::Struct* t) {
		_file << "pub struct " << t->name() << " {" << std::endl;
		_indent->increase();

		// Since people involved in development of Rust prefer to discuss for
		// years instead of actually just doing things, Rust still don't have
		// proper FFI support for unions. That's why we deal with these manually.
		if (t->name() == "VkClearColorValue") {
			_file << "data: [u32; 4]," << std::endl;
		}
		else {
			assert(t->name() == "VkClearValue");
			_file << "data: VkClearColorValue," << std::endl;
		}

		_indent->decrease();
		_file << "}" << std::endl;

		_file << "impl " << t->name() << " {" << std::endl;
		_indent->increase();

		for (auto& m : t->members()) {
			_file << "#[inline]" << std::endl;
			_file << "pub unsafe fn " << m.name << "(&self) -> &" << m.complete_type << " {" << std::endl;
			_indent->increase();
			//_file << "let p = self as *const _ as *const " << m.complete_type << ";" << std::endl;
			//_file << "&*p" << std::endl;
			_file << "::std::mem::transmute(&self.data)" << std::endl;
			_indent->decrease();
			_file << "}" << std::endl;

			_file << "#[inline]" << std::endl;
			_file << "pub unsafe fn " << m.name << "_mut(&mut self) -> &mut " << m.complete_type << " {" << std::endl;
			_indent->increase();
			//_file << "let p = self as *mut _ as *mut " << m.complete_type << ";" << std::endl;
			//_file << "&mut *p" << std::endl;
			_file << "::std::mem::transmute(&mut self.data)" << std::endl;
			_indent->decrease();
			_file << "}" << std::endl;
		}

		_indent->decrease();
		_file << "}" << std::endl;
	}

	void _write_struct(vkspec::Struct* t) {
		_file << "pub struct " << t->name() << " {" << std::endl;
		_indent->increase();

		for (auto& m : t->members()) {
			std::string field = m.name;
			if (field == "type") { field.push_back('_'); }
			_file << "pub " << field << ": " << m.complete_type << "," << std::endl;
		}

		_indent->decrease();
		_file << "}" << std::endl;
	}

private:
	std::ofstream _file;
	IndentingOStreambuf* _indent = nullptr;
	Type _previous_type = Type::Unknown;
	vkspec::Command* _entry_command = nullptr;
	std::vector<vkspec::Command*> _global_commands;
	std::vector<vkspec::Command*> _instance_commands;
	std::vector<vkspec::Command*> _device_commands;
};

class RustTranslator : public vkspec::ITranslator {
	virtual std::string pointer_to(std::string const& type, vkspec::PointerType pointer_type) override final {
		// void as in no parameters or return value is not the same as void
		// pointer and have different syntax in Rust
		std::string t = type;
		if (t == "()") {
			t = "c_void";
		}

		switch (pointer_type) {
		case vkspec::PointerType::CONST_T_P:
			return "*const " + t;
		case vkspec::PointerType::CONST_T_PP:
			return "*mut *const " + t;
		case vkspec::PointerType::CONST_T_P_CONST_P:
			return "*const *const " + t;
		case vkspec::PointerType::T_P:
			return "*mut " + t;
		case vkspec::PointerType::T_PP:
			return "*mut *mut " + t;
		case vkspec::PointerType::T_P_CONST_P:
			return "*const *mut " + t;
		default:
			assert(false);
			return "";
		}
	}

	virtual std::string array_member(std::string const& type_name, std::string const& array_size) override final {
		return "[" + type_name + "; " + array_size + " as usize]";
	}

	virtual std::string array_param(std::string const& type_name, std::string const& array_size, bool const_modifier) override final {
		return "&" + std::string(!const_modifier ? "mut " : "") + "[" + type_name + "; " + array_size + "]";
	}

	virtual std::string bitwise_not(std::string const& value) {
		return "!" + value;
	}
};

int main(int argc, char **argv)
{
	try {
		std::string filename = (argc == 1) ? VK_SPEC : argv[1];

		RustTranslator translator;
		vkspec::Registry reg(&translator);

		reg.add_c_type("void", "()");
		reg.add_c_type("char", "c_char");
		reg.add_c_type("float", "f32");
		reg.add_c_type("uint8_t", "u8");
		reg.add_c_type("uint32_t", "u32");
		reg.add_c_type("uint64_t", "u64");
		reg.add_c_type("int32_t", "i32");
		reg.add_c_type("size_t", "usize"); // unsigned according to reference
		reg.add_c_type("int", "c_int");
		reg.add_c_type("Display", "Display");
		reg.add_c_type("VisualID", "VisualID");
		reg.add_c_type("Window", "Window");
		reg.add_c_type("RROutput", "RROutput");
		reg.add_c_type("ANativeWindow", "ANativeWindow");
		reg.add_c_type("MirConnection", "MirConnection");
		reg.add_c_type("MirSurface", "MirSurface");
		reg.add_c_type("wl_display", "wl_display");
		reg.add_c_type("wl_surface", "wl_surface");
		reg.add_c_type("HINSTANCE", "HINSTANCE");
		reg.add_c_type("HWND", "HWND");
		reg.add_c_type("HANDLE", "HANDLE");
		reg.add_c_type("SECURITY_ATTRIBUTES", "SECURITY_ATTRIBUTES");
		reg.add_c_type("DWORD", "DWORD");
		reg.add_c_type("LPCWSTR", "LPCWSTR");
		reg.add_c_type("xcb_connection_t", "xcb_connection_t");
		reg.add_c_type("xcb_visualid_t", "xcb_visualid_t");
		reg.add_c_type("xcb_window_t", "xcb_window_t");

		reg.parse(filename);
		vkspec::Feature* feature = reg.build_feature("vulkan");

		std::cout << "Writing vulkan.rs to " << VULKAN_HPP << std::endl;

		{
			RustGenerator generator(VULKAN_HPP, reg.license(), feature->major(), feature->minor(), feature->patch());
			feature->generate(&generator);
		}
	}
	catch (std::exception const& e)
	{
		std::cout << "caught exception: " << e.what() << std::endl;
		return -1;
	}
	catch (...)
	{
		std::cout << "caught unknown exception" << std::endl;
		return -1;
	}
}
