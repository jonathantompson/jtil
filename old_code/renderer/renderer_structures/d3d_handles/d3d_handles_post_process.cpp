/*************************************************************
**						d3dHandlesPostProcess				**
**************************************************************/
// File:		d3dHandlesPostProcess.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\d3dHandles\d3dHandlesPostProcess.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		d3dHandlesPostProcess									*/
/* Description:	Default constructor function							*/
/************************************************************************/
d3dHandlesPostProcess::d3dHandlesPostProcess()
{
	
}

/************************************************************************/
/* Name:		~d3dHandlesPostProcess									*/
/* Description:	Default destructor function								*/
/************************************************************************/
d3dHandlesPostProcess::~d3dHandlesPostProcess()
{
	
}

/************************************************************************/
/* Name:		GetHandles												*/
/* Description:	Get the handles from the FX								*/
/************************************************************************/
void d3dHandlesPostProcess::GetHandles(ID3DXEffect * m_FX)
{
	// FX Handles - Techniques	
	h_QuadTexture_Tech							= m_FX->GetTechniqueByName("QuadTexture_Tech");
	h_QuadPositionTexture_Tech					= m_FX->GetTechniqueByName("QuadPositionTexture_Tech");
	h_QuadNormalTexture_Tech					= m_FX->GetTechniqueByName("QuadNormalTexture_Tech");
	h_QuadVelocityTexture_Tech					= m_FX->GetTechniqueByName("QuadVelocityTexture_Tech");
	h_TransformedTexture_Tech					= m_FX->GetTechniqueByName("TransformedTexture_Tech"); 
	h_Transformed1DTexture_Tech					= m_FX->GetTechniqueByName("Transformed1DTexture_Tech"); 
	h_Transformed2DTexture_Tech					= m_FX->GetTechniqueByName("Transformed2DTexture_Tech");  
	h_Transformed3DTexture_Tech					= m_FX->GetTechniqueByName("Transformed3DTexture_Tech");  
	h_OcclusionMap_Tech							= m_FX->GetTechniqueByName("OcclusionMap_Tech");
	h_BoxBlur_Tech								= m_FX->GetTechniqueByName("BoxBlur_Tech");
	h_BoxBlur_SMAtlas_Tech						= m_FX->GetTechniqueByName("BoxBlur_SMAtlas_Tech");
	h_DownScale_4x4_Tech						= m_FX->GetTechniqueByName("DownScale_4x4_Tech");
	h_Upscale_Tech								= m_FX->GetTechniqueByName("Upscale_Tech");
	h_DownScale_NxM_Tech						= m_FX->GetTechniqueByName("DownScale_NxM_Tech");
	h_CalcLuminance_Tech						= m_FX->GetTechniqueByName("CalcLuminance_Tech");
	h_CalcAdaptedLuminance_Tech					= m_FX->GetTechniqueByName("CalcAdaptedLuminance_Tech");
	h_ToneMap_Tech								= m_FX->GetTechniqueByName("ToneMap_Tech");
	h_ApplyToneMapThreshold_Tech				= m_FX->GetTechniqueByName("ApplyToneMapThreshold_Tech");
	h_GaussianBlur_Tech							= m_FX->GetTechniqueByName("GaussianBlur_Tech");
	h_MotionBlur_Tech							= m_FX->GetTechniqueByName("MotionBlur_Tech");
	h_DepthOfField_BlurDisk_Tech				= m_FX->GetTechniqueByName("DepthOfField_BlurDisk_Tech");

	// FX Handles - Variables
	h_gSource									= m_FX->GetParameterByName(0, "gSource");
	h_gSource2									= m_FX->GetParameterByName(0, "gSource2");
	h_gSource3									= m_FX->GetParameterByName(0, "gSource3");
	h_gRandomNormal								= m_FX->GetParameterByName(0, "gRandomNormal");
	h_gDepth									= m_FX->GetParameterByName(0, "gDepth");
	h_gNormal									= m_FX->GetParameterByName(0, "gNormal");
	h_gAlbedo									= m_FX->GetParameterByName(0, "gAlbedo");
	h_gMisc										= m_FX->GetParameterByName(0, "gMisc");
	h_gOcclusionBuffer							= m_FX->GetParameterByName(0, "gOcclusionBuffer");
	h_gCameraNearFar							= m_FX->GetParameterByName(0, "gCameraNearFar");
	h_gCameraTangentFov							= m_FX->GetParameterByName(0, "gCameraTangentFov");

// Blurring (velocity, box blur for SM, gaussian and depth of field)
	h_gBlurSamples								= m_FX->GetParameterByName(0, "gBlurSamples");
	h_gBlurSizeInv								= m_FX->GetParameterByName(0, "gBlurSizeInv");
	h_gBlurSampleOffset							= m_FX->GetParameterByName(0, "gBlurSampleOffset");
	h_gBlurSampleAtlasClampX					= m_FX->GetParameterByName(0, "gBlurSampleAtlasClampX");
	h_gBlurSampleAtlasClampY					= m_FX->GetParameterByName(0, "gBlurSampleAtlasClampY");
	h_gBlurBaseTexCoordOffset					= m_FX->GetParameterByName(0, "gBlurBaseTexCoordOffset");
	h_gGaussianBlurSigma						= m_FX->GetParameterByName(0, "gGaussianBlurSigma");
	h_gGaussianBlurRadius						= m_FX->GetParameterByName(0, "gGaussianBlurRadius");
	h_gMotionBlurNumSamples						= m_FX->GetParameterByName(0, "gMotionBlurNumSamples");
	h_gDOFDiskOffsets							= m_FX->GetParameterByName(0, "gDOFDiskOffsets");
	h_gDOFBounds								= m_FX->GetParameterByName(0, "gDOFBounds");

// Down conversion
	h_gNumHorizontalSamples						= m_FX->GetParameterByName(0, "gNumHorizontalSamples");
	h_gNumVerticalSamples						= m_FX->GetParameterByName(0, "gNumVerticalSamples");
	h_gHorizontalSamples						= m_FX->GetParameterByName(0, "gHorizontalSamples");
	h_gVerticalSamples							= m_FX->GetParameterByName(0, "gVerticalSamples");

// Up conversion
	h_gBilinearTexelSize						= m_FX->GetParameterByName(0, "gBilinearTexelSize");
	h_gBilinearTextureSize						= m_FX->GetParameterByName(0, "gBilinearTextureSize");

// HDR
	h_gLuminanceFTau							= m_FX->GetParameterByName(0, "gLuminanceFTau");
	h_gDeltaT									= m_FX->GetParameterByName(0, "gDeltaT");
	h_gToneMapMiddleGrey						= m_FX->GetParameterByName(0, "gToneMapMiddleGrey");
	h_gToneMapWhiteSq							= m_FX->GetParameterByName(0, "gToneMapWhiteSq");
	h_gToneMapThreshold							= m_FX->GetParameterByName(0, "gToneMapThreshold");
	h_gBloomMultiplier							= m_FX->GetParameterByName(0, "gBloomMultiplier");
	h_gManualLuminance							= m_FX->GetParameterByName(0, "gManualLuminance");

// SSAO
	h_gVectorNoiseSize							= m_FX->GetParameterByName(0, "gVectorNoiseSize");
	h_gScreenSize								= m_FX->GetParameterByName(0, "gScreenSize");
	h_gSSAOSampleRadius							= m_FX->GetParameterByName(0, "gSSAOSampleRadius");
	h_gSSAOBias									= m_FX->GetParameterByName(0, "gSSAOBias");
	h_gSSAOIntensity							= m_FX->GetParameterByName(0, "gSSAOIntensity");
	h_gSSAOScale								= m_FX->GetParameterByName(0, "gSSAOScale");

	h_gWVP										= m_FX->GetParameterByName(0, "gWVP");
	h_gTexelSizeX								= m_FX->GetParameterByName(0, "gTexelSizeX");
	h_gTexelSizeY								= m_FX->GetParameterByName(0, "gTexelSizeY");

	h_gSMTextureAtlasOffsets					= m_FX->GetParameterByName(0, "gSMTextureAtlasOffsets");
	h_gSMTextureAtlasScale						= m_FX->GetParameterByName(0, "gSMTextureAtlasScale");
	h_gCurSMSplit								= m_FX->GetParameterByName(0, "gCurSMSplit");

	// Texture sampler handles
	h_gSampSource.GetHandles(m_FX, "gSampSource");
	h_gSampSource2.GetHandles(m_FX, "gSampSource2");
	h_gSampSource3.GetHandles(m_FX, "gSampSource3");
	h_gSampGBuffer.GetHandles(m_FX, "gSampGBuffer");
	h_gSampRandomNormal.GetHandles(m_FX, "gSampRandomNormal");

}
