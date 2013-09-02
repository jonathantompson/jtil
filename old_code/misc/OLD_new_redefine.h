//
//  new_redefine.h
//
//  Created by Jonathan Tompson on 5/1/12.
//

#if defined(_DEBUG) && defined(_WIN32)
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC  // Give report on memory leaks
#endif
#ifndef _CRTDBG_MAPALLOC
#define _CRTDBG_MAPALLOC  // Give report on memory leaks
#endif
#include <stdlib.h>
#include <crtdbg.h>

#ifndef DEBUG_NEW
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif
#endif
