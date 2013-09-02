/*************************************************************
**						d3dHandlesPostProcess					**
**************************************************************/
// File:		d3dHandlesPostProcess.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com
// Just a convenient storage class

#ifndef d3dHandlesPostProcess_h
#define d3dHandlesPostProcess_h

#include "dxInclude.h"
#include "renderer\renderer_structures\d3dHandles\d3dHandlesSampler.h"

class d3dHandlesPostProcess
{
public:

	// Constructor / Destructor
									d3dHandlesPostProcess();
									~d3dHandlesPostProcess();

	void							GetHandles(ID3DXEffect * m_FX);

	// FX Handles - Techniques
	D3DXHANDLE						h_QuadTexture_Tech, // Render a full screen quad (no transformation of verticies)
									h_QuadPositionTexture_Tech, // Extract the position and render full screen quad
									h_QuadNormalTexture_Tech, // Extract the normal and render full screen quad
									h_QuadVelocityTexture_Tech, // Extract the per-pixel velocity and render full screen quad
									h_TransformedTexture_Tech, // Render a WVP transformed full screen quad
									h_Transformed1DTexture_Tech, // Render a WVP transformed full screen quad, sample only R channel of texture into greyscale
									h_Transformed2DTexture_Tech, // Render a WVP transformed full screen quad, sample RG channels of texture
									h_Transformed3DTexture_Tech, // Render a WVP transformed full screen quad, sample RGB channels of texture
									h_OcclusionMap_Tech,
									h_BoxBlur_Tech,
									h_BoxBlur_SMAtlas_Tech,
									h_DownScale_4x4_Tech,
									h_Upscale_Tech,
									h_DownScale_NxM_Tech,
									h_CalcLuminance_Tech,
									h_CalcAdaptedLuminance_Tech,
									h_ToneMap_Tech,
									h_ApplyToneMapThreshold_Tech,
									h_GaussianBlur_Tech,
									h_MotionBlur_Tech,
									h_DepthOfField_BlurDisk_Tech;

	// FX Handles - Variables
	D3DXHANDLE						h_gSource,					// Source Texture for many effects
									h_gSource2,	
									h_gSource3,
									h_gRandomNormal,
									h_gDepth,					
									h_gNormal,
									h_gAlbedo,
									h_gMisc,
									h_gOcclusionBuffer,
									h_gCameraNearFar,
									h_gCameraTangentFov,
// Blurring
									h_gBlurSamples,
									h_gBlurSizeInv,
									h_gBlurSampleOffset,
									h_gBlurSampleAtlasClampX,
									h_gBlurSampleAtlasClampY,
									h_gBlurBaseTexCoordOffset,
									h_gGaussianBlurSigma,
									h_gGaussianBlurRadius,
									h_gMotionBlurNumSamples,
									h_gDOFDiskOffsets,
									h_gDOFBounds,

// Down conversion
									h_gNumHorizontalSamples,
									h_gNumVerticalSamples,
									h_gHorizontalSamples,
									h_gVerticalSamples,

// Up conversion
									h_gBilinearTexelSize,
									h_gBilinearTextureSize,

// HDR			
									h_gLuminanceFTau,
									h_gDeltaT,
									h_gToneMapMiddleGrey,
									h_gToneMapWhiteSq,
									h_gToneMapThreshold,
									h_gBloomMultiplier,
									h_gManualLuminance,

// SSAO
									h_gVectorNoiseSize,
									h_gScreenSize,
									h_gSSAOSampleRadius,
									h_gSSAOBias,
									h_gSSAOIntensity,
									h_gSSAOScale,

									h_gWVP,						// world * view * proj;
									h_gTexelSizeX,
									h_gTexelSizeY,
									
									h_gSMTextureAtlasOffsets,
									h_gSMTextureAtlasScale,
									h_gCurSMSplit;
								
	// Texture sampler handles
	d3dHandlesSampler				h_gSampSource;
	d3dHandlesSampler				h_gSampSource2;
	d3dHandlesSampler				h_gSampSource3;
	d3dHandlesSampler				h_gSampGBuffer;
	d3dHandlesSampler				h_gSampRandomNormal;

private:
	// Nothing
};

#endif