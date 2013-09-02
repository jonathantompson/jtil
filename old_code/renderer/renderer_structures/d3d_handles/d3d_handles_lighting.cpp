/*************************************************************
**						d3dHandlesLighting					**
**************************************************************/
// File:		d3dHandlesLighting.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\d3dHandles\d3dHandlesLighting.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		d3dHandlesLighting										*/
/* Description:	Default constructor function							*/
/************************************************************************/
d3dHandlesLighting::d3dHandlesLighting()
{
	
}

/************************************************************************/
/* Name:		~d3dHandlesLighting										*/
/* Description:	Default destructor function								*/
/************************************************************************/
d3dHandlesLighting::~d3dHandlesLighting()
{
	
}

/************************************************************************/
/* Name:		GetHandles												*/
/* Description:	Get the handles from the FX								*/
/************************************************************************/
void d3dHandlesLighting::GetHandles(ID3DXEffect * m_FX)
{
	// FX Handles - Techniques
	h_SpotLight_Shadows_Tech					= m_FX->GetTechniqueByName("SpotLight_Shadows_Tech");
	h_SpotLight_Shadows_Bilinear_Tech			= m_FX->GetTechniqueByName("SpotLight_Shadows_Bilinear_Tech");
	h_SpotLight_Tech							= m_FX->GetTechniqueByName("SpotLight_Tech");
	h_PointLight_Tech							= m_FX->GetTechniqueByName("PointLight_Tech");
	h_DirLight_Tech								= m_FX->GetTechniqueByName("DirLight_Tech");
	h_RenderUnlitPixels_Tech					= m_FX->GetTechniqueByName("RenderUnlitPixels_Tech");
	h_CombineFinal_Tech							= m_FX->GetTechniqueByName("CombineFinal_Tech");

	// FX Handles - Variables
	h_gCameraProj								= m_FX->GetParameterByName(0, "gCameraProj");
	h_gWVP										= m_FX->GetParameterByName(0, "gWVP");
	h_gWV										= m_FX->GetParameterByName(0, "gWV");
	h_gSpotLight								= m_FX->GetParameterByName(0, "gSpotLight");
	h_gDirLight									= m_FX->GetParameterByName(0, "gDirLight");
	h_gPointLight								= m_FX->GetParameterByName(0, "gPointLight");
	h_gDepth									= m_FX->GetParameterByName(0, "gDepth");
	h_gNormal									= m_FX->GetParameterByName(0, "gNormal");
	h_gAlbedo									= m_FX->GetParameterByName(0, "gAlbedo");
	h_gMisc										= m_FX->GetParameterByName(0, "gMisc");
	h_gOcclusionBuffer							= m_FX->GetParameterByName(0, "gOcclusionBuffer");
	h_gShadowMap								= m_FX->GetParameterByName(0, "gShadowMap");
	h_gLightAccumulation						= m_FX->GetParameterByName(0, "gLightAccumulation");
	h_gTexelSizeX								= m_FX->GetParameterByName(0, "gTexelSizeX");
	h_gTexelSizeY								= m_FX->GetParameterByName(0, "gTexelSizeY");
	h_gBilinearTexelSize						= m_FX->GetParameterByName(0, "gBilinearTexelSize");
	h_gBilinearTextureSize						= m_FX->GetParameterByName(0, "gBilinearTextureSize");
	h_gSMTextureAtlasOffsets					= m_FX->GetParameterByName(0, "gSMTextureAtlasOffsets");
	h_gSMTextureAtlasScale						= m_FX->GetParameterByName(0, "gSMTextureAtlasScale");
	h_gVisualizeSplits							= m_FX->GetParameterByName(0, "gVisualizeSplits");
	h_gLBRAmount								= m_FX->GetParameterByName(0, "gLBRAmount");
	h_gVSMMinVariance							= m_FX->GetParameterByName(0, "gVSMMinVariance");
	h_gNumShadowMaps							= m_FX->GetParameterByName(0, "gNumShadowMaps");
	h_gShadowMapBlendZone						= m_FX->GetParameterByName(0, "gShadowMapBlendZone");
	h_gShadowsOn								= m_FX->GetParameterByName(0, "gShadowsOn");
	h_gSSAOOn									= m_FX->GetParameterByName(0, "gSSAOOn");
	h_gSplitDistances							= m_FX->GetParameterByName(0, "gSplitDistances");
	h_gSplitVcaminvVlightPlight_Matrices		= m_FX->GetParameterByName(0, "gSplitVcaminvVlightPlight_Matrices");
	h_gCameraNearFar							= m_FX->GetParameterByName(0, "gCameraNearFar");
	h_gCameraTangentFov							= m_FX->GetParameterByName(0, "gCameraTangentFov");
	h_gSMNearFar								= m_FX->GetParameterByName(0, "gSMNearFar");
	h_gGlobalAmbient							= m_FX->GetParameterByName(0, "gGlobalAmbient");

	// Texture sampler handles
	h_gSampShadow.GetHandles(m_FX, "gSampShadow");
	h_gSampOcclusionBuffer.GetHandles(m_FX, "gSampOcclusionBuffer");
	h_gSampGBuffer.GetHandles(m_FX, "gSampGBuffer");
	h_gSampLightAccumulation.GetHandles(m_FX, "gSampLightAccumulation");
								
}
