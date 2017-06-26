#ifndef VKSPEC_INCLUDE
#define VKSPEC_INCLUDE

// What I have done:
// The original code generates modified bindings that leverage C++ features.
// For example it alters how error handling is done, checks what parameters to
// commands represent out-variables, and generates wrapper classes over the raw
// Vulkan functions. The intention of this generator is not to make safe wrapper
// code but rather just keep raw Rust bindings to the Vulkan API. Functions
// keep their original parameters and return values as normal. I do however
// generate dispatch tables that make sure all functions are accounted for when
// loading pointers. I also generate special bitflag structs to get type safety
// for bitflags, preventing accidentally using a flag from another type. It's
// easy enough to implement without chaning the API. Note that no effort is put
// in making sure that types taking FlagBits only accept one of the variants.
// Just as with C any combination still works, and the programmer must make sure
// to use Vulkan correctly. Other than that the API usage is the same. Thus
// large portions have been removed and altered. Type management has been
// completely reimplemented to ease translating to Rust equivalents. The Vulkan
// registry contains inline C code which I can't use directly, so I needed
// something more robust.

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <tinyxml2.h>

namespace vkspec {

struct ExtensionItem {
	bool disabled = false;
	bool extension = false;
};

struct ScalarTypedef {
	std::string alias;
	std::string actual;
};

struct FunctionTypedef {
	struct Parameter {
		std::string type;
		std::string name;
	};

	std::string alias;
	std::string return_type;
	std::vector<Parameter> params;
};

struct Bitmasks : public ExtensionItem {
	struct Member {
		std::string name;
		std::string value;
	};

	std::string name;
	std::vector<Member> members;
};

struct BitmaskTypedef : public ExtensionItem {
	std::string alias;
	std::string bit_definitions; // The actual bitmask containing values
};

struct HandleTypedef : public ExtensionItem {
	std::string alias;
	std::string actual;
};

struct Struct : public ExtensionItem {
	struct Member {
		std::string type;
		std::string name;
	};

	std::string name;
	std::vector<Member> members;
	bool is_union;
};

struct Enum : public ExtensionItem {
	struct Member {
		std::string name;
		std::string value;
	};

	std::string name;
	std::vector<Member> members;
};

struct Command : public ExtensionItem {
	struct Parameter {
		std::string type;
		std::string name;
	};

	std::string return_type;
	std::string name;
	std::vector<Parameter> params;
};

struct Extension {
	std::string name;
	std::string number;
	std::string tag;
	std::string type; // instance or device if not empty string
	std::vector<std::string> commands;
	std::vector<std::string> types;
	bool disabled = false;
};

enum class PointerType {
	T_P,
	T_PP,
	T_P_CONST_P,
	CONST_T_P,
	CONST_T_PP,
	CONST_T_P_CONST_P,
};

class ITranslator {
public:
	virtual std::string pointer_to(std::string const& type_name, PointerType pointer_type) = 0;
	virtual std::string array_member(std::string const& type_name, std::string const& array_size) = 0;
	virtual std::string array_param(std::string const& type_name, std::string const& array_size, bool const_modifier) = 0;
};

class Registry {
public:
	Registry(ITranslator* translator) : _translator(translator) {}

	// I'm working under the assumption that the C and OS types used will
	// be a comparatively small set so that I can deal with those manually.
	// This way I can assume that types not existing at the time I need them
	// are Vulkan types that don't need a specific translation, thus avoiding
	// the need to analyze them at runtime. Where I expect to read a C type,
	// this map will be checked, resulting in a runtime error if a type is not
	// present. Thus the user will have to provide translations for C types
	// before parsing the spec.
	void add_c_type(std::string const& c, std::string const& translation);
	void parse(std::string const& spec);

	std::string const& license(void) const {
		return _license_header;
	}

	std::string const& version(void) const {
		return _version;
	}

	std::vector<std::tuple<std::string, std::string, std::string>> const& get_api_constants(void) const {
		return _api_constants;
	}

	std::vector<ScalarTypedef*> const& get_scalar_typedefs(void) const {
		return _scalar_typedefs;
	}

	std::vector<FunctionTypedef*> const& get_function_typedefs(void) const {
		return _function_typedefs;
	}

	std::vector<Bitmasks*> const& get_bitmasks(void) const {
		return _bitmasks;
	}

	std::vector<BitmaskTypedef*> const& get_bitmask_typedefs(void) const {
		return _bitmask_typedefs;
	}

	std::vector<HandleTypedef*> const& get_handle_typedefs(void) const {
		return _handle_typedefs;
	}

	std::vector<Struct*> const& get_structs(void) const {
		return _structs;
	}

	std::vector<Enum*> const& get_enums(void) const {
		return _enums;
	}

	std::vector<Command*> const& get_commands(void) const {
		return _commands;
	}

	std::map<std::string, Extension> const& get_extensions(void) const {
		return _extensions;
	}

private:
	enum class ItemType {
		CType,
		ScalarTypedef,
		FunctionTypedef,
		Bitmasks,
		BitmaskTypedef,
		HandleTypedef,
		Struct,
		Enum,
		Command,
	};

private:
	void _read_comment(tinyxml2::XMLElement * element);

	void _read_tags(tinyxml2::XMLElement * element);

	void _read_types(tinyxml2::XMLElement * element);
	void _read_type_basetype(tinyxml2::XMLElement * element);
	void _read_type_bitmask(tinyxml2::XMLElement * element);
	void _read_type_define(tinyxml2::XMLElement * element);
	void _read_type_funcpointer(tinyxml2::XMLElement * element);
	void _read_type_handle(tinyxml2::XMLElement * element);
	void _read_type_struct(tinyxml2::XMLElement * element, bool isUnion);
	// Read a member tag of a struct, adding members to the provided struct.
	void _read_type_struct_member(Struct* theStruct, tinyxml2::XMLElement * element);
	// Reads the type tag of a member tag, including potential text nodes around
	// the type tag to get qualifiers. We pass the first node that could potentially
	// be a text node.
	tinyxml2::XMLNode* _read_type_struct_member_type(tinyxml2::XMLNode* element, std::string& type);

	void _read_enums(tinyxml2::XMLElement * element);
	void _read_api_constants(tinyxml2::XMLElement* element);
	void _read_enums_bitmask(tinyxml2::XMLElement * element, std::function<void(const std::string& member, const std::string& value, bool isBitpos)> make);
	void _read_enums_enum(tinyxml2::XMLElement * element, std::function<void(const std::string& member, const std::string& value)> make);

	void _read_commands(tinyxml2::XMLElement * element);
	void _read_commands_command(tinyxml2::XMLElement * element);
	Command* _read_command_proto(tinyxml2::XMLElement * element);
	void _read_command_params(tinyxml2::XMLElement* element, Command* cmd);
	void _read_command_param(tinyxml2::XMLElement * element, Command* cmd);
	tinyxml2::XMLNode* _read_command_param_type(tinyxml2::XMLNode* node, Command const* cmd, std::string& type, bool& const_modifier);

	std::string _read_array_size(tinyxml2::XMLNode * node, std::string& name);
	std::string _trim_end(std::string const& input);
	std::string _extract_tag(std::string const& name);

	void _read_extensions(tinyxml2::XMLElement * element);
	void _read_extensions_extension(tinyxml2::XMLElement * element);
	void _read_disabled_extension_require(tinyxml2::XMLElement * element, Extension& ext);
	// Defines what types, enumerants, and commands are used by an extension
	void _read_extension_require(tinyxml2::XMLElement * element, std::string const& tag, Extension& ext);
	void _read_extension_command(tinyxml2::XMLElement * element, std::vector<std::string>& extensionCommands);
	void _read_extension_type(tinyxml2::XMLElement * element, Extension& ext);
	void _read_extension_enum(tinyxml2::XMLElement * element, std::string const& extensionNumber);

	ScalarTypedef* _define_scalar_typedef(std::string const& alias, std::string const& actual);
	FunctionTypedef* _define_function_typedef(std::string const& alias, std::string const& return_type);
	Bitmasks* _define_bitmasks(std::string const& name);
	BitmaskTypedef* _define_bitmask_typedef(std::string const& alias, std::string const& bit_definitions);
	HandleTypedef* _define_handle_typedef(std::string const& alias, std::string const& actual);
	Struct* _define_struct(std::string const& name, bool is_union);
	Enum* _define_enum(std::string const& name);
	Command* _define_command(std::string const& name, std::string const& return_type);
	void _define_extension(Extension const&& ext);

	std::string const& _type_reference(const std::string& type);

	void _define(std::string const& name, ItemType item_type);

	void _undefined_check();

	void _mark_extension_items();

	std::string _bitpos_to_value(std::string const& bitpos);

private:
	/*
	The base types themselves are only stored once and then their pointers are
	reused. This allows declaring Vulkan types before they are defined by
	returning a type that is VulkanUndefined for the time being but later
	updated when parsing its definition. Another benefit of this is that we can
	define the limited set of C types manually up front instead of trying to
	deduce it from the spec. They need some form of translation into Rust,
	which is why we want to know which types are C and which ones are Vulkan.
	Thus, if a type does not already exist, we assume that it's Vulkan. If it
	turns out that it's not, then it will remain as undefined because the spec
	never tells us what it is, thus indicating that it was C all the time.
	*/

	ITranslator* _translator;
	std::string _version;
	std::string _license_header;
	std::vector<std::tuple<std::string, std::string, std::string>> _api_constants;
	std::set<std::string> _tags;
	std::map<std::string, ItemType> _defined_types; // All types defined in the registry
	std::set<std::string> _undefined_types; // Types referenced, but currently not defined. Should be empty after parsing
	std::map<std::string, std::string> _c_types;
	std::vector<ScalarTypedef*> _scalar_typedefs;
	std::vector<FunctionTypedef*> _function_typedefs;
	std::vector<Bitmasks*> _bitmasks;
	std::vector<BitmaskTypedef*> _bitmask_typedefs;
	std::vector<HandleTypedef*> _handle_typedefs;
	std::vector<Struct*> _structs;
	std::vector<Enum*> _enums;
	std::vector<Command*> _commands;
	std::map<std::string, Extension> _extensions;
};

} // vkspec

#endif // VKSPEC_INCLUDE