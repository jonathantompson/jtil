/*************************************************************
**				obboxRenderitems class						**
*************************************************************/
// File:		obboxRenderitems.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// These items used to be included in obbox class --> Lead to higher than necessary memory overhead.
// Now only include pointers if needed.

#ifndef obboxRenderitems_h
#define obboxRenderitems_h

#include	<windows.h>				// Win32API
#include	<commctrl.h>			// Windows XP Styles
#include	<d3d9.h>				// main Direct3D header

#include	"renderer\renderer_structures\d3dFormats.h"

class obboxRenderitems
{
public:
	friend class rbobjectMesh;
	friend class obbox;

									obboxRenderitems();
									~obboxRenderitems();
// private:

	// Convex hull store - Usually NULL after initialization (unless we're keeping to save it to disk)
	float *							cHullVert;
	UINT							cHullVertCount;
	PUINT							cHullInd;
	UINT							cHullIndCount;

	// Rendering objects
	IDirect3DVertexBuffer9 *		convexHullVertexBuffer;
	IDirect3DIndexBuffer9 *			convexHullIndexBuffer;
	DWORD							convexHullVertexBufferSize;
	DWORD							convexHullIndexBufferSize;
	IDirect3DVertexBuffer9 *		OBBVertexBuffer;
	IDirect3DIndexBuffer9 *			OBBIndexBuffer;
	DWORD							OBBVertexBufferSize;
	DWORD							OBBIndexBufferSize;
	Mtrl							mtrl; // Only used when rendering solid trianlges
};

#endif