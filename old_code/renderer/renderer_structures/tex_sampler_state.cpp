/*************************************************************
**						texSamplerState						**
**************************************************************/
// File:		texSamplerState.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\texSamplerState.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		texSamplerState											*/
/* Description:	Default constructor function							*/
/************************************************************************/
texSamplerState::texSamplerState()
{
	magFilter = (D3DTEXTUREFILTERTYPE)-1; // Invalid state
	minFilter = (D3DTEXTUREFILTERTYPE)-1; // Invalid state
	mipFilter = (D3DTEXTUREFILTERTYPE)-1; // Invalid state
	addressU = (D3DTEXTUREADDRESS)-1; // Invalid state
	addressV = (D3DTEXTUREADDRESS)-1; // Invalid state
	maxAnisotropy = (DWORD)0; // Invalid state
}

/************************************************************************/
/* Name:		~texSamplerState										*/
/* Description:	Default destructor function								*/
/************************************************************************/
texSamplerState::~texSamplerState()
{
	
}

/************************************************************************/
/* Name:		SetNoFilter												*/
/* Description:	Set to a disabled filter state.							*/
/************************************************************************/
void texSamplerState::SetPointFilterWrap()
{
	magFilter = D3DTEXF_POINT;
	minFilter = D3DTEXF_POINT; 
	mipFilter = D3DTEXF_POINT;
	addressU = D3DTADDRESS_WRAP;
	addressV = D3DTADDRESS_WRAP;
	maxAnisotropy = 1; 
}

/************************************************************************/
/* Name:		SetPointFilterClamp										*/
/* Description:	Set to a disabled filter state.							*/
/************************************************************************/
void texSamplerState::SetPointFilterClamp()
{
	magFilter = D3DTEXF_POINT;
	minFilter = D3DTEXF_POINT; 
	mipFilter = D3DTEXF_POINT;
	addressU = D3DTADDRESS_CLAMP;
	addressV = D3DTADDRESS_CLAMP;
	maxAnisotropy = 1; 
}

/************************************************************************/
/* Name:		SetLinearFilterClamp									*/
/* Description:	Set to a linear filter state.							*/
/************************************************************************/
void texSamplerState::SetLinearFilterClamp()
{
	magFilter = D3DTEXF_LINEAR;
	minFilter = D3DTEXF_LINEAR; 
	mipFilter = D3DTEXF_LINEAR;
	addressU = D3DTADDRESS_CLAMP;
	addressV = D3DTADDRESS_CLAMP;
	maxAnisotropy = 1; 
}

/************************************************************************/
/* Name:		SetFilter												*/
/* Description:	Set the filter state.									*/
/************************************************************************/
void texSamplerState::SetFilter(D3DTEXTUREFILTERTYPE _magFilter, D3DTEXTUREFILTERTYPE _minFilter, D3DTEXTUREFILTERTYPE _mipFilter, D3DTEXTUREADDRESS _addressU, D3DTEXTUREADDRESS _addressV, DWORD _maxAnisotropy )
{
	magFilter = _magFilter;
	minFilter = _minFilter; 
	mipFilter = _mipFilter;
	addressU = _addressU;
	addressV = _addressV;
	maxAnisotropy = _maxAnisotropy; 
}

/************************************************************************/
/* Name:		SetFilter												*/
/* Description:	Set the filter state.									*/
/************************************************************************/
void texSamplerState::SetFilter(D3DTEXTUREFILTERTYPE _Filter, D3DTEXTUREADDRESS _address, DWORD _maxAnisotropy )
{
	magFilter = _Filter;
	minFilter = _Filter; 
	mipFilter = _Filter;
	addressU = _address;
	addressV = _address;
	maxAnisotropy = _maxAnisotropy; 
}
