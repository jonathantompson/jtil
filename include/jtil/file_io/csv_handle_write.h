//
//  csv_handle_write.h
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

  class CSVHandleWrite : public CSVHandle {
  public:
    explicit CSVHandleWrite(const std::string& filename);
    ~CSVHandleWrite();

    // Access and Modifier Functions
    void close();
    bool readNextToken(data_str::VectorManaged<const char*>& token_array);
    inline bool checkEOF() const { return writer_.eof(); }
    void writeLine(const char* line);
    void writeToken(const data_str::VectorManaged<const char*>& token_array);
    void flush();

  private:
    std::ofstream writer_;

    // Non-copyable, non-assignable.
    CSVHandleWrite(CSVHandleWrite&);
    CSVHandleWrite& operator=(const CSVHandleWrite&);
  };

};  // namespace file_io
};  // namespace jtil
