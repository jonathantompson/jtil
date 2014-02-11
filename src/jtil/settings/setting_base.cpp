#include <string>
#include "jtil/file_io/csv_handle.h"  // For flattenToken
#include "jtil/settings/setting_base.h"
#include "jtil/settings/setting.h"
#include "jtil/settings/settings_manager.h"
#include "jtil/exceptions/wruntime_error.h"

using std::string;
using std::wruntime_error;

namespace jtil {

using file_io::CSVHandle;
using file_io::CSVHandleWrite;

namespace settings {

  // Format in csv file: 
  // Name, Type, Value
  // Type: string, string, int, bool, Float3, Float4
  // Value: bool must be 0 or 1
  SettingBase* SettingBase::parseToken(
    data_str::VectorManaged<const char*>& cur_token, 
    const string& filename) {
    uint32_t token_size = cur_token.size();
    if (token_size < SETTINGS_SETTING_NUM_TOKENS ||
      token_size > SETTINGS_SETTING_NUM_TOKENS + 3) {  // vector4
        string error = string("SettingBase::parseToken "
          "- Incorrect # tokens: '") + 
          file_io::CSVHandle::flattenToken(cur_token) + 
          string("' in file: ") + filename;
        throw wruntime_error(error);
    }

    SettingBase* new_obj = NULL;
    string type(cur_token[1]);
    string name(cur_token[0]);
    // Switch statement here would be ideal, but using char* in switch is 
    // messy (must be const case values)
    if (type == string("bool")) {
      new_obj = new Setting<bool>();
      reinterpret_cast<Setting<bool>*>(new_obj)->val(
        string_util::Str2Num<int>(string(cur_token[2])) == 1);
    } else if (type == string("int")) {
      new_obj = new Setting<int>();
      reinterpret_cast<Setting<int>*>(new_obj)->val(
         string_util::Str2Num<int>(string(cur_token[2])));
    } else if (type == string("string")) {
      new_obj = new Setting<std::string>();
      reinterpret_cast<Setting<string>*>(new_obj)->val(cur_token[2]);
    } else if (type == string("float")) {
      new_obj = new Setting<float>();
      reinterpret_cast<Setting<float>*>(new_obj)->val(
        string_util::Str2Num<float>(string(cur_token[2])));
    } else if (type == string("Float2")) {
      new_obj = new Setting<math::Float2>();
      reinterpret_cast<Setting<math::Float2>*>(new_obj)->val().set(
        string_util::Str2Num<float>(string(cur_token[2])), 
        string_util::Str2Num<float>(string(cur_token[3])));
    } else if (type == string("Float3")) {
      new_obj = new Setting<math::Float3>();
      reinterpret_cast<Setting<math::Float3>*>(new_obj)->val().set(
        string_util::Str2Num<float>(string(cur_token[2])), 
        string_util::Str2Num<float>(string(cur_token[3])), 
        string_util::Str2Num<float>(string(cur_token[4])));
    } else if (type == string("Float4")) {
      new_obj = new Setting<math::Float4>();
      reinterpret_cast<Setting<math::Float4>*>(new_obj)->val().set(
        string_util::Str2Num<float>(string(cur_token[2])), 
        string_util::Str2Num<float>(string(cur_token[3])), 
        string_util::Str2Num<float>(string(cur_token[4])), 
        string_util::Str2Num<float>(string(cur_token[5])));
    } else if (type == string("Int2")) {
      new_obj = new Setting<math::Int2>();
      reinterpret_cast<Setting<math::Int2>*>(new_obj)->val().set(
        string_util::Str2Num<int>(string(cur_token[2])), 
        string_util::Str2Num<int>(string(cur_token[3])));
    } else if (type == string("Int3")) {
      new_obj = new Setting<math::Int3>();
      reinterpret_cast<Setting<math::Int3>*>(new_obj)->val().set(
        string_util::Str2Num<int>(string(cur_token[2])), 
        string_util::Str2Num<int>(string(cur_token[3])), 
        string_util::Str2Num<int>(string(cur_token[4])));
    } else if (type == string("Int4")) {
      new_obj = new Setting<math::Int4>();
      reinterpret_cast<Setting<math::Int4>*>(new_obj)->val().set(
        string_util::Str2Num<int>(string(cur_token[2])), 
        string_util::Str2Num<int>(string(cur_token[3])), 
        string_util::Str2Num<int>(string(cur_token[4])), 
        string_util::Str2Num<int>(string(cur_token[5])));
    } else if (type == string("FloatQuat")) {
      new_obj = new Setting<math::FloatQuat>();
      reinterpret_cast<Setting<math::FloatQuat>*>(new_obj)->val().set(
      string_util::Str2Num<float>(string(cur_token[2])), 
      string_util::Str2Num<float>(string(cur_token[3])), 
      string_util::Str2Num<float>(string(cur_token[4])), 
      string_util::Str2Num<float>(string(cur_token[5])));
    } else {
      string error = string("SettingBase::parseToken "
        "- Unrecognized setting type: '") + 
        file_io::CSVHandle::flattenToken(cur_token) + 
        string("' in file: ") + filename;
      throw wruntime_error(error);
    }
    new_obj->name(name);

    return new_obj;
  }

  string ExtractName(const data_str::VectorManaged<const char*>& cur_token) {
     // Seperate out the whitespace and get the raw name
    data_str::VectorManaged<const char*> cur_token_header;
    CSVHandle::seperateWhiteSpace(cur_token[0], cur_token_header);
    return string(*cur_token_header.at(1));
  }

  void SettingBase::writeToken(
    data_str::VectorManaged<const char*>& cur_token, 
    CSVHandleWrite& writer, const string& filename) {
    string element_header = ExtractName(cur_token);

    if (element_header == string("//") || element_header == string("")) {
      // Write the comment to file unedited
      writer.writeToken(cur_token);
      return;
    }

    uint32_t token_size = cur_token.size();
    if (token_size < SETTINGS_SETTING_NUM_TOKENS ||
      token_size > SETTINGS_SETTING_NUM_TOKENS + 3) {
        string error = string("Setting<T>::formatToken "
          "- Incorrect # tokens: '") + 
          CSVHandle::flattenToken(cur_token) + string("' in file: ") +
          filename;
        throw wruntime_error(error);
    }

    // Otherwise, find the setting matching this token -> Reformat and 
    // overwrite the data section and write it to disk
    data_str::VectorManaged<const char*> cur_token_type;
    file_io::CSVHandle::seperateWhiteSpace(cur_token[1], cur_token_type);
    string type(*cur_token_type.at(1));

    if (type == string("bool")) {
      formatTokenBool(cur_token, writer, filename);
    } else if (type == string("int")) {
      formatTokenInt(cur_token, writer, filename);
    } else if (type == string("string")) {
      formatTokenString(cur_token, writer, filename);
    } else if (type == string("float")) {
      formatTokenFloat(cur_token, writer, filename);
    } else if (type == string("Float2")) {
      formatTokenFloat2(cur_token, writer, filename);
    } else if (type == string("Float3")) {
      formatTokenFloat3(cur_token, writer, filename);
    } else if (type == string("Float4")) {
      formatTokenFloat4(cur_token, writer, filename);
    } else if (type == string("Int2")) {
      formatTokenInt2(cur_token, writer, filename);
    } else if (type == string("Int3")) {
      formatTokenInt3(cur_token, writer, filename);
    } else if (type == string("Int4")) {
      formatTokenInt4(cur_token, writer, filename);
    } else if (type == string("FloatQuat")) {
      formatTokenFloat4(cur_token, writer, filename);
    } else {
      string error = string("Setting<T>::formatToken "
        "- Unrecognized setting type: '") + 
        CSVHandle::flattenToken(cur_token) + string("' in file: ") + filename;
      throw wruntime_error(error);
    }

    // Write the line to file
    writer.writeToken(cur_token);
    writer.flush();
  }

  // FormatInt is shared by a few formatToken functions
  void SettingBase::formatInt(const int data, 
    data_str::VectorManaged<const char*>& cur_token, 
    const uint32_t data_elem_index) {
    data_str::VectorManaged<const char*> cur_token_data;
    file_io::CSVHandle::seperateWhiteSpace(cur_token[data_elem_index], 
      cur_token_data);
    char * data_str = NULL;

    int new_strlen = 0;
    if (data != 0) {
      int data_prime = data;
      if (data < 0) { data_prime = -data_prime; }
      while (data_prime != 0) { 
        data_prime /= 10; 
        new_strlen++; 
      }
    } else {
      new_strlen = 1;
    }
    if (data < 0) {
      new_strlen += 1;  // Account for '-' character
    }
    data_str = new char[new_strlen + 1];  // Account for '\0' character
    snprintf(data_str, new_strlen+1, "%d", data);

    // Overwrite the Data element
    cur_token_data.deleteAt(1);
    cur_token_data.set(1, data_str);
    char* cur_token_data_str = CSVHandle::joinWhiteSpace(cur_token_data);
    cur_token.deleteAt(data_elem_index);
    cur_token.set(data_elem_index, cur_token_data_str);
  }

  void SettingBase::formatTokenInt(
    data_str::VectorManaged<const char*>& cur_token, CSVHandleWrite& writer, 
    const string& filename) {
    static_cast<void>(filename);
    static_cast<void>(writer);
    string name = ExtractName(cur_token);

    int data; 
    if (!SettingsManager::getSetting<int>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenInt() - Can't "
        "find setting ") + name);
    }
    formatInt(data, cur_token, 2);
  }

  void SettingBase::formatTokenBool(
    data_str::VectorManaged<const char*>& cur_token, CSVHandleWrite& writer, 
    const string& filename) {
    static_cast<void>(writer);
    static_cast<void>(filename);
    string name = ExtractName(cur_token);

    data_str::VectorManaged<const char*> cur_token_data;
    file_io::CSVHandle::seperateWhiteSpace(cur_token[2], cur_token_data);
    char * data_str = new char[2];

    bool data; 
    if (!SettingsManager::getSetting<bool>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenBool() - Can't "
        "find setting ") + name);
    }

    if (data == true) {
      data_str[0] = '1';
    } else {
      data_str[0] = '0';
    }
    data_str[1] = '\0';

    // Overwrite the Data element
    cur_token_data.deleteAt(1);
    cur_token_data.set(1, data_str);
    char * cur_token_data_str = CSVHandle::joinWhiteSpace(cur_token_data);
    cur_token.deleteAt(2);
    cur_token.set(2, cur_token_data_str);
  }

  // Format float is shared by the next few formatToken functions
  void SettingBase::formatFloat(const float data, 
    data_str::VectorManaged<const char*>& cur_token, 
    const uint32_t data_elem_index) {
    data_str::VectorManaged<const char*> cur_token_data;
    file_io::CSVHandle::seperateWhiteSpace(cur_token[data_elem_index], 
      cur_token_data);
    char * data_str = NULL;

    int new_strlen = 0;
    if ((data >= 1) || (data <= -1)) {
      new_strlen = static_cast<int>(ceil(log10(EPSILON + 
        abs(static_cast<double>(data)))));
    } else {
      new_strlen = 1;  // ie 0.xxxxxxx
    }

    // Make the output precision the same as the old precision
    int prec = file_io::CSVHandle::getDoublePrecision(*cur_token_data.at(1));
    new_strlen += prec + 1;  // Account for '.' character
    if (data < 0) {
      new_strlen += 1;  // Account for '-' character
    }

    data_str = new char[new_strlen + 1];  // Account for '\0' character
    snprintf(data_str, new_strlen+1, "%.*f", prec, data);

    // Overwrite the Data element
    cur_token_data.deleteAt(1);
    cur_token_data.set(1, data_str);
    char* cur_tok_str = file_io::CSVHandle::joinWhiteSpace(cur_token_data);
    cur_token.deleteAt(data_elem_index);
    cur_token.set(data_elem_index, cur_tok_str);
  }

  void SettingBase::formatTokenFloat(
    data_str::VectorManaged<const char*>& cur_token, 
    CSVHandleWrite& writer, const string& filename) {
    static_cast<void>(writer);
    static_cast<void>(filename);
    string name = ExtractName(cur_token);

    float data; 
    if (!SettingsManager::getSetting<float>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenFloat() - Can't "
        "find setting ") + name);
    }
    formatFloat(data, cur_token, 2);
  }

  void SettingBase::formatTokenFloat2(
    data_str::VectorManaged<const char*>& cur_token,
    CSVHandleWrite& writer, const string& filename) {
    static_cast<void>(writer);
    static_cast<void>(filename);
    string name = ExtractName(cur_token);

    math::Float2 data; 
    if (!SettingsManager::getSetting<math::Float2>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenFloat2() - Can't "
        "find setting ") + name);
    }
    // Process the X-axis data
    formatFloat(data.m[0], cur_token, 2);

    // Process the Y-axis data
    formatFloat(data.m[1], cur_token, 3);
  }

  void SettingBase::formatTokenFloat3(
    data_str::VectorManaged<const char*>& cur_token,
    CSVHandleWrite& writer, const string& filename) {
    static_cast<void>(writer);
    static_cast<void>(filename);
    string name = ExtractName(cur_token);

    math::Float3 data; 
    if (!SettingsManager::getSetting<math::Float3>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenFloat3() - Can't "
        "find setting ") + name);
    }
    // Process the X-axis data
    formatFloat(data.m[0], cur_token, 2);

    // Process the Y-axis data
    formatFloat(data.m[1], cur_token, 3);

    // Process the Z-axis data
    formatFloat(data.m[2], cur_token, 4);
  }

  void SettingBase::formatTokenFloat4(
    data_str::VectorManaged<const char*>& cur_token,
    CSVHandleWrite& writer, const string& filename) {
    static_cast<void>(writer);
    static_cast<void>(filename);
    string name = ExtractName(cur_token);

    math::Float4 data; 
    if (!SettingsManager::getSetting<math::Float4>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenFloat4() - Can't "
        "find setting ") + name);
    }
    // Process the X-axis data
    formatFloat(data.m[0], cur_token, 2);

    // Process the Y-axis data
    formatFloat(data.m[1], cur_token, 3);

    // Process the Z-axis data
    formatFloat(data.m[2], cur_token, 4);

    // Process the W-axis data
    formatFloat(data.m[3], cur_token, 5);
  }

  void SettingBase::formatTokenInt2(
    data_str::VectorManaged<const char*>& cur_token,
    CSVHandleWrite& writer, const string& filename) {
    static_cast<void>(writer);
    static_cast<void>(filename);
    string name = ExtractName(cur_token);

    math::Int2 data; 
    if (!SettingsManager::getSetting<math::Int2>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenInt2() - Can't "
        "find setting ") + name);
    }
    // Process the X-axis data
    formatInt(data.m[0], cur_token, 2);

    // Process the Y-axis data
    formatInt(data.m[1], cur_token, 3);
  }

  void SettingBase::formatTokenInt3(
    data_str::VectorManaged<const char*>& cur_token,
    CSVHandleWrite& writer, const string& filename) {
    static_cast<void>(writer);
    static_cast<void>(filename);
    string name = ExtractName(cur_token);

    math::Int3 data; 
    if (!SettingsManager::getSetting<math::Int3>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenFloat3() - Can't "
        "find setting ") + name);
    }
    // Process the X-axis data
    formatInt(data.m[0], cur_token, 2);

    // Process the Y-axis data
    formatInt(data.m[1], cur_token, 3);

    // Process the Z-axis data
    formatInt(data.m[2], cur_token, 4);
  }

  void SettingBase::formatTokenInt4(
    data_str::VectorManaged<const char*>& cur_token,
    CSVHandleWrite& writer, const string& filename) {
    static_cast<void>(writer);
    static_cast<void>(filename);
    string name = ExtractName(cur_token);

    math::Int4 data; 
    if (!SettingsManager::getSetting<math::Int4>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenInt4() - Can't "
        "find setting ") + name);
    }
    // Process the X-axis data
    formatInt(data.m[0], cur_token, 2);

    // Process the Y-axis data
    formatInt(data.m[1], cur_token, 3);

    // Process the Z-axis data
    formatInt(data.m[2], cur_token, 4);

    // Process the W-axis data
    formatInt(data.m[3], cur_token, 5);
  }

  void SettingBase::formatTokenString(
    data_str::VectorManaged<const char*>& cur_token,
    CSVHandleWrite& writer, const string& filename) {
    static_cast<void>(writer);
    static_cast<void>(filename);
    string name = ExtractName(cur_token);

    std::string data; 
    if (!SettingsManager::getSetting<std::string>(name, data)) {
      throw wruntime_error(string("SettingBase::formatTokenString() - Can't"
        "find setting ") + name);
    }

    data_str::VectorManaged<const char*> cur_token_data;
    file_io::CSVHandle::seperateWhiteSpace(cur_token[2], cur_token_data);
    char * data_str = NULL;

    data_str = new char[data.length() + 1];
    snprintf(data_str, data.size()+1, "%s", data.c_str());

    // Overwrite the Data element
    cur_token_data.deleteAt(1);
    cur_token_data.set(1, data_str);
    char * cur_token_data_str = CSVHandle::joinWhiteSpace(cur_token_data);
    cur_token.deleteAt(2);
    cur_token.set(2, cur_token_data_str);
  }

}  // namespace settings
}  // namespace jtil
