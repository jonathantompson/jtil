/*************************************************************
**						resettable							**
**************************************************************/
// File:		resettable.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef resettable_h
#define resettable_h

enum resettable_TYPE {
	T_RESETTABLE_UNDEFINED,
    T_RESETTABLE_TEXTURE,
	T_RESETTABLE_DRAWABLETEX2D,
	T_RESETTABLE_DRAWABLETEXATLAS2D,
};

#include "dxInclude.h"

class resettable // Hack: "virtual ~resettable();" makes this class polymorphic --> I think this is OK
{
public:

	// Constructor / Destructor
    resettable();
	virtual ~resettable();

	// To avoid dynamic_cast when dealing with polymorphism
	// Instead: check type and call reinterpret_cast
	inline virtual resettable_TYPE GetType() { return T_RESETTABLE_UNDEFINED; }

	inline virtual void OnLostDevice() { } // Nothing to do for base class
	inline virtual void OnResetDevice() { } // Nothing to do for base class
	inline virtual void Release() { } // Nothing to do for base class
	inline virtual void CheckResettableFormat( D3DFORMAT backBufferFormat ) { } // Nothing to do for base class

private:


};

#endif