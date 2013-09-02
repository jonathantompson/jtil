#ifndef d3dFormats_h
#define d3dFormats_h

#include "dxInclude.h"

//constants
#define D3DFVF_CUSTOMVERTEX	(D3DFVF_XYZ | D3DFVF_NORMAL )

const D3DXCOLOR WHITE(1.0f, 1.0f, 1.0f, 1.0f);
const D3DXCOLOR BLACK(0.0f, 0.0f, 0.0f, 1.0f);
const D3DXCOLOR RED(1.0f, 0.0f, 0.0f, 1.0f);
const D3DXCOLOR GREEN(0.0f, 1.0f, 0.0f, 1.0f);
const D3DXCOLOR BLUE(0.0f, 0.0f, 1.0f, 1.0f);
const D3DXCOLOR YELLOW(1.0f, 1.0f, 0.0f, 1.0f);
const D3DXCOLOR CYAN(0.0f, 1.0f, 1.0f, 1.0f);
const D3DXCOLOR MAGENTA(1.0f, 0.0f, 1.0f, 1.0f);

#define DEFAULT_SPEC_POWER 8.0f

//structures
struct CUSTOMVERTEX
{
float		fX,
			fY,
			fZ;
D3DVECTOR	NORMAL;
};

struct Mtrl
{
	Mtrl()
		:diffuse(WHITE), specPower(DEFAULT_SPEC_POWER), specIntensity(1.0f){}
	Mtrl(const D3DXCOLOR& d, float power, float intensity)
		:diffuse(d), specPower(power), specIntensity(intensity){}

	D3DXCOLOR diffuse;
	float specPower;
	float specIntensity;
};

D3DXVECTOR4 ColorToRGBAVector(D3DCOLOR color);
D3DXVECTOR4 ColorToRGBAVector(D3DXCOLOR _color) ;

bool IsFloatFormat(D3DFORMAT _format);
void GetFormatByteSizePerChannelRGBA(D3DFORMAT _format, D3DXVECTOR4 * retVal);
void DecodePixelData(D3DFORMAT _format, BYTE * pData, D3DXVECTOR4 * retVal);


#endif