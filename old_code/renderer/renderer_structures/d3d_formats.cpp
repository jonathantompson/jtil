#include "renderer\renderer_structures\d3dFormats.h"
#include <stdexcept>

D3DXVECTOR4 ColorToRGBAVector(D3DCOLOR color) 
{
	D3DXCOLOR _color(color); // D3DCOLOR is a four byte value, D3DXCOLOR does conversion for us.
	return D3DXVECTOR4(_color.r,_color.g,_color.b,_color.a);
}

D3DXVECTOR4 ColorToRGBAVector(D3DXCOLOR _color) 
{
	return D3DXVECTOR4(_color.r,_color.g,_color.b,_color.a);
}

bool IsFloatFormat(D3DFORMAT _format)
{
	switch(_format)
	{
	case D3DFMT_R8G8B8:
		return false;
	case D3DFMT_A8R8G8B8:
		return false;
	case D3DFMT_X8R8G8B8:
		return false;
	case D3DFMT_R5G6B5:
		return false;
	case D3DFMT_X1R5G5B5:
		return false;
	case D3DFMT_A1R5G5B5:
		return false;
	case D3DFMT_A4R4G4B4:
		return false;
	case D3DFMT_R3G3B2:
		return false;
	case D3DFMT_A8: 
		return false;
	case D3DFMT_A8R3G3B2:
		return false;
	case D3DFMT_X4R4G4B4: 
		return false;
	case D3DFMT_A2B10G10R10:
		return false;
	case D3DFMT_A8B8G8R8:   
		return false;
	case D3DFMT_X8B8G8R8:   
		return false;
	case D3DFMT_G16R16:    
		return false;
	case D3DFMT_A2R10G10B10:
		return false;
	case D3DFMT_A16B16G16R16:
		return false;
	case D3DFMT_R16F:        
		return true;
	case D3DFMT_G16R16F:     
		return true;
	case D3DFMT_A16B16G16R16F:
		return true;
	case D3DFMT_R32F:         
		return true;
	case D3DFMT_G32R32F:      
		return true;
	case D3DFMT_A32B32G32R32F:
		return true;
	default:
		throw std::runtime_error("IsFloatFormat() - Error: Unrecognised format or return value is not recorded");
	}
}

void GetFormatByteSizePerChannelRGBA(D3DFORMAT _format, D3DXVECTOR4 * retVal)
{
	if(retVal == NULL)
		throw std::runtime_error("GetFormatByteSizePerChannelRGBA() - Error: retVal == NULL");
	// 8bits is 1 byte
	// 16bits is 2 bytes
	// 32bits is 4 bytes (regular float)
	// 64bits is 8 byes
	switch(_format)
	{
	case D3DFMT_R8G8B8:
		retVal->x = 1.0f; retVal->y = 1.0f; retVal->z = 1.0f; retVal->w = 0.0f; 
		break;
	case D3DFMT_A8R8G8B8:
		retVal->x = 1.0f; retVal->y = 1.0f; retVal->z = 1.0f; retVal->w = 1.0f; 
		break;
	case D3DFMT_X8R8G8B8:
		retVal->x = 1.0f; retVal->y = 1.0f; retVal->z = 1.0f; retVal->w = 1.0f; 
		break;
	case D3DFMT_R5G6B5:
		retVal->x = 5.0f/8.0f; retVal->y = 6.0f/8.0f; retVal->z = 5.0f/8.0f; retVal->w = 0.0f; 
		break;
	case D3DFMT_X1R5G5B5:
		retVal->x = 5.0f/8.0f; retVal->y = 5.0f/8.0f; retVal->z = 5.0f/8.0f; retVal->w = 1.0f/8.0f; 
		break;
	case D3DFMT_A1R5G5B5:
		retVal->x = 5.0f/8.0f; retVal->y = 5.0f/8.0f; retVal->z = 5.0f/8.0f; retVal->w = 1.0f/8.0f; 
		break;
	case D3DFMT_A4R4G4B4:
		retVal->x = 4.0f/8.0f; retVal->y = 4.0f/8.0f; retVal->z = 4.0f/8.0f; retVal->w = 4.0f/8.0f; 
		break;
	case D3DFMT_R3G3B2:
		retVal->x = 3.0f/8.0f; retVal->y = 3.0f/8.0f; retVal->z = 2.0f/8.0f; retVal->w = 0.0f; 
		break;
	case D3DFMT_A8: 
		retVal->x = 0.0f; retVal->y = 0.0f; retVal->z = 0.0f; retVal->w = 1.0f; 
		break;
	case D3DFMT_A8R3G3B2:
		retVal->x = 3.0f/8.0f; retVal->y = 3.0f/8.0f; retVal->z = 2.0f/8.0f; retVal->w = 1.0f; 
		break;
	case D3DFMT_X4R4G4B4: 
		retVal->x = 4.0f/8.0f; retVal->y = 4.0f/8.0f; retVal->z = 4.0f/8.0f; retVal->w = 4.0f/8.0f; 
		break;
	case D3DFMT_A2B10G10R10:
		retVal->x = 10.0f/8.0f; retVal->y = 10.0f/8.0f; retVal->z = 10.0f/8.0f; retVal->w = 2.0f/8.0f; 
		break;
	case D3DFMT_A8B8G8R8:   
		retVal->x = 1.0f; retVal->y = 1.0f; retVal->z = 1.0f; retVal->w = 1.0f; 
		break;
	case D3DFMT_X8B8G8R8:   
		retVal->x = 1.0f; retVal->y = 1.0f; retVal->z = 1.0f; retVal->w = 1.0f; 
		break;
	case D3DFMT_G16R16:    
		retVal->x = 2.0f; retVal->y = 2.0f; retVal->z = 0.0f; retVal->w = 0.0f; 
		break;
	case D3DFMT_A2R10G10B10:
		retVal->x = 10.0f/8.0f; retVal->y = 10.0f/8.0f; retVal->z = 10.0f/8.0f; retVal->w = 2.0f/8.0f; 
		break;
	case D3DFMT_A16B16G16R16:
		retVal->x = 2.0f; retVal->y = 2.0f; retVal->z = 2.0f; retVal->w = 2.0f; 
		break;
	case D3DFMT_R16F:        
		retVal->x = 2.0f; retVal->y = 0.0f; retVal->z = 0.0f; retVal->w = 0.0f; 
		break;
	case D3DFMT_G16R16F:     
		retVal->x = 2.0f; retVal->y = 2.0f; retVal->z = 0.0f; retVal->w = 0.0f; 
		break;
	case D3DFMT_A16B16G16R16F:
		retVal->x = 2.0f; retVal->y = 2.0f; retVal->z = 2.0f; retVal->w = 2.0f; 
		break;
	case D3DFMT_R32F:         
		retVal->x = 4.0f; retVal->y = 0.0f; retVal->z = 0.0f; retVal->w = 0.0f; 
		break;
	case D3DFMT_G32R32F:      
		retVal->x = 4.0f; retVal->y = 4.0f; retVal->z = 0.0f; retVal->w = 0.0f; 
		break;
	case D3DFMT_A32B32G32R32F:
		retVal->x = 4.0f; retVal->y = 4.0f; retVal->z = 4.0f; retVal->w = 4.0f; 
		break;
	default:
		throw std::runtime_error("GetFormatByteSizePerChannelRGBA() - Error: Unrecognised format or return value is not recorded");
	}

}

void DecodePixelData(D3DFORMAT _format, BYTE * pData, D3DXVECTOR4 * retVal)
{
	if(retVal == NULL)
		throw std::runtime_error("DecodePixelData() - Error: retVal == NULL");
	// 8bits is 1 byte
	// 16bits is 2 bytes
	// 32bits is 4 bytes (regular float)
	// 64bits is 8 byes
	// All formats are listed from left ot right, most-significant bit to least-significant bit
	// When traversing surface data, the data is stored in memory from least-significant bit to most-significant bit
	D3DXFLOAT16 * pDataFloatHalf;
	float * pDataFloat;
	switch(_format)
	{
	case D3DFMT_R16F:  
		pDataFloatHalf = (D3DXFLOAT16 *)pData;
		retVal->x = (float)*pDataFloatHalf; 
		retVal->y = 0.0f; retVal->z = 0.0f; retVal->w = 0.0f; 
		break;
	case D3DFMT_R32F:  
		pDataFloat = (float *)pData;
		retVal->x = *pDataFloat; 
		retVal->y = 0.0f; retVal->z = 0.0f; retVal->w = 0.0f; 
		break;
	default:
		throw std::runtime_error("DecodePixelData() - Error: Unrecognised format.  Only D3DFMT_R16F & D3DFMT_R32F are supported (lazy).");
	}

}