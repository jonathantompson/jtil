/*************************************************************
**						d3dHandlesGBuffer					**
**************************************************************/
// File:		d3dHandlesGBuffer.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\d3dHandles\d3dHandlesGBuffer.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		d3dHandlesGBuffer										*/
/* Description:	Default constructor function							*/
/************************************************************************/
d3dHandlesGBuffer::d3dHandlesGBuffer()
{
	
}

/************************************************************************/
/* Name:		~d3dHandlesGBuffer										*/
/* Description:	Default destructor function								*/
/************************************************************************/
d3dHandlesGBuffer::~d3dHandlesGBuffer()
{
	
}

/************************************************************************/
/* Name:		GetHandles												*/
/* Description:	Get the handles from the FX								*/
/************************************************************************/
void d3dHandlesGBuffer::GetHandles(ID3DXEffect * m_FX)
{
	// FX Handles - Techniques
	h_ClearGBuffer_Tech							= m_FX->GetTechniqueByName("ClearGBuffer_Tech");
	h_TexturedMesh_Tech							= m_FX->GetTechniqueByName("TexturedMesh_Tech");
	h_TexturedMesh_Wireframe_Tech				= m_FX->GetTechniqueByName("TexturedMesh_Wireframe_Tech");
	h_SingleColorMesh_Wireframe_Tech			= m_FX->GetTechniqueByName("SingleColorMesh_Wireframe_Tech");
	h_SingleColorMesh_Tech						= m_FX->GetTechniqueByName("SingleColorMesh_Tech");
	h_SkyBox_Tech								= m_FX->GetTechniqueByName("SkyBox_Tech");

	// FX Handles - Variables
	h_gWVP										= m_FX->GetParameterByName(0, "gWVP");
	h_gWV										= m_FX->GetParameterByName(0, "gWV");
	h_gWVP_prevFrame							= m_FX->GetParameterByName(0, "gWVP_prevFrame");
	h_gMtrl										= m_FX->GetParameterByName(0, "gMtrl");
	h_gTex										= m_FX->GetParameterByName(0, "gTex");
	h_gEnvMap									= m_FX->GetParameterByName(0, "gEnvMap");
	h_gCameraNearFar							= m_FX->GetParameterByName(0, "gCameraNearFar");
	h_gDepthClearColor							= m_FX->GetParameterByName(0, "gDepthClearColor");
	h_gNormalClearColor							= m_FX->GetParameterByName(0, "gNormalClearColor");
	h_gAlbedoClearColor							= m_FX->GetParameterByName(0, "gAlbedoClearColor");
	h_gMiscClearColor							= m_FX->GetParameterByName(0, "gMiscClearColor");
	h_gTexelSizeX								= m_FX->GetParameterByName(0, "gTexelSizeX");
	h_gTexelSizeY								= m_FX->GetParameterByName(0, "gTexelSizeY");
	h_gSplitDistances							= m_FX->GetParameterByName(0, "gSplitDistances");

	// Texture sampler handles
	h_gSampTex.GetHandles(m_FX, "gSampTex");
	h_gSampEnvMap.GetHandles(m_FX, "gSampEnvMap");
}
