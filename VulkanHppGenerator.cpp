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
#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <exception>
#include <regex>
#include <iterator>

#include <tinyxml2.h>

class IndentingOStreambuf : public std::streambuf
{
	std::streambuf*     myDest;
	bool                myIsAtStartOfLine;
	std::string         myIndent;
	std::ostream*       myOwner;
protected:
	virtual int         overflow(int ch)
	{
		if (myIsAtStartOfLine && ch != '\n') {
			myDest->sputn(myIndent.data(), myIndent.size());
		}
		myIsAtStartOfLine = ch == '\n';
		return myDest->sputc(ch);
	}
public:
	explicit            IndentingOStreambuf(
		std::streambuf* dest, int indent = 4)
		: myDest(dest)
		, myIsAtStartOfLine(true)
		, myIndent(indent, ' ')
		, myOwner(NULL)
	{
	}
	explicit            IndentingOStreambuf(
		std::ostream& dest, int indent = 4)
		: myDest(dest.rdbuf())
		, myIsAtStartOfLine(true)
		, myIndent(indent, ' ')
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
} *indent;

class Type {
	friend class Types;

public:
	enum class Kind {
		IntrinsicC = 0,
		X11,
		Android,
		Mir,
		Wayland,
		Windows,
		Xcb,
		C_MAX,
		Undefined,
		Pointer,
		VulkanScalarTypedef,
		VulkanFunctionTypedef,
		VulkanBitmasks,
		VulkanHandleTypedef,
		VulkanStruct,
	};

	bool isCType() {
		return _kind < Kind::C_MAX;
	}

	void ptrSetInner(Type* ptr) {
		assert(_kind == Kind::Pointer);
		_pointer.inner = ptr;
	}

	void structAddMember(Type* type, const std::string& name, const std::string& arraySize) {
		assert(_kind == Kind::VulkanStruct);
		_struct.members.push_back(std::make_tuple(type, name, arraySize));
	}

	~Type() {

	}

private:
	Type(std::string type) : _type(type), _kind(Kind::Undefined) {}

	// Upgrade undefined to C type
	void makeCType(const std::string& translation, Kind kind) {
		assert(_kind == Kind::Undefined);
		_kind = kind;
		new(&_ctype.translation) std::string(translation);
	}

	// Upgrade undefined to pointer
	void makePointer(const std::string& constness) {
		assert(_kind == Kind::Undefined);
		_kind = Kind::Pointer;
		new(&_pointer.constness) auto(constness);
		_pointer.inner = nullptr;
	}

	// Upgrade undefined to scalar typedef
	void makeScalarTypedef(Type* underlying) {
		assert(_kind == Kind::Undefined);
		_kind = Kind::VulkanScalarTypedef;
		_scalarTypedef.underlying = underlying;
	}

	// Upgrade undefined to function typedef
	void makeFunctionTypedef(Type* returnType, const std::vector<std::pair<Type*, std::string>>& params) {
		assert(_kind == Kind::Undefined);
		_kind = Kind::VulkanFunctionTypedef;
		_functionTypedef.returnType = returnType;
		new(&_functionTypedef.params) auto(params);
	}

	// Upgrade undefined to bitmasks
	void makeBitmasks() {
		assert(_kind == Kind::Undefined);
		_kind = Kind::VulkanBitmasks;
		new(&_bitmasks.variants) std::vector<std::pair<Type*, std::string>>();
	}

	// Upgrade undefined to handle typedef
	void makeHandleTypedef(Type* underlying) {
		assert(_kind == Kind::Undefined);
		_kind = Kind::VulkanHandleTypedef;
		_handleTypedef.underlying = underlying;
	}

	// Upgrade undefined to struct
	void makeStruct(bool isUnion) {
		assert(_kind == Kind::Undefined);
		_kind = Kind::VulkanStruct;
		new(&_struct.members) std::vector<std::tuple<Type*, std::string, std::string>>();
		_struct.isUnion = isUnion;
	}

private:
	std::string _type;
	Kind _kind;

	struct CType {
		std::string translation;
	};

	struct Ptr {
		std::string constness;
		Type* inner;
	};

	struct VulkanScalarTypedef {
		Type* underlying;
	};

	struct VulkanFunctionTypedef {
		Type* returnType;
		std::vector<std::pair<Type*, std::string>> params;
	};

	struct VulkanBitmasks {
		std::vector<std::pair<std::string, std::string>> variants;
	};

	struct VulkanHandleTypedef {
		Type* underlying;
	};

	struct VulkanStruct {
		std::vector<std::tuple<Type*, std::string, std::string>> members;
		bool isUnion;
	};

	union {
		CType _ctype;
		Ptr _pointer;
		VulkanScalarTypedef _scalarTypedef;
		VulkanFunctionTypedef _functionTypedef;
		VulkanBitmasks _bitmasks;
		VulkanHandleTypedef _handleTypedef;
		VulkanStruct _struct;
	};
};

class Types {
public:
	Types()
	{
#define INSERT_C_TYPE(type, rustType, kind) { Type* t = new Type(type); t->makeCType(rustType, kind); assert(_ctypes.insert(std::make_pair(type, t)).second == true); }

		// I'm working under the assumption that the C and OS types used will
		// be a comparatively small set so that I can deal with those manually.
		// This way I can assume that types not existing at the time I need them
		// are Vulkan types that will be defined later. In other words, I can
		// defer definitions of Vulkan types in particular and have a separate
		// API for them.
		INSERT_C_TYPE("void", "()", Type::Kind::IntrinsicC);
		INSERT_C_TYPE("char", "c_char", Type::Kind::IntrinsicC);
		INSERT_C_TYPE("float", "f32", Type::Kind::IntrinsicC);
		INSERT_C_TYPE("uint8_t", "u8", Type::Kind::IntrinsicC);
		INSERT_C_TYPE("uint32_t", "u32", Type::Kind::IntrinsicC);
		INSERT_C_TYPE("uint64_t", "u64", Type::Kind::IntrinsicC);
		INSERT_C_TYPE("int32_t", "i32", Type::Kind::IntrinsicC);
		INSERT_C_TYPE("size_t", "usize", Type::Kind::IntrinsicC); // unsigned according to reference
		INSERT_C_TYPE("int", "c_int", Type::Kind::IntrinsicC);

		INSERT_C_TYPE("Display", "Display", Type::Kind::X11);
		INSERT_C_TYPE("VisualID", "VisualID", Type::Kind::X11);
		INSERT_C_TYPE("Window", "Window", Type::Kind::X11);
		INSERT_C_TYPE("RROutput", "RROutput", Type::Kind::X11);

		INSERT_C_TYPE("ANativeWindow", "ANativeWindow", Type::Kind::Android);

		INSERT_C_TYPE("MirConnection", "MirConnection", Type::Kind::Mir);
		INSERT_C_TYPE("MirSurface", "MirSurface", Type::Kind::Mir);

		INSERT_C_TYPE("wl_display", "wl_display", Type::Kind::Wayland);
		INSERT_C_TYPE("wl_surface", "wl_surface", Type::Kind::Wayland);

		INSERT_C_TYPE("HINSTANCE", "HINSTANCE", Type::Kind::Windows);
		INSERT_C_TYPE("HWND", "HWND", Type::Kind::Windows);
		INSERT_C_TYPE("HANDLE", "HANDLE", Type::Kind::Windows);
		INSERT_C_TYPE("SECURITY_ATTRIBUTES", "SECURITY_ATTRIBUTES", Type::Kind::Windows);
		INSERT_C_TYPE("DWORD", "DWORD", Type::Kind::Windows);
		INSERT_C_TYPE("LPCWSTR", "LPCWSTR", Type::Kind::Windows);

		INSERT_C_TYPE("xcb_connection_t", "xcb_connection_t", Type::Kind::Xcb);
		INSERT_C_TYPE("xcb_visualid_t", "xcb_visualid_t", Type::Kind::Xcb);
		INSERT_C_TYPE("xcb_window_t", "xcb_window_t", Type::Kind::Xcb);

#undef INSERT_C_TYPE
	}

	void scalarTypedef(Type* existing, const std::string& alias) {
		assert(strncmp(alias.c_str(), "Vk", 2) == 0);
		assert(alias.find_first_of("* ") == std::string::npos);

		getVulkanType(alias)->makeScalarTypedef(existing);

		// TODO: Remove _scalarTypedefs and when I need them later I can find on all
		// Vulkan types using a lambda to check that the enum is VulkanTypedef.
		// Maybe using find_if
		//assert(_scalarTypedefs.insert({ alias, existing }).second == true);
	}

	void functionTypedef(const std::string& name, Type* returnType, const std::vector<std::pair<Type*, std::string>> params) {
		assert(strncmp(name.c_str(), "PFN_vk", 6) == 0);
		assert(name.find_first_of("* ") == std::string::npos);

		getVulkanType(name)->makeFunctionTypedef(returnType, params);
	}

	void bitmasks(const std::string& name) {
		assert(strncmp(name.c_str(), "Vk", 2) == 0);
		assert(name.find_first_of("* ") == std::string::npos);

		getVulkanType(name)->makeBitmasks();
	}

	void handleTypedef(Type* underlying, const std::string& alias) {
		assert(strncmp(alias.c_str(), "Vk", 2) == 0);
		assert(alias.find_first_of("* ") == std::string::npos);

		getVulkanType(alias)->makeHandleTypedef(underlying);
	}

	void defineStruct(const std::string& type, bool isUnion) {
		assert(strncmp(type.c_str(), "Vk", 2) == 0);
		assert(type.find_first_of("* ") == std::string::npos);

		getVulkanType(type)->makeStruct(isUnion);
	}

	// The purpose of this function is to return an object that represents some
	// type used in the API. A complete type (including pointers and const mod-
	// ifiers) is accepted, which is then disected into parts. This way, when
	// parsing we can just ask for the type and let somebody else deal with the
	// administration of properly pointing to some base type. The base types
	// themselves are only stored once and then their pointers are reused.
	// This allows declaring Vulkan types before they are defined by returning
	// a type that is VulkanUndefined for the time being but later updated when
	// parsing its definition. Another benefit of this is that we can define the
	// limited set of C types manually up front instead of trying to deduce it
	// from the spec. They need some form of translation into Rust, which is why
	// we want to know which types are C and which ones are Vulkan. Thus, if a
	// type does not already exist, we assume that it's Vulkan. If it turns out
	// that it's not, then it will remain as undefined because the spec never
	// tells us what it is, thus indicating that it was C all the time.
	
	Type* getType(const std::string& name) {
		std::string text = name;
		
		// Split type so we can work with parts
		std::vector<std::string> parts;
		size_t prev_pos = 0;
		size_t this_pos = 0;
		while ((this_pos = text.find_first_of("* ", prev_pos)) != std::string::npos) {
			size_t len = this_pos - prev_pos;
			if (len != 0) {
				parts.push_back(text.substr(prev_pos, len));
			}

			if (text[this_pos] == '*') {
				parts.push_back("*");
			}

			prev_pos = this_pos + 1;
		}

		if (prev_pos < text.length()) {
			parts.push_back(text.substr(prev_pos));
		}

		assert(parts.size());

		// Expected patterns:
		// type
		// type*
		// type**
		// const type*
		// const type* const*
		
		// If we begin with a const, we swap the first two elements as it's
		// semantically equivalent and eases the upcoming processing a bit.
		if (parts.front() == "const") {
			assert(parts.size() > 1);
			std::swap(*parts.begin(), *(parts.begin() + 1));
		}

		// I don't check if the first part is something proper now. If it's not
		// a valid type, it will never be defined (as that would imply the API
		// being broken) as is thus captured as an undefined type. I do however
		// check that all upcoming parts are either 'const' or '*'.
		for (auto it = parts.cbegin() + 1; it != parts.end(); ++it) {
			assert(*it == "*" || *it == "const");
		}

		std::string baseType = parts.front();

		// Now the base type is always the first element, so work it.
		Type* type = nullptr;

		// Attempt to acquire the type first from C types.
		auto c_it = _ctypes.find(baseType);
		if (c_it != _ctypes.end()) {
			type = c_it->second;
		}

		// Attempt to acquire the type from Vulkan types.
		auto vk_it = _vulkanTypes.find(baseType);
		if (vk_it != _vulkanTypes.end()) {
			// If we found a Vulkan type it must not also be a C type
			assert(!type);
			type = vk_it->second;
		}

		// If neither C nor Vulkan types worked, it's a new type that we create
		// as an undefined type for now.
		if (!type) {
			type = new Type(baseType);
			_vulkanTypes.insert(std::make_pair(baseType, type));
		}

		// Now that we have the base type, we can work from the back to generate
		// wrapping pointers if present.

		Type* outer = nullptr;
		Type* inner = nullptr;

		auto hookupPtr = [&outer, &inner](Type* ptr) {
			if (!outer) {
				outer = ptr;
			}
			else if (!inner) {
				outer->ptrSetInner(ptr);
				inner = ptr;
			}
			else {
				inner->ptrSetInner(ptr);
				inner = ptr;
			}
		};

		parts.erase(parts.begin()); // No longer need this

		auto walker = parts.crbegin();
		while (walker != parts.crend()) {
			assert(*walker == "*");
			walker++;
			// No more => mutable pointer. Another pointer immediately => mutable
			if (walker == parts.crend() || *walker == "*") {
				Type* t = new Type("");
				t->makePointer("mut");
				hookupPtr(t);
			}
			// Something more and it's not a pointer
			else {
				assert(*walker == "const");
				walker++; // For next inner pointer
				Type* t = new Type("");
				t->makePointer("const");
				hookupPtr(t);
			}
		}

		// Insert the base type
		hookupPtr(type);

		// Finally we should have a base type potentially wrapped by pointer
		// types.
		return outer;
	}

	const std::map<std::string, std::string>& getScalarTypedefs() {
		return _scalarTypedefs;
	}

private:
	Type* getVulkanType(const std::string& type) {
		Type* t = nullptr;
		auto it = _vulkanTypes.find(type);
		if (it != _vulkanTypes.end()) {
			t = it->second;
		}
		else {
			t = new Type(type);
			_vulkanTypes.insert(std::make_pair(type, t));
		}

		return t;
	}

private:
	std::map<std::string, Type*> _ctypes;
	std::map<std::string, Type*> _vulkanTypes;
	std::map<std::string, std::string> _scalarTypedefs; // typedef <Value> <Key>
} typeOracle;

const std::string flagsMacro = R"(
FLAGSMACRO
)";

std::string replaceWithMap(std::string const &input, std::map<std::string, std::string> replacements)
{
  // This will match ${someVariable} and contain someVariable in match group 1
  std::regex re(R"(\$\{([^\}]+)\})");
  auto it = std::sregex_iterator(input.begin(), input.end(), re);
  auto end = std::sregex_iterator();

  // No match, just return the original string
  if (it == end)
  {
    return input;
  }

  std::string result = "";
  while (it != end)
  {
    std::smatch match = *it;
    auto itReplacement = replacements.find(match[1].str());
    assert(itReplacement != replacements.end());

    result += match.prefix().str() + ((itReplacement != replacements.end()) ? itReplacement->second : match[0].str());
    ++it;

    // we've passed the last match. Append the rest of the orignal string
    if (it == end)
    {
      result += match.suffix().str();
    }
  }
  return result;
}

struct ParamData
{
  std::string type;
  std::string name;
  std::string arraySize;
  std::string pureType;
  std::string len;
  bool        optional;
};

struct CommandData
{
  CommandData(std::string const& t, std::string const& fn)
    : returnType(t)
    , fullName(fn)
    , returnParam(~0)
    , templateParam(~0)
    , twoStep(false)
  {}

  std::string               className;
  std::string               enhancedReturnType;
  std::string               fullName;
  std::vector<ParamData>    params;
  std::string               protect;
  std::string               reducedName;
  size_t                    returnParam;
  std::string               returnType;
  std::set<size_t>          skippedParams;
  std::vector<std::string>  successCodes;
  size_t                    templateParam;
  bool                      twoStep;
  std::map<size_t, size_t>  vectorParams;
};

struct NameValue
{
  std::string name;
  std::string value;
};

struct EnumData
{
  EnumData(std::string const& n, bool b = false)
    : name(n)
    , bitmask(b)
  {}

  void addEnumMember(std::string const& name, std::string const& tag);

  std::string             name;
  std::string             prefix;
  std::string             postfix;
  std::vector<NameValue>  members;
  std::string             protect;
  bool                    bitmask;
};

struct FlagData
{
  std::string protect;
};

struct HandleData
{
  std::vector<std::string>  commands;
  std::string               protect;
};

struct ScalarData
{
  std::string protect;
};

struct MemberData
{
  std::string type;
  std::string name;
  std::string arraySize;
  std::string pureType;
};

struct StructData
{
  StructData()
    : returnedOnly(false)
  {}

  bool                    returnedOnly;
  bool                    isUnion;
  std::vector<MemberData> members;
  std::string             protect;
};

struct DeleterData
{
  std::string pool;
  std::string call;
};

struct VkData
{
  std::map<std::string, CommandData>            commands;
  std::map<std::string, std::set<std::string>>  deleterTypes; // map from parent type to set of child types
  std::map<std::string, DeleterData>            deleterData;  // map from child types to corresponding deleter data
  std::map<std::string, EnumData>               enums;
  std::map<std::string, FlagData>               flags;
  std::map<std::string, HandleData>             handles;
  std::map<std::string, ScalarData>             scalars;
  std::map<std::string, StructData>             structs;
  std::set<std::string>                         tags;
  std::string                                   version;
  std::set<std::string>                         vkTypes;
  std::string                                   vulkanLicenseHeader;
};

void createDefaults( VkData const& vkData, std::map<std::string,std::string> & defaultValues );
void determineEnhancedReturnType(CommandData & commandData);
void determineReducedName(CommandData & commandData);
void determineReturnParam(CommandData & commandData);
void determineSkippedParams(CommandData & commandData);
void determineTemplateParam(CommandData & commandData);
void determineVectorParams(CommandData & commandData);
void enterProtect(std::ostream &os, std::string const& protect);
std::string extractTag(std::string const& name);
std::string findTag(std::string const& name, std::set<std::string> const& tags);
std::string generateEnumNameForFlags(std::string const& name);
bool hasPointerParam(std::vector<ParamData> const& params);
void leaveProtect(std::ostream &os, std::string const& protect);
void linkCommandToHandle(VkData & vkData, CommandData & commandData);
std::string readArraySize(tinyxml2::XMLNode * node, std::string& name);
bool readCommandParam( tinyxml2::XMLElement * element, std::set<std::string> & dependencies, std::vector<ParamData> & params );
void readCommandParams(tinyxml2::XMLElement* element, std::set<std::string> & dependencies, CommandData & commandData);
tinyxml2::XMLNode* readCommandParamType(tinyxml2::XMLNode* node, ParamData& param);
CommandData& readCommandProto(tinyxml2::XMLElement * element, VkData & vkData);
void readCommands( tinyxml2::XMLElement * element, VkData & vkData );
void readCommandsCommand(tinyxml2::XMLElement * element, VkData & vkData);
std::vector<std::string> readCommandSuccessCodes(tinyxml2::XMLElement* element, std::set<std::string> const& tags);
void readComment(tinyxml2::XMLElement * element, std::string & header);
void readEnums( tinyxml2::XMLElement * element, VkData & vkData );
void readEnumsEnum( tinyxml2::XMLElement * element, EnumData & enumData );
void readDisabledExtensionRequire(tinyxml2::XMLElement * element, VkData & vkData);
void readExtensionCommand(tinyxml2::XMLElement * element, std::map<std::string, CommandData> & commands, std::string const& protect);
void readExtensionEnum(tinyxml2::XMLElement * element, std::map<std::string, EnumData> & enums, std::string const& tag);
void readExtensionRequire(tinyxml2::XMLElement * element, VkData & vkData, std::string const& protect, std::string const& tag);
void readExtensions( tinyxml2::XMLElement * element, VkData & vkData );
void readExtensionsExtension(tinyxml2::XMLElement * element, VkData & vkData);
void readExtensionType(tinyxml2::XMLElement * element, VkData & vkData, std::string const& protect);
tinyxml2::XMLNode* readType(tinyxml2::XMLNode* element, std::string & type, std::string & pureType);
void readTypeBasetype( tinyxml2::XMLElement * element );
void readTypeBitmask( tinyxml2::XMLElement * element, VkData & vkData);
void readTypeDefine( tinyxml2::XMLElement * element, VkData & vkData );
void readTypeFuncpointer( tinyxml2::XMLElement * element );
void readTypeHandle(tinyxml2::XMLElement * element, VkData & vkData);
void readTypeStruct( tinyxml2::XMLElement * element, VkData & vkData, bool isUnion );
void readTypeStructMember( tinyxml2::XMLElement * element, std::vector<MemberData> & members, std::set<std::string> & dependencies );
void readTypeStructMember(Type* type, tinyxml2::XMLElement * element);
tinyxml2::XMLNode* readTypeStructMemberType(tinyxml2::XMLNode* element, std::string& type);
void readTags(tinyxml2::XMLElement * element, std::set<std::string> & tags);
void readTypes(tinyxml2::XMLElement * element, VkData & vkData);
std::string reduceName(std::string const& name, bool singular = false);
void registerDeleter(VkData & vkData, CommandData const& commandData);
std::string startLowerCase(std::string const& input);
std::string startUpperCase(std::string const& input);
std::string strip(std::string const& value, std::string const& prefix, std::string const& postfix = std::string());
std::string stripPluralS(std::string const& name);
std::string toCamelCase(std::string const& value);
std::string toUpperCase(std::string const& name);
std::string trimEnd(std::string const& input);
void writeCall(std::ostream & os, CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular);
std::string generateCall(CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular);
void writeCallCountParameter(std::ostream & os, CommandData const& commandData, bool singular, std::map<size_t, size_t>::const_iterator it);
void writeCallParameter(std::ostream & os, ParamData const& paramData, std::set<std::string> const& vkTypes);
void writeCallPlainTypeParameter(std::ostream & os, ParamData const& paramData);
void writeCallVectorParameter(std::ostream & os, CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular, std::map<size_t, size_t>::const_iterator it);
void writeCallVulkanTypeParameter(std::ostream & os, ParamData const& paramData);
void writeDeleterClasses(std::ostream & os, std::pair<std::string, std::set<std::string>> const& deleterTypes, std::map<std::string, DeleterData> const& deleterData);
void writeDeleterForwardDeclarations(std::ostream &os, std::pair<std::string, std::set<std::string>> const& deleterTypes, std::map<std::string, DeleterData> const& deleterData);
void writeFunction(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool definition, bool enhanced, bool singular, bool unique);
void writeFunctionBodyEnhanced(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool singular);
void writeFunctionBodyEnhancedCall(std::ostream &os, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular);
void writeFunctionBodyEnhancedCallResult(std::ostream &os, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular);
void writeFunctionBodyEnhancedCallTwoStep(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData);
void writeFunctionBodyEnhancedCallTwoStepChecked(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData);
void writeFunctionBodyEnhancedCallTwoStepIterate(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData);
void writeFunctionBodyEnhancedLocalCountVariable(std::ostream & os, std::string const& indentation, CommandData const& commandData);
std::string writeFunctionBodyEnhancedLocalReturnVariable(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool singular);
void writeFunctionBodyEnhancedMultiVectorSizeCheck(std::ostream & os, std::string const& indentation, CommandData const& commandData);
void writeFunctionBodyEnhancedReturnResultValue(std::ostream & os, std::string const& indentation, std::string const& returnName, CommandData const& commandData, bool singular);
void writeFunctionBodyStandard(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData);
void writeFunctionBodyUnique(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool singular);
void writeFunctionHeaderArguments(std::ostream & os, VkData const& vkData, CommandData const& commandData, bool enhanced, bool singular, bool withDefaults);
void writeFunctionHeaderArgumentsEnhanced(std::ostream & os, VkData const& vkData, CommandData const& commandData, bool singular, bool withDefaults);
void writeFunctionHeaderArgumentsStandard(std::ostream & os, CommandData const& commandData);
void writeFunctionHeaderName(std::ostream & os, std::string const& name, bool singular, bool unique);
void writeFunctionHeaderReturnType(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool enhanced, bool singular, bool unique);
void writeFunctionHeaderTemplate(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool withDefault);
void writeReinterpretCast(std::ostream & os, bool leadingConst, bool vulkanType, std::string const& type, bool trailingPointerToConst);
void writeStandardOrEnhanced(std::ostream & os, std::string const& standard, std::string const& enhanced);
void writeStructConstructor( std::ostream & os, std::string const& name, StructData const& structData, std::set<std::string> const& vkTypes, std::map<std::string,std::string> const& defaultValues );
void writeStructSetter( std::ostream & os, std::string const& structureName, MemberData const& memberData, std::set<std::string> const& vkTypes, std::map<std::string,StructData> const& structs );
void writeTypeCommand(std::ostream & os, VkData const& vkData);
void writeTypeCommand(std::ostream &os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool definition);
void writeTypeEnum(std::ostream & os, EnumData const& enumData);
bool isErrorEnum(std::string const& enumName);
std::string stripErrorEnumPrefix(std::string const& enumName);
void writeTypeFlags(std::ostream & os, std::string const& flagsName, FlagData const& flagData, EnumData const& enumData);
void writeTypeHandle(std::ostream & os, VkData const& vkData, HandleData const& handle);
void writeTypeScalar( std::ostream & os );
void writeTypeStruct( std::ostream & os, VkData const& vkData, std::map<std::string,std::string> const& defaultValues );
void writeTypeUnion( std::ostream & os, VkData const& vkData, std::map<std::string,std::string> const& defaultValues );
void writeTypes(std::ostream & os, VkData const& vkData, std::map<std::string, std::string> const& defaultValues);
void writeVersionCheck(std::ostream & os, std::string const& version);

void EnumData::addEnumMember(std::string const &name, std::string const& tag)
{
  NameValue nv;
  nv.name = "e" + toCamelCase(strip(name, prefix, postfix));
  nv.value = name;
  if (bitmask)
  {
    size_t pos = nv.name.find("Bit");
    if (pos != std::string::npos)
    {
      nv.name.erase(pos, 3);
    }
  }
  if (!tag.empty() && (nv.name.substr(nv.name.length() - tag.length()) == toCamelCase(tag)))
  {
    nv.name = nv.name.substr(0, nv.name.length() - tag.length()) + tag;
  }
  members.push_back(nv);
}

void createDefaults( VkData const& vkData, std::map<std::string,std::string> & defaultValues )
{
	// TODO: Can probbly delete defaultValues?
  //for (auto dependency : vkData.dependencies)
  //{
  //  assert( defaultValues.find( dependency.name ) == defaultValues.end() );
  //  switch( dependency.category )
  //  {
  //    case DependencyData::Category::COMMAND :    // commands should never be asked for defaults
  //      break;
  //    case DependencyData::Category::ENUM :
  //      {
  //        assert(vkData.enums.find(dependency.name) != vkData.enums.end());
  //        EnumData const & enumData = vkData.enums.find(dependency.name)->second;
  //        if (!enumData.members.empty())
  //        {
  //          defaultValues[dependency.name] = dependency.name + "::" + vkData.enums.find(dependency.name)->second.members.front().name;
  //        }
  //        else
  //        {
  //          defaultValues[dependency.name] = dependency.name + "()";
  //        }
  //      }
  //      break;
  //    case DependencyData::Category::FLAGS :
  //    case DependencyData::Category::HANDLE:
  //    case DependencyData::Category::STRUCT:
  //    case DependencyData::Category::UNION :        // just call the default constructor for flags, structs, and structs (which are mapped to classes)
  //      defaultValues[dependency.name] = dependency.name + "()";
  //      break;
  //    case DependencyData::Category::FUNC_POINTER : // func_pointers default to nullptr
  //      defaultValues[dependency.name] = "nullptr";
  //      break;
  //    case DependencyData::Category::REQUIRED :     // all required default to "0"
  //    case DependencyData::Category::SCALAR :       // all scalars default to "0"
  //      defaultValues[dependency.name] = "0";
  //      break;
  //    default :
  //      assert( false );
  //      break;
  //  }
  //}
}

void determineReducedName(CommandData & commandData)
{
  commandData.reducedName = commandData.fullName;
  std::string searchName = commandData.params[0].pureType;
  size_t pos = commandData.fullName.find(searchName);
  if ((pos == std::string::npos) && isupper(searchName[0]))
  {
    searchName[0] = tolower(searchName[0]);
    pos = commandData.fullName.find(searchName);
  }
  if (pos != std::string::npos)
  {
    commandData.reducedName.erase(pos, searchName.length());
  }
  else if ((searchName == "commandBuffer") && (commandData.fullName.find("cmd") == 0))
  {
    commandData.reducedName.erase(0, 3);
    pos = 0;
  }
  if ((pos == 0) && isupper(commandData.reducedName[0]))
  {
    commandData.reducedName[0] = tolower(commandData.reducedName[0]);
  }
}

void determineEnhancedReturnType(CommandData & commandData)
{
  std::string returnType;
  // if there is a return parameter of type void or Result, and if it's of type Result it either has just one success code
  // or two success codes, where the second one is of type eIncomplete and it's a two-step process
  // -> we can return that parameter
  if ((commandData.returnParam != ~0)
    && ((commandData.returnType == "void")
      || ((commandData.returnType == "Result")
        && ((commandData.successCodes.size() == 1)
          || ((commandData.successCodes.size() == 2)
            && (commandData.successCodes[1] == "eIncomplete")
            && commandData.twoStep)))))
  {
    if (commandData.vectorParams.find(commandData.returnParam) != commandData.vectorParams.end())
    {
      // the return parameter is a vector-type parameter
      if (commandData.params[commandData.returnParam].pureType == "void")
      {
        // for a vector of void, we use a vector of uint8_t, instead
        commandData.enhancedReturnType = "std::vector<uint8_t,Allocator>";
      }
      else
      {
        // for the other parameters, we use a vector of the pure type
        commandData.enhancedReturnType = "std::vector<" + commandData.params[commandData.returnParam].pureType + ",Allocator>";
      }
    }
    else
    {
      // it's a simple parameter -> get the type and just remove the trailing '*' (originally, it's a pointer)
      assert(commandData.params[commandData.returnParam].type.back() == '*');
      assert(commandData.params[commandData.returnParam].type.find("const") == std::string::npos);
      commandData.enhancedReturnType = commandData.params[commandData.returnParam].type;
      commandData.enhancedReturnType.pop_back();
    }
  }
  else if ((commandData.returnType == "Result") && (commandData.successCodes.size() == 1))
  {
    // an original return of type "Result" with just one successCode is changed to void, errors throw an exception
    commandData.enhancedReturnType = "void";
  }
  else
  {
    // the return type just stays the original return type
    commandData.enhancedReturnType = commandData.returnType;
  }
}

void determineReturnParam(CommandData & commandData)
{
  // for return types of type Result or void, we can replace determine a parameter to return
  if ((commandData.returnType == "Result") || (commandData.returnType == "void"))
  {
    for (size_t i = 0; i < commandData.params.size(); i++)
    {
      if ((commandData.params[i].type.find('*') != std::string::npos)
        && (commandData.params[i].type.find("const") == std::string::npos)
        && std::find_if(commandData.vectorParams.begin(), commandData.vectorParams.end(), [i](std::pair<size_t, size_t> const& vp) { return vp.second == i; }) == commandData.vectorParams.end()
        && ((commandData.vectorParams.find(i) == commandData.vectorParams.end()) || commandData.twoStep || (commandData.successCodes.size() == 1)))
      {
        // it's a non-const pointer, not a vector-size parameter, if it's a vector parameter, its a two-step process or there's just one success code
        // -> look for another non-cost pointer argument
        auto paramIt = std::find_if(commandData.params.begin() + i + 1, commandData.params.end(), [](ParamData const& pd)
        {
          return (pd.type.find('*') != std::string::npos) && (pd.type.find("const") == std::string::npos);
        });
        // if there is another such argument, we can't decide which one to return -> return none (~0)
        // otherwise return the index of the selcted parameter
        commandData.returnParam = paramIt != commandData.params.end() ? ~0 : i;
      }
    }
  }
}

void determineSkippedParams(CommandData & commandData)
{
  // the size-parameters of vector parameters are not explicitly used in the enhanced API
  std::for_each(commandData.vectorParams.begin(), commandData.vectorParams.end(), [&commandData](std::pair<size_t, size_t> const& vp) { if (vp.second != ~0) commandData.skippedParams.insert(vp.second); });
  // and the return parameter is also skipped
  if (commandData.returnParam != ~0)
  {
    commandData.skippedParams.insert(commandData.returnParam);
  }
}

void determineTemplateParam(CommandData & commandData)
{
  for (size_t i = 0; i < commandData.params.size(); i++)
  {
    // any vector parameter on the pure type void is templatized in the enhanced API
    if ((commandData.vectorParams.find(i) != commandData.vectorParams.end()) && (commandData.params[i].pureType == "void"))
    {
#if !defined(NDEBUG)
      for (size_t j = i + 1; j < commandData.params.size(); j++)
      {
        assert((commandData.vectorParams.find(j) == commandData.vectorParams.end()) || (commandData.params[j].pureType != "void"));
      }
#endif
      commandData.templateParam = i;
      break;
    }
  }
  assert((commandData.templateParam == ~0) || (commandData.vectorParams.find(commandData.templateParam) != commandData.vectorParams.end()));
}

void determineVectorParams(CommandData & commandData)
{
  // look for the parameters whose len equals the name of an other parameter
  for (auto it = commandData.params.begin(), begin = it, end = commandData.params.end(); it != end; ++it)
  {
    if (!it->len.empty())
    {
      auto findLambda = [it](ParamData const& pd) { return pd.name == it->len; };
      auto findIt = std::find_if(begin, it, findLambda);                        // look for a parameter named as the len of this parameter
      assert((std::count_if(begin, end, findLambda) == 0) || (findIt < it));    // make sure, there is no other parameter like that
      // add this parameter as a vector parameter, using the len-name parameter as the second value (or ~0 if there is nothing like that)
      commandData.vectorParams.insert(std::make_pair(std::distance(begin, it), findIt < it ? std::distance(begin, findIt) : ~0));
      assert((commandData.vectorParams[std::distance(begin, it)] != ~0)
        || (it->len == "null-terminated")
        || (it->len == "pAllocateInfo::descriptorSetCount")
        || (it->len == "pAllocateInfo::commandBufferCount"));
    }
  }
}

void enterProtect(std::ostream &os, std::string const& protect)
{
  if (!protect.empty())
  {
    os << "#ifdef " << protect << std::endl;
  }
}

std::string extractTag(std::string const& name)
{
  // the name is supposed to look like: VK_<tag>_<other>
  size_t start = name.find('_');
  assert((start != std::string::npos) && (name.substr(0, start) == "VK"));
  size_t end = name.find('_', start + 1);
  assert(end != std::string::npos);
  return name.substr(start + 1, end - start - 1);
}

std::string findTag(std::string const& name, std::set<std::string> const& tags)
{
  // find the tag in a name, return that tag or an empty string
  auto tagIt = std::find_if(tags.begin(), tags.end(), [&name](std::string const& t)
  {
    size_t pos = name.find(t);
    return (pos != std::string::npos) && (pos == name.length() - t.length());
  });
  return tagIt != tags.end() ? *tagIt : "";
}

std::string generateEnumNameForFlags(std::string const& name)
{
	// create a string, where the substring "Flags" is replaced by "FlagBits"
	std::string generatedName = name;
	size_t pos = generatedName.rfind("Flags");
	assert(pos != std::string::npos);
	generatedName.replace(pos, 5, "FlagBits");
	return generatedName;
}

bool hasPointerParam(std::vector<ParamData> const& params)
{
  // check if any of the parameters is a pointer
  auto it = std::find_if(params.begin(), params.end(), [](ParamData const& pd)
  {
    return (pd.type.find('*') != std::string::npos);
  });
  return it != params.end();
}

void leaveProtect(std::ostream &os, std::string const& protect)
{
  if (!protect.empty())
  {
    os << "#endif /*" << protect << "*/" << std::endl;
  }
}

void linkCommandToHandle(VkData & vkData, CommandData & commandData)
{
  // first, find the handle named like the type of the first argument
  // if there is no such handle, look for the unnamed "handle", that gathers all the functions not tied to a specific handle
  assert(!commandData.params.empty());
  std::map<std::string, HandleData>::iterator hit = vkData.handles.find(commandData.params[0].pureType);
  if (hit == vkData.handles.end())
  {
    hit = vkData.handles.find("");
  }
  assert(hit != vkData.handles.end());

  // put the command into the handle's list of commands, and store the handle in the commands className
  hit->second.commands.push_back(commandData.fullName);
  commandData.className = hit->first;

  // TODO: Removed dependencies

  //// add the dependencies of the command to the dependencies of the handle
  //DependencyData const& commandDD = vkData.dependencies.back();
  //std::list<DependencyData>::iterator handleDD = std::find_if(vkData.dependencies.begin(), vkData.dependencies.end(), [hit](DependencyData const& dd) { return dd.name == hit->first; });
  //assert((handleDD != vkData.dependencies.end()) || hit->first.empty());
  //if (handleDD != vkData.dependencies.end())
  //{
  //  std::copy_if(commandDD.dependencies.begin(), commandDD.dependencies.end(), std::inserter(handleDD->dependencies, handleDD->dependencies.end()), [hit](std::string const& d) { return d != hit->first; });
  //}
}

std::string readArraySize(tinyxml2::XMLNode * node, std::string& name)
{
  std::string arraySize;
  if (name.back() == ']')
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

bool readCommandParam( tinyxml2::XMLElement * element, std::set<std::string> & dependencies, std::vector<ParamData> & params )
{
  ParamData param;

  if (element->Attribute("len"))
  {
    param.len = element->Attribute("len");
  }

  // get the type of the parameter, and put it into the list of dependencies
  tinyxml2::XMLNode * child = readCommandParamType(element->FirstChild(), param);
  dependencies.insert(param.pureType);

  assert( child->ToElement() && ( strcmp( child->Value(), "name" ) == 0 ) );
  param.name = child->ToElement()->GetText();

  param.arraySize = readArraySize(child, param.name);

  param.optional = element->Attribute("optional") && (strcmp(element->Attribute("optional"), "true") == 0);

  params.push_back(param);

  // an optional parameter with "false,true" value is supposed to be part of a two-step algorithm: first get the size, than use it
  return element->Attribute("optional") && (strcmp(element->Attribute("optional"), "false,true") == 0);
}

void readCommandParams(tinyxml2::XMLElement* element, std::set<std::string> & dependencies, CommandData & commandData)
{
  // iterate over the siblings of the element and read the command parameters
  assert(element);
  while (element = element->NextSiblingElement())
  {
    std::string value = element->Value();
    if (value == "param")
    {
      commandData.twoStep |= readCommandParam(element, dependencies, commandData.params);
    }
    else
    {
      // ignore these values!
      assert((value == "implicitexternsyncparams") || (value == "validity"));
    }
  }
}

tinyxml2::XMLNode* readCommandParamType(tinyxml2::XMLNode* node, ParamData& param)
{
  assert(node);
  if (node->ToText())
  {
    // start type with "const" or "struct", if needed
    std::string value = trimEnd(node->Value());
    assert((value == "const") || (value == "struct"));
    param.type = value + " ";
    node = node->NextSibling();
    assert(node);
  }

  // get the pure type
  assert(node->ToElement() && (strcmp(node->Value(), "type") == 0) && node->ToElement()->GetText());
  std::string type = strip(node->ToElement()->GetText(), "Vk");
  param.type += type;
  param.pureType = type;

  // end with "*", "**", or "* const*", if needed
  node = node->NextSibling();
  assert(node);
  if (node->ToText())
  {
    std::string value = trimEnd(node->Value());
    assert((value == "*") || (value == "**") || (value == "* const*"));
    param.type += value;
    node = node->NextSibling();
  }

  return node;
}

CommandData& readCommandProto(tinyxml2::XMLElement * element, VkData & vkData)
{
  tinyxml2::XMLElement * typeElement = element->FirstChildElement();
  assert( typeElement && ( strcmp( typeElement->Value(), "type" ) == 0 ) );
  tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
  assert( nameElement && ( strcmp( nameElement->Value(), "name" ) == 0 ) );
  assert( !nameElement->NextSiblingElement() );

  // get return type and name of the command
  std::string type = strip( typeElement->GetText(), "Vk" );
  std::string name = startLowerCase(strip(nameElement->GetText(), "vk"));

  // TODO: Removed dependencies

  //// add an empty DependencyData to this name
  //vkData.dependencies.push_back( DependencyData( DependencyData::Category::COMMAND, name ) );

  // insert an empty CommandData into the commands-map, and return the newly created CommandData
  assert( vkData.commands.find( name ) == vkData.commands.end() );
  return vkData.commands.insert( std::make_pair( name, CommandData(type, name) ) ).first->second;
}

void readCommands(tinyxml2::XMLElement * element, VkData & vkData)
{
  for (tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement())
  {
    assert(strcmp(child->Value(), "command") == 0);
    readCommandsCommand(child, vkData);
  }
}

void readCommandsCommand(tinyxml2::XMLElement * element, VkData & vkData)
{
  tinyxml2::XMLElement * child = element->FirstChildElement();
  assert( child && ( strcmp( child->Value(), "proto" ) == 0 ) );

  CommandData& commandData = readCommandProto(child, vkData);
  commandData.successCodes = readCommandSuccessCodes(element, vkData.tags);
  // TODO: Removed dependencies
  //readCommandParams(child, vkData.dependencies.back().dependencies, commandData);
  //determineReducedName(commandData);
  //linkCommandToHandle(vkData, commandData);
  //registerDeleter(vkData, commandData);
  //determineVectorParams(commandData);
  //determineReturnParam(commandData);
  //determineTemplateParam(commandData);
  //determineEnhancedReturnType(commandData);
  //determineSkippedParams(commandData);
}

std::vector<std::string> readCommandSuccessCodes(tinyxml2::XMLElement* element, std::set<std::string> const& tags)
{
  std::vector<std::string> results;
  if (element->Attribute("successcodes"))
  {
    std::string successCodes = element->Attribute("successcodes");

    // tokenize the successCodes string, using ',' as the separator
    size_t start = 0, end;
    do
    {
      end = successCodes.find(',', start);
      std::string code = successCodes.substr(start, end - start);
      std::string tag = findTag(code, tags);
      // on each success code: prepend 'e', strip "VK_" and a tag, convert it to camel case, and add the tag again
      results.push_back(std::string("e") + toCamelCase(strip(code, "VK_", tag)) + tag);
      start = end + 1;
    } while (end != std::string::npos);
  }
  return results;
}

void readComment(tinyxml2::XMLElement * element, std::string & header)
{
	assert(element->GetText());
	assert(header.empty());
	header = element->GetText();
	assert(header.find("\nCopyright") == 0);

	// erase the part after the Copyright text
	size_t pos = header.find("\n\n-----");
	assert(pos != std::string::npos);
	header.erase(pos);

	// replace any '\n' with "\n// to make comments of the license text"
	for (size_t pos = header.find('\n'); pos != std::string::npos; pos = header.find('\n', pos + 1))
	{
		header.replace(pos, 1, "\n// ");
	}

	// and add a little message on our own
	header += "\n\n// This header is generated from the Khronos Vulkan XML API Registry.";
}

void readEnums( tinyxml2::XMLElement * element, VkData & vkData )
{
	// TODO: Remember to add the name to dependencies or types if not done already
	// And fix names for bitmasks (should contain 'FlagBits')

  if (!element->Attribute("name"))
  {
    throw std::runtime_error(std::string("spec error: enums element is missing the name attribute"));
  }
  std::string name = strip(element->Attribute("name"), "Vk");

  if ( name != "API Constants" )    // skip the "API Constants"
  {
	  // TODO: Removed dependencies
    //// add an empty DependencyData on this name into the dependencies list
    //vkData.dependencies.push_back( DependencyData( DependencyData::Category::ENUM, name ) );

    // ad an empty EnumData on this name into the enums map
    std::map<std::string,EnumData>::iterator it = vkData.enums.insert( std::make_pair( name, EnumData(name) ) ).first;

    if (name == "Result")
    {
      // special handling for VKResult, as its enums just have VK_ in common
      it->second.prefix = "VK_";
    }
    else
    {
      if (!element->Attribute("type"))
      {
        throw std::runtime_error(std::string("spec error: enums name=\"" + name + "\" is missing the type attribute"));
      }
      std::string type = element->Attribute("type");

      if (type != "bitmask" && type != "enum")
      {
        throw std::runtime_error(std::string("spec error: enums name=\"" + name + "\" has unknown type " + type));
      }

      it->second.bitmask = (type == "bitmask");
      if (it->second.bitmask)
      {
        // for a bitmask enum, start with "VK", cut off the trailing "FlagBits", and convert that name to upper case
        // end that with "Bit"
        size_t pos = name.find("FlagBits");
        assert(pos != std::string::npos);
        it->second.prefix = "VK" + toUpperCase(name.substr(0, pos)) + "_";
      }
      else
      {
        // for a non-bitmask enum, start with "VK", and convert the name to upper case
        it->second.prefix = "VK" + toUpperCase(name) + "_";
      }

      // if the enum name contains a tag move it from the prefix to the postfix to generate correct enum value names.
      for (std::set<std::string>::const_iterator tit = vkData.tags.begin(); tit != vkData.tags.end(); ++tit)
      {
        if ((tit->length() < it->second.prefix.length()) && (it->second.prefix.substr(it->second.prefix.length() - tit->length() - 1) == (*tit + "_")))
        {
          it->second.prefix.erase(it->second.prefix.length() - tit->length() - 1);
          it->second.postfix = "_" + *tit;
          break;
        }
        else if ((tit->length() < it->second.name.length()) && (it->second.name.substr(it->second.name.length() - tit->length()) == *tit))
        {
          it->second.postfix = "_" + *tit;
          break;
        }
      }
    }

    readEnumsEnum( element, it->second );

    // add this enum to the set of Vulkan data types
    assert( vkData.vkTypes.find( name ) == vkData.vkTypes.end() );
    vkData.vkTypes.insert( name );
  }
}

void readEnumsEnum( tinyxml2::XMLElement * element, EnumData & enumData )
{
  // read the names of the enum values
  tinyxml2::XMLElement * child = element->FirstChildElement();
  while (child)
  {
    if ( child->Attribute( "name" ) )
    {
      enumData.addEnumMember(child->Attribute("name"), "");
    }
    child = child->NextSiblingElement();
  }
}

void readDisabledExtensionRequire(tinyxml2::XMLElement * element, VkData & vkData)
{
  for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
  {
    std::string value = child->Value();

    if ((value == "command") || (value == "type"))
    {
      // disable a command or a type !
      assert(child->Attribute("name"));
      std::string name = (value == "command") ? startLowerCase(strip(child->Attribute("name"), "vk")) : strip(child->Attribute("name"), "Vk");

	  // TODO: Removed dependencies

      //// search this name in the dependencies list and remove it
      //std::list<DependencyData>::const_iterator depIt = std::find_if(vkData.dependencies.begin(), vkData.dependencies.end(), [&name](DependencyData const& dd) { return(dd.name == name); });
      //assert(depIt != vkData.dependencies.end());
      //vkData.dependencies.erase(depIt);

      //// erase it from all dependency sets
      //for (auto & dep : vkData.dependencies)
      //{
      //  dep.dependencies.erase(name);
      //}

      if (value == "command")
      {
        // first unlink the command from its class
        auto commandsIt = vkData.commands.find(name);
        assert(commandsIt != vkData.commands.end());
        assert(!commandsIt->second.className.empty());
        auto handlesIt = vkData.handles.find(commandsIt->second.className);
        assert(handlesIt != vkData.handles.end());
        auto it = std::find(handlesIt->second.commands.begin(), handlesIt->second.commands.end(), name);
        assert(it != handlesIt->second.commands.end());
        handlesIt->second.commands.erase(it);

        // then remove the command
        vkData.commands.erase(name);
      }
      else
      {
        // a type simply needs to be removed from the structs and vkTypes sets
        assert((vkData.structs.find(name) != vkData.structs.end()) && (vkData.vkTypes.find(name) != vkData.vkTypes.end()));
        vkData.structs.erase(name);
        vkData.vkTypes.erase(name);
      }
    }
    else
    {
      // nothing to do for enums, no other values ever encountered
      assert(value == "enum");
    }
  }
}

void readExtensionCommand(tinyxml2::XMLElement * element, std::map<std::string, CommandData> & commands, std::string const& protect)
{
  // just add the protect string to the CommandData
  if (!protect.empty())
  {
    assert(element->Attribute("name"));
    std::string name = startLowerCase(strip(element->Attribute("name"), "vk"));
    std::map<std::string, CommandData>::iterator cit = commands.find(name);
    assert(cit != commands.end());
    cit->second.protect = protect;
  }
}

void readExtensionEnum(tinyxml2::XMLElement * element, std::map<std::string, EnumData> & enums, std::string const& tag)
{
  // TODO process enums which don't extend existing enums
  if (element->Attribute("extends"))
  {
    assert(element->Attribute("name"));
    assert(enums.find(strip(element->Attribute("extends"), "Vk")) != enums.end());
    assert(!!element->Attribute("bitpos") + !!element->Attribute("offset") + !!element->Attribute("value") == 1);
    auto enumIt = enums.find(strip(element->Attribute("extends"), "Vk"));
    assert(enumIt != enums.end());
    enumIt->second.addEnumMember(element->Attribute("name"), tag);
  }
}

void readExtensionRequire(tinyxml2::XMLElement * element, VkData & vkData, std::string const& protect, std::string const& tag)
{
  for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
  {
    std::string value = child->Value();

    if ( value == "command" )
    {
      readExtensionCommand(child, vkData.commands, protect);
    }
    else if (value == "type")
    {
      readExtensionType(child, vkData, protect);
    }
    else if ( value == "enum")
    {
      readExtensionEnum(child, vkData.enums, tag);
    }
    else
    {
      assert(value=="usage");
    }
  }
}

void readExtensions(tinyxml2::XMLElement * element, VkData & vkData)
{
  for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
  {
    assert( strcmp( child->Value(), "extension" ) == 0 );
    readExtensionsExtension( child, vkData );
  }
}

void readExtensionsExtension(tinyxml2::XMLElement * element, VkData & vkData)
{
  assert( element->Attribute( "name" ) );
  std::string tag = extractTag(element->Attribute("name"));
  assert(vkData.tags.find(tag) != vkData.tags.end());

  tinyxml2::XMLElement * child = element->FirstChildElement();
  assert(child && (strcmp(child->Value(), "require") == 0) && !child->NextSiblingElement());

  if (strcmp(element->Attribute("supported"), "disabled") == 0)
  {
    // kick out all the disabled stuff we've read before !!
    readDisabledExtensionRequire(child, vkData);
  }
  else
  {
    std::string protect;
    if (element->Attribute("protect"))
    {
      protect = element->Attribute("protect");
    }

    readExtensionRequire(child, vkData, protect, tag);
  }
}

void readExtensionType(tinyxml2::XMLElement * element, VkData & vkData, std::string const& protect)
{
	// TODO: Removed dependencies

  //// add the protect-string to the appropriate type: enum, flag, handle, scalar, or struct
  //if (!protect.empty())
  //{
  //  assert(element->Attribute("name"));
  //  std::string name = strip(element->Attribute("name"), "Vk");
  //  std::map<std::string, EnumData>::iterator eit = vkData.enums.find(name);
  //  if (eit != vkData.enums.end())
  //  {
  //    eit->second.protect = protect;
  //  }
  //  else
  //  {
  //    std::map<std::string, FlagData>::iterator fit = vkData.flags.find(name);
  //    if (fit != vkData.flags.end())
  //    {
  //      fit->second.protect = protect;

  //      // if the enum of this flags is auto-generated, protect it as well
  //      std::string enumName = generateEnumNameForFlags(name);
  //      std::map<std::string, EnumData>::iterator eit = vkData.enums.find(enumName);
  //      assert(eit != vkData.enums.end());
  //      if (eit->second.members.empty())
  //      {
  //        eit->second.protect = protect;
  //      }
  //    }
  //    else
  //    {
  //      std::map<std::string, HandleData>::iterator hait = vkData.handles.find(name);
  //      if (hait != vkData.handles.end())
  //      {
  //        hait->second.protect = protect;
  //      }
  //      else
  //      {
  //        std::map<std::string, ScalarData>::iterator scit = vkData.scalars.find(name);
  //        if (scit != vkData.scalars.end())
  //        {
  //          scit->second.protect = protect;
  //        }
  //        else
  //        {
  //          std::map<std::string, StructData>::iterator stit = vkData.structs.find(name);
  //          assert(stit != vkData.structs.end() && stit->second.protect.empty());
  //          stit->second.protect = protect;
  //        }
  //      }
  //    }
  //  }
  //}
}

void readTypeBasetype(tinyxml2::XMLElement * element)
{
	tinyxml2::XMLElement * typeElement = element->FirstChildElement();
	assert(typeElement && (strcmp(typeElement->Value(), "type") == 0) && typeElement->GetText());
	std::string type = typeElement->GetText();
	assert(type == "uint32_t" || type == "uint64_t");

	Type* underlying = typeOracle.getType(type);

	tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
	assert(nameElement && (strcmp(nameElement->Value(), "name") == 0) && nameElement->GetText());
	// TODO: Removed stripping Vk
	//std::string name = strip(nameElement->GetText(), "Vk");
	std::string newType = nameElement->GetText();

	typeOracle.scalarTypedef(underlying, newType);

	// TODO: Removed dependencies

	//dependencies.push_back(DependencyData(DependencyData::Category::SCALAR, name));
	//dependencies.back().dependencies.insert(type);
}

void readTypeBitmask(tinyxml2::XMLElement * element, VkData & vkData)
{
	assert(strcmp(element->GetText(), "typedef ") == 0);
	tinyxml2::XMLElement * typeElement = element->FirstChildElement();
	assert(typeElement && (strcmp(typeElement->Value(), "type") == 0) && typeElement->GetText() && (strcmp(typeElement->GetText(), "VkFlags") == 0));
	std::string type = typeElement->GetText();

	tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
	assert(nameElement && (strcmp(nameElement->Value(), "name") == 0) && nameElement->GetText());
	// TODO: Removed strip
	//std::string name = strip(nameElement->GetText(), "Vk");
	std::string name = nameElement->GetText();

	assert(!nameElement->NextSiblingElement());

	// In the regular C API, bitmasks are defined like for example VkQueueFlags
	// which is a typedef to VkFlags. This is the typedef that is used by the
	// various functions. The actual values are defined in enums tags with names
	// similar to VkQueueFlagBits, i.e. with 'Bits' at the end, using implicit
	// compatibility in C. Enums without members are not present in enums tags
	// and thus will remain as unknown Vulkan types (in functions that use them)
	// if I were to only define them in enums tags. For C this is not a problem,
	// but since I use Rust's type system I need to at least have empty enums.
	// By telling the type oracle about these types they are no longer unknown
	// but rather treated as actual enums that can be empty.

	typeOracle.bitmasks(name);
}

void readTypeDefine(tinyxml2::XMLElement * element, VkData & vkData)
{
	tinyxml2::XMLElement * child = element->FirstChildElement();
	if (child && (strcmp(child->GetText(), "VK_HEADER_VERSION") == 0))
	{
		vkData.version = element->LastChild()->ToText()->Value();
	}

	// ignore all the other defines
}

void readTypeFuncpointer(tinyxml2::XMLElement * element)
{
	// The typedef <ret> text node
	tinyxml2::XMLNode * node = element->FirstChild();
	assert(node && node->ToText());
	std::string text = node->Value();

	// This will match 'typedef TYPE (VKAPI_PTR *' and contain TYPE in match
	// group 1.
	std::regex re(R"(^typedef ([^ ]+) \(VKAPI_PTR \*$)");
	auto it = std::sregex_iterator(text.begin(), text.end(), re);
	auto end = std::sregex_iterator();
	assert(it != end);
	std::smatch match = *it;
	std::string returnType = match[1].str();

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
	std::vector<std::pair<Type*, std::string>> params;

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

		params.push_back({
			typeOracle.getType((constModifier ? "const " : "") + paramType + (pointer ? "*" : "")),
			paramName,
		});
	}

	typeOracle.functionTypedef(name, typeOracle.getType(returnType), params);

	// TODO: Removed dependencies

	//dependencies.push_back(DependencyData(DependencyData::Category::FUNC_POINTER, child->GetText()));
}

void readTypeHandle(tinyxml2::XMLElement * element, VkData & vkData)
{
	tinyxml2::XMLElement * typeElement = element->FirstChildElement();
	assert(typeElement && (strcmp(typeElement->Value(), "type") == 0) && typeElement->GetText());
	std::string type = typeElement->GetText();
	Type* underlying = nullptr;
	if (type == "VK_DEFINE_HANDLE") { // Defined as pointer meaning varying size
		underlying = typeOracle.getType("size_t");
	}
	else {
		assert(type == "VK_DEFINE_NON_DISPATCHABLE_HANDLE"); // Pointer on 64-bit and uint64_t otherwise -> always 64 bit
		underlying = typeOracle.getType("uint64_t");
	}

	tinyxml2::XMLElement * nameElement = typeElement->NextSiblingElement();
	assert(nameElement && (strcmp(nameElement->Value(), "name") == 0) && nameElement->GetText());
	// TODO: Removed strip
	//std::string name = strip(nameElement->GetText(), "Vk");
	std::string name = nameElement->GetText();

	typeOracle.handleTypedef(underlying, name);

	// TODO: Removed dependencies
	//vkData.dependencies.push_back(DependencyData(DependencyData::Category::HANDLE, name));
	//assert(vkData.vkTypes.find(name) == vkData.vkTypes.end());
	//vkData.vkTypes.insert(name);
	//assert(vkData.handles.find(name) == vkData.handles.end());
	//vkData.handles[name];
}

void readTypeStruct(tinyxml2::XMLElement * element, VkData & vkData, bool isUnion)
{
	assert(!element->Attribute("returnedonly") || (strcmp(element->Attribute("returnedonly"), "true") == 0));

	assert(element->Attribute("name"));
	// TODO: Removed strip
	//std::string name = strip(element->Attribute("name"), "Vk");
	std::string name = element->Attribute("name");

	// TODO: Removed dependencies
	//vkData.dependencies.push_back( DependencyData( isUnion ? DependencyData::Category::UNION : DependencyData::Category::STRUCT, name ) );

	// element->Attribute("returnedonly") is also applicable for structs and unions
	typeOracle.defineStruct(name, isUnion);
	Type* t = typeOracle.getType(name);

	for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
	{
		assert(child->Value() && strcmp(child->Value(), "member") == 0);
		// TODO: Removed dependencies
		//readTypeStructMember( child, it->second.members, vkData.dependencies.back().dependencies );

		readTypeStructMember(t, child);
	}
}

// Read a member tag of a struct, adding members to the provided struct.
void readTypeStructMember(Type* theStruct, tinyxml2::XMLElement * element) {
	// The attributes of member tags seem to mostly concern documentation
	// generation, so they are not of interest for the bindings.

	// Read the type, parsing modifiers to get a string of the type.
	std::string typeString("");
	tinyxml2::XMLNode* child = readTypeStructMemberType(element->FirstChild(), typeString);
	Type* type = typeOracle.getType(typeString);

	// After we have parsed the type we expect to find the name of the member
	assert(child->ToElement() && strcmp(child->Value(), "name") == 0 && child->ToElement()->GetText());
	std::string memberName = child->ToElement()->GetText();

	// Some members have more information about array size
	std::string arraySize = readArraySize(child, memberName);

	// Add member to struct
	theStruct->structAddMember(type, memberName, arraySize);
}

// Reads the type tag of a member tag, including potential text nodes around
// the type tag to get qualifiers. We pass the first node that could potentially
// be a text node.
tinyxml2::XMLNode* readTypeStructMemberType(tinyxml2::XMLNode* element, std::string& type)
{
	assert(element);

	if (element->ToText())
	{
		std::string value = trimEnd(element->Value());
		if (value == "const") {
			type = value + " ";
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
	// TODO: Removed strip
	//pureType = strip(element->ToElement()->GetText(), "Vk");
	//type += pureType;
	type += element->ToElement()->GetText();

	element = element->NextSibling();
	assert(element);
	if (element->ToText())
	{
		std::string value = trimEnd(element->Value());
		assert((value == "*") || (value == "**") || (value == "* const*"));
		type += value;
		element = element->NextSibling();
	}

	return element;
}

void readTags(tinyxml2::XMLElement * element, std::set<std::string> & tags)
{
	tags.insert("EXT");
	tags.insert("KHR");
	for (tinyxml2::XMLElement * child = element->FirstChildElement(); child; child = child->NextSiblingElement())
	{
		assert(child->Attribute("name"));
		tags.insert(child->Attribute("name"));
	}
}

void readTypes(tinyxml2::XMLElement * element, VkData & vkData)
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
				// TODO: Removed dependencies
				//readTypeBasetype(child, vkData.dependencies);
				readTypeBasetype(child);
			}
			else if (category == "bitmask")
			{
				// C typedefs for enums that are bitmasks.
				// TODO: Could probably be ignored (else clause)
				readTypeBitmask(child, vkData);
			}
			else if (category == "define")
			{
				// C code for #define directives. Generally not interested in
				// defines, but we can get Vulkan header version here.
				readTypeDefine(child, vkData);
			}
			else if (category == "funcpointer")
			{
				// C typedefs for function pointers.
				// TODO: Removed dependencies
				readTypeFuncpointer(child);
			}
			else if (category == "handle")
			{
				// C macros that define handle types such as VkInstance
				readTypeHandle(child, vkData);
			}
			else if (category == "struct")
			{
				readTypeStruct(child, vkData, false);
			}
			else if (category == "union")
			{
				readTypeStruct(child, vkData, true);
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

			// Never used in the API.
			if (name == "int")
			{
				continue;
			}

			Type* t = typeOracle.getType(name);
			assert(t && t->isCType());

			// TODO: Removed dependencies
			//vkData.dependencies.push_back(DependencyData(DependencyData::Category::REQUIRED, child->Attribute("name")));
		}
	}
}

std::string reduceName(std::string const& name, bool singular)
{
  std::string reducedName;
  if ((name[0] == 'p') && (1 < name.length()) && (isupper(name[1]) || name[1] == 'p'))
  {
    reducedName = strip(name, "p");
    reducedName[0] = tolower(reducedName[0]);
  }
  else
  {
    reducedName = name;
  }
  if (singular)
  {
    size_t pos = reducedName.rfind('s');
    assert(pos != std::string::npos);
    reducedName.erase(pos, 1);
  }

  return reducedName;
}

void registerDeleter(VkData & vkData, CommandData const& commandData)
{
  if ((commandData.fullName.substr(0, 7) == "destroy") || (commandData.fullName.substr(0, 4) == "free"))
  {
    std::string key;
    size_t valueIndex;
    switch (commandData.params.size())
    {
    case 2:
    case 3:
      assert(commandData.params.back().pureType == "AllocationCallbacks");
      key = (commandData.params.size() == 2) ? "" : commandData.params[0].pureType;
      valueIndex = commandData.params.size() - 2;
      break;
    case 4:
      key = commandData.params[0].pureType;
      valueIndex = 3;
      assert(vkData.deleterData.find(commandData.params[valueIndex].pureType) == vkData.deleterData.end());
      vkData.deleterData[commandData.params[valueIndex].pureType].pool = commandData.params[1].pureType;
      break;
    default:
      assert(false);
    }
    if (commandData.fullName == "destroyDevice")
    {
      key = "PhysicalDevice";
    }
    assert(vkData.deleterTypes[key].find(commandData.params[valueIndex].pureType) == vkData.deleterTypes[key].end());
    vkData.deleterTypes[key].insert(commandData.params[valueIndex].pureType);
    vkData.deleterData[commandData.params[valueIndex].pureType].call = commandData.reducedName;
  }
}

std::string startLowerCase(std::string const& input)
{
  return static_cast<char>(tolower(input[0])) + input.substr(1);
}

std::string startUpperCase(std::string const& input)
{
  return static_cast<char>(toupper(input[0])) + input.substr(1);
}

std::string strip(std::string const& value, std::string const& prefix, std::string const& postfix)
{
  std::string strippedValue = value;
  if (strippedValue.substr(0, prefix.length()) == prefix)
  {
    strippedValue.erase(0, prefix.length());
  }
  if (!postfix.empty() && (strippedValue.substr(strippedValue.length() - postfix.length()) == postfix))
  {
    strippedValue.erase(strippedValue.length() - postfix.length());
  }
  return strippedValue;
}

std::string stripPluralS(std::string const& name)
{
  std::string strippedName(name);
  size_t pos = strippedName.rfind('s');
  assert(pos != std::string::npos);
  strippedName.erase(pos, 1);
  return strippedName;
}

std::string toCamelCase(std::string const& value)
{
  assert(!value.empty() && (isupper(value[0]) || isdigit(value[0])));
  std::string result;
  result.reserve(value.size());
  result.push_back(value[0]);
  for (size_t i = 1; i < value.size(); i++)
  {
    if (value[i] != '_')
    {
      if ((value[i - 1] == '_') || isdigit(value[i-1]))
      {
        result.push_back(value[i]);
      }
      else
      {
        result.push_back(tolower(value[i]));
      }
    }
  }
  return result;
}

std::string toUpperCase(std::string const& name)
{
  assert(isupper(name.front()));
  std::string convertedName;

  for (size_t i = 0; i<name.length(); i++)
  {
    if (isupper(name[i]) && ((i == 0) || islower(name[i - 1]) || isdigit(name[i-1])))
    {
      convertedName.push_back('_');
    }
    convertedName.push_back(toupper(name[i]));
  }
  return convertedName;
}

// trim from end
std::string trimEnd(std::string const& input)
{
  std::string result = input;
  result.erase(std::find_if(result.rbegin(), result.rend(), [](char c) { return !std::isspace(c); }).base(), result.end());
  return result;
}

std::string generateCall(CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular)
{
  std::ostringstream call;
  writeCall(call, commandData, vkTypes, firstCall, singular);
  return call.str();
}

void writeCall(std::ostream & os, CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular)
{
  // get the parameter indices of the counter for vector parameters
  std::map<size_t,size_t> countIndices;
  for (std::map<size_t, size_t>::const_iterator it = commandData.vectorParams.begin(); it != commandData.vectorParams.end(); ++it)
  {
    countIndices.insert(std::make_pair(it->second, it->first));
  }

  // the original function call
  os << "vk" << startUpperCase(commandData.fullName) << "( ";

  if (!commandData.className.empty())
  {
    // if it's member of a class -> add the first parameter with "m_" as prefix
    os << "m_" << commandData.params[0].name;
  }

  for (size_t i=commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
  {
    if (0 < i)
    {
      os << ", ";
    }

    std::map<size_t, size_t>::const_iterator it = countIndices.find(i);
    if (it != countIndices.end())
    {
      writeCallCountParameter(os, commandData, singular, it);
    }
    else if ((it = commandData.vectorParams.find(i)) != commandData.vectorParams.end())
    {
      writeCallVectorParameter(os, commandData, vkTypes, firstCall, singular, it);
    }
    else
    {
      if (vkTypes.find(commandData.params[i].pureType) != vkTypes.end())
      {
        writeCallVulkanTypeParameter(os, commandData.params[i]);
      }
      else
      {
        writeCallPlainTypeParameter(os, commandData.params[i]);
      }
    }
  }
  os << " )";
}

void writeCallCountParameter(std::ostream & os, CommandData const& commandData, bool singular, std::map<size_t, size_t>::const_iterator it)
{
  // this parameter is a count parameter for a vector parameter
  if ((commandData.returnParam == it->second) && commandData.twoStep)
  {
    // the corresponding vector parameter is the return parameter and it's a two-step algorithm
    // -> use the pointer to a local variable named like the counter parameter without leading 'p'
    os << "&" << startLowerCase(strip(commandData.params[it->first].name, "p"));
  }
  else
  {
    // the corresponding vector parameter is not the return parameter, or it's not a two-step algorithm
    if (singular)
    {
      // for the singular version, the count is just 1.
      os << "1 ";
    }
    else
    {
      // for the non-singular version, the count is the size of the vector parameter
      // -> use the vector parameter name without leading 'p' to get the size (in number of elements, not in bytes)
      os << startLowerCase(strip(commandData.params[it->second].name, "p")) << ".size() ";
    }
    if (commandData.templateParam == it->second)
    {
      // if the vector parameter is templatized -> multiply by the size of that type to get the size in bytes
      os << "* sizeof( T ) ";
    }
  }
}

void writeCallPlainTypeParameter(std::ostream & os, ParamData const& paramData)
{
  // this parameter is just a plain type
  if (paramData.type.back() == '*')
  {
    // it's a pointer
    std::string parameterName = startLowerCase(strip(paramData.name, "p"));
    if (paramData.type.find("const") != std::string::npos)
    {
      // it's a const pointer
      if (paramData.pureType == "char")
      {
        // it's a const pointer to char -> it's a string -> get the data via c_str()
        os << parameterName;
        if (paramData.optional)
        {
          // it's optional -> might use nullptr
          os << " ? " << parameterName << "->c_str() : nullptr";
        }
        else
        {
          os << ".c_str()";
        }
      }
      else
      {
        // it's const pointer to void (only other type that occurs) -> just use the name
        assert((paramData.pureType == "void") && !paramData.optional);
        os << paramData.name;
      }
    }
    else
    {
      // it's a non-const pointer, and char is the only type that occurs -> use the address of the parameter
      assert(paramData.type.find("char") == std::string::npos);
      os << "&" << parameterName;
    }
  }
  else
  {
    // it's a plain parameter -> just use its name
    os << paramData.name;
  }
}

void writeCallVectorParameter(std::ostream & os, CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular, std::map<size_t, size_t>::const_iterator it)
{
  // this parameter is a vector parameter
  assert(commandData.params[it->first].type.back() == '*');
  if ((commandData.returnParam == it->first) && commandData.twoStep && firstCall)
  {
    // this parameter is the return parameter, and it's the first call of a two-step algorithm -> just just nullptr
    os << "nullptr";
  }
  else
  {
    std::string parameterName = startLowerCase(strip(commandData.params[it->first].name, "p"));
    std::set<std::string>::const_iterator vkit = vkTypes.find(commandData.params[it->first].pureType);
    if ((vkit != vkTypes.end()) || (it->first == commandData.templateParam))
    {
      // CHECK for !commandData.params[it->first].optional

      // this parameter is a vulkan type or a templated type -> need to reinterpret cast
      writeReinterpretCast(os, commandData.params[it->first].type.find("const") == 0, vkit != vkTypes.end(), commandData.params[it->first].pureType,
        commandData.params[it->first].type.rfind("* const") != std::string::npos);
      os << "( ";
      if (singular)
      {
        // in singular case, strip the plural-S from the name, and use the pointer to that thing
        os << "&" << stripPluralS(parameterName);
      }
      else
      {
        // in plural case, get the pointer to the data
        os << parameterName << ".data()";
      }
      os << " )";
    }
    else if (commandData.params[it->first].pureType == "char")
    {
      // the parameter is a vector to char -> it might be optional
      // besides that, the parameter now is a std::string -> get the pointer via c_str()
      os << parameterName;
      if (commandData.params[it->first].optional)
      {
        os << " ? " << parameterName << "->c_str() : nullptr";
      }
      else
      {
        os << ".c_str()";
      }
    }
    else
    {
      // this parameter is just a vetor -> get the pointer to its data
      os << parameterName << ".data()";
    }
  }
}

void writeCallVulkanTypeParameter(std::ostream & os, ParamData const& paramData)
{
  // this parameter is a vulkan type
  if (paramData.type.back() == '*')
  {
    // it's a pointer -> needs a reinterpret cast to the vulkan type
    std::string parameterName = startLowerCase(strip(paramData.name, "p"));
    writeReinterpretCast(os, paramData.type.find("const") != std::string::npos, true, paramData.pureType, false);
    os << "( ";
    if (paramData.optional)
    {
      // for an optional parameter, we need also a static_cast from optional type to const-pointer to pure type
      os << "static_cast<const " << paramData.pureType << "*>( " << parameterName << " )";
    }
    else
    {
      // other parameters can just use the pointer
      os << "&" << parameterName;
    }
    os << " )";
  }
  else
  {
    // a non-pointer parameter needs a static_cast from vk::-type to vulkan type
    os << "static_cast<Vk" << paramData.pureType << ">( " << paramData.name << " )";
  }
}

void writeFunction(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool definition, bool enhanced, bool singular, bool unique)
{
  if (enhanced && !singular)
  {
    writeFunctionHeaderTemplate(os, indentation, commandData, !definition);
  }
  os << indentation << (definition ? "VULKAN_HPP_INLINE " : "");
  writeFunctionHeaderReturnType(os, indentation, commandData, enhanced, singular, unique);
  if (definition && !commandData.className.empty())
  {
    os << commandData.className << "::";
  }
  writeFunctionHeaderName(os, commandData.reducedName, singular, unique);
  writeFunctionHeaderArguments(os, vkData, commandData, enhanced, singular, !definition);
  os << (definition ? "" : ";") << std::endl;

  if (definition)
  {
    // write the function body
    os << indentation << "{" << std::endl;
    if (enhanced)
    {
      if (unique)
      {
        writeFunctionBodyUnique(os, indentation, vkData, commandData, singular);
      }
      else
      {
        writeFunctionBodyEnhanced(os, indentation, vkData, commandData, singular);
      }
    }
    else
    {
      writeFunctionBodyStandard(os, indentation, vkData, commandData);
    }
    os << indentation << "}" << std::endl;
  }
}

void writeFunctionBodyEnhanced(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool singular)
{
  if (1 < commandData.vectorParams.size())
  {
    writeFunctionBodyEnhancedMultiVectorSizeCheck(os, indentation, commandData);
  }

  std::string returnName;
  if (commandData.returnParam != ~0)
  {
    returnName = writeFunctionBodyEnhancedLocalReturnVariable(os, indentation, commandData, singular);
  }

  if (commandData.twoStep)
  {
    assert(!singular);
    writeFunctionBodyEnhancedLocalCountVariable(os, indentation, commandData);

    // we now might have to check the result, resize the returned vector accordingly, and call the function again
    std::map<size_t, size_t>::const_iterator returnit = commandData.vectorParams.find(commandData.returnParam);
    assert(returnit != commandData.vectorParams.end() && (returnit->second != ~0));
    std::string sizeName = startLowerCase(strip(commandData.params[returnit->second].name, "p"));

    if (commandData.returnType == "Result")
    {
      if (1 < commandData.successCodes.size())
      {
        writeFunctionBodyEnhancedCallTwoStepIterate(os, indentation, vkData.vkTypes, returnName, sizeName, commandData);
      }
      else
      {
        writeFunctionBodyEnhancedCallTwoStepChecked(os, indentation, vkData.vkTypes, returnName, sizeName, commandData);
      }
    }
    else
    {
      writeFunctionBodyEnhancedCallTwoStep(os, indentation, vkData.vkTypes, returnName, sizeName, commandData);
    }
  }
  else
  {
    if (commandData.returnType == "Result")
    {
      writeFunctionBodyEnhancedCallResult(os, indentation, vkData.vkTypes, commandData, singular);
    }
    else
    {
      writeFunctionBodyEnhancedCall(os, indentation, vkData.vkTypes, commandData, singular);
    }
  }

  if ((commandData.returnType == "Result") || !commandData.successCodes.empty())
  {
    writeFunctionBodyEnhancedReturnResultValue(os, indentation, returnName, commandData, singular);
  }
  else if ((commandData.returnParam != ~0) && (commandData.returnType != commandData.enhancedReturnType))
  {
    // for the other returning cases, when the return type is somhow enhanced, just return the local returnVariable
    os << indentation << "  return " << returnName << ";" << std::endl;
  }
}

void writeFunctionBodyEnhanced(std::ostream &os, std::string const& templateString, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular)
{
  os << replaceWithMap(templateString, {
    { "call", generateCall(commandData, vkTypes, true, singular) },
    { "i", indentation }
  });

}

void writeFunctionBodyEnhancedCall(std::ostream &os, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular)
{
  std::string const templateString = "${i}  return ${call};\n";
  std::string const templateStringVoid = "${i}  ${call};\n";
  writeFunctionBodyEnhanced(os, commandData.returnType == "void" ? templateStringVoid : templateString, indentation, vkTypes, commandData, singular);
}

void writeFunctionBodyEnhancedCallResult(std::ostream &os, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular)
{
  std::string const templateString = "${i}  Result result = static_cast<Result>( ${call} );\n";
  writeFunctionBodyEnhanced(os, templateString, indentation, vkTypes, commandData, singular);
}

void writeFunctionBodyTwoStep(std::ostream & os, std::string const &templateString, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData)
{
  std::map<std::string, std::string> replacements = {
    { "sizeName", sizeName },
    { "returnName", returnName },
    { "call1", generateCall(commandData, vkTypes, true, false) },
    { "call2", generateCall(commandData, vkTypes, false, false) },
    { "i", indentation }
  };

  os << replaceWithMap(templateString, replacements);
}

void writeFunctionBodyEnhancedCallTwoStep(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData)
{
  std::string const templateString = 
R"(${i}  ${call1};
${i}  ${returnName}.resize( ${sizeName} );
${i}  ${call2};
)";
  writeFunctionBodyTwoStep(os, templateString, indentation, vkTypes, returnName, sizeName, commandData);
}

void writeFunctionBodyEnhancedCallTwoStepChecked(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData)
{
  std::string const templateString =
R"(${i}  Result result = static_cast<Result>( ${call1} );
${i}  if ( ( result == Result::eSuccess ) && ${sizeName} )
${i}  {
${i}    ${returnName}.resize( ${sizeName} );
${i}    result = static_cast<Result>( ${call2} );
${i}  }
)";
  writeFunctionBodyTwoStep(os, templateString, indentation, vkTypes, returnName, sizeName, commandData);
}

void writeFunctionBodyEnhancedCallTwoStepIterate(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData)
{
  std::string const templateString = 
R"(${i}  Result result;
${i}  do
${i}  {
${i}    result = static_cast<Result>( ${call1} );
${i}    if ( ( result == Result::eSuccess ) && ${sizeName} )
${i}    {
${i}      ${returnName}.resize( ${sizeName} );
${i}      result = static_cast<Result>( ${call2} );
${i}    }
${i}  } while ( result == Result::eIncomplete );
${i}  assert( ${sizeName} <= ${returnName}.size() );
${i}  ${returnName}.resize( ${sizeName} );
)";
  writeFunctionBodyTwoStep(os, templateString, indentation, vkTypes, returnName, sizeName, commandData);
}

void writeFunctionBodyEnhancedLocalCountVariable(std::ostream & os, std::string const& indentation, CommandData const& commandData)
{
  // local count variable to hold the size of the vector to fill
  assert(commandData.returnParam != ~0);

  std::map<size_t, size_t>::const_iterator returnit = commandData.vectorParams.find(commandData.returnParam);
  assert(returnit != commandData.vectorParams.end() && (returnit->second != ~0));
  assert((commandData.returnType == "Result") || (commandData.returnType == "void"));

  // take the pure type of the size parameter; strip the leading 'p' from its name for its local name
  os << indentation << "  " << commandData.params[returnit->second].pureType << " " << startLowerCase(strip(commandData.params[returnit->second].name, "p")) << ";" << std::endl;
}

std::string writeFunctionBodyEnhancedLocalReturnVariable(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool singular)
{
  std::string returnName = startLowerCase(strip(commandData.params[commandData.returnParam].name, "p"));

  // there is a returned parameter -> we need a local variable to hold that value
  if (commandData.returnType != commandData.enhancedReturnType)
  {
    // the returned parameter is somehow enanced by us
    os << indentation << "  ";
    if (singular)
    {
      // in singular case, just use the return parameters pure type for the return variable
      returnName = stripPluralS(returnName);
      os << commandData.params[commandData.returnParam].pureType << " " << returnName;
    }
    else
    {
      // in non-singular case, use the enhanced type for the return variable (like vector<...>)
      os << commandData.enhancedReturnType << " " << returnName;

      std::map<size_t, size_t>::const_iterator it = commandData.vectorParams.find(commandData.returnParam);
      if (it != commandData.vectorParams.end() && !commandData.twoStep)
      {
        // if the return parameter is a vector parameter, and not part of a two-step algorithm, initialize its size
        std::string size;
        if (it->second == ~0)
        {
          assert(!commandData.params[commandData.returnParam].len.empty());
          // the size of the vector is not given by an other parameter, but by some member of a parameter, described as 'parameter::member'
          // -> replace the '::' by '.' and filter out the leading 'p' to access that value
          size = startLowerCase(strip(commandData.params[commandData.returnParam].len, "p"));
          size_t pos = size.find("::");
          assert(pos != std::string::npos);
          size.replace(pos, 2, ".");
        }
        else
        {
          // the size of the vector is given by an other parameter
          // that means (as this is not a two-step algorithm) it's size is determined by some other vector parameter!
          // -> look for it and get it's actual size
          for (auto const& vectorParam : commandData.vectorParams)
          {
            if ((vectorParam.first != commandData.returnParam) && (vectorParam.second == it->second))
            {
              size = startLowerCase(strip(commandData.params[vectorParam.first].name, "p")) + ".size()";
              break;
            }
          }
        }
        assert(!size.empty());
        os << "( " << size << " )";
      }
    }
    os << ";" << std::endl;
  }
  else
  {
    // the return parameter is not enhanced -> the type is supposed to be a Result and there are more than one success codes!
    assert((commandData.returnType == "Result") && (1 < commandData.successCodes.size()));
    os << indentation << "  " << commandData.params[commandData.returnParam].pureType << " " << returnName << ";" << std::endl;
  }

  return returnName;
}

void writeFunctionBodyEnhancedMultiVectorSizeCheck(std::ostream & os, std::string const& indentation, CommandData const& commandData)
{
  std::string const templateString = 
R"#(#ifdef VULKAN_HPP_NO_EXCEPTIONS
${i}  assert( ${firstVectorName}.size() == ${secondVectorName}.size() );
#else
${i}  if ( ${firstVectorName}.size() != ${secondVectorName}.size() )
${i}  {
${i}    throw LogicError( "vk::${className}::${reducedName}: ${firstVectorName}.size() != ${secondVectorName}.size()" );
${i}  }
#endif  // VULKAN_HPP_NO_EXCEPTIONS
)#";


  // add some error checks if multiple vectors need to have the same size
  for (std::map<size_t, size_t>::const_iterator it0 = commandData.vectorParams.begin(); it0 != commandData.vectorParams.end(); ++it0)
  {
    if (it0->first != commandData.returnParam)
    {
      for (std::map<size_t, size_t>::const_iterator it1 = std::next(it0); it1 != commandData.vectorParams.end(); ++it1)
      {
        if ((it1->first != commandData.returnParam) && (it0->second == it1->second))
        {
          os << replaceWithMap(templateString, std::map<std::string, std::string>( {
            { "firstVectorName", startLowerCase(strip(commandData.params[it0->first].name, "p")) },
            { "secondVectorName", startLowerCase(strip(commandData.params[it1->first].name, "p")) },
            { "className", commandData.className },
            { "reducedName", commandData.reducedName},
            { "i", indentation}
          }));
        }
      }
    }
  }
}

void writeFunctionBodyEnhancedReturnResultValue(std::ostream & os, std::string const& indentation, std::string const& returnName, CommandData const& commandData, bool singular)
{
  // if the return type is "Result" or there is at least one success code, create the Result/Value construct to return
  os << indentation << "  return createResultValue( result, ";
  if (commandData.returnParam != ~0)
  {
    // if there's a return parameter, list it in the Result/Value constructor
    os << returnName << ", ";
  }

  // now the function name (with full namespace) as a string
  os << "\"vk::" << (commandData.className.empty() ? "" : commandData.className + "::") << (singular ? stripPluralS(commandData.reducedName) : commandData.reducedName) << "\"";

  if (!commandData.twoStep && (1 < commandData.successCodes.size()))
  {
    // and for the single-step algorithms with more than one success code list them all
    os << ", { Result::" << commandData.successCodes[0];
    for (size_t i = 1; i < commandData.successCodes.size(); i++)
    {
      os << ", Result::" << commandData.successCodes[i];
    }
    os << " }";
  }
  os << " );" << std::endl;
}

void writeFunctionBodyStandard(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData)
{
  os << indentation << "  ";
  bool castReturn = false;
  if (commandData.returnType != "void")
  {
    // there's something to return...
    os << "return ";

    castReturn = (vkData.vkTypes.find(commandData.returnType) != vkData.vkTypes.end());
    if (castReturn)
    {
      // the return-type is a vulkan type -> need to cast to vk::-type
      os << "static_cast<" << commandData.returnType << ">( ";
    }
  }

  // call the original function
  os << "vk" << startUpperCase(commandData.fullName) << "( ";

  if (!commandData.className.empty())
  {
    // the command is part of a class -> the first argument is the member variable, starting with "m_"
    os << "m_" << commandData.params[0].name;
  }

  // list all the arguments
  for (size_t i = commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
  {
    if (0 < i)
    {
      os << ", ";
    }

    if (vkData.vkTypes.find(commandData.params[i].pureType) != vkData.vkTypes.end())
    {
      // the parameter is a vulkan type
      if (commandData.params[i].type.back() == '*')
      {
        // it's a pointer -> need to reinterpret_cast it
        writeReinterpretCast(os, commandData.params[i].type.find("const") == 0, true, commandData.params[i].pureType, commandData.params[i].type.find("* const") != std::string::npos);
      }
      else
      {
        // it's a value -> need to static_cast ist
        os << "static_cast<Vk" << commandData.params[i].pureType << ">";
      }
      os << "( " << commandData.params[i].name << " )";
    }
    else
    {
      // it's a non-vulkan type -> just use it
      os << commandData.params[i].name;
    }
  }
  os << " )";

  if (castReturn)
  {
    // if we cast the return -> close the static_cast
    os << " )";
  }
  os << ";" << std::endl;
}

void writeFunctionBodyUnique(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool singular)
{
  // the unique version needs a Deleter object for destruction of the newly created stuff
  std::string type = commandData.params[commandData.returnParam].pureType;
  std::string typeValue = startLowerCase(type);
  os << indentation << "  " << type << "Deleter deleter( ";
  if (vkData.deleterData.find(commandData.className) != vkData.deleterData.end())
  {
    // if the Deleter is specific to the command's class, add '*this' to the deleter
    os << "*this, ";
  }

  // get the DeleterData corresponding to the returned type
  std::map<std::string, DeleterData>::const_iterator ddit = vkData.deleterData.find(type);
  assert(ddit != vkData.deleterData.end());
  if (ddit->second.pool.empty())
  {
    // if this type isn't pooled, use the allocator (provided as a function argument)
    os << "allocator";
  }
  else
  {
    // otherwise use the pool, which always is a member of the second argument
    os << startLowerCase(strip(commandData.params[1].name, "p")) << "." << startLowerCase(ddit->second.pool);
  }
  os << " );" << std::endl;

  bool returnsVector = !singular && (commandData.vectorParams.find(commandData.returnParam) != commandData.vectorParams.end());
  if (returnsVector)
  {
    // if a vector of data is returned, use a local variable to hold the returned data from the non-unique function call
    os << indentation << "  std::vector<" << type << ",Allocator> " << typeValue << "s = ";
  }
  else
  {
    // otherwise create a Unique stuff out of the returned data from the non-unique function call
    os << indentation << "  return Unique" << type << "( ";
  }

  // the call to the non-unique function
  os << (singular ? stripPluralS(commandData.fullName) : commandData.fullName) << "( ";
  bool argEncountered = false;
  for (size_t i = commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
  {
    if (commandData.skippedParams.find(i) == commandData.skippedParams.end())
    {
      if (argEncountered)
      {
        os << ", ";
      }
      argEncountered = true;

      // strip off the leading 'p' for pointer arguments
      std::string argumentName = (commandData.params[i].type.back() == '*') ? startLowerCase(strip(commandData.params[i].name, "p")) : commandData.params[i].name;
      if (singular && (commandData.vectorParams.find(i) != commandData.vectorParams.end()))
      {
        // and strip off the plural 's' if appropriate
        argumentName = stripPluralS(argumentName);
      }
      os << argumentName;
    }
  }
  os << " )";
  if (returnsVector)
  {
    std::string const stringTemplate = R"(;
${i}  std::vector<Unique${type}> unique${type}s;
${i}  unique${type}s.reserve( ${typeValue}s.size() );
${i}  for ( auto ${typeValue} : ${typeValue}s )
${i}  {
${i}    unique${type}s.push_back( Unique${type}( ${typeValue}, deleter ) );
${i}  }
${i}  return unique${type}s;
)";
    os << replaceWithMap(stringTemplate, std::map<std::string, std::string>{
      { "i", indentation },
      { "type", type },
      { "typeValue", typeValue }
    });
  }
  else
  {
    // for non-vector returns, just add the deleter (local variable) to the Unique-stuff constructor
    os << ", deleter );" << std::endl;
  }
}

void writeFunctionHeaderArguments(std::ostream & os, VkData const& vkData, CommandData const& commandData, bool enhanced, bool singular, bool withDefaults)
{
  os << "(";
  if (enhanced)
  {
    writeFunctionHeaderArgumentsEnhanced(os, vkData, commandData, singular, withDefaults);
  }
  else
  {
    writeFunctionHeaderArgumentsStandard(os, commandData);
  }
  os << ")";
  if (!commandData.className.empty())
  {
    os << " const";
  }
}

void writeFunctionHeaderArgumentsEnhanced(std::ostream & os, VkData const& vkData, CommandData const& commandData, bool singular, bool withDefaults)
{
  // check if there's at least one argument left to put in here
  if (commandData.skippedParams.size() + (commandData.className.empty() ? 0 : 1) < commandData.params.size())
  {
    // determine the last argument, where we might provide some default for
    size_t lastArgument = ~0;
    for (size_t i = commandData.params.size() - 1; i < commandData.params.size(); i--)
    {
      if (commandData.skippedParams.find(i) == commandData.skippedParams.end())
      {
        lastArgument = i;
        break;
      }
    }

    os << " ";
    bool argEncountered = false;
    for (size_t i = commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
    {
      if (commandData.skippedParams.find(i) == commandData.skippedParams.end())
      {
        if (argEncountered)
        {
          os << ", ";
        }
        std::string strippedParameterName = startLowerCase(strip(commandData.params[i].name, "p"));

        std::map<size_t, size_t>::const_iterator it = commandData.vectorParams.find(i);
        size_t rightStarPos = commandData.params[i].type.rfind('*');
        if (it == commandData.vectorParams.end())
        {
          // the argument ist not a vector
          if (rightStarPos == std::string::npos)
          {
            // and its not a pointer -> just use its type and name here
            os << commandData.params[i].type << " " << commandData.params[i].name;
            if (!commandData.params[i].arraySize.empty())
            {
              os << "[" << commandData.params[i].arraySize << "]";
            }

            if (withDefaults && (lastArgument == i))
            {
              // check if the very last argument is a flag without any bits -> provide some empty default for it
              std::map<std::string, FlagData>::const_iterator flagIt = vkData.flags.find(commandData.params[i].pureType);
              if (flagIt != vkData.flags.end())
              {
				  // TODO: Removed dependencies

                //// get the enum corresponding to this flag, to check if it's empty
                //std::list<DependencyData>::const_iterator depIt = std::find_if(vkData.dependencies.begin(), vkData.dependencies.end(), [&flagIt](DependencyData const& dd) { return(dd.name == flagIt->first); });
                //assert((depIt != vkData.dependencies.end()) && (depIt->dependencies.size() == 1));
                //std::map<std::string, EnumData>::const_iterator enumIt = vkData.enums.find(*depIt->dependencies.begin());
                ///assert(enumIt != vkData.enums.end());
                //if (enumIt->second.members.empty())
                //{
                //  // there are no bits in this flag -> provide the default
                //  os << " = " << commandData.params[i].pureType << "()";
                //}
              }
            }
          }
          else
          {
            // the argument is not a vector, but a pointer
            assert(commandData.params[i].type[rightStarPos] == '*');
            if (commandData.params[i].optional)
            {
              // for an optional argument, trim the trailing '*' from the type, and the leading 'p' from the name
              os << "Optional<" << trimEnd(commandData.params[i].type.substr(0, rightStarPos)) << "> " << strippedParameterName;
              if (withDefaults)
              {
                os << " = nullptr";
              }
            }
            else if (commandData.params[i].pureType == "void")
            {
              // for void-pointer, just use type and name
              os << commandData.params[i].type << " " << commandData.params[i].name;
            }
            else if (commandData.params[i].pureType != "char")
            {
              // for non-char-pointer, change to reference
              os << trimEnd(commandData.params[i].type.substr(0, rightStarPos)) << " & " << strippedParameterName;
            }
            else
            {
              // for char-pointer, change to const reference to std::string
              os << "const std::string & " << strippedParameterName;
            }
          }
        }
        else
        {
          // the argument is a vector
          // it's optional, if it's marked as optional and there's no size specified
          bool optional = commandData.params[i].optional && (it->second == ~0);
          assert((rightStarPos != std::string::npos) && (commandData.params[i].type[rightStarPos] == '*'));
          if (commandData.params[i].type.find("char") != std::string::npos)
          {
            // it's a char-vector -> use a std::string (either optional or a const-reference
            if (optional)
            {
              os << "Optional<const std::string> " << strippedParameterName;
              if (withDefaults)
              {
                os << " = nullptr";
              }
            }
            else
            {
              os << "const std::string & " << strippedParameterName;
            }
          }
          else
          {
            // it's a non-char vector (they are never optional)
            assert(!optional);
            if (singular)
            {
              // in singular case, change from pointer to reference
              os << trimEnd(commandData.params[i].type.substr(0, rightStarPos)) << " & " << stripPluralS(strippedParameterName);
            }
            else
            {
              // otherwise, use our ArrayProxy
              bool isConst = (commandData.params[i].type.find("const") != std::string::npos);
              os << "ArrayProxy<" << ((commandData.templateParam == i) ? (isConst ? "const T" : "T") : trimEnd(commandData.params[i].type.substr(0, rightStarPos))) << "> " << strippedParameterName;
            }
          }
        }
        argEncountered = true;
      }
    }
    os << " ";
  }
}

void writeFunctionHeaderArgumentsStandard(std::ostream & os, CommandData const& commandData)
{
  // for the standard case, just list all the arguments as we've got them
  bool argEncountered = false;
  for (size_t i = commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
  {
    if (argEncountered)
    {
      os << ",";
    }

    os << " " << commandData.params[i].type << " " << commandData.params[i].name;
    if (!commandData.params[i].arraySize.empty())
    {
      os << "[" << commandData.params[i].arraySize << "]";
    }
    argEncountered = true;
  }
  if (argEncountered)
  {
    os << " ";
  }
}

void writeFunctionHeaderName(std::ostream & os, std::string const& name, bool singular, bool unique)
{
  os << (singular ? stripPluralS(name) : name);
  if (unique)
  {
    os << "Unique";
  }
}

void writeFunctionHeaderReturnType(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool enhanced, bool singular, bool unique)
{
  std::string templateString;
  std::string returnType;
  if (enhanced)
  {
    // the enhanced function might return some pretty complex return stuff
    if (unique)
    {
      // the unique version returns something prefixed with 'Unique'; potentially a vector of that stuff
      // it's a vector, if it's not the singular version and the return parameter is a vector parameter
      bool returnsVector = !singular && (commandData.vectorParams.find(commandData.returnParam) != commandData.vectorParams.end());
      templateString = returnsVector ? "std::vector<Unique${returnType}> " : "Unique${returnType} ";
      returnType = commandData.params[commandData.returnParam].pureType;
      //os << replaceWithMap(, {{"returnType", commandData.params[commandData.returnParam].pureType }});
    }
    else if ((commandData.enhancedReturnType != commandData.returnType) && (commandData.returnType != "void"))
    {
      // if the enhanced return type differs from the original return type, and it's not void, we return a ResultValueType<...>::type
      if (!singular && (commandData.enhancedReturnType.find("Allocator") != std::string::npos))
      {
        // for the non-singular case with allocation, we need to prepend with 'typename' to keep compilers happy
        templateString = "typename ResultValueType<${returnType}>::type ";
      }
      else
      {
        templateString = "ResultValueType<${returnType}>::type ";
      }
      assert(commandData.returnType == "Result");
      // in singular case, we create the ResultValueType from the pure return type, otherwise from the enhanced return type
      returnType = singular ? commandData.params[commandData.returnParam].pureType : commandData.enhancedReturnType;
    }
    else if ((commandData.returnParam != ~0) && (1 < commandData.successCodes.size()))
    {
      // if there is a return parameter at all, and there are multiple success codes, we return a ResultValue<...> with the pure return type
      assert(commandData.returnType == "Result");
      templateString = "ResultValue<${returnType}> ";
      returnType = commandData.params[commandData.returnParam].pureType;
    }
    else
    {
      // and in every other case, we just return the enhanced return type.
      templateString = "${returnType} ";
      returnType = commandData.enhancedReturnType;
    }
  }
  else
  {
    // the non-enhanced function just uses the return type
    templateString = "${returnType} ";
    returnType = commandData.returnType;
  }
  os << replaceWithMap(templateString, { { "returnType", returnType } });
}

void writeFunctionHeaderTemplate(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool withDefault)
{
  if ((commandData.templateParam != ~0) && ((commandData.templateParam != commandData.returnParam) || (commandData.enhancedReturnType == "Result")))
  {
    // if there's a template parameter, not being the return parameter or where the enhanced return type is 'Result' -> templatize on type 'T'
    assert(commandData.enhancedReturnType.find("Allocator") == std::string::npos);
    os << indentation << "template <typename T>" << std::endl;
  }
  else if ((commandData.enhancedReturnType.find("Allocator") != std::string::npos))
  {
    // otherwise, if there's an Allocator used in the enhanced return type, we templatize on that Allocator
    assert((commandData.enhancedReturnType.substr(0, 12) == "std::vector<") && (commandData.enhancedReturnType.find(',') != std::string::npos) && (12 < commandData.enhancedReturnType.find(',')));
    os << indentation << "template <typename Allocator";
    if (withDefault)
    {
      // for the default type get the type from the enhancedReturnType, which is of the form 'std::vector<Type,Allocator>'
      os << " = std::allocator<" << commandData.enhancedReturnType.substr(12, commandData.enhancedReturnType.find(',') - 12) << ">";
    }
    os << "> " << std::endl;
  }
}

void writeReinterpretCast(std::ostream & os, bool leadingConst, bool vulkanType, std::string const& type, bool trailingPointerToConst)
{
  os << "reinterpret_cast<";
  if (leadingConst)
  {
    os << "const ";
  }
  if (vulkanType)
  {
    os << "Vk";
  }
  os << type;
  if (trailingPointerToConst)
  {
    os << "* const";
  }
  os << "*>";
}

void writeStandardOrEnhanced(std::ostream & os, std::string const& standard, std::string const& enhanced)
{
  if (standard == enhanced)
  {
    // standard and enhanced string are equal -> just use one of them and we're done
    os << standard;
  }
  else
  {
    // standard and enhanced string differ -> use both, wrapping the enhanced by !VULKAN_HPP_DISABLE_ENHANCED_MODE
    // determine the argument list of that standard, and compare it with that of the enhanced
    // if they are equal -> need to have just one; if they differ -> need to have both
    size_t standardStart = standard.find('(');
    size_t standardCount = standard.find(')', standardStart) - standardStart;
    size_t enhancedStart = enhanced.find('(');
    bool unchangedInterface = (standard.substr(standardStart, standardCount) == enhanced.substr(enhancedStart, standardCount));
    if (unchangedInterface)
    {
      os << "#ifdef VULKAN_HPP_DISABLE_ENHANCED_MODE" << std::endl;
    }
    os << standard
      << (unchangedInterface ? "#else" : "#ifndef VULKAN_HPP_DISABLE_ENHANCED_MODE") << std::endl
      << enhanced
      << "#endif /*VULKAN_HPP_DISABLE_ENHANCED_MODE*/" << std::endl;
  }
}

void writeStructConstructor( std::ostream & os, std::string const& name, StructData const& structData, std::set<std::string> const& vkTypes, std::map<std::string,std::string> const& defaultValues )
{
  // the constructor with all the elements as arguments, with defaults
  os << "    " << name << "( ";
  bool listedArgument = false;
  for (size_t i = 0; i<structData.members.size(); i++)
  {
    if (listedArgument)
    {
      os << ", ";
    }
    // skip members 'pNext' and 'sType', as they are never explicitly set
    if ((structData.members[i].name != "pNext") && (structData.members[i].name != "sType"))
    {
      // find a default value for the given pure type
      std::map<std::string, std::string>::const_iterator defaultIt = defaultValues.find(structData.members[i].pureType);
      assert(defaultIt != defaultValues.end());

      if (structData.members[i].arraySize.empty())
      {
        // the arguments name get a trailing '_', to distinguish them from the actual struct members
        // pointer arguments get a nullptr as default
        os << structData.members[i].type << " " << structData.members[i].name << "_ = " << (structData.members[i].type.back() == '*' ? "nullptr" : defaultIt->second);
      }
      else
      {
        // array members are provided as const reference to a std::array
        // the arguments name get a trailing '_', to distinguish them from the actual struct members
        // list as many default values as there are elements in the array
        os << "std::array<" << structData.members[i].type << "," << structData.members[i].arraySize << "> const& " << structData.members[i].name << "_ = { { " << defaultIt->second;
        size_t n = atoi(structData.members[i].arraySize.c_str());
        assert(0 < n);
        for (size_t j = 1; j < n; j++)
        {
          os << ", " << defaultIt->second;
        }
        os << " } }";
      }
      listedArgument = true;
    }
  }
  os << " )" << std::endl;

  // copy over the simple arguments
  bool firstArgument = true;
  for (size_t i = 0; i < structData.members.size(); i++)
  {
    if (structData.members[i].arraySize.empty())
    {
      // here, we can only handle non-array arguments
      std::string templateString = "      ${sep} ${member}( ${value} )\n";
      std::string sep = firstArgument ? ":" : ",";
      std::string member = structData.members[i].name;
      std::string value;

      // 'pNext' and 'sType' don't get an argument, use nullptr and the correct StructureType enum value to initialize them
      if (structData.members[i].name == "pNext")
      {
        value = "nullptr";
      }
      else if (structData.members[i].name == "sType")
      {
        value = std::string("StructureType::e") + name;
      }
      else
      {
        // the other elements are initialized by the corresponding argument (with trailing '_', as mentioned above)
        value = structData.members[i].name + "_";
      }
      os << replaceWithMap(templateString, { {"sep", sep}, {"member", member}, {"value", value} });
      firstArgument = false;
    }
  }

  // the body of the constructor, copying over data from argument list into wrapped struct
  os << "    {" << std::endl;
  for ( size_t i=0 ; i<structData.members.size() ; i++ )
  {
    if (!structData.members[i].arraySize.empty())
    {
      // here we can handle the arrays, copying over from argument (with trailing '_') to member
      // size is arraySize times sizeof type
      std::string member = structData.members[i].name;
      std::string arraySize = structData.members[i].arraySize;
      std::string type = structData.members[i].type;
      os << replaceWithMap("      memcpy( &${member}, ${member}_.data(), ${arraySize} * sizeof( ${type} ) );\n",
                            { {"member", member}, {"arraySize", arraySize }, {"type", type} });
    }
  }
  os << "    }" << std::endl
      << std::endl;

  std::string templateString = 
R"(    ${name}( Vk${name} const & rhs )
    {
      memcpy( this, &rhs, sizeof( ${name} ) );
    }

    ${name}& operator=( Vk${name} const & rhs )
    {
      memcpy( this, &rhs, sizeof( ${name} ) );
      return *this;
    }
)";

  os << replaceWithMap(templateString, { {"name", name } } );
}

void writeStructSetter( std::ostream & os, std::string const& structureName, MemberData const& memberData, std::set<std::string> const& vkTypes )
{
  if (memberData.type != "StructureType") // filter out StructureType, which is supposed to be immutable !
  {
    // the setters return a reference to the structure
    os << "    " << structureName << "& set" << startUpperCase(memberData.name) << "( ";
    if (memberData.arraySize.empty())
    {
      os << memberData.type << " ";
    }
    else
    {
      os << "std::array<" << memberData.type << "," << memberData.arraySize << "> ";
    }
    // add a trailing '_' to the argument to distinguish it from the structure member
    os << memberData.name << "_ )" << std::endl
      << "    {" << std::endl;
    // copy over the argument, either by assigning simple data, or by memcpy array data
    if (memberData.arraySize.empty())
    {
      os << "      " << memberData.name << " = " << memberData.name << "_";
    }
    else
    {
      os << "      memcpy( &" << memberData.name << ", " << memberData.name << "_.data(), " << memberData.arraySize << " * sizeof( " << memberData.type << " ) )";
    }
    os << ";" << std::endl
      << "      return *this;" << std::endl
      << "    }" << std::endl
      << std::endl;
  }
}

void writeTypeCommand(std::ostream & os, VkData const& vkData)
{
	// TODO: Removed dependencies

  //assert(vkData.commands.find(dependencyData.name) != vkData.commands.end());
  //CommandData const& commandData = vkData.commands.find(dependencyData.name)->second;
  //if (commandData.className.empty())
  //{
  //  if (commandData.fullName == "createInstance")
  //  {
  //    // special handling for createInstance, as we need to explicitly place the forward declarations and the deleter classes here
  //    auto deleterTypesIt = vkData.deleterTypes.find("");
  //    assert((deleterTypesIt != vkData.deleterTypes.end()) && (deleterTypesIt->second.size() == 1));

  //    writeDeleterForwardDeclarations(os, *deleterTypesIt, vkData.deleterData);
  //    writeTypeCommand(os, "  ", vkData, commandData, false);
  //    writeDeleterClasses(os, *deleterTypesIt, vkData.deleterData);
  //  }
  //  else
  //  {
  //    writeTypeCommand(os, "  ", vkData, commandData, false);
  //  }
  //  writeTypeCommand(os, "  ", vkData, commandData, true);
  //  os << std::endl;
  //}
}

void writeTypeCommand(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool definition)
{
  enterProtect(os, commandData.protect);

  // first create the standard version of the function
  std::ostringstream standard;
  writeFunction(standard, indentation, vkData, commandData, definition, false, false, false);

  // then the enhanced version, composed by up to four parts
  std::ostringstream enhanced;
  writeFunction(enhanced, indentation, vkData, commandData, definition, true, false, false);

  // then a singular version, if a sized vector would be returned
  std::map<size_t, size_t>::const_iterator returnVector = commandData.vectorParams.find(commandData.returnParam);
  bool singular = (returnVector != commandData.vectorParams.end()) && (returnVector->second != ~0) && (commandData.params[returnVector->second].type.back() != '*');
  if (singular)
  {
    writeFunction(enhanced, indentation, vkData, commandData, definition, true, true, false);
  }

  // special handling for createDevice and createInstance !
  bool specialWriteUnique = (commandData.reducedName == "createDevice") || (commandData.reducedName == "createInstance");

  // and then the same for the Unique* versions (a Deleter is available for the commandData's class, and the function starts with 'allocate' or 'create')
  if (((vkData.deleterData.find(commandData.className) != vkData.deleterData.end()) || specialWriteUnique) && ((commandData.reducedName.substr(0, 8) == "allocate") || (commandData.reducedName.substr(0, 6) == "create")))
  {
    enhanced << "#ifndef VULKAN_HPP_NO_SMART_HANDLE" << std::endl;
    writeFunction(enhanced, indentation, vkData, commandData, definition, true, false, true);

    if (singular)
    {
      writeFunction(enhanced, indentation, vkData, commandData, definition, true, true, true);
    }
    enhanced << "#endif /*VULKAN_HPP_NO_SMART_HANDLE*/" << std::endl;
  }

  // and write one or both of them
  writeStandardOrEnhanced(os, standard.str(), enhanced.str());
  leaveProtect(os, commandData.protect);
  os << std::endl;
}

void writeTypeEnum( std::ostream & os, EnumData const& enumData )
{
  // a named enum per enum, listing all its values by setting them to the original Vulkan names
  enterProtect(os, enumData.protect);
  os << "  enum class " << enumData.name << std::endl
      << "  {" << std::endl;
  for ( size_t i=0 ; i<enumData.members.size() ; i++ )
  {
    os << "    " << enumData.members[i].name << " = " << enumData.members[i].value;
    if ( i < enumData.members.size() - 1 )
    {
      os << ",";
    }
    os << std::endl;
  }
  os << "  };" << std::endl;
  leaveProtect(os, enumData.protect);
  os << std::endl;
}

bool isErrorEnum(std::string const& enumName)
{
  return (enumName.substr(0, 6) == "eError");
}

std::string stripErrorEnumPrefix(std::string const& enumName)
{
  assert(isErrorEnum(enumName));
  return strip(enumName, "eError");
}

void writeDeleterClasses(std::ostream & os, std::pair<std::string, std::set<std::string>> const& deleterTypes, std::map<std::string, DeleterData> const& deleterData)
{
  // A Deleter class for each of the Unique* classes... but only if smart handles are not switched off
  os << "#ifndef VULKAN_HPP_NO_SMART_HANDLE" << std::endl;
  bool first = true;

  // get type and name of the parent (holder) type
  std::string parentType = deleterTypes.first;
  std::string parentName = parentType.empty() ? "" : startLowerCase(parentType);

  // iterate over the deleter types parented by this type
  for (auto const& deleterType : deleterTypes.second)
  {
    std::string deleterName = startLowerCase(deleterType);
    bool standardDeleter = !parentType.empty() && (deleterType != "Device");    // this detects the 'standard' case for a deleter

    if (!first)
    {
      os << std::endl;
    }
    first = false;

    os << "  class " << deleterType << "Deleter" << std::endl
      << "  {" << std::endl
      << "  public:" << std::endl
      << "    " << deleterType << "Deleter( ";
    if (standardDeleter)
    {
      // the standard deleter gets a parent type in the constructor
      os << parentType << " " << parentName << " = " << parentType << "(), ";
    }

    // if this Deleter is pooled, make such a pool the last argument, otherwise an Optional allocator
    auto const& dd = deleterData.find(deleterType);
    assert(dd != deleterData.end());
    std::string poolName = (dd->second.pool.empty() ? "" : startLowerCase(dd->second.pool));
    if (poolName.empty())
    {
      os << "Optional<const AllocationCallbacks> allocator = nullptr )" << std::endl;
    }
    else
    {
      assert(!dd->second.pool.empty());
      os << dd->second.pool << " " << poolName << " = " << dd->second.pool << "() )" << std::endl;
    }

    // now the initializer list of the Deleter constructor
    os << "      : ";
    if (standardDeleter)
    {
      // the standard deleter has a parent type as a member
      os << "m_" << parentName << "( " << parentName << " )" << std::endl
        << "      , ";
    }
    if (poolName.empty())
    {
      // non-pooled deleter have an allocator as a member
      os << "m_allocator( allocator )" << std::endl;
    }
    else
    {
      // pooled deleter have a pool as a member
      os << "m_" << poolName << "( " << poolName << " )" << std::endl;
    }

    // besides that, the constructor is empty
    os << "    {}" << std::endl
      << std::endl;

    // the operator() calls the delete/destroy function
    os << "    void operator()( " << deleterType << " " << deleterName << " )" << std::endl
      << "    {" << std::endl;

    // the delete/destroy function is either part of the parent member of the deleter argument
    if (standardDeleter)
    {
      os << "      m_" << parentName << ".";
    }
    else
    {
      os << "      " << deleterName << ".";
    }

    os << dd->second.call << "( ";

    if (!poolName.empty())
    {
      // pooled Deleter gets the pool as the first argument
      os << "m_" << poolName << ", ";
    }

    if (standardDeleter)
    {
      // the standard deleter gets the deleter argument as an argument
      os << deleterName;
    }

    // the non-pooled deleter get the allocate as an argument (potentially after the deleterName
    if (poolName.empty())
    {
      if (standardDeleter)
      {
        os << ", ";
      }
      os << "m_allocator";
    }
    os << " );" << std::endl
      << "    }" << std::endl
      << std::endl;

    // now the members of the Deleter class
    os << "  private:" << std::endl;
    if (standardDeleter)
    {
      // the parentType for the standard deleter
      os << "    " << parentType << " m_" << parentName << ";" << std::endl;
    }

    // the allocator for the non-pooled deleters, the pool for the pooled ones
    if (poolName.empty())
    {
      os << "    Optional<const AllocationCallbacks> m_allocator;" << std::endl;
    }
    else
    {
      os << "    " << dd->second.pool << " m_" << poolName << ";" << std::endl;
    }
    os << "  };" << std::endl;
  }

  os << "#endif /*VULKAN_HPP_NO_SMART_HANDLE*/" << std::endl
    << std::endl;
}

void writeDeleterForwardDeclarations(std::ostream &os, std::pair<std::string, std::set<std::string>> const& deleterTypes, std::map<std::string, DeleterData> const& deleterData)
{
  // if smart handles are supported, all the Deleter classes need to be forward declared
  os << "#ifndef VULKAN_HPP_NO_SMART_HANDLE" << std::endl;
  bool first = true;
  std::string firstName = deleterTypes.first.empty() ? "" : startLowerCase(deleterTypes.first);
  for (auto const& dt : deleterTypes.second)
  {
    os << "  class " << dt << "Deleter;" << std::endl;
    os << "  using Unique" << dt << " = UniqueHandle<" << dt << ", " << dt << "Deleter>;" << std::endl;
  }
  os << "#endif /*VULKAN_HPP_NO_SMART_HANDLE*/" << std::endl
    << std::endl;
}

void writeTypeFlags(std::ostream & os, std::string const& flagsName, FlagData const& flagData, EnumData const& enumData)
{
  enterProtect(os, flagData.protect);
  // each Flags class is using on the class 'Flags' with the corresponding FlagBits enum as the template parameter
  os << "  using " << flagsName << " = Flags<" << enumData.name << ", Vk" << flagsName << ">;" << std::endl;

  std::stringstream allFlags;
  for (size_t i = 0; i < enumData.members.size(); i++)
  {
    if (i != 0)
    {
      allFlags << " | ";
    }
    allFlags << "VkFlags(" << enumData.name << "::" << enumData.members[i].name << ")";
  }

  if (!enumData.members.empty())
  {
    const std::string templateString = R"(
  VULKAN_HPP_INLINE ${flagsName} operator|( ${enumName} bit0, ${enumName} bit1 )
  {
    return ${flagsName}( bit0 ) | bit1;
  }

  VULKAN_HPP_INLINE ${flagsName} operator~( ${enumName} bits )
  {
    return ~( ${flagsName}( bits ) );
  }

  template <> struct FlagTraits<${enumName}>
  {
    enum
    {
      allFlags = ${allFlags}
    };
  };
)";
    os << replaceWithMap(templateString, { { "flagsName", flagsName}, { "enumName", enumData.name }, { "allFlags", allFlags.str() } } );
  }
  leaveProtect(os, flagData.protect);
  os << std::endl;
}

void writeTypeHandle(std::ostream & os, VkData const& vkData, HandleData const& handleData)
{
	// TODO: Removed dependencies

//  enterProtect(os, handleData.protect);
//
//  // check if there are any forward dependenices for this handle -> list them first
//  if (!dependencyData.forwardDependencies.empty())
//  {
//    os << "  // forward declarations" << std::endl;
//    for (std::set<std::string>::const_iterator it = dependencyData.forwardDependencies.begin(); it != dependencyData.forwardDependencies.end(); ++it)
//    {
//      assert(vkData.structs.find(*it) != vkData.structs.end());
//      os << "  struct " << *it << ";" << std::endl;
//    }
//    os << std::endl;
//  }
//
//  // then write any forward declaration of Deleters used by this handle
//  std::map<std::string, std::set<std::string>>::const_iterator deleterTypesIt = vkData.deleterTypes.find(dependencyData.name);
//  if (deleterTypesIt != vkData.deleterTypes.end())
//  {
//    writeDeleterForwardDeclarations(os, *deleterTypesIt, vkData.deleterData);
//  }
//
//  const std::string memberName = startLowerCase(dependencyData.name);
//  const std::string templateString = 
//R"(  class ${className}
//  {
//  public:
//    ${className}()
//      : m_${memberName}(VK_NULL_HANDLE)
//    {}
//
//    ${className}( std::nullptr_t )
//      : m_${memberName}(VK_NULL_HANDLE)
//    {}
//
//    VULKAN_HPP_TYPESAFE_EXPLICIT ${className}( Vk${className} ${memberName} )
//       : m_${memberName}( ${memberName} )
//    {}
//
//#if defined(VULKAN_HPP_TYPESAFE_CONVERSION)
//    ${className} & operator=(Vk${className} ${memberName})
//    {
//      m_${memberName} = ${memberName};
//      return *this; 
//    }
//#endif
//
//    ${className} & operator=( std::nullptr_t )
//    {
//      m_${memberName} = VK_NULL_HANDLE;
//      return *this;
//    }
//
//    bool operator==( ${className} const & rhs ) const
//    {
//      return m_${memberName} == rhs.m_${memberName};
//    }
//
//    bool operator!=(${className} const & rhs ) const
//    {
//      return m_${memberName} != rhs.m_${memberName};
//    }
//
//    bool operator<(${className} const & rhs ) const
//    {
//      return m_${memberName} < rhs.m_${memberName};
//    }
//
//${commands}
//
//    VULKAN_HPP_TYPESAFE_EXPLICIT operator Vk${className}() const
//    {
//      return m_${memberName};
//    }
//
//    explicit operator bool() const
//    {
//      return m_${memberName} != VK_NULL_HANDLE;
//    }
//
//    bool operator!() const
//    {
//      return m_${memberName} == VK_NULL_HANDLE;
//    }
//
//  private:
//    Vk${className} m_${memberName};
//  };
//
//  static_assert( sizeof( ${className} ) == sizeof( Vk${className} ), "handle and wrapper have different size!" );
//
//)";
//
//  std::ostringstream commands;
//  // now list all the commands that are mapped to members of this class
//  for (size_t i = 0; i < handleData.commands.size(); i++)
//  {
//    std::string commandName = handleData.commands[i];
//    std::map<std::string, CommandData>::const_iterator cit = vkData.commands.find(commandName);
//    assert((cit != vkData.commands.end()) && !cit->second.className.empty());
//    writeTypeCommand(commands, "    ", vkData, cit->second, false);
//  }
//
//  os << replaceWithMap(templateString, {
//    { "className", dependencyData.name },
//    { "memberName", memberName },
//    { "commands", commands.str() }
//  });
//
//  // then the actual Deleter classes can be listed
//  deleterTypesIt = vkData.deleterTypes.find(dependencyData.name);
//  if (deleterTypesIt != vkData.deleterTypes.end())
//  {
//    writeDeleterClasses(os, *deleterTypesIt, vkData.deleterData);
//  }
//
//  // and finally the commands, that are member functions of this handle
//  for (size_t i = 0; i < handleData.commands.size(); i++)
//  {
//    std::string commandName = handleData.commands[i];
//    std::map<std::string, CommandData>::const_iterator cit = vkData.commands.find(commandName);
//    assert((cit != vkData.commands.end()) && !cit->second.className.empty());
//    std::list<DependencyData>::const_iterator dep = std::find_if(dependencies.begin(), dependencies.end(), [commandName](DependencyData const& dd) { return dd.name == commandName; });
//    assert(dep != dependencies.end() && (dep->name == cit->second.fullName));
//    writeTypeCommand(os, "  ", vkData, cit->second, true);
//  }
//
//  leaveProtect(os, handleData.protect);
}

void writeTypeScalar( std::ostream & os )
{
	// TODO: Removed dependencies

  //assert( dependencyData.dependencies.size() == 1 );
  //os << "  using " << dependencyData.name << " = " << *dependencyData.dependencies.begin() << ";" << std::endl
  //    << std::endl;
}

bool containsUnion(std::string const& type, std::map<std::string, StructData> const& structs)
{
  // a simple recursive check if a type is or contains a union
  std::map<std::string, StructData>::const_iterator sit = structs.find(type);
  bool found = (sit != structs.end());
  if (found)
  {
    found = sit->second.isUnion;
    for (std::vector<MemberData>::const_iterator mit = sit->second.members.begin(); mit != sit->second.members.end() && !found; ++mit)
    {
      found = (mit->type == mit->pureType) && containsUnion(mit->type, structs);
    }
  }
  return found;
}

void writeTypeStruct( std::ostream & os, VkData const& vkData, std::map<std::string,std::string> const& defaultValues )
{
	// TODO: Removed dependencies

  //std::map<std::string,StructData>::const_iterator it = vkData.structs.find( dependencyData.name );
  //assert( it != vkData.structs.end() );

  //enterProtect(os, it->second.protect);
  //os << "  struct " << dependencyData.name << std::endl
  //    << "  {" << std::endl;

  //// only structs that are not returnedOnly get a constructor!
  //if ( !it->second.returnedOnly )
  //{
  //  writeStructConstructor( os, dependencyData.name, it->second, vkData.vkTypes, defaultValues );
  //}

  //// create the setters
  //if (!it->second.returnedOnly)
  //{
  //  for (size_t i = 0; i<it->second.members.size(); i++)
  //  {
  //    writeStructSetter( os, dependencyData.name, it->second.members[i], vkData.vkTypes );
  //  }
  //}

  //// the cast-operator to the wrapped struct
  //os << "    operator const Vk" << dependencyData.name << "&() const" << std::endl
  //    << "    {" << std::endl
  //    << "      return *reinterpret_cast<const Vk" << dependencyData.name << "*>(this);" << std::endl
  //    << "    }" << std::endl
  //    << std::endl;

  //// operator==() and operator!=()
  //// only structs without a union as a member can have a meaningfull == and != operation; we filter them out
  //if (!containsUnion(dependencyData.name, vkData.structs))
  //{
  //  // two structs are compared by comparing each of the elements
  //  os << "    bool operator==( " << dependencyData.name << " const& rhs ) const" << std::endl
  //      << "    {" << std::endl
  //      << "      return ";
  //  for (size_t i = 0; i < it->second.members.size(); i++)
  //  {
  //    if (i != 0)
  //    {
  //      os << std::endl << "          && ";
  //    }
  //    if (!it->second.members[i].arraySize.empty())
  //    {
  //      os << "( memcmp( " << it->second.members[i].name << ", rhs." << it->second.members[i].name << ", " << it->second.members[i].arraySize << " * sizeof( " << it->second.members[i].type << " ) ) == 0 )";
  //    }
  //    else
  //    {
  //      os << "( " << it->second.members[i].name << " == rhs." << it->second.members[i].name << " )";
  //    }
  //  }
  //  os << ";" << std::endl
  //      << "    }" << std::endl
  //      << std::endl
  //      << "    bool operator!=( " << dependencyData.name << " const& rhs ) const" << std::endl
  //      << "    {" << std::endl
  //      << "      return !operator==( rhs );" << std::endl
  //      << "    }" << std::endl
  //      << std::endl;
  //}

  //// the member variables
  //for (size_t i = 0; i < it->second.members.size(); i++)
  //{
  //  if (it->second.members[i].type == "StructureType")
  //  {
  //    assert((i == 0) && (it->second.members[i].name == "sType"));
  //    os << "  private:" << std::endl
  //        << "    StructureType sType;" << std::endl
  //        << std::endl
  //        << "  public:" << std::endl;
  //  }
  //  else
  //  {
  //    os << "    " << it->second.members[i].type << " " << it->second.members[i].name;
  //    if (!it->second.members[i].arraySize.empty())
  //    {
  //      os << "[" << it->second.members[i].arraySize << "]";
  //    }
  //    os << ";" << std::endl;
  //  }
  //}
  //os << "  };" << std::endl
  //    << "  static_assert( sizeof( " << dependencyData.name << " ) == sizeof( Vk" << dependencyData.name << " ), \"struct and wrapper have different size!\" );" << std::endl;

  //leaveProtect(os, it->second.protect);
  //os << std::endl;
}

void writeTypeUnion( std::ostream & os, VkData const& vkData, std::map<std::string,std::string> const& defaultValues )
{
	// TODO: Removed dependencies

  //std::map<std::string, StructData>::const_iterator it = vkData.structs.find(dependencyData.name);
  //assert(it != vkData.structs.end());

  //std::ostringstream oss;
  //os << "  union " << dependencyData.name << std::endl
  //    << "  {" << std::endl;

  //for ( size_t i=0 ; i<it->second.members.size() ; i++ )
  //{
  //  // one constructor per union element
  //  os << "    " << dependencyData.name << "( ";
  //  if ( it->second.members[i].arraySize.empty() )
  //  {
  //    os << it->second.members[i].type << " ";
  //  }
  //  else
  //  {
  //    os << "const std::array<" << it->second.members[i].type << "," << it->second.members[i].arraySize << ">& ";
  //  }
  //  os << it->second.members[i].name << "_";

  //  // just the very first constructor gets default arguments
  //  if ( i == 0 )
  //  {
  //    std::map<std::string,std::string>::const_iterator defaultIt = defaultValues.find( it->second.members[i].pureType );
  //    assert(defaultIt != defaultValues.end() );
  //    if ( it->second.members[i].arraySize.empty() )
  //    {
  //      os << " = " << defaultIt->second;
  //    }
  //    else
  //    {
  //      os << " = { {" << defaultIt->second << "} }";
  //    }
  //  }
  //  os << " )" << std::endl
  //      << "    {" << std::endl
  //      << "      ";
  //  if ( it->second.members[i].arraySize.empty() )
  //  {
  //    os << it->second.members[i].name << " = " << it->second.members[i].name << "_";
  //  }
  //  else
  //  {
  //    os << "memcpy( &" << it->second.members[i].name << ", " << it->second.members[i].name << "_.data(), " << it->second.members[i].arraySize << " * sizeof( " << it->second.members[i].type << " ) )";
  //  }
  //  os << ";" << std::endl
  //      << "    }" << std::endl
  //      << std::endl;
  //  }

  //for (size_t i = 0; i<it->second.members.size(); i++)
  //{
  //  // one setter per union element
  //  assert(!it->second.returnedOnly);
  //  writeStructSetter(os, dependencyData.name, it->second.members[i], vkData.vkTypes);
  //}

  //// the implicit cast operator to the native type
  //os << "    operator Vk" << dependencyData.name << " const& () const" << std::endl
  //    << "    {" << std::endl
  //    << "      return *reinterpret_cast<const Vk" << dependencyData.name << "*>(this);" << std::endl
  //    << "    }" << std::endl
  //    << std::endl;

  //// the union member variables
  //// if there's at least one Vk... type in this union, check for unrestricted unions support
  //bool needsUnrestrictedUnions = false;
  //for (size_t i = 0; i < it->second.members.size() && !needsUnrestrictedUnions; i++)
  //{
  //  needsUnrestrictedUnions = (vkData.vkTypes.find(it->second.members[i].type) != vkData.vkTypes.end());
  //}
  //if (needsUnrestrictedUnions)
  //{
  //  os << "#ifdef VULKAN_HPP_HAS_UNRESTRICTED_UNIONS" << std::endl;
  //  for (size_t i = 0; i < it->second.members.size(); i++)
  //  {
  //    os << "    " << it->second.members[i].type << " " << it->second.members[i].name;
  //    if (!it->second.members[i].arraySize.empty())
  //    {
  //      os << "[" << it->second.members[i].arraySize << "]";
  //    }
  //    os << ";" << std::endl;
  //  }
  //  os << "#else" << std::endl;
  //}
  //for (size_t i = 0; i < it->second.members.size(); i++)
  //{
  //  os << "    ";
  //  if (vkData.vkTypes.find(it->second.members[i].type) != vkData.vkTypes.end())
  //  {
  //    os << "Vk";
  //  }
  //  os << it->second.members[i].type << " " << it->second.members[i].name;
  //  if (!it->second.members[i].arraySize.empty())
  //  {
  //    os << "[" << it->second.members[i].arraySize << "]";
  //  }
  //  os << ";" << std::endl;
  //}
  //if (needsUnrestrictedUnions)
  //{
  //  os << "#endif  // VULKAN_HPP_HAS_UNRESTRICTED_UNIONS" << std::endl;
  //}
  //os << "  };" << std::endl
  //    << std::endl;
}

void writeTypes(std::ostream & os, VkData const& vkData, std::map<std::string, std::string> const& defaultValues)
{
	// TODO: Removed dependencies

  //for ( std::list<DependencyData>::const_iterator it = vkData.dependencies.begin() ; it != vkData.dependencies.end() ; ++it )
  //{
  //  switch( it->category )
  //  {
  //    case DependencyData::Category::COMMAND :
  //      writeTypeCommand( os, vkData, *it );
  //      break;
  //    case DependencyData::Category::ENUM :
  //      assert( vkData.enums.find( it->name ) != vkData.enums.end() );
  //      writeTypeEnum( os, vkData.enums.find( it->name )->second );
  //      break;
  //    case DependencyData::Category::FLAGS :
  //      assert(vkData.flags.find(it->name) != vkData.flags.end());
  //      writeTypeFlags( os, it->name, vkData.flags.find( it->name)->second, vkData.enums.find(generateEnumNameForFlags(it->name))->second );
  //      break;
  //    case DependencyData::Category::FUNC_POINTER :
  //    case DependencyData::Category::REQUIRED :
  //      // skip FUNC_POINTER and REQUIRED, they just needed to be in the dependencies list to resolve dependencies
  //      break;
  //    case DependencyData::Category::HANDLE :
  //      assert(vkData.handles.find(it->name) != vkData.handles.end());
  //      writeTypeHandle(os, vkData, *it, vkData.handles.find(it->name)->second, vkData.dependencies);
  //      break;
  //    case DependencyData::Category::SCALAR :
  //      writeTypeScalar( os, *it );
  //      break;
  //    case DependencyData::Category::STRUCT :
  //      writeTypeStruct( os, vkData, *it, defaultValues );
  //      break;
  //    case DependencyData::Category::UNION :
  //      assert( vkData.structs.find( it->name ) != vkData.structs.end() );
  //      writeTypeUnion( os, vkData, *it, defaultValues );
  //      break;
  //    default :
  //      assert( false );
  //      break;
  //  }
  //}
}

void writeVersionCheck(std::ostream & os, std::string const& version)
{
	os << "const VK_HEADER_VERSION: u32 = " << version << ";" << std::endl;
	os << std::endl;
	os << "pub fn VK_MAKE_VERSION(major: u32, minor: u32, patch: u32) -> u32 {" << std::endl;
	os << "    (major << 22) | (minor << 12) | patch" << std::endl;
	os << "}" << std::endl;
}

int main(int argc, char **argv)
{
	try {
		tinyxml2::XMLDocument doc;

		std::string filename = (argc == 1) ? VK_SPEC : argv[1];
		std::cout << "Loading vk.xml from " << filename << std::endl;
		std::cout << "Writing vulkan.rs to " << VULKAN_HPP << std::endl;

		tinyxml2::XMLError error = doc.LoadFile(filename.c_str());
		if (error != tinyxml2::XML_SUCCESS)
		{
			std::cout << "VkGenerate: failed to load file " << filename << ". Error code: " << error << std::endl;
			return -1;
		}

		// The very first element is expected to be a registry, and it should
		// be the only root element.
		tinyxml2::XMLElement * registryElement = doc.FirstChildElement();
		assert(strcmp(registryElement->Value(), "registry") == 0);
		assert(!registryElement->NextSiblingElement());

		VkData vkData;
		vkData.handles[""];         // insert the default "handle" without class (for createInstance, and such)
		vkData.tags.insert("KHX");  // insert a non-listed tag

		// The root tag contains zero or more of the following tags. Order may change.
		for (tinyxml2::XMLElement * child = registryElement->FirstChildElement(); child; child = child->NextSiblingElement())
		{
			assert(child->Value());
			const std::string value = child->Value();

			if (value == "comment")
			{
				// Get the vulkan license header and skip any leading spaces
				readComment(child, vkData.vulkanLicenseHeader);
				vkData.vulkanLicenseHeader.erase(vkData.vulkanLicenseHeader.begin(), std::find_if(vkData.vulkanLicenseHeader.begin(), vkData.vulkanLicenseHeader.end(), [](char c) { return !std::isspace(c); }));
			}
			else if (value == "tags")
			{
				// Author IDs for extensions and layers
				readTags(child, vkData.tags);
			}
			else if (value == "types")
			{
				// Defines API types
				readTypes(child, vkData);
			}
			else if (value == "enums")
			{
				// Enum definitions
				readEnums(child, vkData);
			}
			else if (value == "commands")
			{
				// Function definitions
				readCommands(child, vkData);
			}
			else if (value == "extensions")
			{
				// Extension interfaces
				readExtensions(child, vkData);
			}
			else
			{
				assert((value == "feature") || (value == "vendorids"));
			}
		}

		// Finished parsing the spec.

		// TODO: Removed dependencies

		//sortDependencies(vkData.dependencies);

		std::map<std::string, std::string> defaultValues;
		createDefaults(vkData, defaultValues);

		std::ofstream ofs(VULKAN_HPP);
		ofs << vkData.vulkanLicenseHeader << std::endl
			<< R"(
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

pub mod core {
    extern crate libloading;
    use ::std::{mem, ptr};
    use ::std::os::raw::{c_void, c_char, c_int};
    use ::std::ffi::CString;
    use ::std::ops::{BitOr, BitAnd};
    use ::std::fmt;
)" << std::endl;

		indent = new IndentingOStreambuf(ofs);

		writeVersionCheck(ofs, vkData.version);

		ofs << std::endl;

		for (auto tdef : typeOracle.getScalarTypedefs()) {
			ofs << "type " << tdef.first << " = " << tdef.second << ";" << std::endl;
		}

		ofs << std::endl;
		ofs << "OPAQUE TYPES HERE?" << std::endl;
		ofs << "SOME CONSTANTS HERE?" << std::endl;

		ofs << std::endl;

		ofs << flagsMacro;

		ofs << std::endl;

		// TODO: Removed dependencies

		//// First of all, write out vk::Result
		//std::list<DependencyData>::const_iterator it = std::find_if(vkData.dependencies.begin(), vkData.dependencies.end(), [](DependencyData const& dp) { return dp.name == "Result"; });
		//assert(it != vkData.dependencies.end());
		//writeTypeEnum(ofs, vkData.enums.find(it->name)->second);

		//// Remove vk::Result because it has been handled
		//vkData.dependencies.erase(it);

		ofs << std::endl;

		// TODO: Removed dependencies

		//assert(vkData.deleterTypes.find("") != vkData.deleterTypes.end());
		//writeTypes(ofs, vkData, defaultValues);

		ofs << "} // mod core" << std::endl;
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
