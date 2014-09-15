#include <sys\stat.h>
#include <sstream>
#include <string>
#include "jtil/file_io/file_io.h"
#include "jtil/data_str/vector_managed.h"
#include "jtil/math/math_types.h"  // for uint

namespace jtil {
namespace file_io {
  bool fileExists(const std::string& filename) {
    // TODO: This is a pretty stupid way to check if a file exists.  I think
    // opening a file handler is probably slow.  Rethink this.
    std::ifstream file(filename.c_str(), std::ios::in | std::ios::binary);
    bool ret_val = false;
    if (file.is_open()) {
      ret_val = true;
      file.close();
    }
    return ret_val;
  }

  PathType getPathType(const std::string& path) {
    struct stat s;
    if (stat(path.c_str(), &s) == 0) {
      if (s.st_mode & S_IFDIR) {
        return DIRECTORY_PATH;
      }
      else if(s.st_mode & S_IFREG) {
        return FILE_PATH;
      } else {
        return UNKNOWN_PATH;  // It's something else
      }
    } else {
      return UNKNOWN_PATH;  // Something went wrong
    }
  }

    void ls(const std::string& path, jtil::data_str::VectorManaged<char*>& files) {
#ifdef WIN32
    // Clear the directory of existing saved frames
    WIN32_FIND_DATAW file_data;
    std::wstring wpath = jtil::string_util::ToWideString(path);
    HANDLE hFind = FindFirstFile(wpath.c_str(), &file_data);
    while (hFind != INVALID_HANDLE_VALUE) {
      std::string filename = 
        jtil::string_util::ToNarrowString(file_data.cFileName);
      char* filename_c_str = new char[filename.length()+1];
      strcpy(filename_c_str, filename.c_str());

      files.pushBack(filename_c_str);
      if (!FindNextFile(hFind, &file_data)) {
        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
      }
    }
#else
    throw std::runtime_error("Not yet implemented for non Windows OS");
#endif
  }

}  // namespace file_io
}  // namespace jtil
