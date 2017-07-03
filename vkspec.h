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
#include <regex>
#include <set>
#include <string>
#include <vector>

#include <tinyxml2.h>

namespace vkspec {

enum class CommandClassification {
	Entry,
	Global,
	Instance,
	Device,
	Unspecified,
};

enum class ExtensionClassification {
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

public:
	virtual CType* to_c() { return nullptr; }
	virtual ScalarTypedef* to_scalar_typedef() { return nullptr; }
	virtual FunctionTypedef* to_function_typedef() { return nullptr; }
	virtual HandleTypedef* to_handle_typedef() { return nullptr; }
	virtual Struct* to_struct() { return nullptr; }
	virtual Enum* to_enum() { return nullptr; }
	virtual ApiConstant* to_api_constant() { return nullptr; }
	virtual Bitmasks* to_bitmasks() { return nullptr; }

protected:
	Type(std::string const& name, tinyxml2::XMLElement* type_element) : Item(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) = 0;
	// The dependency condition is satisfied if all dependencies of a type have
	// either already been added (present in added_set) or if the current set
	// contains the dependency of a lower sort order. The latter case means that
	// we can safely add the type because the lower sort order ensures that the
	// dependency is satisfied when sorting. This is more relaxed than the
	// original solution that only checked if all dependencies were complete.
	virtual bool _dependency_condition(std::set<std::string> const& added_set, std::set<std::string> const& current_set) const = 0;
	// Check individual dependency
	bool _dependency_check(std::set<std::string> const& added_set, std::set<std::string> const& current_set, Type* dep) const {
		// Already added? Succeed immediately
		if (added_set.find(dep->_name) != added_set.end()) {
			return true;
		}

		// Second wind if amongst the current set. In this case, a lower sort
		// order of the dependency than the one we are adding is safe because
		// sorting will place the dependency first. It's also ok with equal
		// sort order because this will not change their relative order (stable
		// sort), and since the dependency is present we are fine.
		if (current_set.find(dep->_name) != current_set.end() && dep->_sort_order() <= _sort_order()) {
			assert(dep->_dependency_order < _dependency_order);
			return true;
		}

		return false;
	}
	virtual SortOrder _sort_order() const = 0;

protected:
	int _dependency_order = 0; // Used when sorting
};

class CType : public Type {
	friend class Registry;

public:
	virtual std::string const& name(void) const { return _translation; }
	virtual CType* to_c() { return this; }
	bool opaque() { return _opaque; }

private:
	CType(std::string const& c, std::string const& translation) : Type(c, nullptr), _translation(translation) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final { chain.push_back(this); }
	virtual bool _dependency_condition(std::set<std::string> const& added_set, std::set<std::string> const& current_set) const {
		return true; // No dependencies; always true
	}
	virtual SortOrder _sort_order() const override final {
		return SortOrder::CType;
	}

private:
	std::string _translation;
	bool _opaque = false; // Here it means we have no actual translation and expect use via pointers
};

class ScalarTypedef : public Type {
	friend class Registry;

public:
	virtual ScalarTypedef* to_scalar_typedef() { return this; }
	Type* actual_type() { return _actual_type; }

private:
	ScalarTypedef(std::string const& name, tinyxml2::XMLElement* type_element) : Type(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_actual_type->_build_dependency_chain(chain);
		chain.push_back(this);
	}
	virtual bool _dependency_condition(std::set<std::string> const& added_set, std::set<std::string> const& current_set) const {
		return _dependency_check(added_set, current_set, _actual_type);
	}
	virtual SortOrder _sort_order() const override final {
		return SortOrder::ScalarTypedef;
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

	virtual FunctionTypedef* to_function_typedef() { return this; }
	std::vector<Parameter>& params() { return _params; }
	std::string const& complete_return_type() { return _return_type_complete; }
	Type* pure_return_type() { return _return_type_pure; }

private:
	FunctionTypedef(std::string const& name, tinyxml2::XMLElement* type_element) : Type(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_return_type_pure->_build_dependency_chain(chain);
		for (auto& p : _params) {
			p.pure_type->_build_dependency_chain(chain);
		}
		chain.push_back(this);
	}
	virtual bool _dependency_condition(std::set<std::string> const& added_set, std::set<std::string> const& current_set) const {
		if (!_dependency_check(added_set, current_set, _return_type_pure)) {
			return false;
		}

		for (auto& p : _params) {
			if (!_dependency_check(added_set, current_set, p.pure_type)) {
				return false;
			}
		}

		return true;
	}
	virtual SortOrder _sort_order() const override final {
		return SortOrder::FunctionTypedef;
	}

private:
	std::string _return_type_complete;
	Type* _return_type_pure = nullptr;
	std::vector<Parameter> _params;
};

class HandleTypedef : public Type {
	friend class Registry;

public:
	virtual HandleTypedef* to_handle_typedef() { return this; }
	Type* actual_type() { return _actual_type; }

private:
	HandleTypedef(std::string const& name, tinyxml2::XMLElement* type_element) : Type(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_actual_type->_build_dependency_chain(chain);
		chain.push_back(this);
	}
	virtual bool _dependency_condition(std::set<std::string> const& added_set, std::set<std::string> const& current_set) const {
		return _dependency_check(added_set, current_set, _actual_type);
	}
	virtual SortOrder _sort_order() const override final {
		return SortOrder::HandleTypedef;
	}
	bool _device_object() const {
		if (_name == "VkDevice") {
			return true;
		}

		for (auto p : _parents) {
			if (p->_device_object()) {
				return true;
			}
		}

		return false;
	}

private:
	Type* _actual_type = nullptr;
	std::vector<HandleTypedef*> _parents;
	bool _dispatchable = false;
};

class Struct : public Type {
	friend class Registry;

public:
	struct Member {
		std::string complete_type;
		Type* pure_type;
		std::string name;
		ApiConstant* array_dependency;
	};

	virtual Struct* to_struct() { return this; }
	bool is_union() { return _is_union; }
	std::vector<Member> const& members() { return _members; }

private:
	Struct(std::string const& name, tinyxml2::XMLElement* type_element, bool is_union) : Type(name, type_element), _is_union(is_union) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		for (auto& m : _members) {
			if (m.array_dependency) {
				((Type*)m.array_dependency)->_build_dependency_chain(chain);
			}
			m.pure_type->_build_dependency_chain(chain);
		}
		chain.push_back(this);
	}
	virtual bool _dependency_condition(std::set<std::string> const& added_set, std::set<std::string> const& current_set) const {
		for (auto& m : _members) {
			if (m.array_dependency && !_dependency_check(added_set, current_set, (Type*)m.array_dependency)) {
				return false;
			}
			if (!_dependency_check(added_set, current_set, m.pure_type)) {
				return false;
			}
		}

		return true;
	}
	virtual SortOrder _sort_order() const override final {
		return SortOrder::Struct;
	}

private:
	std::vector<Member> _members;
	bool _is_union = false;
};

class Enum : public Type {
	friend class Registry;
	friend class Feature;

public:
	struct Member {
		std::string name;
		std::string value;
	};

	virtual Enum* to_enum() { return this; }
	std::vector<Member> const& members() { return _members; }

private:
	Enum(std::string const& name, tinyxml2::XMLElement* type_element, bool bitmask) : Type(name, type_element), _bitmask(bitmask) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		chain.push_back(this);
	}
	virtual bool _dependency_condition(std::set<std::string> const& added_set, std::set<std::string> const& current_set) const {
		return true; // No dependencies; always true
	}
	virtual SortOrder _sort_order() const override final {
		return SortOrder::Enum;
	}

private:
	std::vector<Member> _members;
	bool _bitmask;
};

class Bitmasks : public Type {
	friend class Registry;

public:
	virtual Bitmasks* to_bitmasks() { return this; }
	Enum* flags() { return _flags; }

private:
	Bitmasks(std::string const& name, tinyxml2::XMLElement* type_element) : Type(name, type_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_actual_type->_build_dependency_chain(chain);
		if (_flags) { // Can be nullptr if the bitmasks have no flag definitions
			((Type*)_flags)->_build_dependency_chain(chain);
		}
		chain.push_back(this);
	}
	virtual bool _dependency_condition(std::set<std::string> const& added_set, std::set<std::string> const& current_set) const {
		if (!_dependency_check(added_set, current_set, _actual_type)) {
			return false;
		}

		if (_flags) {
			return _dependency_check(added_set, current_set, _flags);
		}

		return true;
	}
	virtual SortOrder _sort_order() const override final {
		return SortOrder::Bitmasks;
	}

private:
	Type* _actual_type = nullptr; // Should probably always be VkFlags. Not really used, but here for dependency purposes
	Enum* _flags = nullptr; // Enum containing flag definitions
};

class ApiConstant : public Type {
	friend class Registry;

public:
	virtual ApiConstant* to_api_constant() { return this; }
	Type* data_type() { return _data_type; }
	std::string const& value() { return _value; }

private:
	ApiConstant(std::string const& name, tinyxml2::XMLElement* enum_element) : Type(name, enum_element) {}
	virtual void _build_dependency_chain(std::vector<Type*>& chain) override final {
		_data_type->_build_dependency_chain(chain);
		chain.push_back(this);
	}
	virtual bool _dependency_condition(std::set<std::string> const& added_set, std::set<std::string> const& current_set) const {
		return _dependency_check(added_set, current_set, _data_type);
	}
	virtual SortOrder _sort_order() const override final {
		return SortOrder::ApiConstant;
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
		ApiConstant* array_dependency;
	};

	CommandClassification classification() {
		return _classification;
	}
	std::vector<Parameter> const& params() { return _params; }
	std::string const& complete_return_type() { return _return_type_complete; }
	Type* pure_return_type() { return _return_type_pure; }

private:
	Command(std::string const& name, tinyxml2::XMLElement* command_element) : Item(name, command_element) {}

private:
	std::string _return_type_complete;
	Type* _return_type_pure = nullptr;
	std::vector<Parameter> _params;
	CommandClassification _classification = CommandClassification::Unspecified;
};

class Extension : public Item {
	friend class Registry;
	friend class Feature;

public:
	ExtensionClassification classification() { return _classification; }
	std::vector<Command*> const& commands() { return _commands; }

private:
	Extension(std::string const& name, int number, std::string const& supported, tinyxml2::XMLElement* extension_element) : Item(name, extension_element), _number(number), _supported(supported) {}

private:
	int _number = 0;
	std::string _supported;
	std::string _tag;
	ExtensionClassification _classification = ExtensionClassification::Unspecified;
	std::vector<Command*> _commands;
	std::vector<Type*> _required_types; // Provided explicitly by registry
	std::vector<Type*> _types; // Types introduced by this extension
};

class IGenerator {
public:
	virtual void begin_core() = 0;
	virtual void end_core() = 0;
	virtual void gen_scalar_typedef(ScalarTypedef* t) = 0;
	virtual void gen_function_typedef(FunctionTypedef* t) = 0;
	virtual void gen_handle_typedef(HandleTypedef* t) = 0;
	virtual void gen_struct(Struct* t) = 0;
	virtual void gen_enum(Enum* t) = 0;
	virtual void gen_api_constant(ApiConstant* t) = 0;
	virtual void gen_bitmasks(Bitmasks* t) = 0;
	virtual void begin_entry() = 0;
	virtual void gen_entry_command(Command* c) = 0;
	virtual void end_entry() = 0;
	virtual void begin_global_commands() = 0;
	virtual void gen_global_command(Command* c) = 0;
	virtual void end_global_commands() = 0;
	virtual void begin_instance_commands() = 0;
	virtual void gen_instance_command(Command* c) = 0;
	virtual void end_instance_commands() = 0;
	virtual void begin_device_commands() = 0;
	virtual void gen_device_command(Command* c) = 0;
	virtual void end_device_commands() = 0;
	virtual void begin_extensions() = 0;
	virtual void end_extensions() = 0;
	virtual void begin_extension(Extension* e) = 0;
	virtual void end_extension(Extension* e) = 0;
};

class Feature : public Item {
	friend class Registry;

public:
	int major() { return _major; }
	int minor() { return _minor; }
	int patch() { return _patch; }
	void generate(IGenerator* generator) {
		generator->begin_core();

		for (auto t : _dependency_chain) {
			switch (t->_sort_order()) {
				case SortOrder::CType: {
					break; // Only for internal tracking
				}
				case SortOrder::ScalarTypedef: {
					if (!t->_extension) {
						assert(t->to_scalar_typedef());
						generator->gen_scalar_typedef(t->to_scalar_typedef());
					}
					break;
				}
				case SortOrder::FunctionTypedef: {
					if (!t->_extension) {
						assert(t->to_function_typedef());
						generator->gen_function_typedef(t->to_function_typedef());
					}
					break;
				}
				case SortOrder::HandleTypedef: {
					if (!t->_extension) {
						assert(t->to_handle_typedef());
						generator->gen_handle_typedef(t->to_handle_typedef());
					}
					break;
				}
				case SortOrder::Struct: {
					if (!t->_extension) {
						assert(t->to_struct());
						generator->gen_struct(t->to_struct());
					}
					break;
				}
				case SortOrder::Enum: {
					assert(t->to_enum());
					if (!t->_extension && !t->to_enum()->_bitmask) {
						generator->gen_enum(t->to_enum());
					}
					break;
				}
				case SortOrder::ApiConstant: {
					if (!t->_extension) {
						assert(t->to_api_constant());
						generator->gen_api_constant(t->to_api_constant());
					}
					break;
				}
				case SortOrder::Bitmasks: {
					if (!t->_extension) {
						assert(t->to_bitmasks());
						generator->gen_bitmasks(t->to_bitmasks());
					}
					break;
				}
				default: {
					assert(false);
				}
			}
		}

		generator->begin_entry();
		for (auto c : _commands) {
			if (!c->_extension && c->classification() == CommandClassification::Entry) {
				generator->gen_entry_command(c);
			}
		}
		generator->end_entry();

		generator->begin_global_commands();
		for (auto c : _commands) {
			if (!c->_extension && c->classification() == CommandClassification::Global) {
				generator->gen_global_command(c);
			}
		}
		generator->end_global_commands();

		generator->begin_instance_commands();
		for (auto c : _commands) {
			if (!c->_extension && c->classification() == CommandClassification::Instance) {
				generator->gen_instance_command(c);
			}
		}
		generator->end_instance_commands();

		generator->begin_device_commands();
		for (auto c : _commands) {
			if (!c->_extension && c->classification() == CommandClassification::Device) {
				generator->gen_device_command(c);
			}
		}
		generator->end_device_commands();

		generator->end_core();

		generator->begin_extensions();

		for (auto e : _extensions) {
			generator->begin_extension(e);
			for (auto t : e->_types) {
				switch (t->_sort_order()) {
					case SortOrder::CType: {
						break; // Only for internal tracking
					}
					case SortOrder::ScalarTypedef: {
						assert(t->to_scalar_typedef());
						generator->gen_scalar_typedef(t->to_scalar_typedef());
						break;
					}
					case SortOrder::FunctionTypedef: {
						assert(t->to_function_typedef());
						generator->gen_function_typedef(t->to_function_typedef());
						break;
					}
					case SortOrder::HandleTypedef: {
						assert(t->to_handle_typedef());
						generator->gen_handle_typedef(t->to_handle_typedef());
						break;
					}
					case SortOrder::Struct: {
						assert(t->to_struct());
						generator->gen_struct(t->to_struct());
						break;
					}
					case SortOrder::Enum: {
						if (!t->to_enum()->_bitmask) {
							generator->gen_enum(t->to_enum());
						}
						break;
					}
					case SortOrder::ApiConstant: {
						assert(t->to_api_constant());
						generator->gen_api_constant(t->to_api_constant());
						break;
					}
					case SortOrder::Bitmasks: {
						assert(t->to_bitmasks());
						generator->gen_bitmasks(t->to_bitmasks());
						break;
					}
					default: {
						assert(false);
					}
				}
			}
			generator->end_extension(e);
		}

		generator->end_extensions();
	}

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
			if (p.array_dependency) {
				_insert_type_with_dependencies(p.array_dependency);
			}
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
		assert(_extensions.end() == std::find_if(_extensions.begin(), _extensions.end(), [e](Extension* existing) -> bool { return existing->_name == e->_name; }));
		_extensions.push_back(e);

		std::vector<Type*> dependency_chain;

		for (auto c : e->_commands) {
			// Make sure command does not already exist, then add it.
			assert(_commands.end() == std::find_if(_commands.begin(), _commands.end(), [c](Command* existing) -> bool { return existing->_name == c->_name; }));
			assert(c->_api_part == ApiPart::Unspecified);
			c->_api_part = ApiPart::Extension;
			_commands.push_back(c);

			// Add dependencies to dependency chain
			c->_return_type_pure->_build_dependency_chain(dependency_chain);

			for (auto& p : c->_params) {
				if (p.array_dependency) {
					((Type*)p.array_dependency)->_build_dependency_chain(dependency_chain);
				}
				p.pure_type->_build_dependency_chain(dependency_chain);
			}
		}

		// Add all required types to dependency chain
		for (auto t : e->_required_types) {
			t->_build_dependency_chain(dependency_chain);
		}

		// For each dependency, we try to add it. If this turns out to be a new
		// type, we assume that it was added by this extension.
		for (auto dep : dependency_chain) {
			if (_types.insert(std::make_pair(dep->_name, dep)).second == true) {
				assert(dep->_api_part == ApiPart::Unspecified);
				dep->_api_part = ApiPart::Extension;
				assert(!dep->_extension);
				dep->_extension = e;
				e->_types.push_back(dep);
				_dependency_chain.push_back(dep);
			}
		}
	}
	// I guess another strategy that would be more comprehensible is to scan in
	// the sort order. First push all scalar typedefs (first in the sort order)
	// whose dependencies have been added. Then push all handles (second sort)
	// with added dependencies. Do it for all types then start all over until
	// all items have been added. What I do now use a few non-obvious properties
	// for things to work. I have tried to name them where I have thought about
	// it, but I am well aware that it is not trivial to understand why things
	// work out. This method could very well be rewritten in the future, but
	// for now I'll leave it since it seems to work.
	void _group_dependencies(std::map<std::string, CType*>& c_types) {
		// The dependency chain now contain dependencies in the order used by
		// commands. It's likely a wild west of mixed types in there, so here
		// we group types together using the same relative order as the
		// dependency chain, provided that all dependencies are satisfied.
		std::vector<Type*> ungrouped_dependency_chain = _dependency_chain;
		_dependency_chain.clear();

		// In the dependency condition of the upcoming algorithm we can relax
		// even more if we know the relative order of types (see the check)
		for (int i = 0; i < ungrouped_dependency_chain.size(); ++i) {
			ungrouped_dependency_chain[i]->_dependency_order = i;
		}

		std::set<std::string> all_added_dependencies;
		std::set<std::string> current_added_dependencies;
		int new_types = 0;

		// Ultimately, everything depend on C types which are expected to have
		// no dependencies themselves, so we begin by adding those.
		for (auto& c : c_types) {
			_types.insert(c); // Just to make sure it is here
			_dependency_chain.push_back(c.second);
			assert(all_added_dependencies.insert(c.first).second == true);
		}

		// Now the algorithm is as follows. Several iterations is done. We push
		// all types that only depend on types added in previous iterations,
		// that is, we don't consider those added in the current iteration when
		// matching. If a type has a dependency that has not been added, we
		// ignore it until the dependency have been added. This also means that
		// nested dependencies are dealt with automatically; we only need to
		// check that the direct dependencies have been added. When there are
		// no more possible types, the added set is sorted on type (they don't
		// depend on each other). This continues until all types have been
		// added to the dependency chain.
		while (all_added_dependencies.size() != ungrouped_dependency_chain.size()) {
			for (auto& type : ungrouped_dependency_chain) {
				if (all_added_dependencies.find(type->_name) != all_added_dependencies.end()) {
					continue; // Added before
				}

				if (type->_dependency_condition(all_added_dependencies, current_added_dependencies)) {
					assert(current_added_dependencies.insert(type->_name).second == true);
					_dependency_chain.push_back(type);
					new_types++;
				}
			}

			// Some new type must have been added (every type ultimately depend
			// on C types)
			assert(!current_added_dependencies.empty());

			// Stable sort to preserve the relative order of types as used in
			// commands.
			std::stable_sort(_dependency_chain.end() - new_types, _dependency_chain.end(), [](Type* t1, Type* t2) -> bool {
				return t1->_sort_order() < t2->_sort_order();
			});

			all_added_dependencies.insert(current_added_dependencies.begin(), current_added_dependencies.end());
			current_added_dependencies.clear();
			new_types = 0;
		}

		// With the dependency chain built, we set dependency orders on types
		// that are used to sort subsets in the same fashion. While at it we
		// check that all types are accounted for.
		for (int i = 0; i < _dependency_chain.size(); ++i) {
			_dependency_chain[i]->_dependency_order = i;
			assert(_types.find(_dependency_chain[i]->_name) != _types.end());
		}

		assert(_types.size() == _dependency_chain.size());
	}
	void _sort_extension_types() {
		// Using the dependency order from when the dependency chain was grouped
		// we can now easily sort the vectors of types added by each extension.
		// These will essentially be filters of the dependency chain, respecting
		// the dependency chain itself, while also grouping types using the same
		// relative ordering of their usage in the API.
		for (auto e : _extensions) {
			std::sort(e->_types.begin(), e->_types.end(), [](Type* t1, Type* t2) {
				return t1->_dependency_order < t2->_dependency_order;
			});
		}
	}
	void _sanity_check(std::set<std::string>& tags, std::map<std::string, CType*>& c_types) {
		// All types accounted for, and no duplicates
		for (auto t : _dependency_chain) {
			assert(_types.find(t->_name) != _types.end());
		}
		assert(_types.size() == _dependency_chain.size());

		for (auto t : _types) {
			assert(t.first == t.second->_name);

			if (!t.second->_extension) {
				assert(t.second->_api_part == ApiPart::Core);
			}
			else {
				assert(t.second->_api_part == ApiPart::Extension);
			}
		}

		for (auto c : _commands) {
			if (!c->_extension) {
				assert(c->_api_part == ApiPart::Core);
			}
			else {
				assert(c->_api_part == ApiPart::Extension);
			}
		}

		for (auto e : _extensions) {
			assert(e->_classification != ExtensionClassification::Unspecified);
			assert(tags.find(e->_tag) != tags.end());

			for (auto c : e->_commands) {
				assert(c->_api_part == ApiPart::Extension);
				assert(c->_extension && c->_extension == e);

				// All extension commands should match this pattern. This is a second
				// safeguard to make sure the registry doesn't list core functions
				// as extension commands (I have assumed the listed ones are the
				// very commands added by the extension in question).
				std::regex re(R"(^vk[A-Z][a-zA-Z0-9]+[a-z0-9]([A-Z][A-Z]+)$)");
				auto it = std::sregex_iterator(c->_name.begin(), c->_name.end(), re);
				auto end = std::sregex_iterator();
				assert(it != end);
				std::smatch match = *it;
				assert(tags.find(match[1].str()) != tags.end());

				if (e->_classification == ExtensionClassification::Instance) {
					assert(c->classification() == CommandClassification::Instance);
				}
				else if (e->_classification == ExtensionClassification::Device) {
					// Instance-level commands can be added by device extensions
					assert(c->classification() == CommandClassification::Device || c->classification() == CommandClassification::Instance);
				}
			}

			for (auto t : e->_types) {
				assert(t->_api_part == ApiPart::Extension);
				assert(t->_extension && t->_extension == e);

				// Some C types are only used by extensions, and that means it
				// will be first used by some extension. While the extension
				// does not actually add the C type, I'm leaving it be (when
				// writing bindings we can just ignore C types) because it's of
				// no harm, and the information (what extension first used it)
				// just might be useful in the future. We still consider them
				// here as a special case because they are ok, but won't match
				// Vulkan naming conventions.
				if (c_types.find(t->_name) != c_types.end()) {
					continue;
				}

				// Extension types should match these patterns. This is a second
				// safeguard to make sure no core types have been missed previously.
				std::regex re;
				if (t->to_api_constant()) {
					re = std::regex(R"(^VK_[A-Z_]+_([A-Z]+)$)");
				}
				else if (t->to_function_typedef()) {
					re = std::regex(R"(^PFN_vk[A-Z][a-zA-Z0-9]+[a-z0-9]([A-Z][A-Z]+)$)");
				}
				else {
					re = std::regex(R"(^Vk[A-Z][a-zA-Z0-9]+[a-z0-9]([A-Z][A-Z]+)$)");
				}

				auto it = std::sregex_iterator(t->_name.begin(), t->_name.end(), re);
				auto end = std::sregex_iterator();
				assert(it != end);
				std::smatch match = *it;
				assert(tags.find(match[1].str()) != tags.end());
			}
		}
	}

private:
	std::string _version_name;
	int _major = 0;
	int _minor = 0;
	int _patch = 0;
	std::map<std::string, Type*> _types;
	std::vector<Type*> _dependency_chain;
	std::vector<Command*> _commands;
	std::vector<Extension*> _extensions;
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
	virtual std::string pointer_to(Type* type, PointerType pointer_type) = 0;
	virtual std::string array_member(std::string const& type_name, std::string const& array_size) = 0;
	virtual std::string array_param(std::string const& type_name, std::string const& array_size, bool const_modifier) = 0;
	virtual std::string bitwise_not(std::string const& value) = 0;
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
	void add_c_type(std::string const& c, std::string const& translation, bool opaque = false);
	void parse(std::string const& spec);
	Feature* build_feature(std::string const& feature);

	std::string const& license(void) const {
		return _license_header;
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

	std::string _read_array_size(tinyxml2::XMLNode * node, std::string& name, ApiConstant*& api_constant);
	std::string _trim_end(std::string const& input);
	std::string _extract_tag(std::string const& name);
	std::string _bitpos_to_value(std::string const& bitpos);

	void _mark_command_classifications();

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

	std::map<std::string, Item*> _items; // All items used in the registry
	std::map<std::string, Type*> _types; // All types used in the registry (no commands, extensions, constants, etc)
	std::map<std::string, CType*> _c_types; // Keeps the set of passed C types for easy existance checks
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

	ITranslator* _translator;
	int _patch;
	std::string _license_header;
	std::set<std::string> _tags;

	bool _parsed = false;
	bool _feature_acquired = false;
};

} // vkspec

#endif // VKSPEC_INCLUDE