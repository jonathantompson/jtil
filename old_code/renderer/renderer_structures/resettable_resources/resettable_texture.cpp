/*************************************************************
**						resettableTexture					**
**************************************************************/
// File:		resettableTexture.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\resettableResources\resettableTexture.h"
#include "renderer\renderer.h"
#include "main.h"
#include <string>
#include "utils_and_misc_classes\stringUtil.h"
#include "renderer\utils\d3dProfiler.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		resettableTexture										*/
/* Description:	Default constructor function							*/
/************************************************************************/
resettableTexture::resettableTexture(LPCWSTR _pFile, D3DFORMAT _format, UINT _numMips, IDirect3DTexture9 ** _tex)
{
	pFile = std::wstring(_pFile); 
	format = _format; 
	numMips = _numMips; 
	tex = _tex;
}

/************************************************************************/
/* Name:		~resettableTexture										*/
/* Description:	Default destructor function								*/
/************************************************************************/
resettableTexture::~resettableTexture()
{
	
}

/************************************************************************/
/* Name:		OnLostDevice											*/
/* Description:	Free resources before resetting							*/
/************************************************************************/
void resettableTexture::OnLostDevice()
{
	Release();
}

/************************************************************************/
/* Name:		OnResetDevice											*/
/* Description:	Reload resources after resetting						*/
/************************************************************************/
void resettableTexture::OnResetDevice()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	if(FAILED(D3DXCreateTextureFromFileExW(g_renderer->GetD3DDev(),	// LPDIRECT3DDEVICE9 pDevice
										   pFile.c_str(),			// LPCTSTR pSrcFile
										   D3DX_DEFAULT,			// UINT Width - D3DX_DEFAULT = get it from file
										   D3DX_DEFAULT,			// UINT Height
										   numMips,					// UINT MipLevels - D3DX_DEFAULT = create an entire mip chain
										   0,						// DWORD Usage
										   format,					// D3DFORMAT Format
										   D3DPOOL_DEFAULT,			// D3DPOOL Pool
										   D3DX_DEFAULT,			// DWORD Filter - D3DX_DEFAULT = D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER
										   D3DX_DEFAULT,			// DWORD MipFilter - D3DX_DEFAULT = D3DX_FILTER_BOX
										   0,						// D3DCOLOR ColorKey - 0 = disabled
										   NULL,					// D3DXIMAGE_INFO *pSrcInfo
										   NULL,					// PALETTEENTRY *pPalette
										   tex)))					// LPDIRECT3DTEXTURE9 *ppTexture
	{
		std::string error = "resettableTexture::OnResetDevice() - Failed to load texture: ";
		error.append(stringUtil::toNarrowString(pFile.c_str(),-1));
		throw std::runtime_error(error.c_str());
	}
}

/************************************************************************/
/* Name:		Release													*/
/* Description:	Delete the resource on app close						*/
/************************************************************************/
void resettableTexture::Release()
{
	if(tex)
	{
		if(*tex)
		{ 
			(*tex)->Release(); 
			(*tex) = NULL; 
		}
	}
}

/************************************************************************/
/* Name:		CheckResettableFormat									*/
/* Description:	Delete the resource on app close						*/
/************************************************************************/
void resettableTexture::CheckResettableFormat( D3DFORMAT backBufferFormat )
{
	if(FAILED(g_renderer->GetD3DObj()->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFormat, 0, D3DRTYPE_TEXTURE, format)))
		throw std::runtime_error("resettableTexture::CheckResettableFormat() - This texture format is not supported.");
}
