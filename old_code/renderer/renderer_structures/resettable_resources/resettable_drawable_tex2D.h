/*************************************************************
**						resettableDrawableTex2D				**
**************************************************************/
// File:		resettableDrawableTex2D.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef resettableDrawableTex2D_h
#define resettableDrawableTex2D_h

#include "renderer\renderer_structures\resettableResources\resettable.h"

class drawableTex2D;

class resettableDrawableTex2D : public resettable
{
public:

	// Constructor / Destructor
    resettableDrawableTex2D(drawableTex2D * _tex);
	~resettableDrawableTex2D();
	inline drawableTex2D * GetTex() { return tex; }

	// To avoid dynamic_cast when dealing with polymorphism
	// Instead: check type and call reinterpret_cast
	inline virtual resettable_TYPE GetType() { return T_RESETTABLE_DRAWABLETEX2D; }

	virtual void OnLostDevice();
	virtual void OnResetDevice();
	virtual void Release();

private:
	
	drawableTex2D * tex;
};

#endif