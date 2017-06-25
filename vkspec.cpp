#include "vkspec.h"

#include <algorithm>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

namespace vkspec {

	void Registry::add_c_type(std::string const& c, std::string const& translation) {
		_c_types[c] = translation;
	}

	void Registry::parse(std::string const& spec) {
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

		// The root tag contains zero or more of the following tags. Order may change.
		for (tinyxml2::XMLElement * child = registryElement->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			assert(child->Value());
			const std::string value = child->Value();

			if (value == "comment")
			{
				// Get the vulkan license header and skip any leading spaces
				_read_comment(child);
				_license_header.erase(_license_header.begin(), std::find_if(_license_header.begin(), _license_header.end(), [](char c) { return !std::isspace(c); }));
			}
			else if (value == "tags")
			{
				// Author IDs for extensions and layers
				_read_tags(child);
			}
			else if (value == "types")
			{
				// Defines API types
				_read_types(child);
			}
			else if (value == "enums")
			{
				// Enum definitions
				_read_enums(child);
			}
			else if (value == "commands")
			{
				// Function definitions
				_read_commands(child);
			}
			else if (value == "extensions")
			{
				// Extension interfaces
				_read_extensions(child);
			}
			else
			{
				assert((value == "feature") || (value == "vendorids"));
			}
		}

		_undefined_check();

		_mark_extension_items();
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
			assert(strcmp(child->Value(), "type") == 0);
			std::string type = child->Value();
			assert(type == "type");

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
					// enum: These are covered later in 'registry > enums' tags so I ignore them here.
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
		tinyxml2::XMLElement * typeElement = element->FirstChildElement();
		assert(typeElement && (strcmp(typeElement->Value(), "type") == 0) && typeElement->GetText());
		std::string type = typeElement->GetText();
		assert(type == "uint32_t" || type == "uint64_t");

		std::string const& underlying = _type_reference(type);

		tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
		assert(nameElement && (strcmp(nameElement->Value(), "name") == 0) && nameElement->GetText());
		std::string newType = nameElement->GetText();

		_define_scalar_typedef(newType, underlying);
	}

	void Registry::_read_type_bitmask(tinyxml2::XMLElement * element)
	{
		assert(strcmp(element->GetText(), "typedef ") == 0);
		tinyxml2::XMLElement * typeElement = element->FirstChildElement();
		assert(typeElement && (strcmp(typeElement->Value(), "type") == 0) && typeElement->GetText() && (strcmp(typeElement->GetText(), "VkFlags") == 0));
		std::string type = typeElement->GetText();

		tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
		assert(nameElement && (strcmp(nameElement->Value(), "name") == 0) && nameElement->GetText());
		std::string name = nameElement->GetText();

		assert(!nameElement->NextSiblingElement());

		// Requires contains the type that will eventually hold definitions (with
		// a name containing FlagBits). Oftentimes however, a type containing Flags
		// is used instead. This separation is done to indicate that several flags
		// can be used as opposed to just one. For C it's implemented as a regular
		// typedef, but since some of the Flag types do not have members, they are
		// not present in the enums tags. The C typedef makes them work anyway, but
		// in Rust where I gain a little more type safety I at least need to make
		// the oracle aware that these types are bitmask typedefs so that their
		// structs will be generated, albeit with no way to create them with flags.

		std::string bit_definitions = "";
		if (element->Attribute("requires")) {
			// I don't define bitmasks here, but rather when parsing its members.
			// Non-existant definitions should not be a requirement, so this turns
			// into an extra check that the type is not undefined later.
			bit_definitions = _type_reference(element->Attribute("requires"));
		}

		_define_bitmask_typedef(name, bit_definitions);
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
		// The typedef <ret> text node
		tinyxml2::XMLNode * node = element->FirstChild();
		assert(node && node->ToText());
		std::string text = node->Value();

		// This will match 'typedef TYPE (VKAPI_PTR *' and contain TYPE in match
		// group 1.
		std::regex re(R"(^typedef ([^ ^\*]+)(\*)? \(VKAPI_PTR \*$)");
		auto it = std::sregex_iterator(text.begin(), text.end(), re);
		auto end = std::sregex_iterator();
		assert(it != end);
		std::smatch match = *it;
		std::string returnType = _type_reference(match[1].str());
		if (match[2].matched) {
			returnType = _translator->pointer_to(returnType, PointerType::T_P);
		}

		// name tag containing the type def name
		node = node->NextSibling();
		assert(node && node->ToElement());
		tinyxml2::XMLElement * tag = node->ToElement();
		assert(tag && strcmp(tag->Value(), "name") == 0 && tag->GetText());
		std::string name = tag->GetText();
		assert(!tag->FirstChildElement());

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
			std::string paramType = tag->GetText();
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
			std::string paramName = match[2].str();
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

			std::string t = _type_reference(paramType);

			if (constModifier) {
				assert(pointer);
			}

			if (pointer) {
				t = _translator->pointer_to(t, constModifier ? PointerType::CONST_T_P : PointerType::T_P);
			}

			params.push_back({
				t,
				paramName,
			});
		}

		FunctionTypedef* t = _define_function_typedef(name, returnType);
		for (auto p : params) {
			t->params.push_back(p);
		}
	}

	void Registry::_read_type_handle(tinyxml2::XMLElement * element)
	{
		tinyxml2::XMLElement * typeElement = element->FirstChildElement();
		assert(typeElement && (strcmp(typeElement->Value(), "type") == 0) && typeElement->GetText());
		std::string type = typeElement->GetText();
		std::string underlying = "";
		if (type == "VK_DEFINE_HANDLE") { // Defined as pointer meaning varying size
			underlying = _type_reference("size_t");
		}
		else {
			assert(type == "VK_DEFINE_NON_DISPATCHABLE_HANDLE"); // Pointer on 64-bit and uint64_t otherwise -> always 64 bit
			underlying = _type_reference("uint64_t");
		}

		tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
		assert(nameElement && (strcmp(nameElement->Value(), "name") == 0) && nameElement->GetText());
		std::string name = nameElement->GetText();

		_define_handle_typedef(name, underlying);
	}

	void Registry::_read_type_struct(tinyxml2::XMLElement * element, bool isUnion)
	{
		assert(!element->Attribute("returnedonly") || (strcmp(element->Attribute("returnedonly"), "true") == 0));

		assert(element->Attribute("name"));
		std::string name = element->Attribute("name");

		// element->Attribute("returnedonly") is also applicable for structs and unions
		Struct* t = _define_struct(name, isUnion);

		for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			assert(child->Value() && strcmp(child->Value(), "member") == 0);
			_read_type_struct_member(t, child);
		}
	}

	// Read a member tag of a struct, adding members to the provided struct.
	void Registry::_read_type_struct_member(Struct* theStruct, tinyxml2::XMLElement * element) {
		// The attributes of member tags seem to mostly concern documentation
		// generation, so they are not of interest for the bindings.

		// Read the type, parsing modifiers to get a string of the type.
		std::string type;
		tinyxml2::XMLNode* child = _read_type_struct_member_type(element->FirstChild(), type);

		// After we have parsed the type we expect to find the name of the member
		assert(child->ToElement() && strcmp(child->Value(), "name") == 0 && child->ToElement()->GetText());
		std::string memberName = child->ToElement()->GetText();

		// Some members have more information about array size
		std::string arraySize = _read_array_size(child, memberName);
		if (arraySize != "") {
			type = _translator->array_member(type, arraySize);
		}

		// Add member to struct
		Struct::Member m;
		m.type = type;
		m.name = memberName;
		theStruct->members.push_back(m);
	}

	// Reads the type tag of a member tag, including potential text nodes around
	// the type tag to get qualifiers. We pass the first node that could potentially
	// be a text node.
	tinyxml2::XMLNode* Registry::_read_type_struct_member_type(tinyxml2::XMLNode* element, std::string& type)
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
		type = _type_reference(element->ToElement()->GetText());

		element = element->NextSibling();
		assert(element);
		if (element->ToText())
		{
			std::string value = _trim_end(element->Value());
			assert((value == "*") || (value == "**") || (value == "* const*"));
			if (value == "*") {
				type = _translator->pointer_to(type, constant ? PointerType::CONST_T_P : PointerType::T_P);
			}
			else if (value == "**") {
				type = _translator->pointer_to(type, constant ? PointerType::CONST_T_PP : PointerType::T_PP);
			}
			else {
				assert(value == "* const*");
				type = _translator->pointer_to(type, constant ? PointerType::CONST_T_P_CONST_P : PointerType::T_P_CONST_P);
			}
			element = element->NextSibling();
		}
		else {
			// Should not have const qualifier without pointer
			assert(!constant);
		}

		return element;
	}

	void Registry::_read_enums(tinyxml2::XMLElement * element)
	{
		if (!element->Attribute("name"))
		{
			throw std::runtime_error(std::string("spec error: enums element is missing the name attribute"));
		}

		std::string name = element->Attribute("name");

		// Represents hardcoded constants.
		if (name == "API Constants") {
			_read_api_constants(element);
			return;
		}

		if (!element->Attribute("type"))
		{
			throw std::runtime_error(std::string("spec error: enums name=\"" + name + "\" is missing the type attribute"));
		}

		std::string type = element->Attribute("type");

		if (type != "bitmask" && type != "enum")
		{
			throw std::runtime_error(std::string("spec error: enums name=\"" + name + "\" has unknown type " + type));
		}

		if (type == "bitmask") {
			Bitmasks* t = _define_bitmasks(name);

			_read_enums_bitmask(element, [t, this](std::string const& member, std::string const& value, bool isBitpos) {
				Bitmasks::Member m;
				m.name = member;
				m.value = isBitpos ? _bitpos_to_value(value) : value;
				t->members.push_back(m);
			});
		}
		else {
			Enum* t = _define_enum(name);

			_read_enums_enum(element, [t](std::string const& member, std::string const& value) {
				Enum::Member m;
				m.name = member;
				m.value = value;
				t->members.push_back(m);
			});
		}
	}

	void Registry::_read_api_constants(tinyxml2::XMLElement* element) {
		for (tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
			assert(child->Attribute("value") && child->Attribute("name"));
			std::string value = child->Attribute("value");
			std::string constant = child->Attribute("name");

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
				std::string dataType = match[1].matched ? "i32" : "u32";
				_api_constants.push_back(std::make_tuple(constant, dataType, value));
				continue;
			}

			re = std::regex(R"(^[0-9]+\.[0-9]+f$)");
			it = std::sregex_iterator(value.begin(), value.end(), re);

			// Matched float
			if (it != end) {
				value.pop_back();
				_api_constants.push_back(std::make_tuple(constant, "f32", value));
				continue;
			}

			// The rest
			if (value == "(~0U)") {
				_api_constants.push_back(std::make_tuple(constant, "u32", "~0"));
			}
			else if (value == "(~0ULL)") {
				_api_constants.push_back(std::make_tuple(constant, "u64", "~0"));
			}
			else {
				assert(value == "(~0U-1)");
				_api_constants.push_back(std::make_tuple(constant, "u32", "~0u32 - 1"));
			}
		}
	}

	void Registry::_read_enums_bitmask(tinyxml2::XMLElement * element, std::function<void(const std::string& member, const std::string& value, bool isBitpos)> make)
	{
		// read the names of the enum values
		tinyxml2::XMLElement * child = element->FirstChildElement();
		while (child)
		{
			if (strcmp(child->Value(), "unused") == 0) {
				child = child->NextSiblingElement();
				continue;
			}

			assert(child->Attribute("name"));
			std::string name = child->Attribute("name");

			std::string value;
			bool bitpos;
			if (child->Attribute("bitpos")) {
				value = child->Attribute("bitpos");
				bitpos = true;
				assert(!child->Attribute("value"));
			}
			else {
				assert(child->Attribute("value"));
				value = child->Attribute("value"); // Can be arbitrary string but I don't consider that for now
				bitpos = false;
			}

			make(name, value, bitpos);

			child = child->NextSiblingElement();
		}
	}

	void Registry::_read_enums_enum(tinyxml2::XMLElement * element, std::function<void(const std::string& member, const std::string& value)> make)
	{
		// read the names of the enum values
		tinyxml2::XMLElement * child = element->FirstChildElement();
		while (child)
		{
			if (strcmp(child->Value(), "unused") == 0) {
				child = child->NextSiblingElement();
				continue;
			}

			assert(child->Attribute("name") && child->Attribute("value"));
			make(child->Attribute("name"), child->Attribute("value")); // Well, apparently values can be arbitrary string. That could be interesting
			child = child->NextSiblingElement();
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

		Command* cmd = _read_command_proto(child);
		_read_command_params(child, cmd);
	}

	Command* Registry::_read_command_proto(tinyxml2::XMLElement * element)
	{
		// Defines the return type and name of a command

		// Get type and name tags, making sure there are no text nodes inbetween.
		tinyxml2::XMLNode* node = element->FirstChild();
		assert(node && node->ToElement());
		tinyxml2::XMLElement * typeElement = node->ToElement();
		assert(typeElement && (strcmp(typeElement->Value(), "type") == 0));
		node = typeElement->NextSibling();
		assert(node && node->ToElement());
		tinyxml2::XMLElement * nameElement = node->ToElement();
		assert(nameElement && (strcmp(nameElement->Value(), "name") == 0));
		assert(!nameElement->NextSibling());

		// get return type and name of the command
		std::string type = _type_reference(typeElement->GetText());
		std::string name = nameElement->GetText();

		return _define_command(name, type);
	}

	void Registry::_read_command_params(tinyxml2::XMLElement* element, Command* cmd)
	{
		// iterate over the siblings of the element and read the command parameters
		assert(element);
		while (element = element->NextSiblingElement())
		{
			std::string value = element->Value();
			if (value == "param")
			{
				_read_command_param(element, cmd);
			}
			else
			{
				// ignore these values!
				assert((value == "implicitexternsyncparams") || (value == "validity"));
			}
		}
	}

	void Registry::_read_command_param(tinyxml2::XMLElement * element, Command* cmd)
	{
		std::string type;
		tinyxml2::XMLNode * afterType = _read_command_param_type(element->FirstChild(), type);

		assert(afterType->ToElement() && (strcmp(afterType->Value(), "name") == 0) && afterType->ToElement()->GetText());
		std::string name = afterType->ToElement()->GetText();

		std::string arraySize = _read_array_size(afterType, name);
		if (arraySize != "") {
			type = _translator->array_param(type, arraySize);
		}

		Command::Parameter p;
		p.type = type;
		p.name = name;
		cmd->params.push_back(p);
	}

	tinyxml2::XMLNode* Registry::_read_command_param_type(tinyxml2::XMLNode* node, std::string& type)
	{
		bool constModifier = false;

		assert(node);
		if (node->ToText())
		{
			// start type with "const" or "struct", if needed
			std::string value = _trim_end(node->Value());
			if (value == "const") {
				constModifier = true;
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
		type = _type_reference(node->ToElement()->GetText());

		// end with "*", "**", or "* const*", if needed
		node = node->NextSibling();
		assert(node); // If not text node, at least the name tag (processed elsewhere)
		if (node->ToText())
		{
			std::string value = _trim_end(node->Value());
			assert((value == "*") || (value == "**") || (value == "* const*"));
			if (value == "*") {
				type = _translator->pointer_to(type, constModifier ? PointerType::CONST_T_P : PointerType::T_P);
			}
			else if (value == "**") {
				type = _translator->pointer_to(type, constModifier ? PointerType::CONST_T_PP : PointerType::T_PP);
			}
			else {
				assert(value == "* const*");
				type = _translator->pointer_to(type, constModifier ? PointerType::CONST_T_P_CONST_P : PointerType::T_P_CONST_P);
			}
			node = node->NextSibling();
		}

		return node;
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
		Extension ext;

		assert(element->Attribute("name") && element->Attribute("number"));
		ext.name = element->Attribute("name");
		ext.number = element->Attribute("number");
		ext.tag = _extract_tag(ext.name);
		assert(_tags.find(ext.tag) != _tags.end());

		if (element->Attribute("type")) {
			ext.type = element->Attribute("type");
			assert(ext.type == "instance" || ext.type == "device");
		}

		// The original code used protect, which is a preprocessor define that must be
		// present for the definition. This could be for example VK_USE_PLATFORM_WIN32
		// in order to use Windows surface or external semaphores.

		tinyxml2::XMLElement * child = element->FirstChildElement();
		assert(child && (strcmp(child->Value(), "require") == 0) && !child->NextSiblingElement());

		if (strcmp(element->Attribute("supported"), "disabled") == 0)
		{
			ext.disabled = true;

			// Types and commands of disabled extensions should not be present in the
			// final bindings, so mark them as disabled.
			_read_disabled_extension_require(child, ext);
		}
		else
		{
			_read_extension_require(child, ext.tag, ext);
		}

		_define_extension(std::move(ext));
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

	void Registry::_read_disabled_extension_require(tinyxml2::XMLElement * element, Extension& ext)
	{
		for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			std::string value = child->Value();

			// Add commands and types, but skip enums. Commands and types are
			// defined elsewhere in the spec, and thus must be told to not be
			// included in the bindings. This is done after parsing, so for now
			// only the type names are stored to find them later.
			if (value == "command") {
				assert(child->Attribute("name"));
				std::string name = child->Attribute("name");
				ext.commands.push_back(name);
			}
			else if (value == "type") {
				assert(child->Attribute("name"));
				std::string name = child->Attribute("name");
				ext.types.push_back(name);
			}
			else {
				// nothing to do for enums, no other values ever encountered
				assert(value == "enum");
			}
		}
	}

	// Defines what types, enumerants, and commands are used by an extension
	void Registry::_read_extension_require(tinyxml2::XMLElement * element, std::string const& tag, Extension& ext)
	{
		for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			std::string value = child->Value();

			if (value == "command")
			{
				_read_extension_command(child, ext.commands);
			}
			else if (value == "type")
			{
				_read_extension_type(child, ext);
			}
			else
			{
				assert(value == "enum");
				_read_extension_enum(child, ext.number);
			}
		}
	}

	void Registry::_read_extension_command(tinyxml2::XMLElement * element, std::vector<std::string>& extensionCommands)
	{
		// Command is marked as belonging to an extension after parsing is done
		assert(element->Attribute("name"));
		std::string t = _type_reference(element->Attribute("name"));
		extensionCommands.push_back(t);
	}

	void Registry::_read_extension_type(tinyxml2::XMLElement * element, Extension& ext)
	{
		// Save the type name so we can mark it as belonging to an extension.
		assert(element->Attribute("name"));
		std::string t = _type_reference(element->Attribute("name"));
		ext.types.push_back(t);
	}

	void Registry::_read_extension_enum(tinyxml2::XMLElement * element, std::string const& extensionNumber)
	{
		assert(element->Attribute("name"));
		std::string name = element->Attribute("name");

		if (element->Attribute("extends"))
		{
			assert(!!element->Attribute("bitpos") + !!element->Attribute("offset") + !!element->Attribute("value") == 1);
			if (element->Attribute("bitpos")) {
				// Find the extended enum so we can add the member to it. It
				// should exist, but if it does not, a solution is to simply save
				// the enum data and extend the enum in the post-parse cleanup
				// pass where we mark commands as extension commands.
				std::string extends = element->Attribute("extends");
				_type_reference(extends);
				auto it = std::find_if(_bitmasks.begin(), _bitmasks.end(), [&extends](Bitmasks* bitmask) -> bool {
					return bitmask->name == extends;
				});
				assert(it != _bitmasks.end());
				Bitmasks::Member m;
				m.name = name;
				m.value = _bitpos_to_value(element->Attribute("bitpos"));
				(*it)->members.push_back(m);
			}
			else if (element->Attribute("offset")) {
				// The value depends on extension number and offset. See
				// https://www.khronos.org/registry/vulkan/specs/1.0/styleguide.html#_assigning_extension_token_values
				// for calculation.
				int value = 1000000000 + (std::stoi(extensionNumber) - 1) * 1000 + std::stoi(element->Attribute("offset"));

				if (element->Attribute("dir") && strcmp(element->Attribute("dir"), "-") == 0) {
					value = -value;
				}

				std::string valueString = std::to_string(value);

				// If type does not exist, same goes as above
				std::string extends = element->Attribute("extends");
				_type_reference(extends);
				auto it = std::find_if(_enums.begin(), _enums.end(), [&extends](Enum* e) -> bool {
					return e->name == extends;
				});
				assert(it != _enums.end());
				Enum::Member m;
				m.name = name;
				m.value = valueString;
				(*it)->members.push_back(m);
			}
			else {
				// This is a special case for an enum variant that used to be core.
				// It uses value instead of offset.
				std::string extends = element->Attribute("extends");
				_type_reference(extends);
				auto it = std::find_if(_enums.begin(), _enums.end(), [&extends](Enum* e) -> bool {
					return e->name == extends;
				});
				assert(it != _enums.end());
				Enum::Member m;
				m.name = name;
				m.value = element->Attribute("value");
				(*it)->members.push_back(m);
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

	ScalarTypedef* Registry::_define_scalar_typedef(std::string const& alias, std::string const& actual) {
		_define(alias, ItemType::ScalarTypedef);
		ScalarTypedef* t = new ScalarTypedef;
		t->alias = alias;
		t->actual = actual;
		_scalar_typedefs.push_back(t);
		return t;
	}

	FunctionTypedef* Registry::_define_function_typedef(std::string const& alias, std::string const& return_type) {
		_define(alias, ItemType::FunctionTypedef);
		FunctionTypedef* t = new FunctionTypedef;
		t->alias = alias;
		t->return_type = return_type;
		_function_typedefs.push_back(t);
		return t;
	}

	Bitmasks* Registry::_define_bitmasks(std::string const& name) {
		_define(name, ItemType::Bitmasks);
		Bitmasks* t = new Bitmasks;
		t->name = name;
		_bitmasks.push_back(t);
		return t;
	}

	BitmaskTypedef* Registry::_define_bitmask_typedef(std::string const& alias, std::string const& bit_definitions) {
		_define(alias, ItemType::BitmaskTypedef);
		BitmaskTypedef* t = new BitmaskTypedef;
		t->alias = alias;
		t->bit_definitions = bit_definitions;
		_bitmask_typedefs.push_back(t);
		return t;
	}

	HandleTypedef* Registry::_define_handle_typedef(std::string const& alias, std::string const& actual) {
		_define(alias, ItemType::HandleTypedef);
		HandleTypedef* t = new HandleTypedef;
		t->alias = alias;
		t->actual = actual;
		_handle_typedefs.push_back(t);
		return t;
	}

	Struct* Registry::_define_struct(std::string const& name, bool is_union) {
		_define(name, ItemType::Struct);
		Struct* t = new Struct;
		t->name = name;
		t->is_union = is_union;
		_structs.push_back(t);
		return t;
	}

	Enum* Registry::_define_enum(std::string const& name) {
		_define(name, ItemType::Enum);
		Enum* t = new Enum;
		t->name = name;
		_enums.push_back(t);
		return t;
	}

	Command* Registry::_define_command(std::string const& name, std::string const& return_type) {
		_define(name, ItemType::Command);
		Command* t = new Command;
		t->name = name;
		t->return_type = return_type;
		_commands.push_back(t);
		return t;
	}

	void Registry::_define_extension(Extension const&& ext) {
		assert(_extensions.insert(std::make_pair(ext.name, ext)).second == true);
	}

	std::string const& Registry::_type_reference(const std::string& type) {
		assert(type.find_first_of("* ") == std::string::npos);

		auto c = _c_types.find(type);
		if (c != _c_types.end()) {
			return c->second; // translation
		}

		if (_defined_types.find(type) == _defined_types.end()) {
			_undefined_types.insert(type);
		}

		return type;
	}

	void Registry::_define(std::string const& name, ItemType item_type) {
		assert(_defined_types.find(name) == _defined_types.end());
		_defined_types.insert(std::make_pair(name, item_type));
		_undefined_types.erase(name);
	}

	void Registry::_undefined_check() {
		assert(_undefined_types.empty());
	}

	void Registry::_mark_extension_items() {
		for (auto& ext : _extensions) {
			for (auto& type : ext.second.types) {
				// Get the type of item so we know where to find the implementation
				auto item_type_it = _defined_types.find(type);
				assert(item_type_it != _defined_types.end());

				// Get the actual item to work with
				ExtensionItem* item = nullptr;
				switch (item_type_it->second) {
				case ItemType::Struct: {
					auto item_it = std::find_if(_structs.begin(), _structs.end(), [&type](Struct const* s) -> bool {
						return type == s->name;
					});
					assert(item_it != _structs.end());
					item = *item_it;
					break;
				}
				case ItemType::Enum: {
					auto item_it = std::find_if(_enums.begin(), _enums.end(), [&type](Enum const* e) -> bool {
						return type == e->name;
					});
					assert(item_it != _enums.end());
					item = *item_it;
					break;
				}
				case ItemType::BitmaskTypedef: {
					auto item_it = std::find_if(_bitmask_typedefs.begin(), _bitmask_typedefs.end(), [&type](BitmaskTypedef const* b) -> bool {
						return type == b->alias;
					});
					assert(item_it != _bitmask_typedefs.end());
					item = *item_it;
					break;
				}
				case ItemType::Bitmasks: {
					auto item_it = std::find_if(_bitmasks.begin(), _bitmasks.end(), [&type](Bitmasks const* b) -> bool {
						return type == b->name;
					});
					assert(item_it != _bitmasks.end());
					item = *item_it;
					break;
				}
				case ItemType::HandleTypedef: {
					auto item_it = std::find_if(_handle_typedefs.begin(), _handle_typedefs.end(), [&type](HandleTypedef const* h) -> bool {
						return type == h->alias;
					});
					assert(item_it != _handle_typedefs.end());
					item = *item_it;
					break;
				}
				default: throw std::runtime_error("The type of '" + type + "' not implemented when marking extension types.");
				}

				// TODO: This is faulty logic for types. The type tag inside require
				// of an extension does not mean that the extension defines the
				// type. This is some kind of requirement thing for the extension
				// that is not properly explained and does not seem to be consistant.
				// One example is VkDebugReportObjectTypeEXT which is a type tag of
				// both VK_EXT_debug_report (nr 12) and VK_EXT_debug_marker (nr 23).
				//assert(!item->extension);
				//item->extension = true;
				//item->disabled = ext.second.disabled;
			}

			for (auto& command_name : ext.second.commands) {
				auto command_it = std::find_if(_commands.begin(), _commands.end(), [&command_name](Command const* cmd) -> bool {
					return command_name == cmd->name;
				});
				assert(command_it != _commands.end());
				ExtensionItem* item = *command_it;
				assert(!item->extension);
				item->extension = true;
				item->disabled = ext.second.disabled;
			}
		}
	}

	std::string Registry::_bitpos_to_value(std::string const& bitpos) {
		int pos = std::stoi(bitpos);
		uint32_t flag = 1 << pos;
		std::stringstream s;
		s << "0x" << std::setfill('0') << std::setw(sizeof(uint32_t) * 2) << std::hex << flag;
		return s.str();
	}

} // vkspec
