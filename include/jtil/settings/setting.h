//
//  setting.h
//
//  Created by Jonathan Tompson on 5/10/12.
//

#pragma once

#include <string>
#include "jtil/file_io/csv_handle_write.h"
#include "jtil/math/math_types.h"  // for Float3 and Float4
#include "jtil/data_str/vector_managed.h"
#include "jtil/string_util/string_util.h"
#include "jtil/settings/setting_base.h"

#define SETTINGS_SETTING_NUM_TOKENS 3 

#if _WIN32
  #ifndef snprintf
    #define snprintf _snprintf
  #endif
#endif

namespace jtil {
namespace settings {

  template <class T>
  class Setting : public SettingBase {
  public:
    Setting() { }
    virtual ~Setting() { }
    Setting& operator=(const Setting& o);  // Assignment operator

    inline void val(const T& val) { val_ = val; }
    inline T& val() { return val_; }

    static Setting* parseToken(data_str::VectorManaged<const char*>& cur_token,
      const std::string& filename);
    static void writeToken(data_str::VectorManaged<const char*>& cur_token,
      file_io::CSVHandleWrite& writer, const std::string& filename);

  private:
    T val_;  // Variable with which this UI element is attached
    // std::string name_;
  };

  template <typename T>
  Setting<T> & Setting<T>::operator=(const Setting<T> & o) {
    if (this != &o) {  // make sure not same object
      val_ = o.val_; 
      name_ = o.name_;
    }
    return *this;  // Return ref for multiple assignment
  };

};  // namespace settings
};  // namespace jtil
