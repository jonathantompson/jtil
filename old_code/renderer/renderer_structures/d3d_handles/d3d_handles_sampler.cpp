/*************************************************************
**						d3dHandlesSampler					**
**************************************************************/
// File:		d3dHandlesSampler.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\d3dHandles\d3dHandlesSampler.h"
#include <string>

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		d3dHandlesSampler										*/
/* Description:	Default constructor function							*/
/************************************************************************/
d3dHandlesSampler::d3dHandlesSampler()
{

}

/************************************************************************/
/* Name:		~d3dHandlesSampler										*/
/* Description:	Default destructor function								*/
/************************************************************************/
d3dHandlesSampler::~d3dHandlesSampler()
{
}

/************************************************************************/
/* Name:		GetHandles												*/
/* Description:	Get all the handles from the effect						*/
/************************************************************************/
void d3dHandlesSampler::GetHandles(ID3DXEffect * m_FX, char * name)
{
	std::string filterName = name + std::string("MagFilter");		magFilter = m_FX->GetParameterByName(0, filterName.c_str());
				filterName = name + std::string("MinFilter");		minFilter = m_FX->GetParameterByName(0, filterName.c_str());
				filterName = name + std::string("MipFilter");		mipFilter = m_FX->GetParameterByName(0, filterName.c_str());
				filterName = name + std::string("MaxAnisotropy");	maxAnisotropy = m_FX->GetParameterByName(0, filterName.c_str());
				filterName = name + std::string("AddressU");		addressU = m_FX->GetParameterByName(0, filterName.c_str());
				filterName = name + std::string("AddressV");		addressV = m_FX->GetParameterByName(0, filterName.c_str());
}