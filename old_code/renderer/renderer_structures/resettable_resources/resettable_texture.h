/*************************************************************
**						resettableTexture					**
**************************************************************/
// File:		resettableTexture.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef resettableTexture_h
#define resettableTexture_h

#include	"renderer\renderer_structures\resettableResources\resettable.h"
#include	"dxInclude.h"
#include	<string>

class resettableTexture : public resettable
{
public:

	// Constructor / Destructor
    resettableTexture(LPCWSTR _pFile, D3DFORMAT _format, UINT _numMips, IDirect3DTexture9 ** _tex);
	~resettableTexture();

	// To avoid dynamic_cast when dealing with polymorphism
	// Instead: check type and call reinterpret_cast
	inline virtual resettable_TYPE GetType() { return T_RESETTABLE_TEXTURE; }

	virtual void OnLostDevice();
	virtual void OnResetDevice();
	virtual void Release();
	virtual void CheckResettableFormat( D3DFORMAT backBufferFormat );

private:

	IDirect3DTexture9 **	tex;
	std::wstring			pFile; 
	D3DFORMAT				format; 
	UINT					numMips;

};

#endif