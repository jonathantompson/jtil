//
//  csv_handle.h
//
//  Created by Jonathan Tompson on 5/1/12.
//

#pragma once

#include <string>

namespace jtil {

namespace data_str { template <typename T> class VectorManaged; }

namespace file_io {
  class CSVHandle { 
  public:
    CSVHandle();

    inline bool open() const { return open_; }
    inline std::string& filename() { return filename_; }

    // flattenToken - used to generate error strings from the Vector token
    static std::string flattenToken(data_str::VectorManaged<const char*>& cur_token);

    // public since they're used by ui_elem_setting.h
    static void seperateWhiteSpace(const char* cur_token, 
      data_str::VectorManaged<const char*>& return_token);
    static char* joinWhiteSpace(data_str::VectorManaged<const char*>& cur_token);
    static int getDoublePrecision(const char* num);

  protected:
    std::string filename_;
    bool open_;

    // Non-copyable, non-assignable.
    CSVHandle(CSVHandle&);
    CSVHandle& operator=(const CSVHandle&);
  };

};  // namespace file_io
};  // namespace jtil
