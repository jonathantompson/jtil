/*************************************************************
**						d3dHandlesGBuffer					**
**************************************************************/
// File:		d3dHandlesGBuffer.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com
// Just a convenient storage class

#ifndef d3dHandlesGBuffer_h
#define d3dHandlesGBuffer_h

#include "dxInclude.h"
#include "renderer\renderer_structures\d3dHandles\d3dHandlesSampler.h"

class d3dHandlesGBuffer
{
public:

	// Constructor / Destructor
									d3dHandlesGBuffer();
									~d3dHandlesGBuffer();

	void							GetHandles(ID3DXEffect * m_FX);

	// FX Handles - Techniques
	D3DXHANDLE						h_ClearGBuffer_Tech,
									h_TexturedMesh_Tech,
									h_TexturedMesh_Wireframe_Tech,
									h_SingleColorMesh_Wireframe_Tech,
									h_SingleColorMesh_Tech,
									h_SkyBox_Tech;

	// FX Handles - Variables
	D3DXHANDLE						h_gWVP,						// world * view * proj
									h_gWV,						// world * view
									h_gWVP_prevFrame,
									h_gMtrl, 
									h_gTex,
									h_gEnvMap,
									h_gCameraNearFar,
									h_gDepthClearColor,
									h_gNormalClearColor,
									h_gAlbedoClearColor,
									h_gMiscClearColor,
									h_gTexelSizeX,
									h_gTexelSizeY,
									h_gSplitDistances;
								
	// Texture sampler handles
	d3dHandlesSampler				h_gSampTex;
	d3dHandlesSampler				h_gSampEnvMap;
private:
	// Nothing
};

#endif