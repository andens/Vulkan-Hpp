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
		assert(_defined_types.insert(std::make_pair(translation, ItemType::CType)).second == true);
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

		_parse_item_declarations(registryElement);

		// Parse type definitions. We are now able to get any type we depend on
		// since they were created in the pass before.
		//_parse_type_definitions(registryElement);

		_undefined_check();

		_mark_extension_items();
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
			else {
				assert((value == "feature") || (value == "vendorids"));
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
	}

	//void Registry::_read_type_basetype(tinyxml2::XMLElement * element)
	//{
	//	tinyxml2::XMLNode* node = element->FirstChild();
	//	assert(node && node->ToText() && strcmp(node->Value(), "typedef ") == 0);

	//	node = node->NextSibling();
	//	assert(node && node->ToElement() && strcmp(node->Value(), "type") == 0);
	//	tinyxml2::XMLElement* type_element = node->ToElement();
	//	assert(type_element->GetText());
	//	std::string type = type_element->GetText();
	//	assert(type == "uint32_t" || type == "uint64_t");

	//	node = type_element->NextSibling();
	//	assert(node && node->ToElement() && strcmp(node->Value(), "name") == 0);
	//	tinyxml2::XMLElement* name_element = node->ToElement();
	//	assert(name_element->GetText());
	//	std::string alias = name_element->GetText();

	//	ScalarTypedef* t = new ScalarTypedef(alias, reinterpret_cast<Type const*>(type_element));
	//	assert(_items.insert(std::make_pair(alias, t)).second == true);
	//}

	void Registry::_read_type_bitmask(tinyxml2::XMLElement * element)
	{
		tinyxml2::XMLElement* name_element = element->FirstChildElement("name");
		assert(name_element && name_element->GetText());
		std::string name = name_element->GetText();

		Bitmasks* b = new Bitmasks(name, element);
		assert(_items.insert(std::make_pair(name, b)).second == true);
	}

	//void Registry::_read_type_bitmask(tinyxml2::XMLElement * element)
	//{
	//	assert(strcmp(element->GetText(), "typedef ") == 0);
	//	tinyxml2::XMLElement * typeElement = element->FirstChildElement();
	//	assert(typeElement && (strcmp(typeElement->Value(), "type") == 0) && typeElement->GetText() && (strcmp(typeElement->GetText(), "VkFlags") == 0));

	//	tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
	//	assert(nameElement && (strcmp(nameElement->Value(), "name") == 0) && nameElement->GetText());
	//	std::string name = nameElement->GetText();

	//	assert(!nameElement->NextSiblingElement());

	//	// The requires attribute contains the type that will eventually hold
	//	// definitions (a name with FlagBits in it). Oftentimes however, a type
	//	// containing Flags is used instead. This separation is done to indicate
	//	// that several flags can be used as opposed to just one. For C it's
	//	// implemented as a regular typedef, and since some of the Flag types
	//	// do not have members, they are not present in the enums tags. The C
	//	// typedef implicitly makes them work anyway, but in Rust where I gain
	//	// a little more type safety I need to at least make bitflags without
	//	// members so that the type exists.
	//	Bitmasks* b = new Bitmasks(name, reinterpret_cast<Enum const*>(element->Attribute("requires")));
	//	assert(_items.insert(std::make_pair(name, b)).second == true);
	//}

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
	}

	//void Registry::_read_type_funcpointer(tinyxml2::XMLElement * element)
	//{
	//	// The typedef <ret> text node
	//	tinyxml2::XMLNode * node = element->FirstChild();
	//	assert(node && node->ToText());
	//	std::string text = node->Value();

	//	// name tag containing the type def name
	//	node = node->NextSibling();
	//	assert(node && node->ToElement());
	//	tinyxml2::XMLElement * tag = node->ToElement();
	//	assert(tag && strcmp(tag->Value(), "name") == 0 && tag->GetText());
	//	std::string name = tag->GetText();
	//	assert(!tag->FirstChildElement());

	//	// This will match 'typedef TYPE (VKAPI_PTR *' and contain TYPE in match
	//	// group 1.
	//	std::regex re(R"(^typedef ([^ ^\*]+)(\*)? \(VKAPI_PTR \*$)");
	//	auto it = std::sregex_iterator(text.begin(), text.end(), re);
	//	auto end = std::sregex_iterator();
	//	assert(it != end);
	//	std::smatch match = *it;
	//	std::string returnType = _type_reference(match[1].str(), name);
	//	if (match[2].matched) {
	//		returnType = _translator->pointer_to(returnType, PointerType::T_P);
	//	}

	//	// Text node after name tag beginning parameter list. Note that for void
	//	// functions this is the last node that also ends the function definition.
	//	node = node->NextSibling();
	//	assert(node && node->ToText());
	//	text = node->Value();
	//	bool nextParamConst = false;
	//	if (text != ")(void);") {
	//		// In this case we will begin parameters, so we check if the first has
	//		// a const modifier.
	//		re = std::regex(R"(\)\(\n[ ]+(const )?)");
	//		auto it = std::sregex_iterator(text.begin(), text.end(), re);
	//		assert(it != end);
	//		match = *it;
	//		nextParamConst = match[1].matched;
	//	}

	//	// Storage for parsed parameters (type, name)
	//	std::vector<FunctionTypedef::Parameter> params;

	//	// Start processing parameters.
	//	while (node = node->NextSibling()) {
	//		bool constModifier = nextParamConst;
	//		nextParamConst = false;

	//		// Type of parameter
	//		tag = node->ToElement();
	//		assert(tag && strcmp(tag->Value(), "type") == 0 && tag->GetText());
	//		std::string paramType = tag->GetText();
	//		assert(!tag->FirstChildElement());

	//		// Text node containing parameter name and at times a pointer modifier.
	//		node = node->NextSibling();
	//		assert(node && node->ToText());
	//		text = node->ToText()->Value();

	//		// Match optional asterisk (group 1), a bunch of spaces, the parameter
	//		// name (group 2), and the rest (group 3). It doesn't seem that newline
	//		// is a part of this. It's probably good because then I can easily work
	//		// directly with suffix instead of more regex magic.
	//		re = std::regex(R"(^(\*)?[ ]+([a-zA-Z]+)(.*)$)");
	//		it = std::sregex_iterator(text.begin(), text.end(), re);
	//		assert(it != end);
	//		match = *it;
	//		bool pointer = match[1].matched;
	//		std::string paramName = match[2].str();
	//		if (match[3].str() == ");") {
	//			assert(!node->NextSibling());
	//		}
	//		else {
	//			assert(match[3].str() == ",");

	//			// Match on the suffix to know if the upcoming parameter is const.
	//			std::string suffix = match.suffix().str();
	//			re = std::regex(R"(^\n[ ]+(const )?$)");
	//			it = std::sregex_iterator(suffix.begin(), suffix.end(), re);
	//			assert(it != end);
	//			match = *it;
	//			nextParamConst = match[1].matched;
	//		}

	//		std::string t = _type_reference(paramType, name);

	//		if (constModifier) {
	//			assert(pointer);
	//		}

	//		if (pointer) {
	//			t = _translator->pointer_to(t, constModifier ? PointerType::CONST_T_P : PointerType::T_P);
	//		}

	//		params.push_back({
	//			t,
	//			paramName,
	//		});
	//	}

	//	FunctionTypedef* t = _define_function_typedef(name, returnType);
	//	for (auto p : params) {
	//		t->params.push_back(p);
	//	}
	//}

	void Registry::_read_type_handle(tinyxml2::XMLElement * element)
	{
		tinyxml2::XMLElement* name_element = element->FirstChildElement("name");
		assert(name_element && name_element->GetText());
		std::string name = name_element->GetText();

		HandleTypedef* h = new HandleTypedef(name, element);
		assert(_items.insert(std::make_pair(name, h)).second == true);
	}

	//void Registry::_read_type_handle(tinyxml2::XMLElement * element)
	//{
	//	tinyxml2::XMLElement * typeElement = element->FirstChildElement();
	//	assert(typeElement && (strcmp(typeElement->Value(), "type") == 0) && typeElement->GetText());
	//	std::string type = typeElement->GetText();

	//	tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
	//	assert(nameElement && (strcmp(nameElement->Value(), "name") == 0) && nameElement->GetText());
	//	std::string name = nameElement->GetText();

	//	std::string underlying = "";
	//	if (type == "VK_DEFINE_HANDLE") { // Defined as pointer meaning varying size
	//		underlying = _type_reference("size_t", name);
	//	}
	//	else {
	//		assert(type == "VK_DEFINE_NON_DISPATCHABLE_HANDLE"); // Pointer on 64-bit and uint64_t otherwise -> always 64 bit
	//		underlying = _type_reference("uint64_t", name);
	//	}

	//	_define_handle_typedef(name, underlying);
	//}

	void Registry::_read_type_struct(tinyxml2::XMLElement * element, bool isUnion)
	{
		assert(element->Attribute("name"));
		std::string name = element->Attribute("name");

		Struct* s = new Struct(name, element, isUnion);
		assert(_items.insert(std::make_pair(name, s)).second == true);
	}

	//void Registry::_read_type_struct(tinyxml2::XMLElement * element, bool isUnion)
	//{
	//	assert(!element->Attribute("returnedonly") || (strcmp(element->Attribute("returnedonly"), "true") == 0));

	//	assert(element->Attribute("name"));
	//	std::string name = element->Attribute("name");

	//	// element->Attribute("returnedonly") is also applicable for structs and unions
	//	Struct* t = _define_struct(name, isUnion);

	//	for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
	//	{
	//		assert(child->Value() && strcmp(child->Value(), "member") == 0);
	//		_read_type_struct_member(t, child);
	//	}
	//}

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

		//if (type == "bitmask") {
		//	Bitmasks* t = _define_bitmasks(name);

		//	_read_enums_bitmask(element, [t, this](std::string const& member, std::string const& value, bool isBitpos) {
		//		Bitmasks::Member m;
		//		m.name = member;
		//		m.value = isBitpos ? _bitpos_to_value(value) : value;
		//		t->members.push_back(m);
		//	});
		//}
		//else {
		//	Enum* t = _define_enum(name);

		//	_read_enums_enum(element, [t](std::string const& member, std::string const& value) {
		//		Enum::Member m;
		//		m.name = member;
		//		m.value = value;
		//		t->members.push_back(m);
		//	});
		//}
	}

	void Registry::_read_api_constants(tinyxml2::XMLElement* element) {
		for (tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
			assert(child->Attribute("name"));
			std::string constant = child->Attribute("name");

			ApiConstant* c = new ApiConstant(constant, child);
			assert(_items.insert(std::make_pair(constant, c)).second == true);

			/*
			assert(child->Attribute("value"));
			std::string value = child->Attribute("value");

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
				std::string dataType = _type_reference(match[1].matched ? "i32" : "u32", constant);
				_define_api_constant(constant, dataType, value, child);
				continue;
			}

			re = std::regex(R"(^[0-9]+\.[0-9]+f$)");
			it = std::sregex_iterator(value.begin(), value.end(), re);

			// Matched float
			if (it != end) {
				value.pop_back();
				_define_api_constant(constant, _type_reference("f32", constant), value, child);
				continue;
			}

			// The rest
			if (value == "(~0U)") {
				_define_api_constant(constant, _type_reference("u32", constant), "~0", child);
			}
			else if (value == "(~0ULL)") {
				_define_api_constant(constant, _type_reference("u64", constant), "~0", child);
			}
			else {
				assert(value == "(~0U-1)");
				_define_api_constant(constant, _type_reference("u32", constant), "~0u32 - 1", child);
			}*/
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

		// ###############################################################
		//Command* cmd = _read_command_proto(child);
		//_read_command_params(child, cmd);
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
		assert(element->Attribute("name"));
		std::string name = element->Attribute("name");
		
		Extension* e = new Extension(name, element);
		assert(_items.insert(std::make_pair(name, e)).second == true);
	}

	//void Registry::_read_extensions_extension(tinyxml2::XMLElement * element)
	//{
	//	Extension ext;

	//	assert(element->Attribute("name") && element->Attribute("number"));
	//	ext.name = element->Attribute("name");
	//	ext.number = element->Attribute("number");
	//	ext.tag = _extract_tag(ext.name);
	//	assert(_tags.find(ext.tag) != _tags.end());

	//	if (element->Attribute("type")) {
	//		ext.type = element->Attribute("type");
	//		assert(ext.type == "instance" || ext.type == "device");
	//	}

	//	// The original code used protect, which is a preprocessor define that must be
	//	// present for the definition. This could be for example VK_USE_PLATFORM_WIN32
	//	// in order to use Windows surface or external semaphores.

	//	tinyxml2::XMLElement * child = element->FirstChildElement();
	//	assert(child && (strcmp(child->Value(), "require") == 0) && !child->NextSiblingElement());

	//	if (strcmp(element->Attribute("supported"), "disabled") == 0)
	//	{
	//		ext.disabled = true;

	//		// Types and commands of disabled extensions should not be present in the
	//		// final bindings, so mark them as disabled.
	//		_read_disabled_extension_require(child, ext);
	//	}
	//	else
	//	{
	//		_read_extension_require(child, ext.tag, ext);
	//	}

	//	_define_extension(std::move(ext));
	//}

	// Read a member tag of a struct, adding members to the provided struct.
	void Registry::_read_type_struct_member(Struct* theStruct, tinyxml2::XMLElement * element) {
		// The attributes of member tags seem to mostly concern documentation
		// generation, so they are not of interest for the bindings.

		// Read the type, parsing modifiers to get a string of the type.
		std::string type;
		tinyxml2::XMLNode* child = _read_type_struct_member_type(element->FirstChild(), theStruct->name(), type);

		// After we have parsed the type we expect to find the name of the member
		assert(child->ToElement() && strcmp(child->Value(), "name") == 0 && child->ToElement()->GetText());
		std::string memberName = child->ToElement()->GetText();

		// Some members have more information about array size
		std::string arraySize = _read_array_size(child, memberName);
		if (arraySize != "") {
			type = _translator->array_member(type, arraySize);
		}

		//// Add member to struct
		//Struct::Member m;
		//m.type = type;
		//m.name = memberName;
		//theStruct->members.push_back(m);
	}

	// Reads the type tag of a member tag, including potential text nodes around
	// the type tag to get qualifiers. We pass the first node that could potentially
	// be a text node.
	tinyxml2::XMLNode* Registry::_read_type_struct_member_type(tinyxml2::XMLNode* element, std::string const& struct_name, std::string& type)
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
		type = _type_reference(element->ToElement()->GetText(), struct_name);

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
		std::string name = nameElement->GetText();
		std::string type = _type_reference(typeElement->GetText(), name);

		//return _define_command(name, type);
		return nullptr;
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
		bool const_modifier;
		tinyxml2::XMLNode * afterType = _read_command_param_type(element->FirstChild(), cmd, type, const_modifier);

		assert(afterType->ToElement() && (strcmp(afterType->Value(), "name") == 0) && afterType->ToElement()->GetText());
		std::string name = afterType->ToElement()->GetText();

		std::string arraySize = _read_array_size(afterType, name);
		if (arraySize != "") {
			type = _translator->array_param(type, arraySize, const_modifier);
		}

		//Command::Parameter p;
		//p.type = type;
		//p.name = name;
		//cmd->params.push_back(p);
	}

	tinyxml2::XMLNode* Registry::_read_command_param_type(tinyxml2::XMLNode* node, Command const* cmd, std::string& type, bool& const_modifier)
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
		type = _type_reference(node->ToElement()->GetText(), cmd->name());

		// end with "*", "**", or "* const*", if needed
		node = node->NextSibling();
		assert(node); // If not text node, at least the name tag (processed elsewhere)
		if (node->ToText())
		{
			std::string value = _trim_end(node->Value());
			assert((value == "*") || (value == "**") || (value == "* const*"));
			if (value == "*") {
				type = _translator->pointer_to(type, const_modifier ? PointerType::CONST_T_P : PointerType::T_P);
			}
			else if (value == "**") {
				type = _translator->pointer_to(type, const_modifier ? PointerType::CONST_T_PP : PointerType::T_PP);
			}
			else {
				assert(value == "* const*");
				type = _translator->pointer_to(type, const_modifier ? PointerType::CONST_T_P_CONST_P : PointerType::T_P_CONST_P);
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
				//ext.commands.push_back(name);
			}
			else if (value == "type") {
				assert(child->Attribute("name"));
				std::string name = child->Attribute("name");
				//ext.types.push_back(name);
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
				_read_extension_command(child, ext);
			}
			else if (value == "type")
			{
				_read_extension_type(child, ext);
			}
			else
			{
				assert(value == "enum");
				_read_extension_enum(child, ext);
			}
		}
	}

	void Registry::_read_extension_command(tinyxml2::XMLElement * element, Extension& extension)
	{
		// Command is marked as belonging to an extension after parsing is done
		assert(element->Attribute("name"));
		std::string t = _type_reference(element->Attribute("name"), extension.name());
		//extension.commands.push_back(t);
	}

	void Registry::_read_extension_type(tinyxml2::XMLElement * element, Extension& ext)
	{
		// Some types are not found by analyzing dependencies, but the extension
		// may still require some types. These are provided explicitly. One of
		// these is VkBindImageMemorySwapchainInfoKHX which is an extension struct
		// to VkBindImageMemoryInfoKHX and thus is never a direct dependency of
		// another type. However, the extension (VK_KHX_device_group) still adds
		// it, so we collect these types here for when analyzing dependencies.
		assert(element->Attribute("name"));
		std::string t = _type_reference(element->Attribute("name"), ext.name());
		//ext.required_types.push_back(t);
	}

	void Registry::_read_extension_enum(tinyxml2::XMLElement * element, Extension const& extension)
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
				_type_reference(extends, extension.name());
				auto it = std::find_if(_bitmasks.begin(), _bitmasks.end(), [&extends](Bitmasks* bitmask) -> bool {
					return bitmask->name() == extends;
				});
				assert(it != _bitmasks.end());
				//Bitmasks::Member m;
				//m.name = name;
				//m.value = _bitpos_to_value(element->Attribute("bitpos"));
				//(*it)->members.push_back(m);
			}
			else if (element->Attribute("offset")) {
				// The value depends on extension number and offset. See
				// https://www.khronos.org/registry/vulkan/specs/1.0/styleguide.html#_assigning_extension_token_values
				// for calculation.
				//int value = 1000000000 + (std::stoi(extension.number) - 1) * 1000 + std::stoi(element->Attribute("offset"));
				int value = 0;

				if (element->Attribute("dir") && strcmp(element->Attribute("dir"), "-") == 0) {
					value = -value;
				}

				std::string valueString = std::to_string(value);

				// If type does not exist, same goes as above
				std::string extends = element->Attribute("extends");
				_type_reference(extends, extension.name());
				auto it = std::find_if(_enums.begin(), _enums.end(), [&extends](Enum* e) -> bool {
					return e->name() == extends;
				});
				assert(it != _enums.end());
				//Enum::Member m;
				//m.name = name;
				//m.value = valueString;
				//(*it)->members.push_back(m);
			}
			else {
				// This is a special case for an enum variant that used to be core.
				// It uses value instead of offset.
				std::string extends = element->Attribute("extends");
				_type_reference(extends, extension.name());
				auto it = std::find_if(_enums.begin(), _enums.end(), [&extends](Enum* e) -> bool {
					return e->name() == extends;
				});
				assert(it != _enums.end());
				//Enum::Member m;
				//m.name = name;
				//m.value = element->Attribute("value");
				//(*it)->members.push_back(m);
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

	// dependant is the one making a reference to type
	std::string const& Registry::_type_reference(std::string const& type, std::string const& dependant) {
		assert(type.find_first_of("* ") == std::string::npos);

		// Use translation if we work with a C type
		auto c = _c_types.find(type);
		std::string const& referenced = (c != _c_types.end()) ? c->second : type;

		// Set up dependencies while we're at it
		_dependencies[referenced].dependants.insert(dependant);
		_dependencies[dependant].dependencies.insert(referenced);

		if (_defined_types.find(referenced) == _defined_types.end()) {
			_undefined_types.insert(referenced);
		}

		if (_defined_types.find(dependant) == _defined_types.end()) {
			_undefined_types.insert(dependant);
		}

		return referenced;
	}

	void Registry::_define(std::string const& name, ItemType item_type) {
		assert(_defined_types.find(name) == _defined_types.end());
		_defined_types.insert(std::make_pair(name, item_type));
		_undefined_types.erase(name);
	}

	void Registry::_undefined_check() {
		assert(_undefined_types.empty());
	}

	void Registry::_mark_extension_items() {/*
		for (auto& ext : _extensions) {
			// Mark all commands of this extension as being extension commands.
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

		// First we collect all types used directly by non-extension commands.
		std::set<std::string> core_types;
		for (auto c : _commands) {
			if (!c->extension) {
				core_types.insert(_dependencies[c->name].dependencies.begin(), _dependencies[c->name].dependencies.end());
			}
		}

		auto add = [this](std::set<std::string>& set) -> bool {
			for (auto& item : set) {
				for (auto& dep : _dependencies[item].dependencies) {
					if (set.insert(dep).second) {
						// A value was added so we return in order to iterate again
						return true;
					}
				}
			}

			// Nothing added
			return false;
		};

		// Now we recurse and all dependencies of types until nothing new was
		// added. At this point we have all types used directly or indirectly
		// by core commands.
		while (add(core_types));

		// With all types used by the core API known, we collect those used by
		// extensions. I guess we could just have gone with adding types having
		// a tag directly without finding core types, but it's nice to compare
		// to types we know are definitely used by core commands.
		std::set<std::string> extension_types;
		for (auto& t : _defined_types) {
			// Ignore certain classes of items manually
			if (std::find_if(_c_types.begin(), _c_types.end(), [&t](auto& c) -> bool { return c.second == t.first; }) != _c_types.end()) { // C types are not defined by extension
				continue;
			}
			if (_extensions.find(t.first) != _extensions.end()) { // Extensions are not types
				continue;
			}
			if (std::find_if(_api_constants.begin(), _api_constants.end(), [&t](auto& c) -> bool { return std::get<0>(c) == t.first; }) != _api_constants.end()) {
				continue;
			}
			if (std::find_if(_commands.begin(), _commands.end(), [&t](Command* c) -> bool { return c->name == t.first; }) != _commands.end()) { // Commands are not types
				continue;
			}

			// These types are defined in the core API, but never used directly
			if (t.first == "VkDispatchIndirectCommand") { continue; }
			if (t.first == "VkDrawIndexedIndirectCommand") { continue; }
			if (t.first == "VkDrawIndirectCommand") { continue; }
			if (t.first == "VkPipelineCacheHeaderVersion") { continue; } // Defined as an enum, but used as API constant
			if (t.first == "VkRect3D") { continue; } // Defined but never used in the API

			// All extension types should match this pattern. If they do not,
			// we make sure that they are used in the core API as a second safe-
			// guard against leaking types.
			std::regex re(R"(^(PFN_v|V)k[A-Z][a-zA-Z0-9]+[a-z0-9]([A-Z][A-Z]+)$)");
			auto it = std::sregex_iterator(t.first.begin(), t.first.end(), re);
			auto end = std::sregex_iterator();
			if (it == end) { // no match for extension item, should be used in core
				assert(core_types.find(t.first) != core_types.end());
			}
			else { // matched extension item
				std::smatch match = *it;
				assert(_tags.find(match[2].str()) != _tags.end());
				extension_types.insert(t.first);
			}
		}

		// For each extension iterated in order, its dependency set is built
		// and matched against extension types. If any type is found, that type
		// is removed from remaining extension types and marked as being added
		// by the extension in question.
		std::set<std::string> current_set;
		size_t extension_count = _extensions.size();
		for (unsigned counter = 1; counter <= extension_count; ++counter) {
			auto ext_it = std::find_if(_extensions.begin(), _extensions.end(), [counter](std::pair<const std::string, Extension>& e) -> bool {
				return std::stoi(e.second.number) == counter;
			});
			assert(ext_it != _extensions.end());

			Extension& e = ext_it->second;
			if (e.disabled) {
				continue;
			}

			current_set.clear();
			for (auto& c : e.commands) {
				current_set.insert(_dependencies[c].dependencies.begin(), _dependencies[c].dependencies.end());
			}
			for (auto& required : e.required_types) { // Also insert the dependencies not found implicitly via functions
				current_set.insert(required);
			}
			while (add(current_set));

			for (auto& dep : current_set) {
				if (extension_types.erase(dep) > 0) {
					auto type = _defined_types.find(dep);
					assert(type != _defined_types.end());
					ItemType item_type = type->second;
					ExtensionItem* item = nullptr;
					switch (item_type) {
					case ItemType::FunctionTypedef: item = *std::find_if(_function_typedefs.begin(), _function_typedefs.end(), [&dep](FunctionTypedef* t) -> bool { return t->alias == dep; }); break;
					case ItemType::Bitmasks: item = *std::find_if(_bitmasks.begin(), _bitmasks.end(), [&dep](Bitmasks* t) -> bool { return t->name == dep; }); break;
					case ItemType::BitmaskTypedef: item = *std::find_if(_bitmask_typedefs.begin(), _bitmask_typedefs.end(), [&dep](BitmaskTypedef* t) -> bool { return t->alias == dep; }); break;
					case ItemType::HandleTypedef: item = *std::find_if(_handle_typedefs.begin(), _handle_typedefs.end(), [&dep](HandleTypedef* t) -> bool { return t->alias == dep; }); break;
					case ItemType::Struct: item = *std::find_if(_structs.begin(), _structs.end(), [&dep](Struct* t) -> bool { return t->name == dep; }); break;
					case ItemType::Enum: item = *std::find_if(_enums.begin(), _enums.end(), [&dep](Enum* t) -> bool { return t->name == dep; }); break;
					}
					if (!item) {
						throw std::runtime_error("Dependency '" + dep + "' of extension '" + e.name + "' was of unknown type. This is an oversight and should be easily fixed by adding another switch branch.");
					}
					item->extension = true;
					e.types.push_back(dep);
				}
			}
		}

		// By now all extension types should have been added to some extension.
		assert(extension_types.empty());*/
	}

	std::string Registry::_bitpos_to_value(std::string const& bitpos) {
		int pos = std::stoi(bitpos);
		uint32_t flag = 1 << pos;
		std::stringstream s;
		s << "0x" << std::setfill('0') << std::setw(sizeof(uint32_t) * 2) << std::hex << flag;
		return s.str();
	}

} // vkspec
