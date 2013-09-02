//
//  csv_handle_read.h
//
//  Created by Jonathan Tompson on 5/1/12.
//

#pragma once

#include <string>
#include <fstream>
#include "jtil/file_io/csv_handle.h"

namespace jtil {
namespace data_str { template <typename T> class VectorManaged; }

namespace file_io {

  class CSVHandleRead : public CSVHandle {
  public:
    explicit CSVHandleRead(const std::string& filename);
    ~CSVHandleRead();

    void close();
    bool readNextToken(data_str::VectorManaged<const char*>& token_array, 
      const bool include_whitespace);
    inline bool checkEOF() const { return reader_.eof(); }

  private:
    std::ifstream reader_;

    // Non-copyable, non-assignable.
    CSVHandleRead(CSVHandleRead&);
    CSVHandleRead& operator=(const CSVHandleRead&);
  };

};  // namespace file_io
};  // namespace jtil
