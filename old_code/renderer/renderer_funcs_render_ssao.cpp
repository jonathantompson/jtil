/*************************************************************
**						 Renderer							**
**		-> Graphics Rendering functions, Summer 2009		**
*************************************************************/
// File:		renderer.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Some free models: http://artist-3d.com/free_3d_models/

#include	"renderer\renderer.h"
#include	"main.h"
#include	"app.h"
#include	"renderer\renderer_structures\drawableTex2D.h"
#include    "objectManager\objectManager.h"
#include	"UI\varNames.h"
#include	"UI\UI.h"
#include	"renderer\utils\d3dProfiler.h"
#include	"renderer\renderer_structures\vertex.h"
#include	"camera\camera.h"
#include	"renderer\renderer_constants.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		InitSSAO												*/
/* Description:	Initialize the SSAO structures							*/
/************************************************************************/
void renderer::InitSSAO()
{
	D3DFORMAT occlusionBufferFormat = (D3DFORMAT)g_UI->GetComboBoxVal( & var_SSAOFormat ); // Default is typically R32F
	// D3DFORMAT depthFormat = (D3DFORMAT)g_UI->GetComboBoxVal( & var_depthStencilFormat );
	// D3DFORMAT backBufferFormat = m_PresentParameters.BackBufferFormat;
	
	//if(FAILED(m_pD3DObj->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, occlusionBufferFormat)))
	//	throw std::runtime_error("renderer::InitSSAO() - Error: The chosen SSAO Format is not supported!");
	
	// Allocate new space
	if(m_occlusionBuffer) { delete m_occlusionBuffer; }
	m_occlusionBuffer = new drawableTex2D;

	// Create samplers
	m_vectorNoiseSampler.SetFilter(D3DTEXF_POINT, D3DTADDRESS_WRAP, 1);
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampRandomNormal, &curState.randomNormalSampler, &m_vectorNoiseSampler),L"renderer::InitSSAO() - SetSamplerState Failed: ");

	m_occlusionBufferSampler.addressU = D3DTADDRESS_CLAMP; m_occlusionBufferSampler.addressV = D3DTADDRESS_CLAMP; 
	m_occlusionBufferSampler.magFilter = D3DTEXF_LINEAR; m_occlusionBufferSampler.minFilter = D3DTEXF_LINEAR; m_occlusionBufferSampler.mipFilter = D3DTEXF_LINEAR;
	m_occlusionBufferSampler.maxAnisotropy = 1;

	// Make new drawable textures
	UINT _height = m_PresentParameters.BackBufferHeight;
	UINT _width = m_PresentParameters.BackBufferWidth;
	UINT _mipLevels = 1;

	bool autogenMipMaps = false;
	m_occlusionBuffer->InitDrawableTex2D(_width, _height, _mipLevels, occlusionBufferFormat, autogenMipMaps);

	// Load in the vector noise texture
	UINT _numMips = 1;
	std::wstring path = std::wstring(L"models\\vectorNoise_128x128.png");
	D3DXIMAGE_INFO pSrcInfo;
	LoadTexture(path.c_str(), RANDOM_NORMAL_FORMAT, _numMips, &m_vectorNoiseTexture);
	HR(D3DXGetImageInfoFromFile(path.c_str(), &pSrcInfo),L"renderer::InitSSAO() - D3DXGetImageInfoFromFile failed: ");
	
	// Set the textures and sampling in the post processing effect
	HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gOcclusionBuffer, m_occlusionBuffer->d3dTex()),L"renderer::InitSSAO() - Set Texture h_gOcclusionBuffer Failed: ");
	HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gRandomNormal, m_vectorNoiseTexture),L"renderer::InitSSAO() - Set Texture h_gRandomNormal Failed: ");

	// Set the SSAO values that are static in the post processing effect
	float _SSAOSampleRadius = g_UI->GetSetting<float>(& var_SSAOSampleRadius);
	float _SSAOBias = g_UI->GetSetting<float>(& var_SSAOBias);
	float _SSAOIntensity = g_UI->GetSetting<float>(& var_SSAOIntensity);
	float _SSAOScale = g_UI->GetSetting<float>(& var_SSAOScale);
	D3DXVECTOR2 _widthheight((float)_width, (float)_height);
	D3DXVECTOR2 _noisewidthheight((float)pSrcInfo.Width, (float)pSrcInfo.Height);
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSSAOSampleRadius, &_SSAOSampleRadius, sizeof(float)),L"renderer::InitSSAO() - Set Value h_gSSAOSampleRadius Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSSAOBias, &_SSAOBias, sizeof(float)),L"renderer::InitSSAO() - Set Value h_gSSAOBias Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSSAOIntensity, &_SSAOIntensity, sizeof(float)),L"renderer::InitSSAO() - Set Value h_gSSAOIntensity Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSSAOScale, &_SSAOScale, sizeof(float)),L"renderer::InitSSAO() - Set Value h_gSSAOScale Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gScreenSize, &_widthheight, sizeof(D3DXVECTOR2)),L"renderer::InitSSAO() - Set Value h_gScreenSize Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gVectorNoiseSize, &_noisewidthheight, sizeof(D3DXVECTOR2)),L"renderer::InitSSAO() - Set Value h_gVectorNoiseSize Failed: ");
}

/************************************************************************/
/* Name:		UpdateSSAO												*/
/* Description:	Update the SSAO effects variables						*/
/************************************************************************/
void renderer::UpdateSSAO()
{
	float _SSAOSampleRadius = g_UI->GetSetting<float>(& var_SSAOSampleRadius);
	float _SSAOBias = g_UI->GetSetting<float>(& var_SSAOBias);
	float _SSAOIntensity = g_UI->GetSetting<float>(& var_SSAOIntensity);
	float _SSAOScale = g_UI->GetSetting<float>(& var_SSAOScale);
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSSAOSampleRadius, &_SSAOSampleRadius, sizeof(float)),L"renderer::UpdateSSAO() - Set Value h_gSSAOSampleRadius Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSSAOBias, &_SSAOBias, sizeof(float)),L"renderer::UpdateSSAO() - Set Value h_gSSAOBias Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSSAOIntensity, &_SSAOIntensity, sizeof(float)),L"renderer::UpdateSSAO() - Set Value h_gSSAOIntensity Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSSAOScale, &_SSAOScale, sizeof(float)),L"renderer::UpdateSSAO() - Set Value h_gSSAOScale Failed: ");
}

/************************************************************************/
/* Name:		DrawOcclusionBuffer										*/
/* Description:	Calculate screen-space occlusion, ambient term			*/
/************************************************************************/
void renderer::DrawOcclusionBuffer()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif

	// Set the post processing general texture to the noise source
	HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource, m_vectorNoiseTexture),L"renderer::DrawOcclusionBuffer() - Set Texture h_gSource Failed: ");

	m_occlusionBuffer->BeginScene();

	int _numberSamples = g_UI->GetSetting<int>( & var_SSAONumberSamples );
	UINT passNumber;
	switch(_numberSamples)
	{
	case 4:
		passNumber = 0;
		break;
	case 8:
		passNumber = 1;
		break;
	case 16:
		passNumber = 2;
		break;
	default:
		throw std::runtime_error("renderer::DrawOcclusionBuffer() - SSAONumberSamples must be 4, 8 or 16!");
	}

	RenderFullScreenQuad(m_FXPostProcess, POSTPROCESS, (UINT) OCCLUSIONMAP_TECH, true, OCCLUSION_CLEAR_COLOR, passNumber );
	if(curState.occlusionCleared)
		curState.occlusionCleared = false;

	m_occlusionBuffer->EndScene();
}

/************************************************************************/
/* Name:		DrawOcclusionBuffer										*/
/* Description:	Calculate screen-space occlusion, ambient term			*/
/************************************************************************/
void renderer::ClearOcclusionBuffer()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif

	if(curState.occlusionCleared == false)
	{
		ClearTexture(m_occlusionBuffer->d3dTex(), OCCLUSION_CLEAR_COLOR);
		curState.occlusionCleared = true;
	}
}