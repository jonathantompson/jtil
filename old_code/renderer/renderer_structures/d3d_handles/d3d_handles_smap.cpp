/*************************************************************
**						d3dHandlesSMap						**
**************************************************************/
// File:		d3dHandlesSMap.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\d3dHandles\d3dHandlesSMap.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		d3dHandlesSMap											*/
/* Description:	Default constructor function							*/
/************************************************************************/
d3dHandlesSMap::d3dHandlesSMap()
{
	
}

/************************************************************************/
/* Name:		~d3dHandlesSMap											*/
/* Description:	Default destructor function								*/
/************************************************************************/
d3dHandlesSMap::~d3dHandlesSMap()
{
	
}

/************************************************************************/
/* Name:		GetHandles												*/
/* Description:	Get the handles from the FX								*/
/************************************************************************/
void d3dHandlesSMap::GetHandles(ID3DXEffect * m_FX)
{
	// FX Handles - Techniques
	h_BuildShadowMap_Tech						= m_FX->GetTechniqueByName("BuildShadowMap_Tech");

	// FX Handles - Variables
	h_gWVP										= m_FX->GetParameterByName(0, "gWVP");
	h_gWV										= m_FX->GetParameterByName(0, "gWV");
	h_gW										= m_FX->GetParameterByName(0, "gW");
	h_gLight									= m_FX->GetParameterByName(0, "gLight");
	h_gVSMDepthEpsilon							= m_FX->GetParameterByName(0, "gVSMDepthEpsilon");

	// Texture sampler handles
	
}
