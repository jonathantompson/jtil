/*************************************************************
**						resettableDrawableTexAtlas2D		**
**************************************************************/
// File:		resettableDrawableTexAtlas2D.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\resettableResources\resettableDrawableTexAtlas2D.h"
#include "renderer\renderer_structures\drawableTexAtlas2D.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		resettableDrawableTexAtlas2D							*/
/* Description:	Default constructor function							*/
/************************************************************************/
resettableDrawableTexAtlas2D::resettableDrawableTexAtlas2D(drawableTexAtlas2D * _tex)
{
	tex = _tex;
}

/************************************************************************/
/* Name:		~resettableDrawableTexAtlas2D							*/
/* Description:	Default destructor function								*/
/************************************************************************/
resettableDrawableTexAtlas2D::~resettableDrawableTexAtlas2D()
{
	
}

/************************************************************************/
/* Name:		OnLostDevice											*/
/* Description:	Free resources before resetting							*/
/************************************************************************/
void resettableDrawableTexAtlas2D::OnLostDevice()
{
	tex->OnLostDevice();
}

/************************************************************************/
/* Name:		OnResetDevice											*/
/* Description:	Reload resources after resetting						*/
/************************************************************************/
void resettableDrawableTexAtlas2D::OnResetDevice()
{
	tex->OnResetDevice();
}

/************************************************************************/
/* Name:		Release													*/
/* Description:	Delete the resource on app close						*/
/************************************************************************/
void resettableDrawableTexAtlas2D::Release()
{
	if(tex) { tex->Release(); }
}