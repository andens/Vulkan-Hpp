#include "cpp_dispatch_tables.h"

using namespace std;

void print_func_wrapper_h(ofstream& file, vkspec::Command* c) {
  file << c->complete_return_type() << " " << c->name() << "(";
  std::string comma = "";
  for (auto& p : c->params()) {
    file << comma << p.complete_type << " " << p.name;
    comma = ", ";
  }
  file << ") const;" << std::endl;
}

void print_func_wrapper_cpp(ofstream& file, IndentingOStreambuf* ind, vkspec::Command* c, string const& class_name) {
  file << c->complete_return_type() << " " << class_name << "::" << c->name() << "(";
  string comma = "";
  for (auto& p : c->params()) {
    file << comma << p.complete_type << " " << p.name;
    comma = ", ";
  }
  file << ") const {" << endl;
  ind->increase();
  file << "return this->" << c->name() << "_(";
  comma = "";
  for (auto& p : c->params()) {
    file << comma << p.name;
    comma = ", ";
  }
  file << ");" << endl;
  ind->decrease();
  file << "}" << endl;
  file << endl;
}

CppDispatchTableGenerator::CppDispatchTableGenerator(string const& out_dir, string const& license, int major, int minor, int patch) {
  header.open(out_dir + "/vk_dispatch_tables.h");
  cpp.open(out_dir + "/vk_dispatch_tables.cpp");
  if (!header.is_open() || !cpp.is_open()) {
    throw std::runtime_error("Failed to open files for output");
  }

  ind_h = new IndentingOStreambuf(header, 2);
  ind_cpp = new IndentingOStreambuf(cpp, 2);

  header << license << endl;
  header << endl;
  header << "// Dispatch tables for Vulkan " << major << "." << minor << "." << patch << ", generated from the Khronos Vulkan API XML Registry." << endl;
  header << "// See https://github.com/andens/Vulkan-Hpp for generator details." << endl;
  header << endl;
  header << "#ifndef VK_DISPATCH_TABLES_INCLUDE" << endl;
  header << "#define VK_DISPATCH_TABLES_INCLUDE" << endl;
  header << endl;
  header << "#include \"vulkan_include.inl\"" << endl;
  header << "#include <stdexcept>" << endl;
  header << "#include <string>" << endl;
  header << "#if defined(_WIN32)" << endl;
  header << "#include <Windows.h>" << endl;
  header << "#endif" << endl;
  header << endl;
  header << "namespace vkgen {" << endl;
  header << R"(
class VulkanProcNotFound: public std::exception {
public:
  VulkanProcNotFound(std::string const& proc) : proc_(proc) {}
  virtual const char* what() const throw() {
    return "Write better error message here";
  }

private:
  std::string proc_;
};
)";

  cpp << "#include \"vk_dispatch_tables.h\"" << endl;
  cpp << endl;
  cpp << "#include <stdexcept>" << endl;
  cpp << endl;
  cpp << "namespace vkgen {" << endl;
}

CppDispatchTableGenerator::~CppDispatchTableGenerator() {
  header << endl;
  header << "} // vkgen" << endl;
  header << endl;
  header << "#endif // VK_DISPATCH_TABLES_INCLUDE" << endl;
  header << endl;

  cpp << "} // vkgen" << endl;

  delete ind_h;
  delete ind_cpp;

  header.close();
  cpp.close();
}

void CppDispatchTableGenerator::gen_entry_command(vkspec::Command* c) {
  // There should only ever be one entry command. If not, I will need to
  // adapt the bindings accordingly.
  assert(!_entry_command);
  _entry_command = c;
}

void CppDispatchTableGenerator::gen_global_command(vkspec::Command* c) {
  _global_commands.push_back(c);
}

void CppDispatchTableGenerator::end_global_commands() {
  header << R"(
class VulkanGlobalTable {
#if defined(_WIN32)
  typedef HMODULE library_handle;
#elif defined(__linux__)
  typedef void* library_handle;
#else
#error "Unsupported OS"
#endif

public:
  VulkanGlobalTable(std::string const& vulkan_library);
)";

  ind_h->increase();

  print_func_wrapper_h(header, _entry_command);
  for (auto c : _global_commands) {
    print_func_wrapper_h(header, c);
  }

  header << endl;

  ind_h->decrease();
  header << "private:" << endl;
  ind_h->increase();
  header << "library_handle library_ = nullptr;" << endl;
  header << "PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_ = nullptr;" << endl;
  for (auto c : _global_commands) {
    header << "PFN_" << c->name() << " " << c->name() << "_ = nullptr;" << endl;
  }
  ind_h->decrease();
  header << "};" << endl; // End GlobalDispatchTable class

  cpp << endl;
  cpp << "/*" << endl;
  cpp << " * ------------------------------------------------------" << endl;
  cpp << " * " << "VulkanGlobalTable" << endl;
  cpp << " * ------------------------------------------------------" << endl;
  cpp << "*/" << endl;

  cpp << R"(
VulkanGlobalTable::VulkanGlobalTable(std::string const& vulkan_library) {
#if defined(_WIN32)
  library_ = LoadLibraryA(vulkan_library.c_str());
#elif defined(__linux__)
  library_ = dlopen(vulkan_library.c_str(), RTLD_NOW);
#else
#error "Unsupported OS"
#endif

  if (!library_) {
    throw std::runtime_error("Could not load Vulkan loader.");
  }

#if defined(_WIN32)
  vkGetInstanceProcAddr_ = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
      GetProcAddress(library_, "vkGetInstanceProcAddr"));
#elif defined(__linux__)
  vkGetInstanceProcAddr_ = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
      dlsym(library_, "vkGetInstanceProcAddr"));
#else
#error "Unsupported OS"
#endif

  if (!vkGetInstanceProcAddr_) {
    throw VulkanProcNotFound("vkGetInstanceProcAddr");
  }

  // With vkGetInstanceProcAddr loaded, we can load global functions, which
  // are the ones not making use of the instance parameter.
#define LOAD_GLOBAL_FUNC(fun)\
  this->##fun##_ = reinterpret_cast<PFN_##fun>(this->vkGetInstanceProcAddr_(\
      nullptr, #fun));\
  if (!this->##fun##_) {\
    throw VulkanProcNotFound(#fun);\
  }
)" << endl;

  ind_cpp->increase();

  for (auto c : _global_commands) {
    cpp << "LOAD_GLOBAL_FUNC(" << c->name() << ");" << endl;
  }

  ind_cpp->decrease();
  cpp << "}" << endl; // ctor
  cpp << endl;

  print_func_wrapper_cpp(cpp, ind_cpp, _entry_command, "VulkanGlobalTable");
  for (auto c : _global_commands) {
    print_func_wrapper_cpp(cpp, ind_cpp, c, "VulkanGlobalTable");
  }
}
