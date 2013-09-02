/*************************************************************
**						d3dHandlesLighting					**
**************************************************************/
// File:		d3dHandlesLighting.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com
// Just a convenient storage class

#ifndef d3dHandlesLighting_h
#define d3dHandlesLighting_h

#include "dxInclude.h"
#include "renderer\renderer_structures\d3dHandles\d3dHandlesSampler.h"

class d3dHandlesLighting
{
public:

	// Constructor / Destructor
									d3dHandlesLighting();
									~d3dHandlesLighting();

	void							GetHandles(ID3DXEffect * m_FX);

	// FX Handles - Techniques
	D3DXHANDLE						h_SpotLight_Shadows_Tech,
									h_SpotLight_Shadows_Bilinear_Tech,
									h_SpotLight_Tech,
									h_PointLight_Tech,
									h_DirLight_Tech,
									h_RenderUnlitPixels_Tech,
									h_CombineFinal_Tech;

	// FX Handles - Variables
	D3DXHANDLE						h_gCameraProj,
									h_gWVP,
									h_gWV,
									h_gSpotLight,
									h_gDirLight,
									h_gPointLight,
									h_gDepth,					
									h_gNormal,
									h_gAlbedo,
									h_gMisc,
									h_gOcclusionBuffer,
									h_gShadowMap,
									h_gLightAccumulation,
									h_gTexelSizeX,
									h_gTexelSizeY,
									h_gBilinearTexelSize,
									h_gBilinearTextureSize,
									h_gSMTextureAtlasOffsets,
									h_gSMTextureAtlasScale,
									h_gVisualizeSplits,
									h_gLBRAmount,
									h_gVSMMinVariance,
									h_gNumShadowMaps,
									h_gShadowMapBlendZone,
									h_gShadowsOn,
									h_gSSAOOn,
									h_gSplitDistances,
									h_gSplitVcaminvVlightPlight_Matrices,
									h_gCameraNearFar,
									h_gCameraTangentFov,
									h_gSMNearFar,
									h_gGlobalAmbient;
								
	// Texture sampler handles
	d3dHandlesSampler				h_gSampShadow;
	d3dHandlesSampler				h_gSampOcclusionBuffer;
	d3dHandlesSampler				h_gSampGBuffer;
	d3dHandlesSampler				h_gSampLightAccumulation;



private:
	// Nothing
};

#endif