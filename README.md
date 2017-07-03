# Vulkan-Hpp: Vulkan registry parser with complementary Rust binding generator
This code base started off as a fork of a C++ binding generator for Vulkan to get an idea of how parsing the registry could be done. While the original project modifies how the API is used and leverage C++ features to add for example exceptions, the idea of this fork was to generate more or less raw Rust bindings. Thus, mostly code concerning parsing the XML registry was kept, and what remained soon transformed into something completely different. Now the registry parser is separate from the generator, and it should be fairly easy to change the generator as needed.

# Getting started
First the XML registry is parsed using ```vkspec::Registry```. Before parsing begins, translations for C types must be provided. When the registry has been parsed, a desired ```vkspec::Feature``` is built which is used to generate bindings via a user-provided generator. Both ```vkspec::Registry``` and ```vkspec::Feature``` should be independent of the generator used; they act as "oracles", providing information about the API.

# vkspec::Registry
This is where the actual parsing lives. ```vkspec::Registry``` is responsible for collecting API information such as types, commands, extensions, and their dependencies. That is mostly what is done today; ```vkspec::Registry``` could be extended in the future to synthesize as much useful information as possible, for example success/error return values of commands or optional parameters if one wants to generate more elaborate bindings similar to the forked project. A translator is provided by the user so that the parser knows how to treat pointers and arrays.

# vkspec::Feature
Contains the subset of the API that bindings will be generated for. Types are sorted according to when they were first used in the API, respecting dependencies and grouped by type for cleaner output. Bindings can be generated from ```vkspec::Feature``` by passing an implementation of ```vkspec::IGenerator``` that contains the details of how the bindings will be generated.

# RustGenerator
The provided Rust generator outputs mostly raw bindings (although one could of course generate higher-level bindings if need be), using the type system for some free additional type safety regarding enums and bitmasks. Other than that, in this particular generator there is no intention of making a safe API; correct Vulkan usage is still expected from the user. Two modules ```core``` and ```extensions``` are used for various parts of the API, with a third one called ```macros``` which is contains the macros used to generate function pointers, dispatch tables, and bitmask types. Function pointers are collected in dispatch tables, and code is generated to make sure all commands have been properly loaded before successfully returning the table. Due to extensions being optional, they each have their own dispatch table for commands added by them, allowing loading extensions individually while still making sure all commands are loaded correctly.

Using the bindings would look something like this:
```rust
let vulkan_entry = vulkan::core::VulkanEntry::new("vulkan-1.dll")?;
let vkg = vulkan::core::GlobalDispatchTable::new(&vulkan_entry)?;

let mut property_count = 0;
unsafe {
    vkg.vkEnumerateInstanceLayerProperties(&mut property_count, std::ptr::null_mut());
}

// Do stuff, including creating instance

let vki = vulkan::core::InstanceDispatchTable::new(&vulkan_entry, instance)?;

// Call instance functions on vki. Create device and get its dispatch table next.
```

For now, I have only tested enumerating instance layers which seems to work. One thing to keep in mind is that flags (as in *Flags, not *FlagBits) is a struct wrapping an integer, so I don't know right now whether it works when passed to functions or not.
