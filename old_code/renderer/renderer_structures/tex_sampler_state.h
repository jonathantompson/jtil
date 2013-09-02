/*************************************************************
**						texSamplerState						**
**************************************************************/
// File:		texSamplerState.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef texSamplerState_h
#define texSamplerState_h

#include "dxInclude.h"

class texSamplerState
{
public:

	// Constructor / Destructor
    texSamplerState();
	~texSamplerState();

	void SetPointFilterWrap(); // Set to a disabled filter state
	void SetPointFilterClamp();
	void SetLinearFilterClamp();
	void SetFilter(D3DTEXTUREFILTERTYPE _magFilter, D3DTEXTUREFILTERTYPE _minFilter, D3DTEXTUREFILTERTYPE _mipFilter, D3DTEXTUREADDRESS _addressU, D3DTEXTUREADDRESS _addressV, DWORD _maxAnisotropy );
	void SetFilter(D3DTEXTUREFILTERTYPE _Filter, D3DTEXTUREADDRESS _address, DWORD _maxAnisotropy );
	

	D3DTEXTUREFILTERTYPE magFilter; 
	D3DTEXTUREFILTERTYPE minFilter; 
	D3DTEXTUREFILTERTYPE mipFilter; 
	D3DTEXTUREADDRESS addressU; 
	D3DTEXTUREADDRESS addressV; 
	DWORD maxAnisotropy;

private:


};

#endif