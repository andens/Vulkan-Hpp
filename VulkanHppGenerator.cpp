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

const std::string flagsMacro = R"(
FLAGSMACRO
)";

//std::string replaceWithMap(std::string const &input, std::map<std::string, std::string> replacements)
//{
//  // This will match ${someVariable} and contain someVariable in match group 1
//  std::regex re(R"(\$\{([^\}]+)\})");
//  auto it = std::sregex_iterator(input.begin(), input.end(), re);
//  auto end = std::sregex_iterator();
//
//  // No match, just return the original string
//  if (it == end)
//  {
//    return input;
//  }
//
//  std::string result = "";
//  while (it != end)
//  {
//    std::smatch match = *it;
//    auto itReplacement = replacements.find(match[1].str());
//    assert(itReplacement != replacements.end());
//
//    result += match.prefix().str() + ((itReplacement != replacements.end()) ? itReplacement->second : match[0].str());
//    ++it;
//
//    // we've passed the last match. Append the rest of the orignal string
//    if (it == end)
//    {
//      result += match.suffix().str();
//    }
//  }
//  return result;
//}

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

std::string startLowerCase(std::string const& input);
std::string startUpperCase(std::string const& input);
std::string strip(std::string const& value, std::string const& prefix, std::string const& postfix = std::string());
//std::string stripPluralS(std::string const& name);
//std::string toCamelCase(std::string const& value);
//void writeCall(std::ostream & os, CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular);
//std::string generateCall(CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular);
//void writeCallCountParameter(std::ostream & os, CommandData const& commandData, bool singular, std::map<size_t, size_t>::const_iterator it);
//void writeCallPlainTypeParameter(std::ostream & os, ParamData const& paramData);
//void writeCallVectorParameter(std::ostream & os, CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular, std::map<size_t, size_t>::const_iterator it);
//void writeCallVulkanTypeParameter(std::ostream & os, ParamData const& paramData);
//void writeDeleterClasses(std::ostream & os, std::pair<std::string, std::set<std::string>> const& deleterTypes, std::map<std::string, DeleterData> const& deleterData);
void writeDeleterForwardDeclarations(std::ostream &os, std::pair<std::string, std::set<std::string>> const& deleterTypes, std::map<std::string, DeleterData> const& deleterData);
//void writeFunction(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool definition, bool enhanced, bool singular, bool unique);
//void writeFunctionBodyEnhanced(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool singular);
//void writeFunctionBodyEnhancedCall(std::ostream &os, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular);
//void writeFunctionBodyEnhancedCallResult(std::ostream &os, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular);
//void writeFunctionBodyEnhancedCallTwoStep(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData);
//void writeFunctionBodyEnhancedCallTwoStepChecked(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData);
//void writeFunctionBodyEnhancedCallTwoStepIterate(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData);
//void writeFunctionBodyEnhancedLocalCountVariable(std::ostream & os, std::string const& indentation, CommandData const& commandData);
//void writeFunctionBodyEnhancedMultiVectorSizeCheck(std::ostream & os, std::string const& indentation, CommandData const& commandData);
//void writeFunctionBodyStandard(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData);
//void writeFunctionBodyUnique(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool singular);
//void writeFunctionHeaderArguments(std::ostream & os, VkData const& vkData, CommandData const& commandData, bool enhanced, bool singular, bool withDefaults);
//void writeFunctionHeaderArgumentsEnhanced(std::ostream & os, VkData const& vkData, CommandData const& commandData, bool singular, bool withDefaults);
//void writeFunctionHeaderArgumentsStandard(std::ostream & os, CommandData const& commandData);
//void writeFunctionHeaderName(std::ostream & os, std::string const& name, bool singular, bool unique);
//void writeFunctionHeaderReturnType(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool enhanced, bool singular, bool unique);
//void writeFunctionHeaderTemplate(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool withDefault);
void writeReinterpretCast(std::ostream & os, bool leadingConst, bool vulkanType, std::string const& type, bool trailingPointerToConst);
void writeStandardOrEnhanced(std::ostream & os, std::string const& standard, std::string const& enhanced);
//void writeStructConstructor( std::ostream & os, std::string const& name, StructData const& structData, std::set<std::string> const& vkTypes, std::map<std::string,std::string> const& defaultValues );
//void writeTypeCommand(std::ostream & os, VkData const& vkData);
//void writeTypeCommand(std::ostream &os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool definition);
void writeTypeEnum(std::ostream & os, EnumData const& enumData);
//void writeTypeFlags(std::ostream & os, std::string const& flagsName, FlagData const& flagData, EnumData const& enumData);
//void writeTypeHandle(std::ostream & os, VkData const& vkData, HandleData const& handle);
void writeTypeScalar( std::ostream & os );
//void writeTypeStruct( std::ostream & os, VkData const& vkData, std::map<std::string,std::string> const& defaultValues );
//void writeTypeUnion( std::ostream & os, VkData const& vkData, std::map<std::string,std::string> const& defaultValues );
//void writeTypes(std::ostream & os, VkData const& vkData, std::map<std::string, std::string> const& defaultValues);
//void writeVersionCheck(std::ostream & os, std::string const& version);

//void EnumData::addEnumMember(std::string const &name, std::string const& tag)
//{
//  NameValue nv;
//  nv.name = "e" + toCamelCase(strip(name, prefix, postfix));
//  nv.value = name;
//  if (bitmask)
//  {
//    size_t pos = nv.name.find("Bit");
//    if (pos != std::string::npos)
//    {
//      nv.name.erase(pos, 3);
//    }
//  }
//  if (!tag.empty() && (nv.name.substr(nv.name.length() - tag.length()) == toCamelCase(tag)))
//  {
//    nv.name = nv.name.substr(0, nv.name.length() - tag.length()) + tag;
//  }
//  members.push_back(nv);
//}

//void registerDeleter(VkData & vkData, CommandData const& commandData)
//{
//  if ((commandData.fullName.substr(0, 7) == "destroy") || (commandData.fullName.substr(0, 4) == "free"))
//  {
//    std::string key;
//    size_t valueIndex;
//    switch (commandData.params.size())
//    {
//    case 2:
//    case 3:
//      assert(commandData.params.back().pureType == "AllocationCallbacks");
//      key = (commandData.params.size() == 2) ? "" : commandData.params[0].pureType;
//      valueIndex = commandData.params.size() - 2;
//      break;
//    case 4:
//      key = commandData.params[0].pureType;
//      valueIndex = 3;
//      assert(vkData.deleterData.find(commandData.params[valueIndex].pureType) == vkData.deleterData.end());
//      vkData.deleterData[commandData.params[valueIndex].pureType].pool = commandData.params[1].pureType;
//      break;
//    default:
//      assert(false);
//    }
//    if (commandData.fullName == "destroyDevice")
//    {
//      key = "PhysicalDevice";
//    }
//    assert(vkData.deleterTypes[key].find(commandData.params[valueIndex].pureType) == vkData.deleterTypes[key].end());
//    vkData.deleterTypes[key].insert(commandData.params[valueIndex].pureType);
//    vkData.deleterData[commandData.params[valueIndex].pureType].call = commandData.reducedName;
//  }
//}

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

//std::string stripPluralS(std::string const& name)
//{
//  std::string strippedName(name);
//  size_t pos = strippedName.rfind('s');
//  assert(pos != std::string::npos);
//  strippedName.erase(pos, 1);
//  return strippedName;
//}

//std::string toCamelCase(std::string const& value)
//{
//  assert(!value.empty() && (isupper(value[0]) || isdigit(value[0])));
//  std::string result;
//  result.reserve(value.size());
//  result.push_back(value[0]);
//  for (size_t i = 1; i < value.size(); i++)
//  {
//    if (value[i] != '_')
//    {
//      if ((value[i - 1] == '_') || isdigit(value[i-1]))
//      {
//        result.push_back(value[i]);
//      }
//      else
//      {
//        result.push_back(tolower(value[i]));
//      }
//    }
//  }
//  return result;
//}

//std::string generateCall(CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular)
//{
//  std::ostringstream call;
//  writeCall(call, commandData, vkTypes, firstCall, singular);
//  return call.str();
//}

//void writeCall(std::ostream & os, CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular)
//{
//  // get the parameter indices of the counter for vector parameters
//  std::map<size_t,size_t> countIndices;
//  for (std::map<size_t, size_t>::const_iterator it = commandData.vectorParams.begin(); it != commandData.vectorParams.end(); ++it)
//  {
//    countIndices.insert(std::make_pair(it->second, it->first));
//  }
//
//  // the original function call
//  os << "vk" << startUpperCase(commandData.fullName) << "( ";
//
//  if (!commandData.className.empty())
//  {
//    // if it's member of a class -> add the first parameter with "m_" as prefix
//    os << "m_" << commandData.params[0].name;
//  }
//
//  for (size_t i=commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
//  {
//    if (0 < i)
//    {
//      os << ", ";
//    }
//
//    std::map<size_t, size_t>::const_iterator it = countIndices.find(i);
//    if (it != countIndices.end())
//    {
//      writeCallCountParameter(os, commandData, singular, it);
//    }
//    else if ((it = commandData.vectorParams.find(i)) != commandData.vectorParams.end())
//    {
//      writeCallVectorParameter(os, commandData, vkTypes, firstCall, singular, it);
//    }
//    else
//    {
//      if (vkTypes.find(commandData.params[i].pureType) != vkTypes.end())
//      {
//        writeCallVulkanTypeParameter(os, commandData.params[i]);
//      }
//      else
//      {
//        writeCallPlainTypeParameter(os, commandData.params[i]);
//      }
//    }
//  }
//  os << " )";
//}

//void writeCallCountParameter(std::ostream & os, CommandData const& commandData, bool singular, std::map<size_t, size_t>::const_iterator it)
//{
//  // this parameter is a count parameter for a vector parameter
//  if ((commandData.returnParam == it->second) && commandData.twoStep)
//  {
//    // the corresponding vector parameter is the return parameter and it's a two-step algorithm
//    // -> use the pointer to a local variable named like the counter parameter without leading 'p'
//    os << "&" << startLowerCase(strip(commandData.params[it->first].name, "p"));
//  }
//  else
//  {
//    // the corresponding vector parameter is not the return parameter, or it's not a two-step algorithm
//    if (singular)
//    {
//      // for the singular version, the count is just 1.
//      os << "1 ";
//    }
//    else
//    {
//      // for the non-singular version, the count is the size of the vector parameter
//      // -> use the vector parameter name without leading 'p' to get the size (in number of elements, not in bytes)
//      os << startLowerCase(strip(commandData.params[it->second].name, "p")) << ".size() ";
//    }
//    if (commandData.templateParam == it->second)
//    {
//      // if the vector parameter is templatized -> multiply by the size of that type to get the size in bytes
//      os << "* sizeof( T ) ";
//    }
//  }
//}

//void writeCallPlainTypeParameter(std::ostream & os, ParamData const& paramData)
//{
//  // this parameter is just a plain type
//  if (paramData.type.back() == '*')
//  {
//    // it's a pointer
//    std::string parameterName = startLowerCase(strip(paramData.name, "p"));
//    if (paramData.type.find("const") != std::string::npos)
//    {
//      // it's a const pointer
//      if (paramData.pureType == "char")
//      {
//        // it's a const pointer to char -> it's a string -> get the data via c_str()
//        os << parameterName;
//        if (paramData.optional)
//        {
//          // it's optional -> might use nullptr
//          os << " ? " << parameterName << "->c_str() : nullptr";
//        }
//        else
//        {
//          os << ".c_str()";
//        }
//      }
//      else
//      {
//        // it's const pointer to void (only other type that occurs) -> just use the name
//        assert((paramData.pureType == "void") && !paramData.optional);
//        os << paramData.name;
//      }
//    }
//    else
//    {
//      // it's a non-const pointer, and char is the only type that occurs -> use the address of the parameter
//      assert(paramData.type.find("char") == std::string::npos);
//      os << "&" << parameterName;
//    }
//  }
//  else
//  {
//    // it's a plain parameter -> just use its name
//    os << paramData.name;
//  }
//}

//void writeCallVectorParameter(std::ostream & os, CommandData const& commandData, std::set<std::string> const& vkTypes, bool firstCall, bool singular, std::map<size_t, size_t>::const_iterator it)
//{
//  // this parameter is a vector parameter
//  assert(commandData.params[it->first].type.back() == '*');
//  if ((commandData.returnParam == it->first) && commandData.twoStep && firstCall)
//  {
//    // this parameter is the return parameter, and it's the first call of a two-step algorithm -> just just nullptr
//    os << "nullptr";
//  }
//  else
//  {
//    std::string parameterName = startLowerCase(strip(commandData.params[it->first].name, "p"));
//    std::set<std::string>::const_iterator vkit = vkTypes.find(commandData.params[it->first].pureType);
//    if ((vkit != vkTypes.end()) || (it->first == commandData.templateParam))
//    {
//      // CHECK for !commandData.params[it->first].optional
//
//      // this parameter is a vulkan type or a templated type -> need to reinterpret cast
//      writeReinterpretCast(os, commandData.params[it->first].type.find("const") == 0, vkit != vkTypes.end(), commandData.params[it->first].pureType,
//        commandData.params[it->first].type.rfind("* const") != std::string::npos);
//      os << "( ";
//      if (singular)
//      {
//        // in singular case, strip the plural-S from the name, and use the pointer to that thing
//        os << "&" << stripPluralS(parameterName);
//      }
//      else
//      {
//        // in plural case, get the pointer to the data
//        os << parameterName << ".data()";
//      }
//      os << " )";
//    }
//    else if (commandData.params[it->first].pureType == "char")
//    {
//      // the parameter is a vector to char -> it might be optional
//      // besides that, the parameter now is a std::string -> get the pointer via c_str()
//      os << parameterName;
//      if (commandData.params[it->first].optional)
//      {
//        os << " ? " << parameterName << "->c_str() : nullptr";
//      }
//      else
//      {
//        os << ".c_str()";
//      }
//    }
//    else
//    {
//      // this parameter is just a vetor -> get the pointer to its data
//      os << parameterName << ".data()";
//    }
//  }
//}

//void writeCallVulkanTypeParameter(std::ostream & os, ParamData const& paramData)
//{
//  // this parameter is a vulkan type
//  if (paramData.type.back() == '*')
//  {
//    // it's a pointer -> needs a reinterpret cast to the vulkan type
//    std::string parameterName = startLowerCase(strip(paramData.name, "p"));
//    writeReinterpretCast(os, paramData.type.find("const") != std::string::npos, true, paramData.pureType, false);
//    os << "( ";
//    if (paramData.optional)
//    {
//      // for an optional parameter, we need also a static_cast from optional type to const-pointer to pure type
//      os << "static_cast<const " << paramData.pureType << "*>( " << parameterName << " )";
//    }
//    else
//    {
//      // other parameters can just use the pointer
//      os << "&" << parameterName;
//    }
//    os << " )";
//  }
//  else
//  {
//    // a non-pointer parameter needs a static_cast from vk::-type to vulkan type
//    os << "static_cast<Vk" << paramData.pureType << ">( " << paramData.name << " )";
//  }
//}

//void writeFunction(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool definition, bool enhanced, bool singular, bool unique)
//{
//  if (enhanced && !singular)
//  {
//    writeFunctionHeaderTemplate(os, indentation, commandData, !definition);
//  }
//  os << indentation << (definition ? "VULKAN_HPP_INLINE " : "");
//  writeFunctionHeaderReturnType(os, indentation, commandData, enhanced, singular, unique);
//  if (definition && !commandData.className.empty())
//  {
//    os << commandData.className << "::";
//  }
//  writeFunctionHeaderName(os, commandData.reducedName, singular, unique);
//  writeFunctionHeaderArguments(os, vkData, commandData, enhanced, singular, !definition);
//  os << (definition ? "" : ";") << std::endl;
//
//  if (definition)
//  {
//    // write the function body
//    os << indentation << "{" << std::endl;
//    if (enhanced)
//    {
//      if (unique)
//      {
//        writeFunctionBodyUnique(os, indentation, vkData, commandData, singular);
//      }
//      else
//      {
//        writeFunctionBodyEnhanced(os, indentation, vkData, commandData, singular);
//      }
//    }
//    else
//    {
//      writeFunctionBodyStandard(os, indentation, vkData, commandData);
//    }
//    os << indentation << "}" << std::endl;
//  }
//}

//void writeFunctionBodyEnhanced(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool singular)
//{
//  if (1 < commandData.vectorParams.size())
//  {
//    writeFunctionBodyEnhancedMultiVectorSizeCheck(os, indentation, commandData);
//  }
//
//  std::string returnName;
//  if (commandData.returnParam != ~0)
//  {
//    returnName = writeFunctionBodyEnhancedLocalReturnVariable(os, indentation, commandData, singular);
//  }
//
//  if (commandData.twoStep)
//  {
//    assert(!singular);
//    writeFunctionBodyEnhancedLocalCountVariable(os, indentation, commandData);
//
//    // we now might have to check the result, resize the returned vector accordingly, and call the function again
//    std::map<size_t, size_t>::const_iterator returnit = commandData.vectorParams.find(commandData.returnParam);
//    assert(returnit != commandData.vectorParams.end() && (returnit->second != ~0));
//    std::string sizeName = startLowerCase(strip(commandData.params[returnit->second].name, "p"));
//
//    if (commandData.returnType == "Result")
//    {
//      if (1 < commandData.successCodes.size())
//      {
//        writeFunctionBodyEnhancedCallTwoStepIterate(os, indentation, vkData.vkTypes, returnName, sizeName, commandData);
//      }
//      else
//      {
//        writeFunctionBodyEnhancedCallTwoStepChecked(os, indentation, vkData.vkTypes, returnName, sizeName, commandData);
//      }
//    }
//    else
//    {
//      writeFunctionBodyEnhancedCallTwoStep(os, indentation, vkData.vkTypes, returnName, sizeName, commandData);
//    }
//  }
//  else
//  {
//    if (commandData.returnType == "Result")
//    {
//      writeFunctionBodyEnhancedCallResult(os, indentation, vkData.vkTypes, commandData, singular);
//    }
//    else
//    {
//      writeFunctionBodyEnhancedCall(os, indentation, vkData.vkTypes, commandData, singular);
//    }
//  }
//
//  if ((commandData.returnType == "Result") || !commandData.successCodes.empty())
//  {
//    writeFunctionBodyEnhancedReturnResultValue(os, indentation, returnName, commandData, singular);
//  }
//  else if ((commandData.returnParam != ~0) && (commandData.returnType != commandData.enhancedReturnType))
//  {
//    // for the other returning cases, when the return type is somhow enhanced, just return the local returnVariable
//    os << indentation << "  return " << returnName << ";" << std::endl;
//  }
//}

//void writeFunctionBodyEnhanced(std::ostream &os, std::string const& templateString, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular)
//{
//  os << replaceWithMap(templateString, {
//    { "call", generateCall(commandData, vkTypes, true, singular) },
//    { "i", indentation }
//  });
//
//}

//void writeFunctionBodyEnhancedCall(std::ostream &os, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular)
//{
//  std::string const templateString = "${i}  return ${call};\n";
//  std::string const templateStringVoid = "${i}  ${call};\n";
//  //writeFunctionBodyEnhanced(os, commandData.returnType == "void" ? templateStringVoid : templateString, indentation, vkTypes, commandData, singular);
//}

//void writeFunctionBodyEnhancedCallResult(std::ostream &os, std::string const& indentation, std::set<std::string> const& vkTypes, CommandData const& commandData, bool singular)
//{
//  std::string const templateString = "${i}  Result result = static_cast<Result>( ${call} );\n";
//  writeFunctionBodyEnhanced(os, templateString, indentation, vkTypes, commandData, singular);
//}

//void writeFunctionBodyTwoStep(std::ostream & os, std::string const &templateString, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData)
//{
//  std::map<std::string, std::string> replacements = {
//    { "sizeName", sizeName },
//    { "returnName", returnName },
//    { "call1", generateCall(commandData, vkTypes, true, false) },
//    { "call2", generateCall(commandData, vkTypes, false, false) },
//    { "i", indentation }
//  };
//
//  os << replaceWithMap(templateString, replacements);
//}

//void writeFunctionBodyEnhancedCallTwoStep(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData)
//{
//  std::string const templateString = 
//R"(${i}  ${call1};
//${i}  ${returnName}.resize( ${sizeName} );
//${i}  ${call2};
//)";
//  writeFunctionBodyTwoStep(os, templateString, indentation, vkTypes, returnName, sizeName, commandData);
//}

//void writeFunctionBodyEnhancedCallTwoStepChecked(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData)
//{
//  std::string const templateString =
//R"(${i}  Result result = static_cast<Result>( ${call1} );
//${i}  if ( ( result == Result::eSuccess ) && ${sizeName} )
//${i}  {
//${i}    ${returnName}.resize( ${sizeName} );
//${i}    result = static_cast<Result>( ${call2} );
//${i}  }
//)";
//  writeFunctionBodyTwoStep(os, templateString, indentation, vkTypes, returnName, sizeName, commandData);
//}

//void writeFunctionBodyEnhancedCallTwoStepIterate(std::ostream & os, std::string const& indentation, std::set<std::string> const& vkTypes, std::string const& returnName, std::string const& sizeName, CommandData const& commandData)
//{
//  std::string const templateString = 
//R"(${i}  Result result;
//${i}  do
//${i}  {
//${i}    result = static_cast<Result>( ${call1} );
//${i}    if ( ( result == Result::eSuccess ) && ${sizeName} )
//${i}    {
//${i}      ${returnName}.resize( ${sizeName} );
//${i}      result = static_cast<Result>( ${call2} );
//${i}    }
//${i}  } while ( result == Result::eIncomplete );
//${i}  assert( ${sizeName} <= ${returnName}.size() );
//${i}  ${returnName}.resize( ${sizeName} );
//)";
//  writeFunctionBodyTwoStep(os, templateString, indentation, vkTypes, returnName, sizeName, commandData);
//}

//void writeFunctionBodyEnhancedLocalCountVariable(std::ostream & os, std::string const& indentation, CommandData const& commandData)
//{
//  // local count variable to hold the size of the vector to fill
//  assert(commandData.returnParam != ~0);
//
//  std::map<size_t, size_t>::const_iterator returnit = commandData.vectorParams.find(commandData.returnParam);
//  assert(returnit != commandData.vectorParams.end() && (returnit->second != ~0));
//  //assert((commandData.returnType == "Result") || (commandData.returnType == "void"));
//
//  // take the pure type of the size parameter; strip the leading 'p' from its name for its local name
//  os << indentation << "  " << commandData.params[returnit->second].pureType << " " << startLowerCase(strip(commandData.params[returnit->second].name, "p")) << ";" << std::endl;
//}

//std::string writeFunctionBodyEnhancedLocalReturnVariable(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool singular)
//{
//  std::string returnName = startLowerCase(strip(commandData.params[commandData.returnParam].name, "p"));
//
//  // there is a returned parameter -> we need a local variable to hold that value
//  if (commandData.returnType != commandData.enhancedReturnType)
//  {
//    // the returned parameter is somehow enanced by us
//    os << indentation << "  ";
//    if (singular)
//    {
//      // in singular case, just use the return parameters pure type for the return variable
//      returnName = stripPluralS(returnName);
//      os << commandData.params[commandData.returnParam].pureType << " " << returnName;
//    }
//    else
//    {
//      // in non-singular case, use the enhanced type for the return variable (like vector<...>)
//      os << commandData.enhancedReturnType << " " << returnName;
//
//      std::map<size_t, size_t>::const_iterator it = commandData.vectorParams.find(commandData.returnParam);
//      if (it != commandData.vectorParams.end() && !commandData.twoStep)
//      {
//        // if the return parameter is a vector parameter, and not part of a two-step algorithm, initialize its size
//        std::string size;
//        if (it->second == ~0)
//        {
//          assert(!commandData.params[commandData.returnParam].len.empty());
//          // the size of the vector is not given by an other parameter, but by some member of a parameter, described as 'parameter::member'
//          // -> replace the '::' by '.' and filter out the leading 'p' to access that value
//          size = startLowerCase(strip(commandData.params[commandData.returnParam].len, "p"));
//          size_t pos = size.find("::");
//          assert(pos != std::string::npos);
//          size.replace(pos, 2, ".");
//        }
//        else
//        {
//          // the size of the vector is given by an other parameter
//          // that means (as this is not a two-step algorithm) it's size is determined by some other vector parameter!
//          // -> look for it and get it's actual size
//          for (auto const& vectorParam : commandData.vectorParams)
//          {
//            if ((vectorParam.first != commandData.returnParam) && (vectorParam.second == it->second))
//            {
//              size = startLowerCase(strip(commandData.params[vectorParam.first].name, "p")) + ".size()";
//              break;
//            }
//          }
//        }
//        assert(!size.empty());
//        os << "( " << size << " )";
//      }
//    }
//    os << ";" << std::endl;
//  }
//  else
//  {
//    // the return parameter is not enhanced -> the type is supposed to be a Result and there are more than one success codes!
//    assert((commandData.returnType == "Result") && (1 < commandData.successCodes.size()));
//    os << indentation << "  " << commandData.params[commandData.returnParam].pureType << " " << returnName << ";" << std::endl;
//  }
//
//  return returnName;
//}

//void writeFunctionBodyEnhancedMultiVectorSizeCheck(std::ostream & os, std::string const& indentation, CommandData const& commandData)
//{
//  std::string const templateString = 
//R"#(#ifdef VULKAN_HPP_NO_EXCEPTIONS
//${i}  assert( ${firstVectorName}.size() == ${secondVectorName}.size() );
//#else
//${i}  if ( ${firstVectorName}.size() != ${secondVectorName}.size() )
//${i}  {
//${i}    throw LogicError( "vk::${className}::${reducedName}: ${firstVectorName}.size() != ${secondVectorName}.size()" );
//${i}  }
//#endif  // VULKAN_HPP_NO_EXCEPTIONS
//)#";
//
//
//  // add some error checks if multiple vectors need to have the same size
//  for (std::map<size_t, size_t>::const_iterator it0 = commandData.vectorParams.begin(); it0 != commandData.vectorParams.end(); ++it0)
//  {
//    if (it0->first != commandData.returnParam)
//    {
//      for (std::map<size_t, size_t>::const_iterator it1 = std::next(it0); it1 != commandData.vectorParams.end(); ++it1)
//      {
//        if ((it1->first != commandData.returnParam) && (it0->second == it1->second))
//        {
//          os << replaceWithMap(templateString, std::map<std::string, std::string>( {
//            { "firstVectorName", startLowerCase(strip(commandData.params[it0->first].name, "p")) },
//            { "secondVectorName", startLowerCase(strip(commandData.params[it1->first].name, "p")) },
//            { "className", commandData.className },
//            { "reducedName", commandData.reducedName},
//            { "i", indentation}
//          }));
//        }
//      }
//    }
//  }
//}

//void writeFunctionBodyEnhancedReturnResultValue(std::ostream & os, std::string const& indentation, std::string const& returnName, CommandData const& commandData, bool singular)
//{
//  // if the return type is "Result" or there is at least one success code, create the Result/Value construct to return
//  os << indentation << "  return createResultValue( result, ";
//  if (commandData.returnParam != ~0)
//  {
//    // if there's a return parameter, list it in the Result/Value constructor
//    os << returnName << ", ";
//  }
//
//  // now the function name (with full namespace) as a string
//  os << "\"vk::" << (commandData.className.empty() ? "" : commandData.className + "::") << (singular ? stripPluralS(commandData.reducedName) : commandData.reducedName) << "\"";
//
//  if (!commandData.twoStep && (1 < commandData.successCodes.size()))
//  {
//    // and for the single-step algorithms with more than one success code list them all
//    os << ", { Result::" << commandData.successCodes[0];
//    for (size_t i = 1; i < commandData.successCodes.size(); i++)
//    {
//      os << ", Result::" << commandData.successCodes[i];
//    }
//    os << " }";
//  }
//  os << " );" << std::endl;
//}

//void writeFunctionBodyStandard(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData)
//{
//  os << indentation << "  ";
//  bool castReturn = false;
//  if (commandData.returnType != "void")
//  {
//    // there's something to return...
//    os << "return ";
//
//    castReturn = (vkData.vkTypes.find(commandData.returnType) != vkData.vkTypes.end());
//    if (castReturn)
//    {
//      // the return-type is a vulkan type -> need to cast to vk::-type
//      os << "static_cast<" << commandData.returnType << ">( ";
//    }
//  }
//
//  // call the original function
//  os << "vk" << startUpperCase(commandData.fullName) << "( ";
//
//  if (!commandData.className.empty())
//  {
//    // the command is part of a class -> the first argument is the member variable, starting with "m_"
//    os << "m_" << commandData.params[0].name;
//  }
//
//  // list all the arguments
//  for (size_t i = commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
//  {
//    if (0 < i)
//    {
//      os << ", ";
//    }
//
//    if (vkData.vkTypes.find(commandData.params[i].pureType) != vkData.vkTypes.end())
//    {
//      // the parameter is a vulkan type
//      if (commandData.params[i].type.back() == '*')
//      {
//        // it's a pointer -> need to reinterpret_cast it
//        writeReinterpretCast(os, commandData.params[i].type.find("const") == 0, true, commandData.params[i].pureType, commandData.params[i].type.find("* const") != std::string::npos);
//      }
//      else
//      {
//        // it's a value -> need to static_cast ist
//        os << "static_cast<Vk" << commandData.params[i].pureType << ">";
//      }
//      os << "( " << commandData.params[i].name << " )";
//    }
//    else
//    {
//      // it's a non-vulkan type -> just use it
//      os << commandData.params[i].name;
//    }
//  }
//  os << " )";
//
//  if (castReturn)
//  {
//    // if we cast the return -> close the static_cast
//    os << " )";
//  }
//  os << ";" << std::endl;
//}

//void writeFunctionBodyUnique(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool singular)
//{
//  // the unique version needs a Deleter object for destruction of the newly created stuff
//  std::string type = commandData.params[commandData.returnParam].pureType;
//  std::string typeValue = startLowerCase(type);
//  os << indentation << "  " << type << "Deleter deleter( ";
//  if (vkData.deleterData.find(commandData.className) != vkData.deleterData.end())
//  {
//    // if the Deleter is specific to the command's class, add '*this' to the deleter
//    os << "*this, ";
//  }
//
//  // get the DeleterData corresponding to the returned type
//  std::map<std::string, DeleterData>::const_iterator ddit = vkData.deleterData.find(type);
//  assert(ddit != vkData.deleterData.end());
//  if (ddit->second.pool.empty())
//  {
//    // if this type isn't pooled, use the allocator (provided as a function argument)
//    os << "allocator";
//  }
//  else
//  {
//    // otherwise use the pool, which always is a member of the second argument
//    os << startLowerCase(strip(commandData.params[1].name, "p")) << "." << startLowerCase(ddit->second.pool);
//  }
//  os << " );" << std::endl;
//
//  bool returnsVector = !singular && (commandData.vectorParams.find(commandData.returnParam) != commandData.vectorParams.end());
//  if (returnsVector)
//  {
//    // if a vector of data is returned, use a local variable to hold the returned data from the non-unique function call
//    os << indentation << "  std::vector<" << type << ",Allocator> " << typeValue << "s = ";
//  }
//  else
//  {
//    // otherwise create a Unique stuff out of the returned data from the non-unique function call
//    os << indentation << "  return Unique" << type << "( ";
//  }
//
//  // the call to the non-unique function
//  os << (singular ? stripPluralS(commandData.fullName) : commandData.fullName) << "( ";
//  bool argEncountered = false;
//  for (size_t i = commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
//  {
//    if (commandData.skippedParams.find(i) == commandData.skippedParams.end())
//    {
//      if (argEncountered)
//      {
//        os << ", ";
//      }
//      argEncountered = true;
//
//      // strip off the leading 'p' for pointer arguments
//      std::string argumentName = (commandData.params[i].type.back() == '*') ? startLowerCase(strip(commandData.params[i].name, "p")) : commandData.params[i].name;
//      if (singular && (commandData.vectorParams.find(i) != commandData.vectorParams.end()))
//      {
//        // and strip off the plural 's' if appropriate
//        argumentName = stripPluralS(argumentName);
//      }
//      os << argumentName;
//    }
//  }
//  os << " )";
//  if (returnsVector)
//  {
//    std::string const stringTemplate = R"(;
//${i}  std::vector<Unique${type}> unique${type}s;
//${i}  unique${type}s.reserve( ${typeValue}s.size() );
//${i}  for ( auto ${typeValue} : ${typeValue}s )
//${i}  {
//${i}    unique${type}s.push_back( Unique${type}( ${typeValue}, deleter ) );
//${i}  }
//${i}  return unique${type}s;
//)";
//    os << replaceWithMap(stringTemplate, std::map<std::string, std::string>{
//      { "i", indentation },
//      { "type", type },
//      { "typeValue", typeValue }
//    });
//  }
//  else
//  {
//    // for non-vector returns, just add the deleter (local variable) to the Unique-stuff constructor
//    os << ", deleter );" << std::endl;
//  }
//}

//void writeFunctionHeaderArguments(std::ostream & os, VkData const& vkData, CommandData const& commandData, bool enhanced, bool singular, bool withDefaults)
//{
//  os << "(";
//  if (enhanced)
//  {
//    writeFunctionHeaderArgumentsEnhanced(os, vkData, commandData, singular, withDefaults);
//  }
//  else
//  {
//    writeFunctionHeaderArgumentsStandard(os, commandData);
//  }
//  os << ")";
//  if (!commandData.className.empty())
//  {
//    os << " const";
//  }
//}

//void writeFunctionHeaderArgumentsEnhanced(std::ostream & os, VkData const& vkData, CommandData const& commandData, bool singular, bool withDefaults)
//{
//  // check if there's at least one argument left to put in here
//  if (commandData.skippedParams.size() + (commandData.className.empty() ? 0 : 1) < commandData.params.size())
//  {
//    // determine the last argument, where we might provide some default for
//    size_t lastArgument = ~0;
//    for (size_t i = commandData.params.size() - 1; i < commandData.params.size(); i--)
//    {
//      if (commandData.skippedParams.find(i) == commandData.skippedParams.end())
//      {
//        lastArgument = i;
//        break;
//      }
//    }
//
//    os << " ";
//    bool argEncountered = false;
//    for (size_t i = commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
//    {
//      if (commandData.skippedParams.find(i) == commandData.skippedParams.end())
//      {
//        if (argEncountered)
//        {
//          os << ", ";
//        }
//        std::string strippedParameterName = startLowerCase(strip(commandData.params[i].name, "p"));
//
//        std::map<size_t, size_t>::const_iterator it = commandData.vectorParams.find(i);
//        size_t rightStarPos = commandData.params[i].type.rfind('*');
//        if (it == commandData.vectorParams.end())
//        {
//          // the argument ist not a vector
//          if (rightStarPos == std::string::npos)
//          {
//            // and its not a pointer -> just use its type and name here
//            os << commandData.params[i].type << " " << commandData.params[i].name;
//            if (!commandData.params[i].arraySize.empty())
//            {
//              os << "[" << commandData.params[i].arraySize << "]";
//            }
//
//            if (withDefaults && (lastArgument == i))
//            {
//              // check if the very last argument is a flag without any bits -> provide some empty default for it
//              std::map<std::string, FlagData>::const_iterator flagIt = vkData.flags.find(commandData.params[i].pureType);
//              if (flagIt != vkData.flags.end())
//              {
//				  // TODO: Removed dependencies
//
//                //// get the enum corresponding to this flag, to check if it's empty
//                //std::list<DependencyData>::const_iterator depIt = std::find_if(vkData.dependencies.begin(), vkData.dependencies.end(), [&flagIt](DependencyData const& dd) { return(dd.name == flagIt->first); });
//                //assert((depIt != vkData.dependencies.end()) && (depIt->dependencies.size() == 1));
//                //std::map<std::string, EnumData>::const_iterator enumIt = vkData.enums.find(*depIt->dependencies.begin());
//                ///assert(enumIt != vkData.enums.end());
//                //if (enumIt->second.members.empty())
//                //{
//                //  // there are no bits in this flag -> provide the default
//                //  os << " = " << commandData.params[i].pureType << "()";
//                //}
//              }
//            }
//          }
//          else
//          {
//            // the argument is not a vector, but a pointer
//            assert(commandData.params[i].type[rightStarPos] == '*');
//            if (commandData.params[i].optional)
//            {
//              // for an optional argument, trim the trailing '*' from the type, and the leading 'p' from the name
//              os << "Optional<" << trimEnd(commandData.params[i].type.substr(0, rightStarPos)) << "> " << strippedParameterName;
//              if (withDefaults)
//              {
//                os << " = nullptr";
//              }
//            }
//            else if (commandData.params[i].pureType == "void")
//            {
//              // for void-pointer, just use type and name
//              os << commandData.params[i].type << " " << commandData.params[i].name;
//            }
//            else if (commandData.params[i].pureType != "char")
//            {
//              // for non-char-pointer, change to reference
//              os << trimEnd(commandData.params[i].type.substr(0, rightStarPos)) << " & " << strippedParameterName;
//            }
//            else
//            {
//              // for char-pointer, change to const reference to std::string
//              os << "const std::string & " << strippedParameterName;
//            }
//          }
//        }
//        else
//        {
//          // the argument is a vector
//          // it's optional, if it's marked as optional and there's no size specified
//          bool optional = commandData.params[i].optional && (it->second == ~0);
//          assert((rightStarPos != std::string::npos) && (commandData.params[i].type[rightStarPos] == '*'));
//          if (commandData.params[i].type.find("char") != std::string::npos)
//          {
//            // it's a char-vector -> use a std::string (either optional or a const-reference
//            if (optional)
//            {
//              os << "Optional<const std::string> " << strippedParameterName;
//              if (withDefaults)
//              {
//                os << " = nullptr";
//              }
//            }
//            else
//            {
//              os << "const std::string & " << strippedParameterName;
//            }
//          }
//          else
//          {
//            // it's a non-char vector (they are never optional)
//            assert(!optional);
//            if (singular)
//            {
//              // in singular case, change from pointer to reference
//              os << trimEnd(commandData.params[i].type.substr(0, rightStarPos)) << " & " << stripPluralS(strippedParameterName);
//            }
//            else
//            {
//              // otherwise, use our ArrayProxy
//              bool isConst = (commandData.params[i].type.find("const") != std::string::npos);
//              os << "ArrayProxy<" << ((commandData.templateParam == i) ? (isConst ? "const T" : "T") : trimEnd(commandData.params[i].type.substr(0, rightStarPos))) << "> " << strippedParameterName;
//            }
//          }
//        }
//        argEncountered = true;
//      }
//    }
//    os << " ";
//  }
//}

//void writeFunctionHeaderArgumentsStandard(std::ostream & os, CommandData const& commandData)
//{
//  // for the standard case, just list all the arguments as we've got them
//  bool argEncountered = false;
//  for (size_t i = commandData.className.empty() ? 0 : 1; i < commandData.params.size(); i++)
//  {
//    if (argEncountered)
//    {
//      os << ",";
//    }
//
//    os << " " << commandData.params[i].type << " " << commandData.params[i].name;
//    if (!commandData.params[i].arraySize.empty())
//    {
//      os << "[" << commandData.params[i].arraySize << "]";
//    }
//    argEncountered = true;
//  }
//  if (argEncountered)
//  {
//    os << " ";
//  }
//}

//void writeFunctionHeaderName(std::ostream & os, std::string const& name, bool singular, bool unique)
//{
//  os << (singular ? stripPluralS(name) : name);
//  if (unique)
//  {
//    os << "Unique";
//  }
//}

//void writeFunctionHeaderReturnType(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool enhanced, bool singular, bool unique)
//{
//  std::string templateString;
//  std::string returnType;
//  if (enhanced)
//  {
//    // the enhanced function might return some pretty complex return stuff
//    if (unique)
//    {
//      // the unique version returns something prefixed with 'Unique'; potentially a vector of that stuff
//      // it's a vector, if it's not the singular version and the return parameter is a vector parameter
//      bool returnsVector = !singular && (commandData.vectorParams.find(commandData.returnParam) != commandData.vectorParams.end());
//      templateString = returnsVector ? "std::vector<Unique${returnType}> " : "Unique${returnType} ";
//      returnType = commandData.params[commandData.returnParam].pureType;
//      //os << replaceWithMap(, {{"returnType", commandData.params[commandData.returnParam].pureType }});
//    }
//    else if ((commandData.enhancedReturnType != commandData.returnType) && (commandData.returnType != "void"))
//    {
//      // if the enhanced return type differs from the original return type, and it's not void, we return a ResultValueType<...>::type
//      if (!singular && (commandData.enhancedReturnType.find("Allocator") != std::string::npos))
//      {
//        // for the non-singular case with allocation, we need to prepend with 'typename' to keep compilers happy
//        templateString = "typename ResultValueType<${returnType}>::type ";
//      }
//      else
//      {
//        templateString = "ResultValueType<${returnType}>::type ";
//      }
//      assert(commandData.returnType == "Result");
//      // in singular case, we create the ResultValueType from the pure return type, otherwise from the enhanced return type
//      returnType = singular ? commandData.params[commandData.returnParam].pureType : commandData.enhancedReturnType;
//    }
//    else if ((commandData.returnParam != ~0) && (1 < commandData.successCodes.size()))
//    {
//      // if there is a return parameter at all, and there are multiple success codes, we return a ResultValue<...> with the pure return type
//      assert(commandData.returnType == "Result");
//      templateString = "ResultValue<${returnType}> ";
//      returnType = commandData.params[commandData.returnParam].pureType;
//    }
//    else
//    {
//      // and in every other case, we just return the enhanced return type.
//      templateString = "${returnType} ";
//      returnType = commandData.enhancedReturnType;
//    }
//  }
//  else
//  {
//    // the non-enhanced function just uses the return type
//    templateString = "${returnType} ";
//    returnType = commandData.returnType;
//  }
//  os << replaceWithMap(templateString, { { "returnType", returnType } });
//}

//void writeFunctionHeaderTemplate(std::ostream & os, std::string const& indentation, CommandData const& commandData, bool withDefault)
//{
//  if ((commandData.templateParam != ~0) && ((commandData.templateParam != commandData.returnParam) || (commandData.enhancedReturnType == "Result")))
//  {
//    // if there's a template parameter, not being the return parameter or where the enhanced return type is 'Result' -> templatize on type 'T'
//    assert(commandData.enhancedReturnType.find("Allocator") == std::string::npos);
//    os << indentation << "template <typename T>" << std::endl;
//  }
//  else if ((commandData.enhancedReturnType.find("Allocator") != std::string::npos))
//  {
//    // otherwise, if there's an Allocator used in the enhanced return type, we templatize on that Allocator
//    assert((commandData.enhancedReturnType.substr(0, 12) == "std::vector<") && (commandData.enhancedReturnType.find(',') != std::string::npos) && (12 < commandData.enhancedReturnType.find(',')));
//    os << indentation << "template <typename Allocator";
//    if (withDefault)
//    {
//      // for the default type get the type from the enhancedReturnType, which is of the form 'std::vector<Type,Allocator>'
//      os << " = std::allocator<" << commandData.enhancedReturnType.substr(12, commandData.enhancedReturnType.find(',') - 12) << ">";
//    }
//    os << "> " << std::endl;
//  }
//}

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

//void writeStructConstructor( std::ostream & os, std::string const& name, StructData const& structData, std::set<std::string> const& vkTypes, std::map<std::string,std::string> const& defaultValues )
//{
//  // the constructor with all the elements as arguments, with defaults
//  os << "    " << name << "( ";
//  bool listedArgument = false;
//  for (size_t i = 0; i<structData.members.size(); i++)
//  {
//    if (listedArgument)
//    {
//      os << ", ";
//    }
//    // skip members 'pNext' and 'sType', as they are never explicitly set
//    if ((structData.members[i].name != "pNext") && (structData.members[i].name != "sType"))
//    {
//      // find a default value for the given pure type
//      std::map<std::string, std::string>::const_iterator defaultIt = defaultValues.find(structData.members[i].pureType);
//      assert(defaultIt != defaultValues.end());
//
//      if (structData.members[i].arraySize.empty())
//      {
//        // the arguments name get a trailing '_', to distinguish them from the actual struct members
//        // pointer arguments get a nullptr as default
//        os << structData.members[i].type << " " << structData.members[i].name << "_ = " << (structData.members[i].type.back() == '*' ? "nullptr" : defaultIt->second);
//      }
//      else
//      {
//        // array members are provided as const reference to a std::array
//        // the arguments name get a trailing '_', to distinguish them from the actual struct members
//        // list as many default values as there are elements in the array
//        os << "std::array<" << structData.members[i].type << "," << structData.members[i].arraySize << "> const& " << structData.members[i].name << "_ = { { " << defaultIt->second;
//        size_t n = atoi(structData.members[i].arraySize.c_str());
//        assert(0 < n);
//        for (size_t j = 1; j < n; j++)
//        {
//          os << ", " << defaultIt->second;
//        }
//        os << " } }";
//      }
//      listedArgument = true;
//    }
//  }
//  os << " )" << std::endl;
//
//  // copy over the simple arguments
//  bool firstArgument = true;
//  for (size_t i = 0; i < structData.members.size(); i++)
//  {
//    if (structData.members[i].arraySize.empty())
//    {
//      // here, we can only handle non-array arguments
//      std::string templateString = "      ${sep} ${member}( ${value} )\n";
//      std::string sep = firstArgument ? ":" : ",";
//      std::string member = structData.members[i].name;
//      std::string value;
//
//      // 'pNext' and 'sType' don't get an argument, use nullptr and the correct StructureType enum value to initialize them
//      if (structData.members[i].name == "pNext")
//      {
//        value = "nullptr";
//      }
//      else if (structData.members[i].name == "sType")
//      {
//        value = std::string("StructureType::e") + name;
//      }
//      else
//      {
//        // the other elements are initialized by the corresponding argument (with trailing '_', as mentioned above)
//        value = structData.members[i].name + "_";
//      }
//      os << replaceWithMap(templateString, { {"sep", sep}, {"member", member}, {"value", value} });
//      firstArgument = false;
//    }
//  }
//
//  // the body of the constructor, copying over data from argument list into wrapped struct
//  os << "    {" << std::endl;
//  for ( size_t i=0 ; i<structData.members.size() ; i++ )
//  {
//    if (!structData.members[i].arraySize.empty())
//    {
//      // here we can handle the arrays, copying over from argument (with trailing '_') to member
//      // size is arraySize times sizeof type
//      std::string member = structData.members[i].name;
//      std::string arraySize = structData.members[i].arraySize;
//      std::string type = structData.members[i].type;
//      os << replaceWithMap("      memcpy( &${member}, ${member}_.data(), ${arraySize} * sizeof( ${type} ) );\n",
//                            { {"member", member}, {"arraySize", arraySize }, {"type", type} });
//    }
//  }
//  os << "    }" << std::endl
//      << std::endl;
//
//  std::string templateString = 
//R"(    ${name}( Vk${name} const & rhs )
//    {
//      memcpy( this, &rhs, sizeof( ${name} ) );
//    }
//
//    ${name}& operator=( Vk${name} const & rhs )
//    {
//      memcpy( this, &rhs, sizeof( ${name} ) );
//      return *this;
//    }
//)";
//
//  os << replaceWithMap(templateString, { {"name", name } } );
//}

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

//void writeTypeCommand(std::ostream & os, VkData const& vkData)
//{
//  assert(vkData.commands.find(dependencyData.name) != vkData.commands.end());
//  CommandData const& commandData = vkData.commands.find(dependencyData.name)->second;
//  if (commandData.className.empty())
//  {
//    if (commandData.fullName == "createInstance")
//    {
//      // special handling for createInstance, as we need to explicitly place the forward declarations and the deleter classes here
//      auto deleterTypesIt = vkData.deleterTypes.find("");
//      assert((deleterTypesIt != vkData.deleterTypes.end()) && (deleterTypesIt->second.size() == 1));
//
//      writeDeleterForwardDeclarations(os, *deleterTypesIt, vkData.deleterData);
//      writeTypeCommand(os, "  ", vkData, commandData, false);
//      writeDeleterClasses(os, *deleterTypesIt, vkData.deleterData);
//    }
//    else
//    {
//      writeTypeCommand(os, "  ", vkData, commandData, false);
//    }
//    writeTypeCommand(os, "  ", vkData, commandData, true);
//    os << std::endl;
//  }
//}

//void writeTypeCommand(std::ostream & os, std::string const& indentation, VkData const& vkData, CommandData const& commandData, bool definition)
//{
//  //enterProtect(os, commandData.protect);
//
//  // first create the standard version of the function
//  std::ostringstream standard;
//  writeFunction(standard, indentation, vkData, commandData, definition, false, false, false);
//
//  // then the enhanced version, composed by up to four parts
//  std::ostringstream enhanced;
//  writeFunction(enhanced, indentation, vkData, commandData, definition, true, false, false);
//
//  // then a singular version, if a sized vector would be returned
//  std::map<size_t, size_t>::const_iterator returnVector = commandData.vectorParams.find(commandData.returnParam);
//  bool singular = (returnVector != commandData.vectorParams.end()) && (returnVector->second != ~0) /*&& (commandData.params[returnVector->second].type.back() != '*')*/;
//  if (singular)
//  {
//    writeFunction(enhanced, indentation, vkData, commandData, definition, true, true, false);
//  }
//
//  // special handling for createDevice and createInstance !
//  bool specialWriteUnique = (commandData.reducedName == "createDevice") || (commandData.reducedName == "createInstance");
//
//  // and then the same for the Unique* versions (a Deleter is available for the commandData's class, and the function starts with 'allocate' or 'create')
//  if (((vkData.deleterData.find(commandData.className) != vkData.deleterData.end()) || specialWriteUnique) && ((commandData.reducedName.substr(0, 8) == "allocate") || (commandData.reducedName.substr(0, 6) == "create")))
//  {
//    enhanced << "#ifndef VULKAN_HPP_NO_SMART_HANDLE" << std::endl;
//    writeFunction(enhanced, indentation, vkData, commandData, definition, true, false, true);
//
//    if (singular)
//    {
//      writeFunction(enhanced, indentation, vkData, commandData, definition, true, true, true);
//    }
//    enhanced << "#endif /*VULKAN_HPP_NO_SMART_HANDLE*/" << std::endl;
//  }
//
//  // and write one or both of them
//  writeStandardOrEnhanced(os, standard.str(), enhanced.str());
//  //leaveProtect(os, commandData.protect);
//  os << std::endl;
//}

void writeTypeEnum( std::ostream & os, EnumData const& enumData )
{
  // a named enum per enum, listing all its values by setting them to the original Vulkan names
  //enterProtect(os, enumData.protect);
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
  //leaveProtect(os, enumData.protect);
  os << std::endl;
}

//void writeDeleterClasses(std::ostream & os, std::pair<std::string, std::set<std::string>> const& deleterTypes, std::map<std::string, DeleterData> const& deleterData)
//{
//  // A Deleter class for each of the Unique* classes... but only if smart handles are not switched off
//  os << "#ifndef VULKAN_HPP_NO_SMART_HANDLE" << std::endl;
//  bool first = true;
//
//  // get type and name of the parent (holder) type
//  std::string parentType = deleterTypes.first;
//  std::string parentName = parentType.empty() ? "" : startLowerCase(parentType);
//
//  // iterate over the deleter types parented by this type
//  for (auto const& deleterType : deleterTypes.second)
//  {
//    std::string deleterName = startLowerCase(deleterType);
//    bool standardDeleter = !parentType.empty() && (deleterType != "Device");    // this detects the 'standard' case for a deleter
//
//    if (!first)
//    {
//      os << std::endl;
//    }
//    first = false;
//
//    os << "  class " << deleterType << "Deleter" << std::endl
//      << "  {" << std::endl
//      << "  public:" << std::endl
//      << "    " << deleterType << "Deleter( ";
//    if (standardDeleter)
//    {
//      // the standard deleter gets a parent type in the constructor
//      os << parentType << " " << parentName << " = " << parentType << "(), ";
//    }
//
//    // if this Deleter is pooled, make such a pool the last argument, otherwise an Optional allocator
//    auto const& dd = deleterData.find(deleterType);
//    assert(dd != deleterData.end());
//    std::string poolName = (dd->second.pool.empty() ? "" : startLowerCase(dd->second.pool));
//    if (poolName.empty())
//    {
//      os << "Optional<const AllocationCallbacks> allocator = nullptr )" << std::endl;
//    }
//    else
//    {
//      assert(!dd->second.pool.empty());
//      os << dd->second.pool << " " << poolName << " = " << dd->second.pool << "() )" << std::endl;
//    }
//
//    // now the initializer list of the Deleter constructor
//    os << "      : ";
//    if (standardDeleter)
//    {
//      // the standard deleter has a parent type as a member
//      os << "m_" << parentName << "( " << parentName << " )" << std::endl
//        << "      , ";
//    }
//    if (poolName.empty())
//    {
//      // non-pooled deleter have an allocator as a member
//      os << "m_allocator( allocator )" << std::endl;
//    }
//    else
//    {
//      // pooled deleter have a pool as a member
//      os << "m_" << poolName << "( " << poolName << " )" << std::endl;
//    }
//
//    // besides that, the constructor is empty
//    os << "    {}" << std::endl
//      << std::endl;
//
//    // the operator() calls the delete/destroy function
//    os << "    void operator()( " << deleterType << " " << deleterName << " )" << std::endl
//      << "    {" << std::endl;
//
//    // the delete/destroy function is either part of the parent member of the deleter argument
//    if (standardDeleter)
//    {
//      os << "      m_" << parentName << ".";
//    }
//    else
//    {
//      os << "      " << deleterName << ".";
//    }
//
//    os << dd->second.call << "( ";
//
//    if (!poolName.empty())
//    {
//      // pooled Deleter gets the pool as the first argument
//      os << "m_" << poolName << ", ";
//    }
//
//    if (standardDeleter)
//    {
//      // the standard deleter gets the deleter argument as an argument
//      os << deleterName;
//    }
//
//    // the non-pooled deleter get the allocate as an argument (potentially after the deleterName
//    if (poolName.empty())
//    {
//      if (standardDeleter)
//      {
//        os << ", ";
//      }
//      os << "m_allocator";
//    }
//    os << " );" << std::endl
//      << "    }" << std::endl
//      << std::endl;
//
//    // now the members of the Deleter class
//    os << "  private:" << std::endl;
//    if (standardDeleter)
//    {
//      // the parentType for the standard deleter
//      os << "    " << parentType << " m_" << parentName << ";" << std::endl;
//    }
//
//    // the allocator for the non-pooled deleters, the pool for the pooled ones
//    if (poolName.empty())
//    {
//      os << "    Optional<const AllocationCallbacks> m_allocator;" << std::endl;
//    }
//    else
//    {
//      os << "    " << dd->second.pool << " m_" << poolName << ";" << std::endl;
//    }
//    os << "  };" << std::endl;
//  }
//
//  os << "#endif /*VULKAN_HPP_NO_SMART_HANDLE*/" << std::endl
//    << std::endl;
//}

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

//void writeTypeFlags(std::ostream & os, std::string const& flagsName, FlagData const& flagData, EnumData const& enumData)
//{
//  //enterProtect(os, flagData.protect);
//  // each Flags class is using on the class 'Flags' with the corresponding FlagBits enum as the template parameter
//  os << "  using " << flagsName << " = Flags<" << enumData.name << ", Vk" << flagsName << ">;" << std::endl;
//
//  std::stringstream allFlags;
//  for (size_t i = 0; i < enumData.members.size(); i++)
//  {
//    if (i != 0)
//    {
//      allFlags << " | ";
//    }
//    allFlags << "VkFlags(" << enumData.name << "::" << enumData.members[i].name << ")";
//  }
//
//  if (!enumData.members.empty())
//  {
//    const std::string templateString = R"(
//  VULKAN_HPP_INLINE ${flagsName} operator|( ${enumName} bit0, ${enumName} bit1 )
//  {
//    return ${flagsName}( bit0 ) | bit1;
//  }
//
//  VULKAN_HPP_INLINE ${flagsName} operator~( ${enumName} bits )
//  {
//    return ~( ${flagsName}( bits ) );
//  }
//
//  template <> struct FlagTraits<${enumName}>
//  {
//    enum
//    {
//      allFlags = ${allFlags}
//    };
//  };
//)";
//    os << replaceWithMap(templateString, { { "flagsName", flagsName}, { "enumName", enumData.name }, { "allFlags", allFlags.str() } } );
//  }
//  //leaveProtect(os, flagData.protect);
//  os << std::endl;
//}

//void writeTypeHandle(std::ostream & os, VkData const& vkData, HandleData const& handleData)
//{
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
//}

void writeTypeScalar( std::ostream & os )
{
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

//void writeTypeStruct( std::ostream & os, VkData const& vkData, std::map<std::string,std::string> const& defaultValues )
//{
//  std::map<std::string,StructData>::const_iterator it = vkData.structs.find( dependencyData.name );
//  assert( it != vkData.structs.end() );
//
//  enterProtect(os, it->second.protect);
//  os << "  struct " << dependencyData.name << std::endl
//      << "  {" << std::endl;
//
//  // only structs that are not returnedOnly get a constructor!
//  if ( !it->second.returnedOnly )
//  {
//    writeStructConstructor( os, dependencyData.name, it->second, vkData.vkTypes, defaultValues );
//  }
//
//  // create the setters
//  if (!it->second.returnedOnly)
//  {
//    for (size_t i = 0; i<it->second.members.size(); i++)
//    {
//      writeStructSetter( os, dependencyData.name, it->second.members[i], vkData.vkTypes );
//    }
//  }
//
//  // the cast-operator to the wrapped struct
//  os << "    operator const Vk" << dependencyData.name << "&() const" << std::endl
//      << "    {" << std::endl
//      << "      return *reinterpret_cast<const Vk" << dependencyData.name << "*>(this);" << std::endl
//      << "    }" << std::endl
//      << std::endl;
//
//  // operator==() and operator!=()
//  // only structs without a union as a member can have a meaningfull == and != operation; we filter them out
//  if (!containsUnion(dependencyData.name, vkData.structs))
//  {
//    // two structs are compared by comparing each of the elements
//    os << "    bool operator==( " << dependencyData.name << " const& rhs ) const" << std::endl
//        << "    {" << std::endl
//        << "      return ";
//    for (size_t i = 0; i < it->second.members.size(); i++)
//    {
//      if (i != 0)
//      {
//        os << std::endl << "          && ";
//      }
//      if (!it->second.members[i].arraySize.empty())
//      {
//        os << "( memcmp( " << it->second.members[i].name << ", rhs." << it->second.members[i].name << ", " << it->second.members[i].arraySize << " * sizeof( " << it->second.members[i].type << " ) ) == 0 )";
//      }
//      else
//      {
//        os << "( " << it->second.members[i].name << " == rhs." << it->second.members[i].name << " )";
//      }
//    }
//    os << ";" << std::endl
//        << "    }" << std::endl
//        << std::endl
//        << "    bool operator!=( " << dependencyData.name << " const& rhs ) const" << std::endl
//        << "    {" << std::endl
//        << "      return !operator==( rhs );" << std::endl
//        << "    }" << std::endl
//        << std::endl;
//  }
//
//  // the member variables
//  for (size_t i = 0; i < it->second.members.size(); i++)
//  {
//    if (it->second.members[i].type == "StructureType")
//    {
//      assert((i == 0) && (it->second.members[i].name == "sType"));
//      os << "  private:" << std::endl
//          << "    StructureType sType;" << std::endl
//          << std::endl
//          << "  public:" << std::endl;
//    }
//    else
//    {
//      os << "    " << it->second.members[i].type << " " << it->second.members[i].name;
//      if (!it->second.members[i].arraySize.empty())
//      {
//        os << "[" << it->second.members[i].arraySize << "]";
//      }
//      os << ";" << std::endl;
//    }
//  }
//  os << "  };" << std::endl
//      << "  static_assert( sizeof( " << dependencyData.name << " ) == sizeof( Vk" << dependencyData.name << " ), \"struct and wrapper have different size!\" );" << std::endl;
//
//  leaveProtect(os, it->second.protect);
//  os << std::endl;
//}

//void writeTypeUnion( std::ostream & os, VkData const& vkData, std::map<std::string,std::string> const& defaultValues )
//{
//  std::map<std::string, StructData>::const_iterator it = vkData.structs.find(dependencyData.name);
//  assert(it != vkData.structs.end());
//
//  std::ostringstream oss;
//  os << "  union " << dependencyData.name << std::endl
//      << "  {" << std::endl;
//
//  for ( size_t i=0 ; i<it->second.members.size() ; i++ )
//  {
//    // one constructor per union element
//    os << "    " << dependencyData.name << "( ";
//    if ( it->second.members[i].arraySize.empty() )
//    {
//      os << it->second.members[i].type << " ";
//    }
//    else
//    {
//      os << "const std::array<" << it->second.members[i].type << "," << it->second.members[i].arraySize << ">& ";
//    }
//    os << it->second.members[i].name << "_";
//
//    // just the very first constructor gets default arguments
//    if ( i == 0 )
//    {
//      std::map<std::string,std::string>::const_iterator defaultIt = defaultValues.find( it->second.members[i].pureType );
//      assert(defaultIt != defaultValues.end() );
//      if ( it->second.members[i].arraySize.empty() )
//      {
//        os << " = " << defaultIt->second;
//      }
//      else
//      {
//        os << " = { {" << defaultIt->second << "} }";
//      }
//    }
//    os << " )" << std::endl
//        << "    {" << std::endl
//        << "      ";
//    if ( it->second.members[i].arraySize.empty() )
//    {
//      os << it->second.members[i].name << " = " << it->second.members[i].name << "_";
//    }
//    else
//    {
//      os << "memcpy( &" << it->second.members[i].name << ", " << it->second.members[i].name << "_.data(), " << it->second.members[i].arraySize << " * sizeof( " << it->second.members[i].type << " ) )";
//    }
//    os << ";" << std::endl
//        << "    }" << std::endl
//        << std::endl;
//    }
//
//  for (size_t i = 0; i<it->second.members.size(); i++)
//  {
//    // one setter per union element
//    assert(!it->second.returnedOnly);
//    writeStructSetter(os, dependencyData.name, it->second.members[i], vkData.vkTypes);
//  }
//
//  // the implicit cast operator to the native type
//  os << "    operator Vk" << dependencyData.name << " const& () const" << std::endl
//      << "    {" << std::endl
//      << "      return *reinterpret_cast<const Vk" << dependencyData.name << "*>(this);" << std::endl
//      << "    }" << std::endl
//      << std::endl;
//
//  // the union member variables
//  // if there's at least one Vk... type in this union, check for unrestricted unions support
//  bool needsUnrestrictedUnions = false;
//  for (size_t i = 0; i < it->second.members.size() && !needsUnrestrictedUnions; i++)
//  {
//    needsUnrestrictedUnions = (vkData.vkTypes.find(it->second.members[i].type) != vkData.vkTypes.end());
//  }
//  if (needsUnrestrictedUnions)
//  {
//    os << "#ifdef VULKAN_HPP_HAS_UNRESTRICTED_UNIONS" << std::endl;
//    for (size_t i = 0; i < it->second.members.size(); i++)
//    {
//      os << "    " << it->second.members[i].type << " " << it->second.members[i].name;
//      if (!it->second.members[i].arraySize.empty())
//      {
//        os << "[" << it->second.members[i].arraySize << "]";
//      }
//      os << ";" << std::endl;
//    }
//    os << "#else" << std::endl;
//  }
//  for (size_t i = 0; i < it->second.members.size(); i++)
//  {
//    os << "    ";
//    if (vkData.vkTypes.find(it->second.members[i].type) != vkData.vkTypes.end())
//    {
//      os << "Vk";
//    }
//    os << it->second.members[i].type << " " << it->second.members[i].name;
//    if (!it->second.members[i].arraySize.empty())
//    {
//      os << "[" << it->second.members[i].arraySize << "]";
//    }
//    os << ";" << std::endl;
//  }
//  if (needsUnrestrictedUnions)
//  {
//    os << "#endif  // VULKAN_HPP_HAS_UNRESTRICTED_UNIONS" << std::endl;
//  }
//  os << "  };" << std::endl
//      << std::endl;
//}

//void writeTypes(std::ostream & os, VkData const& vkData, std::map<std::string, std::string> const& defaultValues)
//{
//  for ( std::list<DependencyData>::const_iterator it = vkData.dependencies.begin() ; it != vkData.dependencies.end() ; ++it )
//  {
//    switch( it->category )
//    {
//      case DependencyData::Category::COMMAND :
//        writeTypeCommand( os, vkData, *it );
//        break;
//      case DependencyData::Category::ENUM :
//        assert( vkData.enums.find( it->name ) != vkData.enums.end() );
//        writeTypeEnum( os, vkData.enums.find( it->name )->second );
//        break;
//      case DependencyData::Category::FLAGS :
//        assert(vkData.flags.find(it->name) != vkData.flags.end());
//        writeTypeFlags( os, it->name, vkData.flags.find( it->name)->second, vkData.enums.find(generateEnumNameForFlags(it->name))->second );
//        break;
//      case DependencyData::Category::FUNC_POINTER :
//      case DependencyData::Category::REQUIRED :
//        // skip FUNC_POINTER and REQUIRED, they just needed to be in the dependencies list to resolve dependencies
//        break;
//      case DependencyData::Category::HANDLE :
//        assert(vkData.handles.find(it->name) != vkData.handles.end());
//        writeTypeHandle(os, vkData, *it, vkData.handles.find(it->name)->second, vkData.dependencies);
//        break;
//      case DependencyData::Category::SCALAR :
//        writeTypeScalar( os, *it );
//        break;
//      case DependencyData::Category::STRUCT :
//        writeTypeStruct( os, vkData, *it, defaultValues );
//        break;
//      case DependencyData::Category::UNION :
//        assert( vkData.structs.find( it->name ) != vkData.structs.end() );
//        writeTypeUnion( os, vkData, *it, defaultValues );
//        break;
//      default :
//        assert( false );
//        break;
//    }
//  }
//}

void writeVersionCheck(std::ostream & os, std::string const& version)
{
	os << "const VK_HEADER_VERSION: u32 = " << version << ";" << std::endl;
	os << std::endl;
	os << "pub fn VK_MAKE_VERSION(major: u32, minor: u32, patch: u32) -> u32 {" << std::endl;
	os << "    (major << 22) | (minor << 12) | patch" << std::endl;
	os << "}" << std::endl;
}

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
		return "[" + type_name + "; " + array_size + "]";
	}

	virtual std::string array_param(std::string const& type_name, std::string const& array_size) override final {
		return "&[" + type_name + "; " + array_size + "]";
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

		std::cout << "Writing vulkan.rs to " << VULKAN_HPP << std::endl;

		std::ofstream ofs(VULKAN_HPP);
		ofs << reg.license() << std::endl
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

		writeVersionCheck(ofs, reg.version());

		ofs << std::endl;

		for (auto tdef : reg.get_scalar_typedefs()) {
			ofs << "type " << tdef->actual << " = " << tdef->alias << ";" << std::endl;
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
