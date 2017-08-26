#include "cpp_dispatch_tables.h"

using namespace std;

void print_func_wrapper_h(ofstream& file, IndentingOStreambuf* ind, vkspec::Command* c, bool dispatchable) {
  if (c->extension() && c->extension()->protect() != "") {
    ind->decrease();
    file << "#if defined(" << c->extension()->protect() << ")" << endl;
    ind->increase();
  }

  file << c->complete_return_type() << " " << c->name() << "(";
  std::string comma = "";
  for (auto p = c->params().begin(); p != c->params().end(); ++p) {
    // Skip first parameter if we have a dispatchable object
    if (p == c->params().begin() && dispatchable) {
      continue;
    }
    file << comma << p->complete_type << " " << p->name;
    if (p->array_size != "") {
      file << "[" << p->array_size << "]";
    }
    comma = ", ";
  }
  file << ") const;" << std::endl;

  if (c->extension() && c->extension()->protect() != "") {
    ind->decrease();
    file << "#endif" << endl;
    ind->increase();
  }
}

void print_func_member(ofstream& file, IndentingOStreambuf* ind, vkspec::Command* c) {
  if (c->extension() && c->extension()->protect() != "") {
    ind->decrease();
    file << "#if defined(" << c->extension()->protect() << ")" << endl;
    ind->increase();
  }

  file << "PFN_" << c->name() << " " << c->name() << "_ = nullptr;" << endl;

  if (c->extension() && c->extension()->protect() != "") {
    ind->decrease();
    file << "#endif" << endl;
    ind->increase();
  }
}

// If |dispatchable| is not empty, it means the first parameter should be
// omitted and the cached member variable is used when calling the underlying
// function pointer. The variable name itself is the value of |dispatchable|.
void print_func_wrapper_cpp(ofstream& file, IndentingOStreambuf* ind, vkspec::Command* c, string const& class_name, string const& dispatchable) {
  if (c->extension() && c->extension()->protect() != "") {
    file << "#if defined(" << c->extension()->protect() << ")" << endl;
  }

  file << c->complete_return_type() << " " << class_name << "::" << c->name() << "(";
  string comma = "";
  for (auto p = c->params().begin(); p != c->params().end(); ++p) {
    // Skip first parameter if we have a dispatchable object
    if (p == c->params().begin() && dispatchable != "") {
      continue;
    }
    file << comma << p->complete_type << " " << p->name;
    if (p->array_size != "") {
      file << "[" << p->array_size << "]";
    }
    comma = ", ";
  }
  file << ") const {" << endl;
  ind->increase();
  file << "return this->" << c->name() << "_(";
  comma = "";
  for (auto p = c->params().begin(); p != c->params().end(); ++p) {
    file << comma;
    if (p == c->params().begin() && dispatchable != "") {
      file << dispatchable;
    }
    else {
      file << p->name;
    }
    comma = ", ";
  }
  file << ");" << endl;
  ind->decrease();
  file << "}" << endl;

  if (c->extension() && c->extension()->protect() != "") {
    file << "#endif" << endl;
  }

  file << endl;
}

void print_load_instance_proc(ofstream& file, IndentingOStreambuf* ind, string const& context, char const* dispatchable, vkspec::Command* c) {
  if (c->extension() && c->extension()->protect() != "") {
    ind->decrease();
    file << "#if defined(" << c->extension()->protect() << ")" << endl;
    ind->increase();
  }

  file << c->name() << "_ = reinterpret_cast<PFN_" << c->name() << ">(" << context << "->vkGetInstanceProcAddr(";
  if (dispatchable) {
    file << dispatchable << ", ";
  }
  file << "\"" << c->name() << "\"));" << endl;

  // Core functions must be present
  if (!c->extension()) {
    file << "if (!" << c->name() << "_) {" << endl;
    ind->increase();
    file << "throw VulkanProcNotFound(\"" << c->name() << "\");" << endl;
    ind->decrease();
    file << "}" << endl;
  }

  if (c->extension() && c->extension()->protect() != "") {
    ind->decrease();
    file << "#endif" << endl;
    ind->increase();
  }
}

void print_load_device_proc(ofstream& file, IndentingOStreambuf* ind, string const& context, vkspec::Command* c) {
  if (c->extension() && c->extension()->protect() != "") {
    ind->decrease();
    file << "#if defined(" << c->extension()->protect() << ")" << endl;
    ind->increase();
  }
  
  file << c->name() << "_ = reinterpret_cast<PFN_" << c->name() << ">(" << context << "->vkGetDeviceProcAddr(\"" << c->name() << "\"));" << endl;

  // Core functions must exist
  if (!c->extension()) {
    file << "if (!" << c->name() << "_) {" << endl;
    ind->increase();
    file << "throw VulkanProcNotFound(\"" << c->name() << "\");" << endl;
    ind->decrease();
    file << "}" << endl;
  }

  if (c->extension() && c->extension()->protect() != "") {
    ind->decrease();
    file << "#endif" << endl;
    ind->increase();
  }
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
class GlobalFunctions {
#if defined(_WIN32)
  typedef HMODULE library_handle;
#elif defined(__linux__)
  typedef void* library_handle;
#else
#error "Unsupported OS"
#endif

public:
  GlobalFunctions(std::string const& vulkan_library);
  ~GlobalFunctions();
)";

  ind_h->increase();

  print_func_wrapper_h(header, ind_h, _entry_command, false);
  for (auto c : _global_commands) {
    print_func_wrapper_h(header, ind_h, c, false);
  }

  header << endl;

  ind_h->decrease();
  header << "private:" << endl;
  ind_h->increase();
  header << "GlobalFunctions(GlobalFunctions& other) = delete;" << endl;
  header << "void operator=(GlobalFunctions& rhs) = delete;" << endl;
  header << endl;
  ind_h->decrease();

  header << "private:" << endl;
  ind_h->increase();
  header << "library_handle library_ = nullptr;" << endl;
  print_func_member(header, ind_h, _entry_command);
  for (auto c : _global_commands) {
    print_func_member(header, ind_h, c);
  }
  ind_h->decrease();
  header << "};" << endl; // End GlobalDispatchTable class

  cpp << endl;
  cpp << "/*" << endl;
  cpp << " * ------------------------------------------------------" << endl;
  cpp << " * " << "GlobalFunctions" << endl;
  cpp << " * ------------------------------------------------------" << endl;
  cpp << "*/" << endl;

  cpp << R"(
GlobalFunctions::GlobalFunctions(std::string const& vulkan_library) {
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
)" << endl;

  ind_cpp->increase();

  for (auto c : _global_commands) {
    print_load_instance_proc(cpp, ind_cpp, "this", "nullptr", c);
  }

  ind_cpp->decrease();
  cpp << "}" << endl; // ctor
  cpp << R"(
GlobalFunctions::~GlobalFunctions() {
#if defined(_WIN32)
  FreeLibrary(library_);
#elif defined(__linux__)
  dlclose(library_);
#else
#error "Unsupported OS"
#endif
}
)" << endl;

  print_func_wrapper_cpp(cpp, ind_cpp, _entry_command, "GlobalFunctions", "");
  for (auto c : _global_commands) {
    print_func_wrapper_cpp(cpp, ind_cpp, c, "GlobalFunctions", "");
  }
}

void CppDispatchTableGenerator::gen_instance_command(vkspec::Command* c) {
  preprocess_command(c);
}

void CppDispatchTableGenerator::gen_device_command(vkspec::Command* c) {
  preprocess_command(c);
}

void CppDispatchTableGenerator::end_extensions() {
  for (auto& table : tables_) {
    header << endl;
    assert(table.dispatchable_object.substr(0, 2) == "Vk");
    string class_name = table.dispatchable_object.substr(2) + "Functions";
    header << "class " << class_name << " {" << endl;
    header << "public:" << endl;

    ind_h->increase();
    header << table.dispatchable_object << " " << table.dispatchable_object_snake_case << "() const { return " << table.dispatchable_object_snake_case << "_; }" << endl;

    // Special case: add vkGetInstanceProcAddr for convenience
    if (table.dispatchable_object == "VkInstance") {
      print_func_wrapper_h(header, ind_h, _entry_command, true);
    }
    // Special case: add vkGetDeviceProcAddr manually since it's otherwise
    // treated as an instance level function.
    else if (table.dispatchable_object == "VkDevice") {
      print_func_wrapper_h(header, ind_h, get_device_proc_, true);
    }

    for (auto c : table.commands) {
      print_func_wrapper_h(header, ind_h, c, true);
    }
    ind_h->decrease();

    header << endl;
    header << "protected:" << endl;
    ind_h->increase();
    header << class_name << "(";

    if (table.dispatchable_object == "VkInstance") {
      header << table.dispatchable_object << " " << table.dispatchable_object_snake_case << ", GlobalFunctions* globals";
    } else if (table.dispatchable_object == "VkDevice") {
      header << table.dispatchable_object << " " << table.dispatchable_object_snake_case << ", InstanceFunctions* instance";
    } else if (table.classification == vkspec::CommandClassification::Instance) {
      header << table.dispatchable_object << " " << table.dispatchable_object_snake_case << ", InstanceFunctions* instance";
    } else {
      assert(table.classification == vkspec::CommandClassification::Device);
      header << table.dispatchable_object << " " << table.dispatchable_object_snake_case << ", DeviceFunctions* device";
    }

    header << ");" << endl;
    ind_h->decrease();

    header << endl;
    header << "private:" << endl;
    ind_h->increase();
    header << table.dispatchable_object << " " << table.dispatchable_object_snake_case << "_ = VK_NULL_HANDLE;" << endl;
    
    // Special case: add vkGetInstanceProcAddr for convenience
    if (table.dispatchable_object == "VkInstance") {
      print_func_member(header, ind_h, _entry_command);
    }
    // Special case: add vkGetDeviceProcAddr manually since it's otherwise
    // treated as an instance level function.
    else if (table.dispatchable_object == "VkDevice") {
      print_func_member(header, ind_h, get_device_proc_);
    }

    for (auto c : table.commands) {
      print_func_member(header, ind_h, c);
    }
    ind_h->decrease();

    header << "};" << endl;

    cpp << "/*" << endl;
    cpp << " * ------------------------------------------------------" << endl;
    cpp << " * " << class_name << endl;
    cpp << " * ------------------------------------------------------" << endl;
    cpp << "*/" << endl;
    cpp << endl;

    // Special case: add vkGetInstanceProcAddr for convenience
    if (table.dispatchable_object == "VkInstance") {
      print_func_wrapper_cpp(cpp, ind_cpp, _entry_command, class_name, table.dispatchable_object_snake_case + "_");
    }
    // Special case: add vkGetDeviceProcAddr manually since it's otherwise
    // treated as an instance level function.
    else if (table.dispatchable_object == "VkDevice") {
      print_func_wrapper_cpp(cpp, ind_cpp, get_device_proc_, class_name, table.dispatchable_object_snake_case + "_");
    }

    for (auto c : table.commands) {
      print_func_wrapper_cpp(cpp, ind_cpp, c, class_name, table.dispatchable_object_snake_case + "_");
    }

    cpp << class_name << "::" << class_name << "(";

    if (table.dispatchable_object == "VkInstance") {
      cpp << table.dispatchable_object << " " << table.dispatchable_object_snake_case << ", GlobalFunctions* globals";
    } else if (table.dispatchable_object == "VkDevice") {
      cpp << table.dispatchable_object << " " << table.dispatchable_object_snake_case << ", InstanceFunctions* instance";
    } else if (table.classification == vkspec::CommandClassification::Instance) {
      cpp << table.dispatchable_object << " " << table.dispatchable_object_snake_case << ", InstanceFunctions* instance";
    } else {
      assert(table.classification == vkspec::CommandClassification::Device);
      cpp << table.dispatchable_object << " " << table.dispatchable_object_snake_case << ", DeviceFunctions* device";
    }

    cpp << ") {" << endl;

    ind_cpp->increase();

    cpp << table.dispatchable_object_snake_case << "_ = " << table.dispatchable_object_snake_case << ";" << endl;

    if (table.dispatchable_object == "VkInstance") {
      print_load_instance_proc(cpp, ind_cpp, "globals", "instance", _entry_command);
      for (auto c : table.commands) {
        print_load_instance_proc(cpp, ind_cpp, "this", nullptr, c);
      }
    }
    else if (table.dispatchable_object == "VkDevice") {
      print_load_instance_proc(cpp, ind_cpp, "instance", nullptr, get_device_proc_);
      for (auto c : table.commands) {
        print_load_device_proc(cpp, ind_cpp, "this", c);
      }
    }
    else if (table.classification == vkspec::CommandClassification::Instance) {
      for (auto c : table.commands) {
        print_load_instance_proc(cpp, ind_cpp, "instance", nullptr, c);
      }
    }
    else {
      assert(table.classification == vkspec::CommandClassification::Device);
      for (auto c : table.commands) {
        print_load_device_proc(cpp, ind_cpp, "device", c);
      }
    }

    ind_cpp->decrease();

    cpp << "}" << endl;
    cpp << endl;
  }
}

void CppDispatchTableGenerator::end_extension(vkspec::Extension* e) {
  for (auto c : e->commands()) {
    preprocess_command(c);
  }
}

void CppDispatchTableGenerator::preprocess_command(vkspec::Command* c) {
  // Special case dealt with in DeviceFunctions.
  if (c->name() == "vkGetDeviceProcAddr") {
    get_device_proc_ = c;
    return;
  }

  // The first parameter of a command should always be a dispatchable handle
  auto dispatchable = c->params().front().pure_type->to_handle_typedef();
  assert(dispatchable && dispatchable->dispatchable());

  auto table = find_if(tables_.begin(), tables_.end(), [dispatchable](DispatchTable& t) -> bool {
    return t.dispatchable_object == dispatchable->name();
  });

  if (table == tables_.end()) {
    DispatchTable t;
    t.dispatchable_object = dispatchable->name();
    assert(t.dispatchable_object.substr(0, 2) == "Vk");
    string snake_case = t.dispatchable_object.substr(2); // Skip Vk
    snake_case[0] = tolower(snake_case[0]);
    while (true) {
      auto it = std::find_if( snake_case.begin(), snake_case.end(),
          [](char c) { return isupper(c); });
      if (it == snake_case.end()) {
        break;
      }
      *it = tolower(*it);
      snake_case.insert(it, '_');
    }
    t.dispatchable_object_snake_case = snake_case;
    t.classification = c->classification();
    t.commands.push_back(c);
    tables_.push_back(t);
  } else {
    assert(table->classification == c->classification());
    table->commands.push_back(c);
  }
}
