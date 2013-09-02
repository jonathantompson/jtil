#include <wchar.h>
#include "jtil/windowing/window.h"
#include "jtil/string_util/string_util.h"

namespace jtil {
namespace windowing {

  void NativeErrorBox(const wchar_t* str) {
    wprintf(L"WARNING: NativeErrorBox not yet implemented for the Mac OS");
    wprintf(L" X platform.\n");
  } 

}  // namespace windowing
}  // namespace jtil
