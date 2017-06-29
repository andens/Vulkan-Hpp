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

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <tinyxml2.h>

namespace vkspec {

enum class Association {
	Instance,
	Device,
	Disabled,
	Unspecified,
};

enum class ApiPart {
	Core,
	Extension,
	Unspecified,
};

enum class SortOrder {
	CType = 0,
	ScalarTypedef,
	HandleTypedef,
	ApiConstant,
	Enum,
	Bitmasks,
	FunctionTypedef,
	Struct,
};

class Extension;
class Item {
public:
	// Returns the type name, which is translated for C types. When parsing the
	// registry, the parser should access _name for the type used in the spec.
	virtual std::string const& name(void) const { return _name; }

protected:
	Item(std::string const& name, tinyxml2::XMLElement* xml_node) : _name(name), _xml_node(xml_node) {}

protected:
	std::string _name;
	Extension* _extension = nullptr; // Owning extension, if any
	ApiPart _api_part = ApiPart::Unspecified;
	tinyxml2::XMLElement* _xml_node;

private:
	Item(Item const&) = delete;
	void operator=(Item const&) = delete;
};

class Type : public Item {
	friend class Registry;
	friend class CType;
	friend class ScalarTypedef;
	friend class FunctionTypedef;
	friend class HandleTypedef;
	friend class Struct;
	friend class Enum;
	friend class ApiConstant;
	friend class Bitmasks;
	friend class Feature;

protected:
	Type(std::string const& name, tinyxml2::XMLElement* type_element) : Item(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) = 0;
	virtual bool _all_dependencies_in_set(std::set<std::string> const& set) const = 0;
	virtual uint32_t _sort_order() const = 0;

protected:
	int _dependency_order = 0; // Used when sorting
};

class CType : public Type {
	friend class Registry;

public:
	virtual std::string const& name(void) const { return _translation; }

private:
	CType(std::string const& c, std::string const& translation) : Type(c, nullptr), _translation(translation) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final { chain.push_back(this); }
	virtual bool _all_dependencies_in_set(std::set<std::string> const& set) const {
		return true; // No dependencies; always true
	}
	virtual uint32_t _sort_order() const override final {
		return static_cast<uint32_t>(SortOrder::CType);
	}

private:
	std::string _translation;
};

class ScalarTypedef : public Type {
	friend class Registry;

private:
	ScalarTypedef(std::string const& name, tinyxml2::XMLElement* type_element) : Type(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_actual_type->_build_dependency_chain(chain);
		chain.push_back(this);
	}
	virtual bool _all_dependencies_in_set(std::set<std::string> const& set) const {
		return set.find(_actual_type->_name) != set.end();
	}
	virtual uint32_t _sort_order() const override final {
		return static_cast<uint32_t>(SortOrder::ScalarTypedef);
	}

private:
	Type* _actual_type = nullptr;
};

class FunctionTypedef : public Type {
	friend class Registry;

public:
	struct Parameter {
		std::string complete_type;
		Type* pure_type;
		std::string name;
	};

private:
	FunctionTypedef(std::string const& name, tinyxml2::XMLElement* type_element) : Type(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_return_type_pure->_build_dependency_chain(chain);
		for (auto& p : _params) {
			p.pure_type->_build_dependency_chain(chain);
		}
		chain.push_back(this);
	}
	virtual bool _all_dependencies_in_set(std::set<std::string> const& set) const {
		if (set.find(_return_type_pure->_name) == set.end()) {
			return false;
		}

		for (auto& p : _params) {
			if (set.find(p.pure_type->_name) == set.end()) {
				return false;
			}
		}

		return true;
	}
	virtual uint32_t _sort_order() const override final {
		return static_cast<uint32_t>(SortOrder::FunctionTypedef);
	}

private:
	std::string _return_type_complete;
	Type* _return_type_pure = nullptr;
	std::vector<Parameter> _params;
};

class HandleTypedef : public Type {
	friend class Registry;

private:
	HandleTypedef(std::string const& name, tinyxml2::XMLElement* type_element) : Type(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_actual_type->_build_dependency_chain(chain);
		chain.push_back(this);
	}
	virtual bool _all_dependencies_in_set(std::set<std::string> const& set) const {
		return set.find(_actual_type->_name) != set.end();
	}
	virtual uint32_t _sort_order() const override final {
		return static_cast<uint32_t>(SortOrder::HandleTypedef);
	}

private:
	Type* _actual_type = nullptr;
};

class Struct : public Type {
	friend class Registry;

public:
	struct Member {
		std::string complete_type;
		Type* pure_type;
		std::string name;
	};

private:
	Struct(std::string const& name, tinyxml2::XMLElement* type_element, bool is_union) : Type(name, type_element), _is_union(is_union) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		for (auto& m : _members) {
			m.pure_type->_build_dependency_chain(chain);
		}
		chain.push_back(this);
	}
	virtual bool _all_dependencies_in_set(std::set<std::string> const& set) const {
		for (auto& m : _members) {
			if (set.find(m.pure_type->_name) == set.end()) {
				return false;
			}
		}

		return true;
	}
	virtual uint32_t _sort_order() const override final {
		return static_cast<uint32_t>(SortOrder::Struct);
	}

private:
	std::vector<Member> _members;
	bool _is_union = false;
};

class Enum : public Type {
	friend class Registry;

public:
	struct Member {
		std::string name;
		std::string value;
	};

private:
	Enum(std::string const& name, tinyxml2::XMLElement* type_element, bool bitmask) : Type(name, type_element), _bitmask(bitmask) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		chain.push_back(this);
	}
	virtual bool _all_dependencies_in_set(std::set<std::string> const& set) const {
		return true; // No dependencies; always true
	}
	virtual uint32_t _sort_order() const override final {
		return static_cast<uint32_t>(SortOrder::Enum);
	}

private:
	std::vector<Member> _members;
	bool _bitmask;
};

class Bitmasks : public Type {
	friend class Registry;

private:
	Bitmasks(std::string const& name, tinyxml2::XMLElement* type_element) : Type(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_actual_type->_build_dependency_chain(chain);
		if (_flags) { // Can be nullptr if the bitmasks have no flag definitions
			((Type*)_flags)->_build_dependency_chain(chain);
		}
		chain.push_back(this);
	}
	virtual bool _all_dependencies_in_set(std::set<std::string> const& set) const {
		if (set.find(_actual_type->_name) == set.end()) {
			return false;
		}

		if (_flags) {
			return set.find(_flags->_name) != set.end();
		}

		return true;
	}
	virtual uint32_t _sort_order() const override final {
		return static_cast<uint32_t>(SortOrder::Bitmasks);
	}

private:
	Type* _actual_type = nullptr; // Should probably always be VkFlags. Not really used, but here for dependency purposes
	Enum* _flags = nullptr; // Enum containing flag definitions
};

class ApiConstant : public Type {
	friend class Registry;

private:
	ApiConstant(std::string const& name, tinyxml2::XMLElement* enum_element) : Type(name, enum_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_data_type->_build_dependency_chain(chain);
		chain.push_back(this);
	}
	virtual bool _all_dependencies_in_set(std::set<std::string> const& set) const {
		return set.find(_data_type->_name) != set.end();
	}
	virtual uint32_t _sort_order() const override final {
		return static_cast<uint32_t>(SortOrder::ApiConstant);
	}

private:
	Type* _data_type = nullptr;
	std::string _value;
};

class Command : public Item {
	friend class Registry;
	friend class Feature;

public:
	struct Parameter {
		std::string complete_type;
		Type* pure_type;
		std::string name;
	};

private:
	Command(std::string const& name, tinyxml2::XMLElement* command_element) : Item(name, command_element) {}

private:
	std::string _return_type_complete;
	Type* _return_type_pure = nullptr;
	std::vector<Parameter> _params;
};

class Extension : public Item {
	friend class Registry;
	friend class Feature;

private:
	Extension(std::string const& name, int number, std::string const& supported, tinyxml2::XMLElement* extension_element) : Item(name, extension_element), _number(number), _supported(supported) {}

private:
	int _number = 0;
	std::string _supported;
	std::string _tag;
	Association _association = Association::Unspecified;
	std::vector<Command*> _commands;
	std::vector<Type*> _required_types; // Provided explicitly by registry
	std::vector<Type*> _types; // Types introduced by this extension
};

class Feature : public Item {
	friend class Registry;

private:
	Feature(std::string const& api, std::string const& name, int major, int minor, tinyxml2::XMLElement* feature_element) : Item(api, feature_element), _version_name(name), _major(major), _minor(minor) {}
	void _insert_type_with_dependencies(Type* t) {
		std::vector<Type*> dependency_chain;
		t->_build_dependency_chain(dependency_chain);
		for (auto dep : dependency_chain) {
			if (_types.insert(std::make_pair(dep->_name, dep)).second == true) {
				_dependency_chain.push_back(dep);
			}
		}
	}
	void _require_command(Command* c) {
		assert(_commands.end() == std::find_if(_commands.begin(), _commands.end(), [c](Command* existing) -> bool { return existing->_name == c->_name; }));
		_commands.push_back(c);
		_insert_type_with_dependencies(c->_return_type_pure);
		for (auto& p : c->_params) {
			_insert_type_with_dependencies(p.pure_type);
		}
	}
	void _require_type(Type* t) {
		_insert_type_with_dependencies(t);
	}
	void _require_enum(ApiConstant* a) {
		_insert_type_with_dependencies(a);
	}
	void _mark_all_core() {
		for (auto& t : _types) {
			assert(t.second->_api_part == ApiPart::Unspecified);
			t.second->_api_part = ApiPart::Core;
		}

		for (auto c : _commands) {
			assert(c->_api_part == ApiPart::Unspecified);
			c->_api_part = ApiPart::Core;
		}
	}
	void _use_extension(Extension* e) {

	}

private:
	std::string _version_name;
	int _major = 0;
	int _minor = 0;
	std::map<std::string, Type*> _types;
	std::vector<Type*> _dependency_chain;
	std::vector<Command*> _commands;
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
	Feature* build_feature(std::string const& feature);

	std::string const& license(void) const {
		return _license_header;
	}

	std::string const& version(void) const {
		return _version;
	}

	std::vector<ScalarTypedef*> const& get_scalar_typedefs(void) const {
		return _scalar_typedefs;
	}

	std::vector<Bitmasks*> const& get_bitmasks(void) const {
		return _bitmasks;
	}

	std::vector<FunctionTypedef*> const& get_function_typedefs(void) const {
		return _function_typedefs;
	}

	std::vector<HandleTypedef*> const& get_handle_typedefs(void) const {
		return _handle_typedefs;
	}

	std::vector<Struct*> const& get_structs(void) const {
		return _structs;
	}

	std::vector<ApiConstant*> const& get_api_constants(void) const {
		return _api_constants;
	}

	std::vector<Enum*> const& get_enums(void) const {
		return _enums;
	}

	std::vector<Command*> const& get_commands(void) const {
		return _commands;
	}

	std::vector<Extension*> const& get_extensions(void) const {
		return _extensions;
	}

private:
	void _parse_item_declarations(tinyxml2::XMLElement* registry_element);
	void _read_comment(tinyxml2::XMLElement * element);
	void _read_tags(tinyxml2::XMLElement * element);
	void _read_types(tinyxml2::XMLElement * element);
	void _read_type_basetype(tinyxml2::XMLElement * element);
	void _read_type_bitmask(tinyxml2::XMLElement * element);
	void _read_type_define(tinyxml2::XMLElement * element);
	void _read_type_funcpointer(tinyxml2::XMLElement * element);
	void _read_type_handle(tinyxml2::XMLElement * element);
	void _read_type_struct(tinyxml2::XMLElement * element, bool isUnion);
	void _read_enums(tinyxml2::XMLElement * element);
	void _read_api_constants(tinyxml2::XMLElement* element);
	void _read_commands(tinyxml2::XMLElement * element);
	void _read_commands_command(tinyxml2::XMLElement * element);
	void _read_extensions(tinyxml2::XMLElement * element);
	void _read_extensions_extension(tinyxml2::XMLElement * element);
	void _read_feature(tinyxml2::XMLElement* element);

	void _sort_extensions();

	void _parse_item_definitions(tinyxml2::XMLElement* registry_element);
	void _parse_scalar_typedef_definition(ScalarTypedef* t);
	void _parse_bitmasks_definition(Bitmasks* b);
	void _parse_function_typedef_definition(FunctionTypedef* f);
	void _parse_handle_typedef_definition(HandleTypedef* h);
	void _parse_struct_definition(Struct* s);
	void _read_type_struct_member(Struct* theStruct, tinyxml2::XMLElement * element);
	tinyxml2::XMLNode* _read_type_struct_member_type(tinyxml2::XMLNode* element, std::string& complete_type, Type*& pure_type);
	void _parse_api_constant_definition(ApiConstant* a);
	void _parse_enum_definition(Enum* e);
	void _parse_command_definition(Command* c);
	void _read_command_proto(tinyxml2::XMLElement * element, Command* c);
	void _read_command_params(tinyxml2::XMLElement* element, Command* c);
	void _read_command_param(tinyxml2::XMLElement * element, Command* c);
	tinyxml2::XMLNode* _read_command_param_type(tinyxml2::XMLNode* node, std::string& complete_type, Type*& pure_type, bool& const_modifier);

	void _build_dependency_chain();
	void _build_ungrouped_dependency_chain(std::vector<Type*>& chain);

	void _mark_extension_items();

	void _sort_types();

	std::string _read_array_size(tinyxml2::XMLNode * node, std::string& name);
	std::string _trim_end(std::string const& input);
	std::string _extract_tag(std::string const& name);
	std::string _bitpos_to_value(std::string const& bitpos);

	void _build_feature(Feature* f);
	void _parse_feature_definition(Feature* f);
	void _read_feature_require(tinyxml2::XMLElement* element, Feature* f);
	void _read_feature_command(tinyxml2::XMLElement* element, Feature* f);
	void _read_feature_type(tinyxml2::XMLElement* element, Feature* f);
	void _read_feature_enum(tinyxml2::XMLElement* element, Feature* f);
	void _parse_extension_definition(Extension* e);
	void _read_extension_require(tinyxml2::XMLElement * element, Extension* e);
	void _read_extension_command(tinyxml2::XMLElement * element, Extension* e);
	void _read_extension_type(tinyxml2::XMLElement * element, Extension* e);
	void _read_extension_enum(tinyxml2::XMLElement * element, Extension* e);

private:
	tinyxml2::XMLDocument _doc;

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

	std::map<std::string, Item*> _items; // All items used in the registry
	std::map<std::string, Type*> _types; // All types used in the registry (no commands, extensions, constants, etc)
	std::set<std::string> _c_types; // Keeps the set of passed C types for easy existance checks
	std::vector<ScalarTypedef*> _scalar_typedefs;
	std::vector<FunctionTypedef*> _function_typedefs;
	std::vector<Bitmasks*> _bitmasks;
	std::vector<HandleTypedef*> _handle_typedefs;
	std::vector<Struct*> _structs;
	std::vector<ApiConstant*> _api_constants;
	std::vector<Enum*> _enums;
	std::vector<Command*> _commands;
	std::vector<Extension*> _extensions;
	std::vector<Feature*> _features;
	std::vector<Type*> _dependency_chain; // The order in which dependencies are required

	ITranslator* _translator;
	std::string _version;
	std::string _license_header;
	std::set<std::string> _tags;

	bool _parsed = false;
	bool _feature_acquired = false;
};

} // vkspec

#endif // VKSPEC_INCLUDE