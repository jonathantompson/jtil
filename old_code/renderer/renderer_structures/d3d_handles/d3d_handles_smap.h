/*************************************************************
**						d3dHandlesSMap					**
**************************************************************/
// File:		d3dHandlesSMap.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com
// Just a convenient storage class

#ifndef d3dHandlesSMap_h
#define d3dHandlesSMap_h

#include "dxInclude.h"
#include "renderer\renderer_structures\d3dHandles\d3dHandlesSampler.h"

class d3dHandlesSMap
{
public:

	// Constructor / Destructor
									d3dHandlesSMap();
									~d3dHandlesSMap();

	void							GetHandles(ID3DXEffect * m_FX);

	// FX Handles - Techniques
	D3DXHANDLE						h_BuildShadowMap_Tech;

	// FX Handles - Variables
	D3DXHANDLE						h_gWVP,						// world * view * proj
									h_gWV,						// world * view
									h_gW,						// world
									h_gLight,
									h_gVSMDepthEpsilon;					
								
	// Texture sampler handles
	

private:
	// Nothing
};

#endif