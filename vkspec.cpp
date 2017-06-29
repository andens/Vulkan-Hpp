#include "vkspec.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

namespace vkspec {

	void Registry::add_c_type(std::string const& c, std::string const& translation) {
		CType* c_type = new CType(c, translation);
		assert(_items.insert(std::make_pair(c, c_type)).second == true);
		assert(_types.insert(std::make_pair(c, c_type)).second == true);
		assert(_c_types.insert(c).second == true);
	}

	void Registry::parse(std::string const& spec) {
		if (_parsed) {
			throw std::runtime_error("The current instance has already parsed a registry. Please make another instance to parse again.");
		}

		std::cout << "Loading vk.xml from " << spec << std::endl;

		tinyxml2::XMLDocument doc;
		tinyxml2::XMLError error = doc.LoadFile(spec.c_str());
		if (error != tinyxml2::XML_SUCCESS)
		{
			throw std::runtime_error("VkGenerate: failed to load file " + spec + ". Error code: " + std::to_string(error));
		}

		// The very first element is expected to be a registry, and it should
		// be the only root element.
		tinyxml2::XMLElement * registryElement = doc.FirstChildElement();
		assert(strcmp(registryElement->Value(), "registry") == 0);
		assert(!registryElement->NextSiblingElement());

		_parse_item_declarations(registryElement);

		// Sort extensions on number before going further (should already be
		// sorted but you never know). That way I can process them in order
		// which is useful when determining which extension types were added by
		// what extension.
		_sort_extensions();

		// Parse type definitions. We are now able to get any type we depend on
		// since they were created in the pass before.
		_parse_item_definitions(registryElement);

		// TODO: When feature is parsed, commands will be sorted according to that
		// list. Then my ordering of types will probably be like vulkan.h (as in
		// VkFormat after VkInternalAllocationType and not much later)

		// Build dependency chain in order to sort types according to how
		// early they are used by commands (I think this is the sorting used in
		// vulkan.h). This pass also marks whether a type belongs to core or an
		// extension, albeit not which extension. That is done in the next pass.
		_build_dependency_chain();

		// Find out what extension adds what types.
		_mark_extension_items();

		// Sort type vectors according to dependency chain
		_sort_types();

		_parsed = true;
	}

	Feature * Registry::build_feature(std::string const& feature) {
		if (!_feature_acquired) {
			auto feature_it = std::find_if(_features.begin(), _features.end(), [&feature](Feature* f) -> bool {
				return f->_name == feature;
			});

			if (feature_it == _features.end()) {
				std::cout << "Feature '" + feature + "' not found" << std::endl;
				return nullptr;
			}

			_feature_acquired = true;
			_build_feature(*feature_it);
			return *feature_it;
		}
		else {
			throw std::runtime_error("A feature may only be built once as extensions modify internal data that may not be accurate for other features. Instead, parse a new registry and get the feature from that one.");
		}
	}

	void Registry::_parse_item_declarations(tinyxml2::XMLElement* registry_element) {
		// The root tag contains zero or more of the following tags. Order may
		// change. Here we parse item declarations, but do not define them. That
		// is deferred until later when all types exist so that we can refer to
		// them. While at it, we also collect general independent information
		// such as license header and tags. To avoid parsing again, the XML node
		// is saved to easily start reading definitions later.
		for (tinyxml2::XMLElement * child = registry_element->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			assert(child->Value());
			const std::string value = child->Value();

			if (value == "comment") {
				// Get the vulkan license header and skip any leading spaces
				_read_comment(child);
				_license_header.erase(_license_header.begin(), std::find_if(_license_header.begin(), _license_header.end(), [](char c) { return !std::isspace(c); }));
			}
			else if (value == "tags") {
				// Author IDs for extensions and layers
				_read_tags(child);
			}
			else if (value == "types") {
				// Types used in the API
				_read_types(child);
			}
			else if (value == "enums") {
				// Enum definitions, but we only declare them for now
				_read_enums(child);
			}
			else if (value == "commands") {
				// Declarations of commands used in the API
				_read_commands(child);
			}
			else if (value == "extensions") {
				// Extension interfaces
				_read_extensions(child);
			}
			else if (value == "feature") {
				_read_feature(child);
			}
			else {
				assert(value == "vendorids");
			}
		}
	}

	void Registry::_read_comment(tinyxml2::XMLElement * element)
	{
		assert(element->GetText());
		assert(_license_header.empty());
		_license_header = element->GetText();
		assert(_license_header.find("\nCopyright") == 0);

		// erase the part after the Copyright text
		size_t pos = _license_header.find("\n\n-----");
		assert(pos != std::string::npos);
		_license_header.erase(pos);

		// replace any '\n' with '\n//' to make comments of the license text
		for (size_t pos = _license_header.find('\n'); pos != std::string::npos; pos = _license_header.find('\n', pos + 1))
		{
			_license_header.replace(pos, 1, "\n// ");
		}

		// and add a little message on our own
		_license_header += "\n\n// This header is generated from the Khronos Vulkan XML API Registry.";
	}

	void Registry::_read_tags(tinyxml2::XMLElement * element)
	{
		_tags.insert("KHX"); // Not listed
		_tags.insert("EXT");
		_tags.insert("KHR");
		for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			assert(child->Attribute("name"));
			_tags.insert(child->Attribute("name"));
		}
	}

	void Registry::_read_types(tinyxml2::XMLElement * element)
	{
		// The types tag consists of individual type tags that each describe types used in the API.
		for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			assert(child->Value() && strcmp(child->Value(), "type") == 0);
			std::string type = child->Value();

			// A present category indicates a type has a more complex definition.
			// I.e, it's not just a basic C type.
			if (child->Attribute("category"))
			{
				std::string category = child->Attribute("category");

				if (category == "basetype")
				{
					// C code for scalar typedefs.
					_read_type_basetype(child);
				}
				else if (category == "bitmask")
				{
					// C typedefs for enums that are bitmasks.
					_read_type_bitmask(child);
				}
				else if (category == "define")
				{
					// C code for #define directives. Generally not interested in
					// defines, but we can get Vulkan header version here.
					_read_type_define(child);
				}
				else if (category == "funcpointer")
				{
					// C typedefs for function pointers.
					_read_type_funcpointer(child);
				}
				else if (category == "handle")
				{
					// C macros that define handle types such as VkInstance
					_read_type_handle(child);
				}
				else if (category == "struct")
				{
					_read_type_struct(child, false);
				}
				else if (category == "union")
				{
					_read_type_struct(child, true);
				}
				else
				{
					// enum: These are covered later in 'registry > enums' tags
					// so I ignore them here and use enums instead where I can
					// save the element containing definitions.
					// include: C code for #include directives
					assert((category == "enum") || (category == "include"));
				}
			}
			// Unspecified category: non-structured definition. These should be some
			// C type.
			else
			{
				assert(child->FirstChildElement() == nullptr);
				assert(child->Attribute("name"));
				std::string name = child->Attribute("name");

				if (_c_types.find(name) == _c_types.end()) {
					throw std::runtime_error("Translation for C type '" + name + "' not provided.");
				}
			}
		}
	}

	void Registry::_read_type_basetype(tinyxml2::XMLElement * element)
	{
		tinyxml2::XMLElement* name_element = element->FirstChildElement("name");
		assert(name_element && name_element->GetText());
		std::string name = name_element->GetText();

		ScalarTypedef* t = new ScalarTypedef(name, element);
		assert(_items.insert(std::make_pair(name, t)).second == true);
		assert(_types.insert(std::make_pair(name, t)).second == true);
		_scalar_typedefs.push_back(t);
	}

	void Registry::_read_type_bitmask(tinyxml2::XMLElement * element)
	{
		tinyxml2::XMLElement* name_element = element->FirstChildElement("name");
		assert(name_element && name_element->GetText());
		std::string name = name_element->GetText();

		Bitmasks* b = new Bitmasks(name, element);
		assert(_items.insert(std::make_pair(name, b)).second == true);
		assert(_types.insert(std::make_pair(name, b)).second == true);
		_bitmasks.push_back(b);
	}

	void Registry::_read_type_define(tinyxml2::XMLElement * element)
	{
		tinyxml2::XMLElement * child = element->FirstChildElement();
		if (child && (strcmp(child->GetText(), "VK_HEADER_VERSION") == 0))
		{
			_version = element->LastChild()->ToText()->Value();
		}

		// ignore all the other defines
	}

	void Registry::_read_type_funcpointer(tinyxml2::XMLElement * element)
	{
		tinyxml2::XMLElement* name_element = element->FirstChildElement("name");
		assert(name_element && name_element->GetText());
		std::string name = name_element->GetText();

		FunctionTypedef* f = new FunctionTypedef(name, element);
		assert(_items.insert(std::make_pair(name, f)).second == true);
		assert(_types.insert(std::make_pair(name, f)).second == true);
		_function_typedefs.push_back(f);
	}

	void Registry::_read_type_handle(tinyxml2::XMLElement * element)
	{
		tinyxml2::XMLElement* name_element = element->FirstChildElement("name");
		assert(name_element && name_element->GetText());
		std::string name = name_element->GetText();

		HandleTypedef* h = new HandleTypedef(name, element);
		assert(_items.insert(std::make_pair(name, h)).second == true);
		assert(_types.insert(std::make_pair(name, h)).second == true);
		_handle_typedefs.push_back(h);
	}

	void Registry::_read_type_struct(tinyxml2::XMLElement * element, bool isUnion)
	{
		assert(element->Attribute("name"));
		std::string name = element->Attribute("name");

		// Defined, but never used!
		if (name == "VkRect3D") {
			return;
		}

		Struct* s = new Struct(name, element, isUnion);
		assert(_items.insert(std::make_pair(name, s)).second == true);
		assert(_types.insert(std::make_pair(name, s)).second == true);
		_structs.push_back(s);
	}

	void Registry::_read_enums(tinyxml2::XMLElement * element)
	{
		if (!element->Attribute("name")) {
			throw std::runtime_error(std::string("spec error: enums element is missing the name attribute"));
		}

		std::string name = element->Attribute("name");

		// Represents hardcoded constants. Might as well define them when we
		// stumble upon them.
		if (name == "API Constants") {
			_read_api_constants(element);
			return;
		}

		if (!element->Attribute("type")) {
			throw std::runtime_error(std::string("spec error: enums name=\"" + name + "\" is missing the type attribute"));
		}

		std::string type = element->Attribute("type");

		if (type != "bitmask" && type != "enum") {
			throw std::runtime_error(std::string("spec error: enums name=\"" + name + "\" has unknown type " + type));
		}

		bool bitmask = type == "bitmask";
		Enum* e = new Enum(name, element, bitmask);
		assert(_items.insert(std::make_pair(name, e)).second == true);
		assert(_types.insert(std::make_pair(name, e)).second == true);
		_enums.push_back(e);
	}

	void Registry::_read_api_constants(tinyxml2::XMLElement* element) {
		for (tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
			assert(child->Attribute("name"));
			std::string constant = child->Attribute("name");

			ApiConstant* c = new ApiConstant(constant, child);
			assert(_items.insert(std::make_pair(constant, c)).second == true);
			assert(_types.insert(std::make_pair(constant, c)).second == true);
			_api_constants.push_back(c);
		}
	}

	void Registry::_read_commands(tinyxml2::XMLElement * element)
	{
		for (tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			assert(strcmp(child->Value(), "command") == 0);
			_read_commands_command(child);
		}
	}

	void Registry::_read_commands_command(tinyxml2::XMLElement * element)
	{
		// Attributes on the command tag concerns documentation. Unless of course one
		// could restrict possible return values from an enum (or make a completely
		// new enum type), in which case successcodes and errorcodes could be used.

		tinyxml2::XMLElement * child = element->FirstChildElement();
		assert(child && (strcmp(child->Value(), "proto") == 0));

		tinyxml2::XMLElement* name_element = child->FirstChildElement("name");
		assert(name_element && name_element->GetText());
		std::string name = name_element->GetText();

		Command* c = new Command(name, element);
		assert(_items.insert(std::make_pair(name, c)).second == true);
		// Note: not a type, so no insertion to _types
		_commands.push_back(c);
	}

	void Registry::_read_extensions(tinyxml2::XMLElement * element)
	{
		for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			assert(strcmp(child->Value(), "extension") == 0);
			_read_extensions_extension(child);
		}
	}

	void Registry::_read_extensions_extension(tinyxml2::XMLElement * element)
	{
		assert(element->Attribute("name") && element->Attribute("number"));
		std::string name = element->Attribute("name");
		int number = std::stoi(element->Attribute("number"));
		
		Extension* e = new Extension(name, number, element);
		assert(_items.insert(std::make_pair(name, e)).second == true);
		// Note: not a type, so no insertion to _types
		_extensions.push_back(e);
	}

	void Registry::_read_feature(tinyxml2::XMLElement * element) {
		assert(element->Attribute("api") && element->Attribute("name") && element->Attribute("number"));
		std::string api = element->Attribute("api");
		std::string name = element->Attribute("name");
		std::string number = element->Attribute("number");
		std::regex re(R"(^([0-9]+)\.([0-9]+)$)");
		auto it = std::sregex_iterator(number.begin(), number.end(), re);
		auto end = std::sregex_iterator();
		assert(it != end);
		std::smatch match = *it;
		int major = std::stoi(match[1].str());
		int minor = std::stoi(match[2].str());

		Feature* f = new Feature(api, name, major, minor, element);
		assert(_items.insert(std::make_pair(api, f)).second == true);
		_features.push_back(f);
	}

	void Registry::_sort_extensions() {
		std::sort(_extensions.begin(), _extensions.end(), [](Extension* e1, Extension* e2) -> bool {
			return e1->_number < e2->_number;
		});
	}

	void Registry::_parse_item_definitions(tinyxml2::XMLElement* registry_element) {
		for (auto t : _scalar_typedefs) {
			_parse_scalar_typedef_definition(t);
		}

		for (auto b : _bitmasks) {
			_parse_bitmasks_definition(b);
		}

		for (auto f : _function_typedefs) {
			_parse_function_typedef_definition(f);
		}

		for (auto h : _handle_typedefs) {
			_parse_handle_typedef_definition(h);
		}

		for (auto s : _structs) {
			_parse_struct_definition(s);
		}

		for (auto a : _api_constants) {
			_parse_api_constant_definition(a);
		}

		for (auto e : _enums) {
			_parse_enum_definition(e);
		}

		for (auto c : _commands) {
			_parse_command_definition(c);
		}

		for (auto e : _extensions) {
			_parse_extension_definition(e);
		}

		for (auto f : _features) {
			_parse_feature_definition(f);
		}
	}

	void Registry::_parse_scalar_typedef_definition(ScalarTypedef* t) {
		tinyxml2::XMLNode* node = t->_xml_node->FirstChild();
		assert(node && node->ToText() && strcmp(node->Value(), "typedef ") == 0);

		node = node->NextSibling();
		assert(node && node->ToElement() && strcmp(node->Value(), "type") == 0);
		tinyxml2::XMLElement* type_element = node->ToElement();
		assert(type_element->GetText());
		std::string type = type_element->GetText();
		assert(type == "uint32_t" || type == "uint64_t");

		node = type_element->NextSibling();
		assert(node && node->ToElement() && strcmp(node->Value(), "name") == 0);
		tinyxml2::XMLElement* name_element = node->ToElement();
		assert(name_element->GetText());

		auto type_it = _types.find(type);
		assert(type_it != _types.end());
		t->_actual_type = type_it->second;
	}

	void Registry::_parse_bitmasks_definition(Bitmasks* b) {
		// Note: this is just the bitmask typedef. Actual flags are parsed as enum

		assert(strcmp(b->_xml_node->GetText(), "typedef ") == 0);
		tinyxml2::XMLElement * type_element = b->_xml_node->FirstChildElement();
		assert(type_element && (strcmp(type_element->Value(), "type") == 0) && type_element->GetText() && (strcmp(type_element->GetText(), "VkFlags") == 0));

		tinyxml2::XMLElement * name_element = type_element->NextSiblingElement();
		assert(name_element && (strcmp(name_element->Value(), "name") == 0) && name_element->GetText());

		assert(!name_element->NextSiblingElement());

		auto type_it = _types.find(type_element->GetText());
		assert(type_it != _types.end());

		// The requires attribute contains the type that will eventually hold
		// definitions (a name with FlagBits in it). Oftentimes however, a type
		// containing Flags is used instead. This separation is done to indicate
		// that several flags can be used as opposed to just one. For C it's
		// implemented as a regular typedef, and since some of the Flag types
		// do not have members, they are not present in the enums tags. The C
		// typedef implicitly makes them work anyway, but in Rust where I gain
		// a little more type safety I need to at least make bitflags without
		// members so that the type exists.
		Enum* bit_definitions = nullptr;
		char const* requires = b->_xml_node->Attribute("requires");
		if (requires) {
			auto enum_it = std::find_if(_enums.begin(), _enums.end(), [requires](Enum* e) -> bool {
				return e->_name == requires;
			});
			assert(enum_it != _enums.end());
			bit_definitions = *enum_it;
		}

		b->_actual_type = type_it->second;
		b->_flags = bit_definitions;
	}

	void Registry::_parse_function_typedef_definition(FunctionTypedef* f) {
		// The typedef <ret> text node
		tinyxml2::XMLNode * node = f->_xml_node->FirstChild();
		assert(node && node->ToText());
		std::string text = node->Value();

		// name tag containing the type def name
		node = node->NextSibling();
		assert(node && node->ToElement());
		tinyxml2::XMLElement * tag = node->ToElement();
		assert(tag && strcmp(tag->Value(), "name") == 0 && tag->GetText());
		std::string name = tag->GetText();
		assert(!tag->FirstChildElement());

		// This will match 'typedef TYPE* (VKAPI_PTR *' and contain TYPE in match
		// group 1 with optional * in group 2.
		std::regex re(R"(^typedef ([^ ^\*]+)(\*)? \(VKAPI_PTR \*$)");
		auto it = std::sregex_iterator(text.begin(), text.end(), re);
		auto end = std::sregex_iterator();
		assert(it != end);
		std::smatch match = *it;
		auto type_it = _types.find(match[1].str());
		assert(type_it != _types.end());
		Type* return_type = type_it->second;
		std::string return_type_complete = match[2].matched ? _translator->pointer_to(return_type->name(), PointerType::T_P) : return_type->name();

		// Text node after name tag beginning parameter list. Note that for void
		// functions this is the last node that also ends the function definition.
		node = node->NextSibling();
		assert(node && node->ToText());
		text = node->Value();
		bool nextParamConst = false;
		if (text != ")(void);") {
			// In this case we will begin parameters, so we check if the first has
			// a const modifier.
			re = std::regex(R"(\)\(\n[ ]+(const )?)");
			auto it = std::sregex_iterator(text.begin(), text.end(), re);
			assert(it != end);
			match = *it;
			nextParamConst = match[1].matched;
		}

		// Storage for parsed parameters (type, name)
		std::vector<FunctionTypedef::Parameter> params;

		// Start processing parameters.
		while (node = node->NextSibling()) {
			bool constModifier = nextParamConst;
			nextParamConst = false;

			// Type of parameter
			tag = node->ToElement();
			assert(tag && strcmp(tag->Value(), "type") == 0 && tag->GetText());
			auto type_it = _types.find(tag->GetText());
			assert(type_it != _types.end());
			Type* param_type = type_it->second;
			assert(!tag->FirstChildElement());

			// Text node containing parameter name and at times a pointer modifier.
			node = node->NextSibling();
			assert(node && node->ToText());
			text = node->ToText()->Value();

			// Match optional asterisk (group 1), a bunch of spaces, the parameter
			// name (group 2), and the rest (group 3). It doesn't seem that newline
			// is a part of this. It's probably good because then I can easily work
			// directly with suffix instead of more regex magic.
			re = std::regex(R"(^(\*)?[ ]+([a-zA-Z]+)(.*)$)");
			it = std::sregex_iterator(text.begin(), text.end(), re);
			assert(it != end);
			match = *it;
			bool pointer = match[1].matched;
			std::string param_name = match[2].str();
			if (match[3].str() == ");") {
				assert(!node->NextSibling());
			}
			else {
				assert(match[3].str() == ",");

				// Match on the suffix to know if the upcoming parameter is const.
				std::string suffix = match.suffix().str();
				re = std::regex(R"(^\n[ ]+(const )?$)");
				it = std::sregex_iterator(suffix.begin(), suffix.end(), re);
				assert(it != end);
				match = *it;
				nextParamConst = match[1].matched;
			}

			FunctionTypedef::Parameter p;
			p.pure_type = param_type;
			p.name = param_name;

			if (constModifier) {
				assert(pointer);
			}

			if (pointer) {
				p.complete_type = _translator->pointer_to(param_type->name(), constModifier ? PointerType::CONST_T_P : PointerType::T_P);
			}
			else {
				p.complete_type = param_type->name();
			}

			params.push_back(p);
		}

		f->_return_type_complete = return_type_complete;
		f->_return_type_pure = return_type;
		f->_params = params;
	}

	void Registry::_parse_handle_typedef_definition(HandleTypedef* h) {
		tinyxml2::XMLElement * typeElement = h->_xml_node->FirstChildElement();
		assert(typeElement && (strcmp(typeElement->Value(), "type") == 0) && typeElement->GetText());
		std::string type = typeElement->GetText();

		tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
		assert(nameElement && (strcmp(nameElement->Value(), "name") == 0) && nameElement->GetText());
		std::string name = nameElement->GetText();

		Type* actual_type = nullptr;
		if (type == "VK_DEFINE_HANDLE") { // Defined as pointer meaning varying size
			auto type_it = _types.find("size_t");
			assert(type_it != _types.end());
			actual_type = type_it->second;
		}
		else {
			assert(type == "VK_DEFINE_NON_DISPATCHABLE_HANDLE"); // Pointer on 64-bit and uint64_t otherwise -> always 64 bit
			auto type_it = _types.find("uint64_t");
			assert(type_it != _types.end());
			actual_type = type_it->second;
		}

		h->_actual_type = actual_type;
	}

	void Registry::_parse_struct_definition(Struct* s) {
		assert(!s->_xml_node->Attribute("returnedonly") || (strcmp(s->_xml_node->Attribute("returnedonly"), "true") == 0));

		assert(s->_xml_node->Attribute("name"));
		std::string name = s->_xml_node->Attribute("name");

		for (tinyxml2::XMLElement * child = s->_xml_node->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			assert(child->Value() && strcmp(child->Value(), "member") == 0);
			_read_type_struct_member(s, child);
		}
	}

	// Read a member tag of a struct, adding members to the provided struct.
	void Registry::_read_type_struct_member(Struct* theStruct, tinyxml2::XMLElement * element) {
		// The attributes of member tags seem to mostly concern documentation
		// generation, so they are not of interest for the bindings.

		// Read the type, parsing modifiers to get a string of the type.
		std::string complete_type;
		Type* pure_type = nullptr;
		tinyxml2::XMLNode* child = _read_type_struct_member_type(element->FirstChild(), complete_type, pure_type);

		// After we have parsed the type we expect to find the name of the member
		assert(child->ToElement() && strcmp(child->Value(), "name") == 0 && child->ToElement()->GetText());
		std::string member_name = child->ToElement()->GetText();

		// Some members have more information about array size
		std::string array_size = _read_array_size(child, member_name);
		if (array_size != "") {
			assert(complete_type == pure_type->name());
			complete_type = _translator->array_member(complete_type, array_size);
		}

		Struct::Member m;
		m.complete_type = complete_type;
		m.pure_type = pure_type;
		m.name = member_name;

		theStruct->_members.push_back(m);
	}

	// Reads the type tag of a member tag, including potential text nodes around
	// the type tag to get qualifiers. We pass the first node that could potentially
	// be a text node.
	tinyxml2::XMLNode* Registry::_read_type_struct_member_type(tinyxml2::XMLNode* element, std::string& complete_type, Type*& pure_type)
	{
		assert(element);

		bool constant = false;
		if (element->ToText())
		{
			std::string value = _trim_end(element->Value());
			if (value == "const") {
				constant = true;
			}
			else {
				// struct can happen as in VkWaylandSurfaceCreateInfoKHR. I just
				// ignore them because I don't need them in Rust.
				assert(value == "struct");
			}
			element = element->NextSibling();
			assert(element);
		}

		assert(element->ToElement());
		assert((strcmp(element->Value(), "type") == 0) && element->ToElement()->GetText());
		auto type_it = _types.find(element->ToElement()->GetText());
		assert(type_it != _types.end());
		pure_type = type_it->second;
		complete_type = pure_type->name(); // In case of no pointer

		element = element->NextSibling();
		assert(element);
		if (element->ToText())
		{
			std::string value = _trim_end(element->Value());
			assert((value == "*") || (value == "**") || (value == "* const*"));
			if (value == "*") {
				complete_type = _translator->pointer_to(pure_type->name(), constant ? PointerType::CONST_T_P : PointerType::T_P);
			}
			else if (value == "**") {
				complete_type = _translator->pointer_to(pure_type->name(), constant ? PointerType::CONST_T_PP : PointerType::T_PP);
			}
			else {
				assert(value == "* const*");
				complete_type = _translator->pointer_to(pure_type->name(), constant ? PointerType::CONST_T_P_CONST_P : PointerType::T_P_CONST_P);
			}
			element = element->NextSibling();
		}
		else {
			// Should not have const qualifier without pointer
			assert(!constant);
		}

		return element;
	}

	void Registry::_parse_api_constant_definition(ApiConstant* a) {
		assert(a->_xml_node->Attribute("value"));
		std::string value = a->_xml_node->Attribute("value");

		// Most are fine, but some may become troublesome at times depending on
		// how large an unsigned int will be. Those of U suffix seem to be used
		// in places of type uint32_t, and ULL is used in places where the type
		// is VkDeviceSize, which is typedefed to uint64_t.

		std::regex re(R"(^(-)?[0-9]+$)");
		auto it = std::sregex_iterator(value.begin(), value.end(), re);
		auto end = std::sregex_iterator();

		// Matched a regular integer
		if (it != end) {
			std::smatch match = *it;
			auto type_it = _types.find(match[1].matched ? "int32_t" : "uint32_t");
			assert(type_it != _types.end());
			a->_data_type = type_it->second;
			a->_value = value;
			return;
		}

		re = std::regex(R"(^[0-9]+\.[0-9]+f$)");
		it = std::sregex_iterator(value.begin(), value.end(), re);

		// Matched float
		if (it != end) {
			value.pop_back();
			auto type_it = _types.find("float");
			assert(type_it != _types.end());
			a->_data_type = type_it->second;
			a->_value = value;
			return;
		}

		// The rest
		if (value == "(~0U)") {
			auto type_it = _types.find("uint32_t");
			assert(type_it != _types.end());
			a->_data_type = type_it->second;
			a->_value = "~0";
		}
		else if (value == "(~0ULL)") {
			auto type_it = _types.find("uint64_t");
			assert(type_it != _types.end());
			a->_data_type = type_it->second;
			a->_value = "~0";
		}
		else {
			assert(value == "(~0U-1)");
			auto type_it = _types.find("uint32_t");
			assert(type_it != _types.end());
			a->_data_type = type_it->second;
			a->_value = "(~0) - 1";
		}
	}

	void Registry::_parse_enum_definition(Enum* e) {
		// read the names of the enum values
		for (tinyxml2::XMLElement * child = e->_xml_node->FirstChildElement(); child; child = child->NextSiblingElement()) {
			if (strcmp(child->Value(), "unused") == 0) {
				continue;
			}

			assert(child->Attribute("name"));
			std::string name = child->Attribute("name");

			std::string value;
			if (child->Attribute("bitpos")) {
				value = _bitpos_to_value(child->Attribute("bitpos"));
				assert(!child->Attribute("value"));
			}
			else {
				assert(child->Attribute("value"));
				value = child->Attribute("value"); // Can be arbitrary string but I don't consider that for now
			}

			Enum::Member m;
			m.name = name;
			m.value = value;
			
			e->_members.push_back(m);
		}
	}

	void Registry::_parse_command_definition(Command* c) {
		tinyxml2::XMLElement * proto = c->_xml_node->FirstChildElement();
		assert(proto && (strcmp(proto->Value(), "proto") == 0));

		_read_command_proto(proto, c);
		_read_command_params(proto, c);
	}

	void Registry::_read_command_proto(tinyxml2::XMLElement * element, Command* c)
	{
		// Defines the return type and name of a command

		// Get type and name tags, making sure there are no text nodes inbetween.
		tinyxml2::XMLNode* node = element->FirstChild();
		assert(node && node->ToElement());
		tinyxml2::XMLElement * typeElement = node->ToElement();
		assert(strcmp(typeElement->Value(), "type") == 0 && typeElement->GetText());
		node = typeElement->NextSibling();
		assert(node && node->ToElement());
		tinyxml2::XMLElement * nameElement = node->ToElement();
		assert(strcmp(nameElement->Value(), "name") == 0 && nameElement->GetText());
		assert(!nameElement->NextSibling());

		// get return type and name of the command
		auto type_it = _types.find(typeElement->GetText());
		assert(type_it != _types.end());
		c->_return_type_complete = type_it->second->name();
		c->_return_type_pure = type_it->second;
	}

	void Registry::_read_command_params(tinyxml2::XMLElement* element, Command* c)
	{
		// iterate over the siblings of the element and read the command parameters
		assert(element);
		while (element = element->NextSiblingElement())
		{
			std::string value = element->Value();
			if (value == "param") {
				_read_command_param(element, c);
			}
			else {
				// ignore these values!
				assert((value == "implicitexternsyncparams") || (value == "validity"));
			}
		}
	}

	void Registry::_read_command_param(tinyxml2::XMLElement * element, Command* c)
	{
		std::string complete_type;
		Type* pure_type = nullptr;
		bool const_modifier = false;
		tinyxml2::XMLNode * after_type = _read_command_param_type(element->FirstChild(), complete_type, pure_type, const_modifier);

		assert(after_type->ToElement() && (strcmp(after_type->Value(), "name") == 0) && after_type->ToElement()->GetText());
		std::string name = after_type->ToElement()->GetText();

		std::string array_size = _read_array_size(after_type, name);
		if (array_size != "") {
			assert(complete_type == pure_type->name());
			complete_type = _translator->array_param(complete_type, array_size, const_modifier);
		}

		Command::Parameter p;
		p.complete_type = complete_type;
		p.pure_type = pure_type;
		p.name = name;

		c->_params.push_back(p);
	}

	tinyxml2::XMLNode* Registry::_read_command_param_type(tinyxml2::XMLNode* node, std::string& complete_type, Type*& pure_type, bool& const_modifier)
	{
		const_modifier = false;

		assert(node);
		if (node->ToText())
		{
			// start type with "const" or "struct", if needed
			std::string value = _trim_end(node->Value());
			if (value == "const") {
				const_modifier = true;
			}
			else {
				// Struct parameter C syntax. Not needed in Rust
				assert(value == "struct");
			}
			node = node->NextSibling();
			assert(node);
		}

		// get the pure type
		assert(node->ToElement() && (strcmp(node->Value(), "type") == 0) && node->ToElement()->GetText());
		auto type_it = _types.find(node->ToElement()->GetText());
		assert(type_it != _types.end());
		pure_type = type_it->second;
		complete_type = pure_type->name(); // In case of no pointer

		// end with "*", "**", or "* const*", if needed
		node = node->NextSibling();
		assert(node); // If not text node, at least the name tag (processed elsewhere)
		if (node->ToText())
		{
			std::string value = _trim_end(node->Value());
			assert((value == "*") || (value == "**") || (value == "* const*"));
			if (value == "*") {
				complete_type = _translator->pointer_to(pure_type->name(), const_modifier ? PointerType::CONST_T_P : PointerType::T_P);
			}
			else if (value == "**") {
				complete_type = _translator->pointer_to(pure_type->name(), const_modifier ? PointerType::CONST_T_PP : PointerType::T_PP);
			}
			else {
				assert(value == "* const*");
				complete_type = _translator->pointer_to(pure_type->name(), const_modifier ? PointerType::CONST_T_P_CONST_P : PointerType::T_P_CONST_P);
			}
			node = node->NextSibling();
		}

		return node;
	}

	void Registry::_parse_extension_definition(Extension* e) {
		e->_tag = _extract_tag(e->_name);
		assert(_tags.find(e->_tag) != _tags.end());

		if (strcmp(e->_xml_node->Attribute("supported"), "disabled") == 0) {
			e->_disabled = true;
		}

		if (e->_xml_node->Attribute("type")) {
			std::string extension_type = e->_xml_node->Attribute("type");
			assert(extension_type == "instance" || extension_type == "device");
			e->_association = extension_type == "instance" ? Association::Instance : Association::Device;
		}
		else {
			assert(e->_disabled);
		}

		// The original code used protect, which is a preprocessor define that must be
		// present for the definition. This could be for example VK_USE_PLATFORM_WIN32
		// in order to use Windows surface or external semaphores.

		tinyxml2::XMLElement * child = e->_xml_node->FirstChildElement();
		assert(child && (strcmp(child->Value(), "require") == 0) && !child->NextSiblingElement());
		
		_read_extension_require(child, e);
	}

	// Defines what types, enumerants, and commands are used by an extension
	void Registry::_read_extension_require(tinyxml2::XMLElement * element, Extension* e)
	{
		// TODO: api attribute (ch. 16.1).

		for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			std::string value = child->Value();

			if (value == "command")
			{
				_read_extension_command(child, e);
			}
			else if (value == "type")
			{
				_read_extension_type(child, e);
			}
			else
			{
				assert(value == "enum");
				// For disabled extensions, there is nothing to do for enum
				// because these are new definitions that have not been used
				// anywhere else so there is nothing to remove.
				if (!e->_disabled) {
					_read_extension_enum(child, e);
				}
			}
		}
	}

	void Registry::_read_extension_command(tinyxml2::XMLElement * element, Extension* e)
	{
		char const* name = element->Attribute("name");
		assert(name);
		auto cmd_it = std::find_if(_commands.begin(), _commands.end(), [name](Command* c) -> bool {
			return c->_name == name;
		});
		assert(cmd_it != _commands.end());
		Command* c = *cmd_it;
		e->_commands.push_back(c);
		assert(!c->_extension);
		c->_extension = e;
	}

	void Registry::_read_extension_type(tinyxml2::XMLElement * element, Extension* e)
	{
		// Some types are not found by analyzing dependencies, but the extension
		// may still require some types. These are provided explicitly. One of
		// these is VkBindImageMemorySwapchainInfoKHX which is an extension struct
		// to VkBindImageMemoryInfoKHX and thus is never a direct dependency of
		// another type. However, the extension (VK_KHX_device_group) still adds
		// it, so we collect these types here for when analyzing dependencies.
		assert(element->Attribute("name"));
		auto type_it = _types.find(element->Attribute("name"));
		assert(type_it != _types.end());
		e->_required_types.push_back(type_it->second);
	}

	void Registry::_read_extension_enum(tinyxml2::XMLElement * element, Extension* e)
	{
		assert(element->Attribute("name"));
		std::string name = element->Attribute("name");

		if (element->Attribute("extends"))
		{
			assert(!!element->Attribute("bitpos") + !!element->Attribute("offset") + !!element->Attribute("value") == 1);
			if (element->Attribute("bitpos")) {
				// Find the extended enum so we can add the member to it.
				std::string extends = element->Attribute("extends");
				auto enum_it = std::find_if(_enums.begin(), _enums.end(), [&extends](Enum* e) -> bool {
					return extends == e->_name;
				});
				assert(enum_it != _enums.end());
				assert((*enum_it)->_bitmask);

				Enum::Member m;
				m.name = name;
				m.value = _bitpos_to_value(element->Attribute("bitpos"));

				(*enum_it)->_members.push_back(m);
			}
			else if (element->Attribute("offset")) {
				// The value depends on extension number and offset. See
				// https://www.khronos.org/registry/vulkan/specs/1.0/styleguide.html#_assigning_extension_token_values
				// for calculation.
				int value = 1000000000 + (e->_number - 1) * 1000 + std::stoi(element->Attribute("offset"));

				if (element->Attribute("dir") && strcmp(element->Attribute("dir"), "-") == 0) {
					value = -value;
				}

				std::string value_string = std::to_string(value);

				// Like above, find extended enum to add value
				std::string extends = element->Attribute("extends");
				auto enum_it = std::find_if(_enums.begin(), _enums.end(), [&extends](Enum* e) -> bool {
					return extends == e->_name;
				});
				assert(enum_it != _enums.end());
				assert(!(*enum_it)->_bitmask);

				Enum::Member m;
				m.name = name;
				m.value = value_string;
				
				(*enum_it)->_members.push_back(m);
			}
			else {
				assert(element->Attribute("value"));
				// This is a special case for an enum variant that used to be core.
				// It uses value instead of offset.
				std::string extends = element->Attribute("extends");
				auto enum_it = std::find_if(_enums.begin(), _enums.end(), [&extends](Enum* e) -> bool {
					return extends == e->_name;
				});
				assert(enum_it != _enums.end());
				assert(!(*enum_it)->_bitmask);

				Enum::Member m;
				m.name = name;
				m.value = element->Attribute("value");
				
				(*enum_it)->_members.push_back(m);
			}
		}
		// Inline definition of extension-specific constant.
		else if (element->Attribute("value")) {
			// Unimplemented.
			// All extensions have a constant for spec version and one for the extension
			// name as a string literal. Other than that, some have redefines. I guess
			// I could read the redefines, determine its enum, and mark a variant to
			// use the new name instead.
			//std::cout << "Unimplemented: extension enum with inline constants" << std::endl;
		}
		// Inline definition of extension-specific bitmask value.
		else if (element->Attribute("bitpos")) {
			assert(false); // Not implemented
		}
		// Should be a reference enum, which only supports name and comment. These
		// pull in already existing definitions from other enums blocks. They only
		// seem to be used for purposes of listing items the extension depends on,
		// and since they are defined elsewhere I ignore them.
		else {
			const tinyxml2::XMLAttribute* att = element->FirstAttribute();
			assert(strcmp(att->Name(), "name") == 0 || strcmp(att->Name(), "comment") == 0);
			att = att->Next();
			if (att) {
				assert(strcmp(att->Name(), "name") == 0 || strcmp(att->Name(), "comment") == 0);
				assert(!att->Next());
			}
		}
	}

	void Registry::_parse_feature_definition(Feature * f) {
		for (tinyxml2::XMLElement* child = f->_xml_node->FirstChildElement(); child; child = child->NextSiblingElement()) {
			assert(strcmp(child->Value(), "require") == 0);
			assert(!child->Attribute("profile")); // Profiles are not used yet, so they are not implemented
			assert(!child->Attribute("api")); // Not supported in feature tags

			_read_feature_require(child, f);
		}
	}

	void Registry::_read_feature_require(tinyxml2::XMLElement * element, Feature * f) {
		for (tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
			std::string value = child->Value();

			if (value == "command") {
				_read_feature_command(child, f);
			}
			else if (value == "type") {
				_read_feature_type(child, f);
			}
			else {
				assert(value == "enum");
				_read_feature_enum(child, f);
			}
		}
	}

	void Registry::_read_feature_command(tinyxml2::XMLElement * element, Feature * f) {
		assert(element->Attribute("name"));
		std::string name = element->Attribute("name");
		auto cmd_it = std::find_if(_commands.begin(), _commands.end(), [name](Command* c) -> bool {
			return c->_name == name;
		});
		assert(cmd_it != _commands.end());
		f->_require_command(*cmd_it);
	}

	void Registry::_read_feature_type(tinyxml2::XMLElement * element, Feature * f) {
		// Mostly includes and defines that can be ignored manually I guess.
		// Every now and then there is an actual type that should have been
		// parsed before, and then it seems to be types not used directly by
		// the API. Other types are picked up as dependencies of commands.

		std::set<std::string> ignored = {
			"vk_platform", // include type
			"VK_API_VERSION", // C define to pack version number into a uint32_t (deprecated)
			"VK_API_VERSION_1_0", // C define to pack this version
			"VK_VERSION_MAJOR", // C define to extract major version from packed number
			"VK_VERSION_MINOR", // C define to extract minor version from packed number
			"VK_VERSION_PATCH", // C define to extract patch version from packed number
			"VK_HEADER_VERSION", // I have read this separately and do not treat it as a type
			"VK_NULL_HANDLE", // Defined to 0
		};

		assert(element->Attribute("name"));
		std::string name = element->Attribute("name");

		if (ignored.find(name) != ignored.end()) {
			return;
		}

		auto type_it = _types.find(name);
		assert(type_it != _types.end());
		f->_require_type(type_it->second);
	}

	void Registry::_read_feature_enum(tinyxml2::XMLElement * element, Feature * f) {
		// I have only ever seen reference enums here, that is, pulling in an
		// already existing definition. It makes sense, since the extension enum
		// information says it's an inline definition inside an extensions block.

		const tinyxml2::XMLAttribute* first = element->FirstAttribute();
		assert(first);
		const tinyxml2::XMLAttribute* second = first->Next();

		std::string enum_name;
		if (strcmp(first->Name(), "name") == 0) {
			enum_name = first->Value();

			if (second) {
				assert(strcmp(second->Name(), "comment") == 0);
				assert(!second->Next());
			}
		}
		else {
			assert(strcmp(first->Name(), "comment") == 0);
			assert(second && strcmp(second->Name(), "name") == 0 && !second->Next());
			enum_name = second->Value();
		}

		// I think these should always be API constants. Actual enums are read
		// as types. Unless of course a subset is required, in which case I would
		// have to revise how I deal with enums. This would likely lead to adding
		// an Enumeration item type so that I can find them individually and
		// have enum members be objects of this type.
		auto item_it = std::find_if(_api_constants.begin(), _api_constants.end(), [&enum_name](ApiConstant* a) -> bool {
			return a->_name == enum_name;
		});
		assert(item_it != _api_constants.end());
		f->_require_enum(*item_it);
	}

	void Registry::_build_dependency_chain() {
		// First we build a na�ve dependency chain that collects dependencies
		// in the order they are used by functions. While this chain is likely
		// a mess of interleaved types, it allows us to scan the types in the
		// order used by the API and have the same relative order within groups
		// produced here. This is also where stuff is marked if they are core
		// or extension.
		std::vector<Type*> ungrouped_dependency_chain;
		_build_ungrouped_dependency_chain(ungrouped_dependency_chain);

		std::set<std::string> all_added_dependencies;
		std::set<std::string> current_added_dependencies;
		int new_types = 0;

		// Ultimately, everything depend on C types which are expected to have
		// no dependencies themselves, so we begin by adding those.
		for (auto& c : _c_types) {
			auto type_it = _types.find(c);
			assert(type_it != _types.end());
			_dependency_chain.push_back(type_it->second);
			assert(all_added_dependencies.insert(c).second == true);
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

				if (type->_all_dependencies_in_set(all_added_dependencies)) {
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

	// TODO: These dependency chain things (one other method as well) should be
	// fairly unnecesary with features, as the dependency chain is built when
	// items are added to the feature. I do however want to mark things as core
	// or extension, so I probably want to be able to call _for_each_item with
	// a lambda on dependencies of core functions.
	void Registry::_build_ungrouped_dependency_chain(std::vector<Type*>& chain) {
		std::vector<Type*> current_sub_chain;
		std::set<std::string> added_dependencies;

		for (auto c : _commands) {
			c->_return_type_pure->_build_dependency_chain(current_sub_chain);
			for (auto t : current_sub_chain) {
				if (added_dependencies.insert(t->_name).second == true) {
					chain.push_back(t);
				}

				// If core function, all dependencies also belong in core. Note
				// that an extension function does not imply that dependencies
				// are also in extension. Core types can of course be used!
				assert(t->_api_part == ApiPart::Unspecified || t->_api_part == ApiPart::Core);
				if (!c->_extension) {
					t->_api_part = ApiPart::Core;
				}
			}
			current_sub_chain.clear();

			for (auto& p : c->_params) {
				p.pure_type->_build_dependency_chain(current_sub_chain);
				for (auto t : current_sub_chain) {
					if (added_dependencies.insert(t->_name).second == true) {
						chain.push_back(t);
					}

					// Like above
					assert(t->_api_part == ApiPart::Unspecified || t->_api_part == ApiPart::Core);
					if (!c->_extension) {
						t->_api_part = ApiPart::Core;
					}
				}
				current_sub_chain.clear();
			}
		}

		// Some types are never used directly and will thus not be collected in
		// the dependency chain. This could be VkDispatchIndirectCommand which
		// defines elements used in raw buffers of indirect calls or structs
		// used as extension structs (pNext). These are all present in require
		// tags of feature, which could be included in the future. When that
		// happens, this piece of code should not be required.
		std::vector<std::string> manual_dependencies = {
			"VkDispatchIndirectCommand",
			"VkDrawIndexedIndirectCommand",
			"VkDrawIndirectCommand",
			"VkPipelineCacheHeaderVersion",
		};
		for (auto a : _api_constants) {
			manual_dependencies.push_back(a->_name);
		}
		for (auto& dep : manual_dependencies) {
			auto type_it = _types.find(dep);
			assert(type_it != _types.end());
			type_it->second->_build_dependency_chain(current_sub_chain);
			for (auto t : current_sub_chain) {
				if (added_dependencies.insert(t->_name).second == true) {
					chain.push_back(t);
				}

				// Like above
				assert(t->_api_part == ApiPart::Unspecified || t->_api_part == ApiPart::Core);
				t->_api_part = ApiPart::Core;
			}
			current_sub_chain.clear();
		}

		// All commands have been processed, and as such everything used in the
		// core API has been marked. As a consequence, every unspecified type
		// can now be marked as belonging to an extension.
		for (auto& t : _types) {
			if (t.second->_api_part == ApiPart::Core) {
				continue;
			}

			assert(t.second->_api_part == ApiPart::Unspecified);
			t.second->_api_part = ApiPart::Extension;
		}

		// Finish off dependency chain generation by adding types not used
		// directly (they have not been covered in the previous step) but still
		// required. These types were parsed during extension definitions.
		for (auto e : _extensions) {
			for (auto t : e->_required_types) {
				t->_build_dependency_chain(current_sub_chain);
				for (auto t : current_sub_chain) {
					if (added_dependencies.insert(t->_name).second == true) {
						chain.push_back(t);
					}
				}
				current_sub_chain.clear();
			}
		}

		// Check that all types are represented in the dependency chain
		for (auto t : chain) {
			assert(_types.find(t->_name) != _types.end());
		}
		assert(_types.size() == chain.size());
	}

	void Registry::_mark_extension_items() {
		// Get all extension types. The idea is to successively erase elements
		// of the map as they are depended on by some extension. The first time
		// a type is used is assumed to be the extension that added it.
		std::map<std::string, Type*> extension_types;
		for (auto& t : _types) {
			assert(t.second->_extension == nullptr); // not yet added

			if (t.second->_api_part == ApiPart::Core) {
				continue;
			}

			assert(t.second->_api_part == ApiPart::Extension);

			// C types only used by extensions will have the same API part and
			// will reach here. We are not interested in those as no extension
			// actually adds them.
			if (_c_types.find(t.first) != _c_types.end()) {
				continue;
			}

			// All extension types should match this pattern. This is a second
			// safeguard to make sure no core types have been missed previously.
			std::regex re(R"(^(PFN_v|V)k[A-Z][a-zA-Z0-9]+[a-z0-9]([A-Z][A-Z]+)$)");
			auto it = std::sregex_iterator(t.first.begin(), t.first.end(), re);
			auto end = std::sregex_iterator();
			assert(it != end);
			std::smatch match = *it;
			assert(_tags.find(match[2].str()) != _tags.end());

			assert(extension_types.insert(t).second == true);
		}

		// For each extension iterated in order, its dependency set is built
		// and matched against extension types. If any type is found, that type
		// is removed from remaining extension types and marked as being added
		// by the extension in question.
		std::vector<Type*> current_dep_chain;
		for (auto e : _extensions) {
			assert(e->_types.empty());

			for (auto c : e->_commands) {
				c->_return_type_pure->_build_dependency_chain(current_dep_chain);
				for (auto& p : c->_params) {
					p.pure_type->_build_dependency_chain(current_dep_chain);
				}
			}

			for (auto t : e->_required_types) {
				t->_build_dependency_chain(current_dep_chain);
			}

			for (auto t : current_dep_chain) {
				assert(t->_api_part == ApiPart::Core || t->_api_part == ApiPart::Extension);

				// If we could remove one of the dependencies from the map, it
				// means we have found the first extension where it's used and
				// we indicate that this extension adds the type.
				if (extension_types.erase(t->_name) > 0) {
					assert(t->_extension == nullptr);
					t->_extension = e;
					e->_types.push_back(t);
				}
			}

			current_dep_chain.clear();
		}

		// By now all extension types should have been added to some extension.
		assert(extension_types.empty());
	}

	void Registry::_sort_types() {
		std::sort(_scalar_typedefs.begin(), _scalar_typedefs.end(), [](ScalarTypedef* t1, ScalarTypedef* t2) {
			return t1->_dependency_order < t2->_dependency_order;
		});

		std::sort(_function_typedefs.begin(), _function_typedefs.end(), [](FunctionTypedef* t1, FunctionTypedef* t2) {
			return t1->_dependency_order < t2->_dependency_order;
		});

		std::sort(_bitmasks.begin(), _bitmasks.end(), [](Bitmasks* t1, Bitmasks* t2) {
			return t1->_dependency_order < t2->_dependency_order;
		});

		std::sort(_handle_typedefs.begin(), _handle_typedefs.end(), [](HandleTypedef* t1, HandleTypedef* t2) {
			return t1->_dependency_order < t2->_dependency_order;
		});

		std::sort(_structs.begin(), _structs.end(), [](Struct* t1, Struct* t2) {
			return t1->_dependency_order < t2->_dependency_order;
		});

		std::sort(_enums.begin(), _enums.end(), [](Enum* t1, Enum* t2) {
			return t1->_dependency_order < t2->_dependency_order;
		});
	}

	std::string Registry::_read_array_size(tinyxml2::XMLNode * node, std::string& name)
	{
		std::string arraySize;
		if (name.back() == ']') // Can happen for example [4] in unions
		{
			// if the parameter has '[' and ']' in its name, get the stuff inbetween those as the array size and erase that part from the parameter name
			assert(!node->NextSibling());
			size_t pos = name.find('[');
			assert(pos != std::string::npos);
			arraySize = name.substr(pos + 1, name.length() - 2 - pos);
			name.erase(pos);
		}
		else
		{
			// otherwise look for a sibling of this node
			node = node->NextSibling();
			if (node && node->ToText())
			{
				std::string value = node->Value();
				if (value == "[")
				{
					// if this node has '[' as its value, the next node holds the array size, and the node after that needs to hold ']', and there should be no more siblings
					node = node->NextSibling();
					assert(node && node->ToElement() && (strcmp(node->Value(), "enum") == 0));
					arraySize = node->ToElement()->GetText();
					node = node->NextSibling();
					assert(node && node->ToText() && (strcmp(node->Value(), "]") == 0) && !node->NextSibling());
				}
				else
				{
					// otherwise, the node holds '[' and ']', so get the stuff in between those as the array size
					assert((value.front() == '[') && (value.back() == ']'));
					arraySize = value.substr(1, value.length() - 2);
					assert(!node->NextSibling());
				}
			}
		}
		return arraySize;
	}

	// trim from end
	std::string Registry::_trim_end(std::string const& input)
	{
		std::string result = input;
		result.erase(std::find_if(result.rbegin(), result.rend(), [](char c) { return !std::isspace(c); }).base(), result.end());
		return result;
	}

	std::string Registry::_extract_tag(std::string const& name)
	{
		// the name is supposed to look like: VK_<tag>_<other>
		size_t start = name.find('_');
		assert((start != std::string::npos) && (name.substr(0, start) == "VK"));
		size_t end = name.find('_', start + 1);
		assert(end != std::string::npos);
		return name.substr(start + 1, end - start - 1);
	}

	std::string Registry::_bitpos_to_value(std::string const& bitpos) {
		int pos = std::stoi(bitpos);
		uint32_t flag = 1 << pos;
		std::stringstream s;
		s << "0x" << std::setfill('0') << std::setw(sizeof(uint32_t) * 2) << std::hex << flag;
		return s.str();
	}

	void Registry::_build_feature(Feature * f) {

	}

} // vkspec
