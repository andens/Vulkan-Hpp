#ifndef CPP_DISPATCH_TABLES_INCLUDE
#define CPP_DISPATCH_TABLES_INCLUDE

#include "vkspec.h"
#include "indenting_stream_buf.h"
#include <fstream>

class CppDispatchTableGenerator : public vkspec::IGenerator {
public:
  CppDispatchTableGenerator(std::string const& out_dir, std::string const& license, int major, int minor, int patch);
  ~CppDispatchTableGenerator();

  virtual void begin_core() override final {}
  virtual void end_core() override final {}
  virtual void gen_scalar_typedef(vkspec::ScalarTypedef* t) override final {}
  virtual void gen_function_typedef(vkspec::FunctionTypedef* t) override final {}
  virtual void gen_handle_typedef(vkspec::HandleTypedef* t) override final {}
  virtual void gen_struct(vkspec::Struct* t) override final {}
  virtual void gen_enum(vkspec::Enum* t) override final {}
  virtual void gen_api_constant(vkspec::ApiConstant* t) override final {}
  virtual void gen_bitmasks(vkspec::Bitmasks* t) override final {}
  virtual void begin_entry() override final {}
  virtual void gen_entry_command(vkspec::Command* c) override final;
  virtual void end_entry() override final {}
  virtual void begin_global_commands() override final {}
  virtual void gen_global_command(vkspec::Command* c) override final;
  virtual void end_global_commands() override final;
  virtual void begin_instance_commands() override final {}
  virtual void gen_instance_command(vkspec::Command* c) override final;
  virtual void end_instance_commands() override final;
  virtual void begin_device_commands() override final {}
  virtual void gen_device_command(vkspec::Command* c) override final {}
  virtual void end_device_commands() override final {}
  virtual void begin_extensions() override final {}
  virtual void end_extensions() override final {}
  virtual void begin_extension(vkspec::Extension* e) override final {}
  virtual void end_extension(vkspec::Extension* e) override final {}

private:
  std::ofstream header;
  std::ofstream cpp;
  IndentingOStreambuf* ind_h = nullptr;
  IndentingOStreambuf* ind_cpp = nullptr;
  vkspec::Command* _entry_command = nullptr;
  std::vector<vkspec::Command*> _global_commands;
  std::vector<vkspec::Command*> _instance_commands;
};

class CppTranslator : public vkspec::ITranslator {
  virtual std::string translate_c(std::string const& c) override final {
    return c;
  }

  virtual std::string pointer_to(vkspec::Type* type, vkspec::PointerType pointer_type) override final {
    std::string t = type->name();
    switch (pointer_type) {
      case vkspec::PointerType::CONST_T_P:
        return "const " + t + "*";
      case vkspec::PointerType::CONST_T_PP:
        return "const " + t + "**";
      case vkspec::PointerType::CONST_T_P_CONST_P:
        return "const " + t + "* const*";
      case vkspec::PointerType::T_P:
        return t + "*";
      case vkspec::PointerType::T_PP:
        return t + "**";
      case vkspec::PointerType::T_P_CONST_P:
        return t + "* const*";
      default:
        assert(false);
        return "";
    }
  }

  virtual std::string array_member(std::string const& type_name, std::string const& array_size) override final {
    return ""; // Not implemented
  }

  virtual std::string array_param(std::string const& type_name, std::string const& array_size, bool const_modifier) override final {
    return ""; // Not implemented
  }

  virtual std::string bitwise_not(std::string const& value) {
    return ""; // Not implemented
  }
};

#endif