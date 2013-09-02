/*************************************************************
**						resettableDrawableTexAtlas2D		**
**************************************************************/
// File:		resettableDrawableTexAtlas2D.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef resettableDrawableTexAtlas2D_h
#define resettableDrawableTexAtlas2D_h

#include "renderer\renderer_structures\resettableResources\resettable.h"

class drawableTexAtlas2D;

class resettableDrawableTexAtlas2D : public resettable
{
public:

	// Constructor / Destructor
    resettableDrawableTexAtlas2D(drawableTexAtlas2D * _tex);
	~resettableDrawableTexAtlas2D();
	inline drawableTexAtlas2D * GetTex() { return tex; }

	// To avoid dynamic_cast when dealing with polymorphism
	// Instead: check type and call reinterpret_cast
	inline virtual resettable_TYPE GetType() { return T_RESETTABLE_DRAWABLETEXATLAS2D; }

	virtual void OnLostDevice();
	virtual void OnResetDevice();
	virtual void Release();

private:
	
	drawableTexAtlas2D * tex;
};

#endif