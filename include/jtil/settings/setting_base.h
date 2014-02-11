//
//  setting_base.h
//
//  Created by Jonathan Tompson on 5/10/12.
//
//  An base class so that we can have an array of Setting pointers 
//  (settings/setting.h) without specifying template types.  Also holds all the
//  static functoins for processing tokens to and from file.
//  

#pragma once

#include <string>
#include "jtil/math/math_types.h"  // for uint32_t

namespace jtil {
namespace file_io { class CSVHandleWrite; }

namespace settings {

  struct SettingBase {
  public:
    SettingBase() { }
    virtual ~SettingBase() { }

    static SettingBase* parseToken(
      data_str::VectorManaged<const char*>& cur_token, 
      const std::string& filename);

    static void writeToken(data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);

    void name(const std::string& name) { name_ = name; }
    std::string& name() { return name_; }
    std::string name() const { return name_; }
    
  protected:
    std::string name_;

    static void formatTokenInt(data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);
    static void formatTokenBool(
      data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);
    static void formatTokenFloat(
      data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);
    static void formatTokenFloat2(
      data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);
    static void formatTokenFloat3(
      data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);
    static void formatTokenFloat4(
      data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);
    static void formatTokenInt2(
      data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);
    static void formatTokenInt3(
      data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);
    static void formatTokenInt4(
      data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);
    static void formatTokenString(
      data_str::VectorManaged<const char*>& cur_token, 
      file_io::CSVHandleWrite& writer, const std::string& filename);


    static void formatFloat(const float data, 
      data_str::VectorManaged<const char*>& cur_token, 
      const uint32_t data_elem_index);
    static void formatInt(const int data, 
      data_str::VectorManaged<const char*>& cur_token, 
      const uint32_t data_elem_index);
  };

};  // namespace settings
};  // namespace jtil
