/*************************************************************
**						resettableDrawableTex2D				**
**************************************************************/
// File:		resettableDrawableTex2D.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\resettableResources\resettableDrawableTex2D.h"
#include "renderer\renderer_structures\drawableTex2D.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		resettableDrawableTex2D									*/
/* Description:	Default constructor function							*/
/************************************************************************/
resettableDrawableTex2D::resettableDrawableTex2D(drawableTex2D * _tex)
{
	tex = _tex;
}

/************************************************************************/
/* Name:		~resettableDrawableTex2D								*/
/* Description:	Default destructor function								*/
/************************************************************************/
resettableDrawableTex2D::~resettableDrawableTex2D()
{
	
}

/************************************************************************/
/* Name:		OnLostDevice											*/
/* Description:	Free resources before resetting							*/
/************************************************************************/
void resettableDrawableTex2D::OnLostDevice()
{
	tex->OnLostDevice();
}

/************************************************************************/
/* Name:		OnResetDevice											*/
/* Description:	Reload resources after resetting						*/
/************************************************************************/
void resettableDrawableTex2D::OnResetDevice()
{
	tex->OnResetDevice();
}

/************************************************************************/
/* Name:		Release													*/
/* Description:	Delete the resource on app close						*/
/************************************************************************/
void resettableDrawableTex2D::Release()
{
	if(tex) { tex->Release(); }
}