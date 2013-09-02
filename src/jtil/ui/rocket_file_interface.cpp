#include "jtil/ui/rocket_file_interface.h"
#include <stdio.h>

using Rocket::Core::String;

namespace jtil {
namespace ui {

  RocketFileInterface::RocketFileInterface(const std::string& root) : 
    root(root.c_str()) {
  }

  RocketFileInterface::~RocketFileInterface() {
  }

  // Opens a file.
  Rocket::Core::FileHandle RocketFileInterface::Open(const String& path) {
    // Attempt to open the file relative to the application's root.
    FILE* fp = fopen((root + path).CString(), "rb");
    if (fp != NULL)
      return (Rocket::Core::FileHandle) fp;

    // Attempt to open the file relative to the current working directory.
    fp = fopen(path.CString(), "rb");
    return (Rocket::Core::FileHandle) fp;
  }

  // Closes a previously opened file.
  void RocketFileInterface::Close(Rocket::Core::FileHandle file) {
    fclose((FILE*) file);
  }

  // Reads data from a previously opened file.
  size_t RocketFileInterface::Read(void* buffer, size_t size, 
    Rocket::Core::FileHandle file) {
    return fread(buffer, 1, size, (FILE*) file);
  }

  // Seeks to a point in a previously opened file.
  bool RocketFileInterface::Seek(Rocket::Core::FileHandle file, 
    long offset, int origin) {
    return fseek((FILE*) file, offset, origin) == 0;
  }

  // Returns the current position of the file pointer.
  size_t RocketFileInterface::Tell(Rocket::Core::FileHandle file) {
    return ftell((FILE*) file);
  }

}  // namespace ui
}  // namespace jtil