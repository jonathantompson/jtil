
#ifndef d3dProfiler_h
#define d3dProfiler_h

// These first two macros are taken from the
// VStudio help files - necessary to convert the
// __FUNCTION__ symbol from char to wchar_t.
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)

// Only the first of these macro's should be used. The _INTERNAL
// one is so that the sp##id part generates "sp1234" type identifiers
// instead of always "sp__LINE__"...
#define PROFILE_BLOCK PROFILE_BLOCK_INTERNAL( __LINE__ );
#define PROFILE_BLOCK_INTERNAL(id) d3dProfiler sp##id ( WIDEN(__FUNCTION__), __LINE__ );

class d3dProfiler
{
	public:
		d3dProfiler( WCHAR *Name, int Line );
		~d3dProfiler( );

	private:
		d3dProfiler( );
};

#endif