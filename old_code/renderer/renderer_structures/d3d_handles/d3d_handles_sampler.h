/*************************************************************
**						d3dHandlesSampler					**
**************************************************************/
// File:		d3dHandlesSampler.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Stores the D3DXHANDLE for a single sampler

#ifndef d3dHandlesSampler_h
#define d3dHandlesSampler_h

#include "dxInclude.h"

class d3dHandlesSampler
{
public:

	// Constructor / Destructor
					d3dHandlesSampler();
					~d3dHandlesSampler();

	D3DXHANDLE		minFilter;
	D3DXHANDLE		magFilter;
	D3DXHANDLE		mipFilter;
	D3DXHANDLE		maxAnisotropy;
	D3DXHANDLE		addressU;
	D3DXHANDLE		addressV;

	void			GetHandles(ID3DXEffect * m_FX, char * name);

private:
	
};

#endif