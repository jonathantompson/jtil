#include "dxInclude.h"
#include <windows.h>
#include <math.h>
#include "renderer\utils\d3dProfiler.h"
#include <strsafe.h>

#include <time.h>

// Class constructor. Takes the necessary information and
// composes a string that will appear in PIXfW.
d3dProfiler::d3dProfiler( WCHAR* Name, int Line )
{
	WCHAR wc[ MAX_PATH ];
	StringCchPrintf( wc, MAX_PATH, L"%s @ Line %d.\0", Name, Line );
	srand( static_cast< unsigned >( time( NULL ) ) );
	D3DPERF_BeginEvent( D3DCOLOR_XRGB( rand() % 255, rand() % 255, rand() % 255 ), wc );
	
}

// Makes sure that the BeginEvent() has a matching EndEvent()
// if used via the macro in D3DUtils.h this will be called when
// the variable goes out of scope.
d3dProfiler::~d3dProfiler( )
{
	D3DPERF_EndEvent( );
}