/*************************************************************
**						 Renderer							**
**		-> Graphics Rendering functions, Summer 2009		**
*************************************************************/
// File:		renderer.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include	"renderer\renderer.h"
#include	"main.h"
#include	"renderer\renderer_structures\drawableTex2D.h"
#include	"renderer\renderer_structures\drawableTexAtlas2D.h"
#include	"objectManager\objectManager.h"
#include	"lights\light.h"
#include	"renderer\renderer_structures\vertex.h"
#include	"renderer\renderer_structures\debugObject.h"
#include	"physics\objects\rbobjectMeshData.h"
#include	"UI\varNames.h"
#include	"UI\UI.h"
#include	"camera\camera.h"
#include	"renderer\utils\d3dProfiler.h"
#include	<limits>
#include	"renderer\renderer_constants.h"
#include	"clk\clk.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST
 
#ifndef lerp
	#define lerp(t, a, b) ( a + t * (b - a) )
#endif

/************************************************************************/
/* Name:		InitPostProcessing										*/
/* Description: Draw all the cascaded shadow maps						*/
/************************************************************************/
void renderer::InitPostProcessing()
{
	// Check we can support 32bit floating point render targets
	D3DFORMAT backBufferFormat = m_PresentParameters.BackBufferFormat;
	D3DFORMAT lumianceFormat;
	if(FAILED(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), backBufferFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_R16F)))
		if(FAILED(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), backBufferFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_R32F)))
			throw std::runtime_error("renderer::InitPostProcessing() - Error: Neither D3DFMT_R16F or D3DFMT_R32F are supported!");
		else
			lumianceFormat = D3DFMT_R32F;
	else
		lumianceFormat = D3DFMT_R16F;

	// Get some values first
	D3DFORMAT sceneFormat = (D3DFORMAT)g_UI->GetComboBoxVal(& var_SceneFormat);
	UINT _height = m_PresentParameters.BackBufferHeight;
	UINT _width = m_PresentParameters.BackBufferWidth;
	UINT _mipLevels = 1;
	bool autogenMipMaps = false;

	// We want a scene buffer with 1/16 resolution --> Bloom filtering will be applied to this.
	// To get there we first need to downconvert by 1/4, so we need this texture as well.
	if(m_sceneBuffer_1_4th_WL) { delete m_sceneBuffer_1_4th_WL; m_sceneBuffer_1_4th_WL = NULL; } 
	m_sceneBuffer_1_4th_WL = new drawableTex2D();
	m_sceneBuffer_1_4th_WL->InitDrawableTex2D(_width / 4, _height / 4, _mipLevels, sceneFormat, autogenMipMaps);
	for(UINT i = 0; i < 2; i ++)
	{
		if(m_sceneBuffer_1_16th_WL[i]) 
		{ delete m_sceneBuffer_1_16th_WL[i]; m_sceneBuffer_1_16th_WL[i] = NULL; } 
		m_sceneBuffer_1_16th_WL[i] = new drawableTex2D();
		m_sceneBuffer_1_16th_WL[i]->InitDrawableTex2D(_width / 16, _height / 16, _mipLevels, sceneFormat, autogenMipMaps);
	
	}
	
	// Setup the luminance measurement structures:
	if(m_currentFrameLuminance) { delete m_currentFrameLuminance; m_currentFrameLuminance = NULL; } 
	m_currentFrameLuminance = new drawableTex2D();
	if(m_currentFrameAdaptedLuminance) { delete m_currentFrameAdaptedLuminance; m_currentFrameAdaptedLuminance = NULL; } 
	m_currentFrameAdaptedLuminance = new drawableTex2D();
	if(m_lastFrameAdaptedLuminance) { delete m_lastFrameAdaptedLuminance; m_lastFrameAdaptedLuminance = NULL; } 
	m_lastFrameAdaptedLuminance = new drawableTex2D();

	// Now initialize the targets
	m_currentFrameLuminance->InitDrawableTex2D(1, 1, _mipLevels, lumianceFormat, autogenMipMaps);
	m_currentFrameAdaptedLuminance->InitDrawableTex2D(1, 1, _mipLevels, lumianceFormat, autogenMipMaps);
	m_lastFrameAdaptedLuminance->InitDrawableTex2D(1, 1, _mipLevels, lumianceFormat, autogenMipMaps);

	// Work out how long the luminance chain needs to be (we'll reduce resolution by 4 each time)
	// We start calculating luminance AFTER already downsampling twice by 1/4 width and height (so 1/4*1/4 = 1/16)
	// We need to get within 1/4 dimentions of the scaled down scene buffer and we'll use a linear filter on source target to interpolate.
	// So if scene buffer is 1280 x 720 --> After downscaling it will be 80 x 45.  So first luminance target is 16 x 16.
	m_luminanceChainLength = 1;
    UINT startSize = max(_width / 16, _height / 16);
    UINT size = 16;
    for (size = 16; size < startSize; size *= 4)
        m_luminanceChainLength++;
	size /= 4; // Size goes 1 multiple of 4 too far.
	m_luminanceChainLength++; // Add other texture for the first liminance value.

	if(m_luminanceChain) { delete [] m_luminanceChain; m_luminanceChain = NULL; } 
	m_luminanceChain = new drawableTex2D[m_luminanceChainLength];
	m_luminanceChain[0].InitDrawableTex2D(_width / 16, _height / 16, _mipLevels, lumianceFormat, autogenMipMaps);
	for(UINT i = 1; i < m_luminanceChainLength; i ++)
	{
		m_luminanceChain[i].InitDrawableTex2D(size, size, _mipLevels, lumianceFormat, autogenMipMaps);
		size /= 4;
	}

	// Initialize depth of field values
	float dx = 0.5f / (float)_width; // Scale tap offsets based on render target size
	float dy = 0.5f / (float)_height;

	// Generate the texture coordinate offsets for our disc
	m_DOFDiskOffsets = new D3DXVECTOR2[12];
	m_DOFDiskOffsets[0] = D3DXVECTOR2(-0.326212f * dx, -0.40581f * dy);
	m_DOFDiskOffsets[1] = D3DXVECTOR2(-0.840144f * dx, -0.07358f * dy);
	m_DOFDiskOffsets[2] = D3DXVECTOR2(-0.840144f * dx, 0.457137f * dy);
	m_DOFDiskOffsets[3] = D3DXVECTOR2(-0.203345f * dx, 0.620716f * dy);
	m_DOFDiskOffsets[4] = D3DXVECTOR2(0.96234f * dx, -0.194983f * dy);
	m_DOFDiskOffsets[5] = D3DXVECTOR2(0.473434f * dx, -0.480026f * dy);
	m_DOFDiskOffsets[6] = D3DXVECTOR2(0.519456f * dx, 0.767022f * dy);
	m_DOFDiskOffsets[7] = D3DXVECTOR2(0.185461f * dx, -0.893124f * dy);
	m_DOFDiskOffsets[8] = D3DXVECTOR2(0.507431f * dx, 0.064425f * dy);
	m_DOFDiskOffsets[9] = D3DXVECTOR2(0.89642f * dx, 0.412458f * dy);
	m_DOFDiskOffsets[10] = D3DXVECTOR2(-0.32194f * dx, -0.932615f * dy);
	m_DOFDiskOffsets[11] = D3DXVECTOR2(-0.791559f * dx, -0.59771f * dy);
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gDOFDiskOffsets, m_DOFDiskOffsets, 12*sizeof(D3DXVECTOR2)),L"renderer::InitPostProcessing() - Set Value h_gDOFDiskOffsets Failed: ");

	SetPostProcessingTextures();

	// Set the sampler states
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::InitPostProcessing() - SetSamplerState Failed: ");
}

/************************************************************************/
/* Name:		PostProcessing											*/
/* Description:	Perform post processing after rendering the scene		*/
/************************************************************************/
void renderer::PostProcessing()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	bool _bloomEnable = g_UI->GetSetting<bool>( & var_HDRBloomEnable );
	bool _DOFEnable = g_UI->GetSetting<bool>( & var_DOFEnable );
	bool _manualLuminance = g_UI->GetSetting<bool>( & var_HDRManualLuminanceEnable);
	bool _displayLuminance = g_UI->GetSetting<bool>(& var_HDRDisplayLumianceValue);

	if(_DOFEnable)
	{
		PerformDOFBlur(m_sceneBuffer[0], m_sceneBuffer[1], BLACK);
		SwapDrawableTex2D(&m_sceneBuffer[1], &m_sceneBuffer[0]);
	}

	if(g_UI->GetSetting<bool>( & var_MotionBlurEnable ))
	{
		// Borrow the render target "m_sceneBuffer2", to avoid making another one, then ping-pong targets when finished
		PerformMotionBlur(m_sceneBuffer[0], m_sceneBuffer[1], BLACK);
		SwapDrawableTex2D(&m_sceneBuffer[1], &m_sceneBuffer[0]);
	}

	if(g_UI->GetSetting<bool>( & var_HDREnable ))
	{
		// First downscale the scene buffer by 1/16th width and length
		DownscaleTexture4x4(m_sceneBuffer[0], m_sceneBuffer_1_4th_WL, SCENE_CLEAR_COLOR, false);
		DownscaleTexture4x4(m_sceneBuffer_1_4th_WL, m_sceneBuffer_1_16th_WL[0], SCENE_CLEAR_COLOR, false);

		// First set some constants in the shader
		SetToneMapMiddleGrey(g_UI->GetSetting<float>( & var_HDRToneMapMiddleGrey ));
		SetToneMapWhiteSq(g_UI->GetSetting<float>( & var_HDRToneMapWhiteSq ));

		// Find the average luminance value if we're not putting it in manually OR we want it displayed
		if(!_manualLuminance || _displayLuminance)
			CalculateAverageLuminance(m_sceneBuffer_1_16th_WL[0], (float)g_clk->m_deltaTime, _manualLuminance); // g_clk->m_deltaTime --> Delta rendering time.

		// Either set the manual lumiance value or set the texture for adapted lumiance
		if(_manualLuminance)
			SetHDRManualLuminance(g_UI->GetSetting<float>( & var_HDRManualLuminance) );
		else
			HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource2, m_currentFrameAdaptedLuminance->d3dTex()),L"renderer::PostProcessing() - Set Texture h_gSource2 Failed: ")

		if(_bloomEnable)
		{
			// Create the bloom effect step 1: Tone map downconverted scene and apply a threshold
			ApplyToneMapThreshold(m_sceneBuffer_1_16th_WL[0], m_sceneBuffer_1_16th_WL[1], BLACK, g_UI->GetSetting<float>(& var_HDRBloomThreshold ), _manualLuminance);

			// Create the bloom effect step 2: Blur it using a gaussian filter
			GaussianBlurTextureHorizontal(m_sceneBuffer_1_16th_WL[1], m_sceneBuffer_1_16th_WL[0], BLACK, 
								          g_UI->GetSetting<float>(& var_HDRBloomBlurSigma ), g_UI->GetSetting<int>(& var_HDRBloomBlurRadius ));
			GaussianBlurTextureVertical(m_sceneBuffer_1_16th_WL[0], m_sceneBuffer_1_16th_WL[1], BLACK, 
								        g_UI->GetSetting<float>(& var_HDRBloomBlurSigma ), g_UI->GetSetting<int>(& var_HDRBloomBlurRadius ));

			// Upscale bloom to 1/4 scene buffer size (final upscale will be done when rendering HDR scene)
			UpscaleTexture4x4(m_sceneBuffer_1_16th_WL[1], m_sceneBuffer_1_4th_WL, BLACK);
		}

		// Now do tone mapping on the main source image, and add in the bloom
		PerformToneMapping(m_sceneBuffer[1], m_sceneBuffer[0], m_currentFrameAdaptedLuminance, m_sceneBuffer_1_4th_WL, BLACK, _bloomEnable, _manualLuminance);

		// Swap the adapted luminance targets for next frame
		SwapDrawableTex2D(&m_currentFrameAdaptedLuminance, &m_lastFrameAdaptedLuminance);

		// Also swap the sceneBuffer with the HDRToneMapped version so the present function will present the new version...
		SwapDrawableTex2D(&m_sceneBuffer[1], &m_sceneBuffer[0]);
	}

}

/************************************************************************/
/* Name:		SetPostProcessingTextures								*/
/* Description: Set the textures in the post processing effect			*/
/************************************************************************/
void renderer::SetPostProcessingTextures()
{
	if(m_FXPostProcess)
	{
		// Set the textures and sampling in the post processing effect
		HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gDepth, m_viewDepthMap->d3dTex()),L"renderer::SetPostProcessingTextures() - Set Texture h_gDepth Failed: ");
		HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gNormal, m_viewNormalMap->d3dTex()),L"renderer::SetPostProcessingTextures() - Set Texture h_gNormal Failed: ");
		HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gAlbedo, m_albedoMap->d3dTex()),L"renderer::SetPostProcessingTextures() - Set Texture h_gAlbedo Failed: ");
		HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gMisc, m_miscMap->d3dTex()),L"renderer::SetPostProcessingTextures() - Set Texture h_gMisc Failed: ");
		HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampGBuffer, &curState.postProcessingGBufferSampler, &m_gBufferSampler),L"renderer::SetPostProcessingTextures() - SetSamplerState Failed: ");
	}
}

/************************************************************************/
/* Name:		BoxBlurSMTextureAtlas									*/
/* Description: Perform a simple box blur vertically then horizontally  */
/************************************************************************/
void renderer::BoxBlurSMTextureAtlas(drawableTexAtlas2D * texture, drawableTexAtlas2D * tempTexture )
{
#ifdef D3DPROFILE
    PROFILE_BLOCK 
#endif

	// Set sampler state to use no filtering
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::BoxBlurSMTextureAtlas() - SetSamplerState Failed: ");
	
	// Set the correct texel size
	D3DXVECTOR2 texelSize(1.0f / ((float)m_SMSize), 1.0f / ((float)m_SMSize));
	SetPostProcessingTexelSize(& texelSize);

#ifdef _DEBUG 
	if(texture->Width() != tempTexture->Width()|| texture->Height() != tempTexture->Height())
		throw std::runtime_error("renderer::BoxBlurSMTextureAtlas() - Textures are different sizes!");
#endif

	LPDIRECT3DTEXTURE9 tex = texture->d3dTex();
	D3DXVECTOR2 vert(0,1);
	D3DXVECTOR2 horiz(1,0);
	D3DXVECTOR2 size((float)texture->Width(),(float)texture->Height());
	D3DXVECTOR2 TexelSize;
	TexelSize.x = 1.0f / size.x; TexelSize.y = 1.0f / size.y;
	D3DXVECTOR2 * atlasOffsets = texture->GetTextureOffsets();
	D3DXVECTOR2 * atlasScale = texture->GetTextureScale();

// HORIZONTAL BLUR - RENDER FROM TEXTURE INTO TEMPTEXTURE
	tempTexture->BeginSceneFullViewport();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, SMAP_CLEAR_COLOR, 0, 0),L"renderer::BoxBlurSMTextureAtlas() - Failed to clear device: ");
	
    SetPostProcessTechnique(BOXBLUR_SMATLAS_TECH);

	HR(m_pD3DDev->SetVertexDeclaration(VertexPosTex::Decl),L"renderer::BoxBlurSMTextureAtlas() - SetVertexDeclaration failed: ");
	HR(m_pD3DDev->SetStreamSource(0, m_FSQuadVertBuff, 0, sizeof(VertexPosTex)),L"renderer::BoxBlurSMTextureAtlas() - SetStreamSource failed: ")
	HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource, tex),L"renderer::BoxBlurSMTextureAtlas() - Set Texture h_gSource Failed: ")

	for(int curSM = 0; curSM < (int)m_numShadowMaps; curSM ++)
	{
		// Set the correct viewport and the current SM split in the shader
		tempTexture->SetViewport(curSM);
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gCurSMSplit, &curSM, sizeof(int)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSamples Failed: ");
		D3DXVECTOR2 TexClampX = D3DXVECTOR2(atlasOffsets[curSM].x, atlasOffsets[curSM].x + (atlasScale->x * 1.0f) - 0.5f*TexelSize.x); // Add half texel boundry, just in case
		D3DXVECTOR2 TexClampY = D3DXVECTOR2(atlasOffsets[curSM].y, atlasOffsets[curSM].y + (atlasScale->y * 1.0f) - 0.5f*TexelSize.y);
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSampleAtlasClampX, &TexClampX, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSampleAtlasClampX Failed: ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSampleAtlasClampY, &TexClampY, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSampleAtlasClampY Failed: ");
		
		// Work out the filter width for the current SMAP
		D3DXVECTOR2 SplitMinFilterWidth = m_MinFilterWidth * m_ShadowMapSplitScales[curSM];
		int HorizFilterSamples = (int)floor(SplitMinFilterWidth.x + 0.5f);

		// Preshader code - No longer relying on FX compiler to detect preshader code
		float  BlurSizeInv = 1.0f / float(HorizFilterSamples);
		D3DXVECTOR2 BlurSampleOffset;
		BlurSampleOffset.x = TexelSize.x; BlurSampleOffset.y = 0.0f;
		D3DXVECTOR2 BlurBaseTexCoordOffset;
		BlurBaseTexCoordOffset.x = 0.5f * (float)(HorizFilterSamples - 1) * BlurSampleOffset.x; BlurBaseTexCoordOffset.y = 0.0f;

		// Set the internal variables for horizontal blur
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSamples, &HorizFilterSamples, sizeof(int)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSamples Failed: ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSizeInv, &BlurSizeInv, sizeof(float)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSizeInv Failed: ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSampleOffset, &BlurSampleOffset, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSampleOffset Failed: ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurBaseTexCoordOffset, &BlurBaseTexCoordOffset, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurBaseTexCoordOffset Failed: ");

		UINT numPasses = 0;
		HR(m_FXPostProcess->Begin(&numPasses, 0),L"renderer::BoxBlurSMTextureAtlas() - m_FXPostProcess->Begin Failed: ");
		HR(m_FXPostProcess->BeginPass(0),L"renderer::BoxBlurSMTextureAtlas() - m_FXPostProcess->BeginPass Failed: "); // Pass zero is the variable sized blur filter

		HR(m_FXPostProcess->CommitChanges(),L"renderer::BoxBlurSMTextureAtlas() - CommitChanges failed: ");

		// Now draw the Primative
		HR(m_pD3DDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_FSQuadVertBuffSize/3), L"renderer::BoxBlurSMTextureAtlas() - DrawPrimitive failed: ");

		HR(m_FXPostProcess->EndPass(),L"renderer::BoxBlurSMTextureAtlas() - m_FXPostProcess->EndPass Failed: ");
		HR(m_FXPostProcess->End(),L"renderer::BoxBlurSMTextureAtlas() - m_FXPostProcess->End Failed: ");
	}

	tempTexture->EndScene();

// VERTICAL BLUR - RENDER FROM TEMPTEXTURE INTO TEXTURE
	tex = tempTexture->d3dTex();

	texture->BeginSceneFullViewport();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, SMAP_CLEAR_COLOR, 0, 0),L"renderer::BoxBlurSMTextureAtlas() - Failed to clear device: ");

	HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource, tex),L"renderer::BoxBlurSMTextureAtlas() - Set Texture h_gSource Failed: ")
	
	for(int curSM = 0; curSM < (int)m_numShadowMaps; curSM ++)
	{
		// Set the correct viewport and the current SM split in the shader
		tempTexture->SetViewport(curSM);
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gCurSMSplit, &curSM, sizeof(int)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSamples Failed: ");
		D3DXVECTOR2 TexClampX = D3DXVECTOR2(atlasOffsets[curSM].x, atlasOffsets[curSM].x + (atlasScale->x * 1.0f) - 0.5f*TexelSize.x); // Add half texel boundry, just in case
		D3DXVECTOR2 TexClampY = D3DXVECTOR2(atlasOffsets[curSM].y, atlasOffsets[curSM].y + (atlasScale->y * 1.0f) - 0.5f*TexelSize.y);
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSampleAtlasClampX, &TexClampX, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSampleAtlasClampX Failed: ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSampleAtlasClampY, &TexClampY, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSampleAtlasClampY Failed: ");
		
		// Work out the filter width for the current SMAP
		D3DXVECTOR2 SplitMinFilterWidth = m_MinFilterWidth * m_ShadowMapSplitScales[curSM];
		int VertFilterSamples = (int)floor(SplitMinFilterWidth.y + 0.5f);

		// Preshader code - No longer relying on FX compiler to detect preshader code
		float  BlurSizeInv = 1.0f / float(VertFilterSamples);
		D3DXVECTOR2 BlurSampleOffset;
		BlurSampleOffset.x = 0.0f; BlurSampleOffset.y = TexelSize.y;
		D3DXVECTOR2 BlurBaseTexCoordOffset;
		BlurBaseTexCoordOffset.x = 0.0f; BlurBaseTexCoordOffset.y = 0.5f * (float)(VertFilterSamples - 1) * BlurSampleOffset.y; 

		// Set the internal variables for horizontal blur
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSamples, &VertFilterSamples, sizeof(int)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSamples Failed: ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSizeInv, &BlurSizeInv, sizeof(float)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSizeInv Failed: ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSampleOffset, &BlurSampleOffset, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurSampleOffset Failed: ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurBaseTexCoordOffset, &BlurBaseTexCoordOffset, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurSMTextureAtlas() - Set Value h_gBlurBaseTexCoordOffset Failed: ");

		UINT numPasses = 0;
		HR(m_FXPostProcess->Begin(&numPasses, 0),L"renderer::BoxBlurSMTextureAtlas() - m_FXPostProcess->Begin Failed: ");
		HR(m_FXPostProcess->BeginPass(0),L"renderer::BoxBlurSMTextureAtlas() - m_FXPostProcess->BeginPass Failed: "); // Pass zero is the variable sized blur filter

		HR(m_FXPostProcess->CommitChanges(),L"renderer::BoxBlurSMTextureAtlas() - CommitChanges failed: ");

		// Now draw the Primative
		HR(m_pD3DDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_FSQuadVertBuffSize/3), L"renderer::BoxBlurSMTextureAtlas() - DrawPrimitive failed: ");

		HR(m_FXPostProcess->EndPass(),L"renderer::BoxBlurSMTextureAtlas() - m_FXPostProcess->EndPass Failed: ");
		HR(m_FXPostProcess->End(),L"renderer::BoxBlurSMTextureAtlas() - m_FXPostProcess->End Failed: ");
	}

	texture->EndScene();
}


/************************************************************************/
/* Name:		BoxBlurTexture											*/
/* Description: Perform a simple box blur vertically then horizontally  */
/************************************************************************/
void renderer::BoxBlurTexture(drawableTex2D * texture, drawableTex2D * tempTexture, D3DXVECTOR2 * filterWidth, D3DXVECTOR2 * texelSize )
{
#ifdef D3DPROFILE
    PROFILE_BLOCK 
#endif

	// Set sampler state to use no filtering
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::BoxBlurTexture() - SetSamplerState Failed: ");
	
	// Set the correct texel size
	SetPostProcessingTexelSize(texelSize);

#ifdef _DEBUG 
	if(texture->Width() != tempTexture->Width()|| texture->Height() != tempTexture->Height())
		throw std::runtime_error("renderer::BoxBlurTexture() - Textures are different sizes!");
#endif

	LPDIRECT3DTEXTURE9 tex = texture->d3dTex();
	int HorizFilterSamples = (int)floor(filterWidth->x + 0.5f);
	int VertFilterSamples  = (int)floor(filterWidth->y + 0.5f); 
	if (HorizFilterSamples <= 1 && VertFilterSamples <= 1) {
		return;
	}
	D3DXVECTOR2 vert(0,1);
	D3DXVECTOR2 horiz(1,0);
	D3DXVECTOR2 size((float)texture->Width(),(float)texture->Height());
	D3DXVECTOR2 TexelSize;
	TexelSize.x = 1.0f / size.x; TexelSize.y = 1.0f / size.y;

// HORIZONTAL BLUR - RENDER FROM TEXTURE INTO TEMPTEXTURE
	tempTexture->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, SMAP_CLEAR_COLOR, 0, 0),L"renderer::BoxBlurTexture() - Failed to clear device: ");
	
    SetPostProcessTechnique(BOXBLUR_TECH);

	HR(m_pD3DDev->SetVertexDeclaration(VertexPosTex::Decl),L"renderer::BoxBlurTexture() - SetVertexDeclaration failed: ");
	HR(m_pD3DDev->SetStreamSource(0, m_FSQuadVertBuff, 0, sizeof(VertexPosTex)),L"renderer::BoxBlurTexture() - SetStreamSource failed: ")
	HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource, tex),L"renderer::BoxBlurTexture() - Set Texture h_gSource Failed: ")

	// Preshader code - No longer relying on FX compiler to detect preshader code
    float  BlurSizeInv = 1.0f / float(HorizFilterSamples);
    D3DXVECTOR2 BlurSampleOffset;
	BlurSampleOffset.x = TexelSize.x; BlurSampleOffset.y = 0.0f;
	D3DXVECTOR2 BlurBaseTexCoordOffset;
	BlurBaseTexCoordOffset.x = 0.5f * (float)(HorizFilterSamples - 1) * BlurSampleOffset.x; BlurBaseTexCoordOffset.y = 0.0f;

	// Set the internal variables for horizontal blur
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSamples, &HorizFilterSamples, sizeof(int)),L"renderer::BoxBlurTexture() - Set Value h_gBlurSamples Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSizeInv, &BlurSizeInv, sizeof(float)),L"renderer::BoxBlurTexture() - Set Value h_gBlurSizeInv Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSampleOffset, &BlurSampleOffset, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurTexture() - Set Value h_gBlurSampleOffset Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurBaseTexCoordOffset, &BlurBaseTexCoordOffset, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurTexture() - Set Value h_gBlurBaseTexCoordOffset Failed: ");

	UINT numPasses = 0;
	HR(m_FXPostProcess->Begin(&numPasses, 0),L"renderer::BoxBlurTexture() - m_FXPostProcess->Begin Failed: ");
	HR(m_FXPostProcess->BeginPass(0),L"renderer::BoxBlurTexture() - m_FXPostProcess->BeginPass Failed: "); // Pass zero is the variable sized blur filter

	HR(m_FXPostProcess->CommitChanges(),L"renderer::BoxBlurTexture() - CommitChanges failed: ");

	// Now draw the Primative
	HR(m_pD3DDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_FSQuadVertBuffSize/3), L"renderer::BoxBlurTexture() - DrawPrimitive failed: ");

	HR(m_FXPostProcess->EndPass(),L"renderer::BoxBlurTexture() - m_FXPostProcess->EndPass Failed: ");
	HR(m_FXPostProcess->End(),L"renderer::BoxBlurTexture() - m_FXPostProcess->End Failed: ");

	tempTexture->EndScene();

// VERTICAL BLUR - RENDER FROM TEMPTEXTURE INTO TEXTURE
	tex = tempTexture->d3dTex();

	texture->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, SMAP_CLEAR_COLOR, 0, 0),L"renderer::BoxBlurTexture() - Failed to clear device: ");

	HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource, tex),L"renderer::BoxBlurTexture() - Set Texture h_gSource Failed: ")
	
	// Preshader code - No longer relying on FX compiler to detect preshader code
    BlurSizeInv = 1.0f / float(VertFilterSamples);
	BlurSampleOffset.x = 0.0f; BlurSampleOffset.y = TexelSize.y;
	BlurBaseTexCoordOffset.x = 0.0f; BlurBaseTexCoordOffset.y = 0.5f * (float)(VertFilterSamples - 1) * BlurSampleOffset.y; 

	// Set the internal variables for horizontal blur
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSamples, &VertFilterSamples, sizeof(int)),L"renderer::BoxBlurTexture() - Set Value h_gBlurSamples Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSizeInv, &BlurSizeInv, sizeof(float)),L"renderer::BoxBlurTexture() - Set Value h_gBlurSizeInv Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurSampleOffset, &BlurSampleOffset, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurTexture() - Set Value h_gBlurSampleOffset Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBlurBaseTexCoordOffset, &BlurBaseTexCoordOffset, sizeof(D3DXVECTOR2)),L"renderer::BoxBlurTexture() - Set Value h_gBlurBaseTexCoordOffset Failed: ");

	HR(m_FXPostProcess->Begin(&numPasses, 0),L"renderer::BoxBlurTexture() - m_FXPostProcess->Begin Failed: ");
	HR(m_FXPostProcess->BeginPass(0),L"renderer::BoxBlurTexture() - m_FXPostProcess->BeginPass Failed: "); // Pass zero is the variable sized blur filter

	HR(m_FXPostProcess->CommitChanges(),L"renderer::BoxBlurTexture() - CommitChanges failed: ");

	// Now draw the Primative
	HR(m_pD3DDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_FSQuadVertBuffSize/3), L"renderer::BoxBlurTexture() - DrawPrimitive failed: ");

	HR(m_FXPostProcess->EndPass(),L"renderer::BoxBlurTexture() - m_FXPostProcess->EndPass Failed: ");
	HR(m_FXPostProcess->End(),L"renderer::BoxBlurTexture() - m_FXPostProcess->End Failed: ");

	texture->EndScene();
}

/************************************************************************/
/* Name:		RenderShadowMapTexturesToScreen							*/
/* Description: Render the shadowmaps using the temporary surface		*/
/************************************************************************/
// EDIT: NO LONGER SPACING SMAPS OUT, BUT KEPT THE CODE IN CASE I WANT TO LATER
void renderer::RenderShadowMapTexturesToScreen()
{
	// Get screen width
	D3DVIEWPORT9 Viewport;
	HR(m_pD3DDev->GetViewport(&Viewport),L"renderer::RenderShadowMapTexturesToScreen() - GetViewport failed: "); // Get the current viewport (might be different in fullscreen vs. windowed)
	float fAspect = Viewport.Width/(float)Viewport.Height; // aspect ratio so we can scale shadowmap for 1:1 aspect
	// Divide X space into N chunks
	float screenSpaceWidth = DXSCREENSPACE_MAXX - DXSCREENSPACE_MINX;
	//float xWidth = screenSpaceWidth / (float)m_numShadowMaps;
	float xWidth = screenSpaceWidth; // EDIT: NO LONGER SPACING THEM OUT
	if(xWidth > (MAX_SHADOWMAP_WIDTH * screenSpaceWidth))
		xWidth = MAX_SHADOWMAP_WIDTH * screenSpaceWidth;
	float xSpacing = xWidth * SHADOWMAP_SPACING;
	float yWidth = xWidth * fAspect;
	float xScale = (xWidth - 2*xSpacing) / screenSpaceWidth;
	float yScale = xScale * fAspect; // This is 1:1 on the screen
	yScale = yScale * (float)m_shadowMap->Height() / (float)m_shadowMap->Width(); // Texture atlas might not be 1:1
	D3DXVECTOR2 texelSize(0.0f, 0.0f); // Not 1:1 pixel mapping so don't worry about 1/2 texel offset
	
	// for(UINT iSplit = 0; iSplit < m_numShadowMaps; iSplit ++)
	for(UINT iSplit = 0; iSplit < 1; iSplit ++) // EDIT: NO LONGER SPACING THEM OUT
	{
		// Calculate shift and scale matrix to place it in a nice position.
		float xPos = -1.0f + (iSplit*xWidth + 0.5f*xWidth);
		float yPos = -1.0f + 0.5f*yWidth ;
		
		D3DXVECTOR2 translation_vec(xPos/xScale, yPos/yScale); // Need to scale up the translation
		D3DXVECTOR2 scale_vec(xScale, yScale);

		RenderTextureToScreen(0, m_shadowMap->d3dTex(), & translation_vec, & scale_vec, TRANSFORMED2DTEXTURE_TECH, & texelSize );
	}
}	

/************************************************************************/
/* Name:		RenderSSAOPreviews										*/
/* Description:	Render preview textures									*/
/************************************************************************/
void renderer::RenderGBufferPreviews()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	// Get screen width
	D3DVIEWPORT9 Viewport;
	HR(m_pD3DDev->GetViewport(&Viewport),L"renderer::RenderGBufferPreviews() - GetViewport failed: "); // Get the current viewport (might be different in fullscreen vs. windowed
	// Divide X space into 2 chunks
	float numMaps = 5;
	float screenSpaceWidth = DXSCREENSPACE_MAXX - DXSCREENSPACE_MINX;
	float xWidth = screenSpaceWidth / numMaps;
	if(xWidth > (MAX_SSAO_PREVIEW_WIDTH * screenSpaceWidth))
		xWidth = MAX_SSAO_PREVIEW_WIDTH * screenSpaceWidth;
	float xSpacing = xWidth * SSAO_PREVIEW_SPACING;
	float yWidth = xWidth;
	float xScale = (xWidth - 2*xSpacing) / screenSpaceWidth;
	float yScale = xScale;
	D3DXVECTOR2 texelSize(0.0f, 0.0f); // Not 1:1 pixel mapping so don't worry about 1/2 texel offset

	// Render Depth map
	UINT iSplit = 0;
	float xPos = 1.0f - (iSplit*xWidth + 0.5f*xWidth);
	float yPos = 1.0f - 0.5f*yWidth ;
	D3DXVECTOR2 translation_vec(xPos/xScale, yPos/yScale); // Need to scale up the translation
	D3DXVECTOR2 scale_vec(xScale, yScale);
	RenderTextureToScreen(0, m_viewDepthMap->d3dTex(), & translation_vec, & scale_vec, TRANSFORMED1DTEXTURE_TECH, &texelSize );

	// Render Normal map
	iSplit = 1;
	xPos = 1.0f - (iSplit*xWidth + 0.5f*xWidth);
	yPos = 1.0f - 0.5f*yWidth ;
	translation_vec = D3DXVECTOR2(xPos/xScale, yPos/yScale); // Need to scale up the translation
	scale_vec = D3DXVECTOR2(xScale, yScale);
	RenderTextureToScreen(0, m_viewNormalMap->d3dTex(), & translation_vec, & scale_vec, TRANSFORMED2DTEXTURE_TECH, &texelSize );

	// Render Albedo map
	iSplit = 2;
	xPos = 1.0f - (iSplit*xWidth + 0.5f*xWidth);
	yPos = 1.0f - 0.5f*yWidth ;
	translation_vec = D3DXVECTOR2(xPos/xScale, yPos/yScale); // Need to scale up the translation
	scale_vec = D3DXVECTOR2(xScale, yScale);
	RenderTextureToScreen(0, m_albedoMap->d3dTex(), & translation_vec, & scale_vec, TRANSFORMED3DTEXTURE_TECH, &texelSize );

	// Render Misc map
	iSplit = 3;
	xPos = 1.0f - (iSplit*xWidth + 0.5f*xWidth);
	yPos = 1.0f - 0.5f*yWidth ;
	translation_vec = D3DXVECTOR2(xPos/xScale, yPos/yScale); // Need to scale up the translation
	scale_vec = D3DXVECTOR2(xScale, yScale);
	RenderTextureToScreen(0, m_miscMap->d3dTex(), & translation_vec, & scale_vec, TRANSFORMED2DTEXTURE_TECH, &texelSize );

	// Render Occlusion map
	iSplit = 4;
	xPos = 1.0f - (iSplit*xWidth + 0.5f*xWidth);
	yPos = 1.0f - 0.5f*yWidth ;
	translation_vec = D3DXVECTOR2(xPos/xScale, yPos/yScale); // Need to scale up the translation
	scale_vec = D3DXVECTOR2(xScale, yScale);
	RenderTextureToScreen(0, m_occlusionBuffer->d3dTex(), & translation_vec, & scale_vec, TRANSFORMED1DTEXTURE_TECH, &texelSize );

}

/************************************************************************/
/* Name:		RenderGBufferPreviewFullScreen							*/
/* Description:	Render preview textures									*/
/************************************************************************/
void renderer::RenderGBufferPreviewFullScreen(int bufferIndex)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	IDirect3DTexture9 * tex;
	POSTPROCESS_TECH tech;
	switch(bufferIndex)
	{
	case 1:
		tex = NULL; // Textures are hard coded
		tech = QUADPOSITIONTEXTURE_TECH;
		break;
	case 2:
		tex = m_viewNormalMap->d3dTex();
		tech = QUADNORMALTEXTURE_TECH;
		break;
	case 3:
		tex = m_albedoMap->d3dTex();
		tech = TRANSFORMED3DTEXTURE_TECH;
		break;
	case 4:
		tex = m_miscMap->d3dTex();
		tech = TRANSFORMED2DTEXTURE_TECH;
		break;
	case 5:
		tex = m_occlusionBuffer->d3dTex();
		tech = TRANSFORMED1DTEXTURE_TECH;
		break;
	case 6:
		tex = m_vectorNoiseTexture;
		tech = TRANSFORMED3DTEXTURE_TECH;
		break;
	case 7:
		tex = m_accumulationBuffer->d3dTex();
		tech = TRANSFORMED3DTEXTURE_TECH;
		break;
	case 8:
		tex = m_miscMap->d3dTex();
		tech = QUADVELOCITYTEXTURE_TECH;
		break;
	default:
		throw std::runtime_error("renderer::RenderGBufferPreviewFullScreen() - Incorrect buffer index.  Please input 1 thru 8.");
	}

	// Get screen width
	D3DVIEWPORT9 Viewport;
	HR(m_pD3DDev->GetViewport(&Viewport),L"renderer::RenderGBufferPreviewFullScreen() - GetViewport failed: "); // Get the current viewport (might be different in fullscreen vs. windowed
	D3DXVECTOR2 texelSize(1.0f / ((float)Viewport.Width), 1.0f / ((float)Viewport.Height));

	D3DXVECTOR2 translation_vec(0.0f, 0.0f); // Need to scale up the translation
	D3DXVECTOR2 scale_vec(1.0f, 1.0f);
	RenderTextureToScreen(0, tex, & translation_vec, & scale_vec, tech, & texelSize );
}

/************************************************************************/
/* Name:		RenderTextureToScreen									*/
/* Description: Render a texture to screenspace							*/
/*        The texture will fill entire viewport if scale_vec = 1		*/
/************************************************************************/
void renderer::RenderTextureToScreen(UINT pass, LPDIRECT3DTEXTURE9 tex, D3DXVECTOR2 * translation_vec, D3DXVECTOR2 * scale_vec, POSTPROCESS_TECH tech, D3DXVECTOR2 * texelSize )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif

	// Set the correct texel size
	if(texelSize != NULL)
		SetPostProcessingTexelSize(texelSize);

	HR(m_pD3DDev->SetVertexDeclaration(VertexPosTex::Decl),L"renderer::RenderTextureToScreen() - SetVertexDeclaration failed: ");
	HR(m_pD3DDev->SetStreamSource(0, m_FSQuadVertBuff, 0, sizeof(VertexPosTex)),L"renderer::RenderTextureToScreen() - SetStreamSource failed: ")
	if(tex != NULL)
		HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource, tex),L"renderer::RenderTextureToScreen() - Set Texture h_gSource Failed: ")
	SetPostProcessTechnique(tech);

	// Set the translation and scale matricies
	if(translation_vec != NULL && scale_vec != NULL)
	{
		D3DXMATRIXA16 translation_mat, scale_mat, m_temp_mat;
		D3DXMatrixTranslation(& translation_mat, translation_vec->x, translation_vec->y, 0.0f);
		D3DXMatrixScaling(& scale_mat, scale_vec->x, scale_vec->y, 1.0f);
		D3DXMatrixMultiply(&m_temp_mat, &translation_mat, &scale_mat); 
		HR(m_FXPostProcess->SetMatrix(m_FXHandlesPostProcess.h_gWVP, &m_temp_mat), L"renderer::RenderTextureToScreen() - Failed to set h_gWVP matrix: ");
	}

	UINT numPasses = 0;
	HR(m_FXPostProcess->Begin(&numPasses, 0),L"renderer::RenderTextureToScreen() - m_FXPostProcess->Begin Failed: ");
	HR(m_FXPostProcess->BeginPass(pass),L"renderer::RenderTextureToScreen() - m_FXPostProcess->BeginPass Failed: ");

	HR(m_FXPostProcess->CommitChanges(),L"renderer::RenderTextureToScreen() - CommitChanges failed: ");

	// Now draw the Primative
	HR(m_pD3DDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_FSQuadVertBuffSize/3), L"renderer::RenderTextureToScreen() - DrawPrimitive failed: ");

	HR(m_FXPostProcess->EndPass(),L"renderer::RenderTextureToScreen() - m_FXPostProcess->EndPass Failed: ");
	HR(m_FXPostProcess->End(),L"renderer::RenderTextureToScreen() - m_FXPostProcess->End Failed: ");

}	

/************************************************************************/
/* Name:		SetPostProcessTechnique									*/
/* Description:	Set the desired technique								*/
/************************************************************************/
void renderer::SetPostProcessTechnique(POSTPROCESS_TECH _technique)
{
	if(curState.postProcess_tech == _technique) // Avoid redundant set render states
		return; 
	switch(_technique)
	{
	case QUADTEXTURE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_QuadTexture_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_QuadTexture_Tech: ");
		curState.postProcess_tech = QUADTEXTURE_TECH;
		break;
	case QUADPOSITIONTEXTURE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_QuadPositionTexture_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_QuadPositionTexture_Tech: ");
		curState.postProcess_tech = QUADPOSITIONTEXTURE_TECH;
		break;
	case QUADNORMALTEXTURE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_QuadNormalTexture_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_QuadNormalTexture_Tech: ");
		curState.postProcess_tech = QUADNORMALTEXTURE_TECH;
		break;
	case QUADVELOCITYTEXTURE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_QuadVelocityTexture_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_QuadVelocityTexture_Tech: ");
		curState.postProcess_tech = QUADVELOCITYTEXTURE_TECH;
		break;
	case TRANSFORMEDTEXTURE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_TransformedTexture_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_TransformedTexture_Tech: ");
		curState.postProcess_tech = TRANSFORMEDTEXTURE_TECH;
		break;
	case TRANSFORMED1DTEXTURE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_Transformed1DTexture_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_Transformed1DTexture_Tech: ");
		curState.postProcess_tech = TRANSFORMED1DTEXTURE_TECH;
		break;
	case TRANSFORMED2DTEXTURE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_Transformed2DTexture_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_Transformed1DTexture_Tech: ");
		curState.postProcess_tech = TRANSFORMED2DTEXTURE_TECH;
		break;
	case TRANSFORMED3DTEXTURE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_Transformed3DTexture_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_Transformed3DTexture_Tech: ");
		curState.postProcess_tech = TRANSFORMED3DTEXTURE_TECH;
		break;
	case OCCLUSIONMAP_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_OcclusionMap_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_OcclusionMap_Tech: ");
		curState.postProcess_tech = OCCLUSIONMAP_TECH;
		break;
	case BOXBLUR_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_BoxBlur_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_BoxBlur_Tech: ");
		curState.postProcess_tech = BOXBLUR_TECH;
		break;
	case BOXBLUR_SMATLAS_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_BoxBlur_SMAtlas_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_BoxBlur_SMAtlas_Tech: ");
		curState.postProcess_tech = BOXBLUR_SMATLAS_TECH;
		break;
	case DOWNSCALE_4x4_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_DownScale_4x4_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_DownScale_4x4_Tech: ");
		curState.postProcess_tech = DOWNSCALE_4x4_TECH;
		break;
	case UPSCALE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_Upscale_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_Upscale_Tech: ");
		curState.postProcess_tech = UPSCALE_TECH;
		break;
	case DOWNSCALE_NxM_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_DownScale_NxM_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_DownScale_NxM_Tech: ");
		curState.postProcess_tech = DOWNSCALE_NxM_TECH;
		break;
	case CALCLUMINANCE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_CalcLuminance_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_CalcLuminance_Tech: ");
		curState.postProcess_tech = CALCLUMINANCE_TECH;
		break;
	case CALCADAPTEDLUMINANCE_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_CalcAdaptedLuminance_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_CalcAdaptedLuminance_Tech: ");
		curState.postProcess_tech = CALCADAPTEDLUMINANCE_TECH;
		break;
	case TONEMAP_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_ToneMap_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_ToneMap_Tech: ");
		curState.postProcess_tech = TONEMAP_TECH;
		break;
	case APPLYTONEMAPTHRESHOLD_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_ApplyToneMapThreshold_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_ApplyToneMapThreshold_Tech: ");
		curState.postProcess_tech = APPLYTONEMAPTHRESHOLD_TECH;
		break;
	case GAUSSIANBLUR_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_GaussianBlur_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_GaussianBlur_Tech: ");
		curState.postProcess_tech = GAUSSIANBLUR_TECH;
		break;
	case MOTIONBLUR_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_MotionBlur_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_MotionBlur_Tech: ");
		curState.postProcess_tech = MOTIONBLUR_TECH;
		break;
	case DEPTHOFFIELD_BLURDISK_TECH:
		HR(m_FXPostProcess->SetTechnique(m_FXHandlesPostProcess.h_DepthOfField_BlurDisk_Tech), L"renderer::SetPostProcessTechnique() - Failed to set h_DepthOfField_BlurDisk_Tech: ");
		curState.postProcess_tech = DEPTHOFFIELD_BLURDISK_TECH;
		break;
	default:
		throw std::runtime_error("renderer::SetPostProcessTechnique() - Technique not recognised");
	}
}

/************************************************************************/
/* Name:		SetPostProcessingTexelSize								*/
/* Description:	Set the FX parameters for Texel and Texture size		*/
/************************************************************************/
void renderer::SetPostProcessingTexelSize(D3DXVECTOR2 * _texelSize)
{
	if(curState.postProcessingTexelSize.x != _texelSize->x ) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gTexelSizeX, &_texelSize->x, sizeof(float)), L"renderer::SetPostProcessingTexelSize() - Set Value h_gTexelSizeX Failed: ");
		curState.postProcessingTexelSize.x = _texelSize->x;
	}
	if(curState.postProcessingTexelSize.y != _texelSize->y ) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gTexelSizeY, &_texelSize->y, sizeof(float)), L"renderer::SetPostProcessingTexelSize() - Set Value h_gTexelSizeY Failed: ");
		curState.postProcessingTexelSize.y = _texelSize->y;
	}
}

/************************************************************************/
/* Name:		DownscaleTexture										*/
/* Description:	Down scale texture to 1/16th size (width /4, height/4) 	*/
/************************************************************************/
void renderer::DownscaleTexture4x4(drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor, bool decodeLuminance )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif

#ifdef _DEBUG
	if(((source->Width()) / 4 != dest->Width()) || ((source->Height()) / 4 != dest->Height()))
		throw std::runtime_error("renderer::DownscaleTexture() - Error: Source and destination textures are not of compatable size");
#endif

	// Set the sample sizes in texture coordinates
	D3DXVECTOR4 horizontalSamples(-1.5f, -0.5f, 0.5f, 1.5f);
	D3DXVECTOR4 verticalSamples(-1.5f, -0.5f, 0.5f, 1.5f);
	horizontalSamples /= (float)source->Width();
	verticalSamples /= (float)source->Height();
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gHorizontalSamples, &horizontalSamples, sizeof(D3DXVECTOR4)), L"renderer::DownscaleTexture() - Set Value h_gHorizontalSamples Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gVerticalSamples, &verticalSamples, sizeof(D3DXVECTOR4)), L"renderer::DownscaleTexture() - Set Value h_gVerticalSamples Failed: ");
	
	// Set sampler state to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::DownscaleTexture() - SetSamplerState Failed: ");
	
	dest->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::DownscaleTexture() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) dest->Width(), 1.0f / (float) dest->Height());
	UINT pass;
	if(decodeLuminance)
		pass = 1;
	else
		pass = 0;
	RenderTextureToScreen(pass, source->d3dTex(), NULL, NULL, DOWNSCALE_4x4_TECH, & destTexelSize);

	dest->EndScene();

}

/************************************************************************/
/* Name:		UpscaleTexture4x4										*/
/* Description:	Upscale the texture to 16th size (width *4, height*4) 	*/
/************************************************************************/
void renderer::UpscaleTexture4x4(drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	
#ifdef _DEBUG
	if(((float)source->Width() - (float)dest->Width()/4.0f > 1) || 
	   (((float)source->Height()) - (float)dest->Height()/4.0f > 1))
		throw std::runtime_error("renderer::UpscaleTexture4x4() - Error: Source and destination textures are not of compatable size");
#endif
	// Send the texture and texel size so bilinear interpolation can interpolate between two values
	D3DXVECTOR2 _textureSize = D3DXVECTOR2((float)source->Width(),(float)source->Height());
	D3DXVECTOR2 _texelSize = D3DXVECTOR2(1.0f/_textureSize.x, 1.0f/_textureSize.y);
	SetPostProcessBilinearTexelSize(& _texelSize);
	SetPostProcessBilinearTextureSize(& _textureSize);

	// Set sampler state to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::UpscaleTexture4x4() - SetSamplerState Failed: ");
	
	dest->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::UpscaleTexture4x4() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) dest->Width(), 1.0f / (float) dest->Height());
	RenderTextureToScreen(0, source->d3dTex(), NULL, NULL, UPSCALE_TECH, & destTexelSize);

	dest->EndScene();
	
}

/************************************************************************/
/* Name:		UpscaleTexture4x4										*/
/* Description:	Upscale the texture to 16th size (width *4, height*4) 	*/
/************************************************************************/
void renderer::PerformToneMapping(drawableTex2D * result, drawableTex2D * sceneBuffer, drawableTex2D * adaptedLuminance, drawableTex2D * bloomTexture_1_4th_WL, D3DCOLOR clearColor, bool bloomEnable, bool manualLuminance )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	
	// bloomTexture_1_4th_WL is 1/4th width and height, so final stage will do interpolation.
	// Send the texture and texel size for the  so bilinear interpolation can interpolate between two values
	D3DXVECTOR2 _textureSize = D3DXVECTOR2((float)bloomTexture_1_4th_WL->Width(),(float)bloomTexture_1_4th_WL->Height());
	D3DXVECTOR2 _texelSize = D3DXVECTOR2(1.0f/_textureSize.x, 1.0f/_textureSize.y);
	SetPostProcessBilinearTexelSize(& _texelSize);
	SetPostProcessBilinearTextureSize(& _textureSize);

	SetBloomMultiplier(g_UI->GetSetting<float>( & var_HDRBloomMultiplier ));

	// Set sampler state to use point filtering and clamping and setup the textures
	m_sourceSampler.SetPointFilterClamp();
	m_source2Sampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::PerformToneMapping() - SetSamplerState Failed: ");
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource2, &curState.source2Sampler, &m_source2Sampler),L"renderer::PerformToneMapping() - SetSamplerState Failed: ");
	HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource2, adaptedLuminance->d3dTex()),L"renderer::PerformToneMapping() - Set Texture h_gSource2 Failed: ")
	if(bloomEnable)
	{
		m_source3Sampler.SetPointFilterClamp();
		HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource3, &curState.source3Sampler, &m_source3Sampler),L"renderer::PerformToneMapping() - SetSamplerState Failed: ");
		HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource3, bloomTexture_1_4th_WL->d3dTex()),L"renderer::PerformToneMapping() - Set Texture h_gSource3 Failed: ")
	}

	result->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::PerformToneMapping() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) result->Width(), 1.0f / (float) result->Height());
	UINT pass;
	if(manualLuminance)
		if(bloomEnable)
			pass = HDR_TONEMAP_BLOOM_MANUAL_LUMINANCE_PASS;
		else
			pass = HDR_TONEMAP_NO_BLOOM_MANUAL_LUMINANCE_PASS;
	else
		if(bloomEnable)
			pass = HDR_TONEMAP_BLOOM_ADAPTED_LUMINANCE_PASS;
		else
			pass = HDR_TONEMAP_NO_BLOOM_ADAPTED_LUMINANCE_PASS;

	RenderTextureToScreen(pass, sceneBuffer->d3dTex(), NULL, NULL, TONEMAP_TECH, & destTexelSize);

	result->EndScene();
	
}


/************************************************************************/
/* Name:		ApplyToneMapThreshold									*/
/* Description:	Render texture and clamp the output to some threshold 	*/
/************************************************************************/
void renderer::ApplyToneMapThreshold(drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor, float threshold, bool manualLuminance )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	SetToneMapThreshold(threshold);
	UINT pass;
	if(manualLuminance)
		pass = HDR_TONEMAP_THRESHOLD_MANUAL_LUMINANCE_PASS;
	else
		pass = HDR_TONEMAP_THRESHOLD_ADAPTED_LUMINANCE_PASS;

	// Set sampler state to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::UpscaleTexture4x4() - SetSamplerState Failed: ");
	
	dest->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::UpscaleTexture4x4() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) dest->Width(), 1.0f / (float) dest->Height());
	RenderTextureToScreen(0, source->d3dTex(), NULL, NULL, APPLYTONEMAPTHRESHOLD_TECH, & destTexelSize);

	dest->EndScene();
	
}

/************************************************************************/
/* Name:		GaussianBlur											*/
/* Description:	Apply a gaussian blur to the source texture				*/
/************************************************************************/
void renderer::GaussianBlurTexture(drawableTex2D * source, drawableTex2D * tempTexture, D3DCOLOR clearColor, float sigma, int radius )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	SetGaussianBlurRadius(radius);
	SetGaussianBlurSigma(sigma);

	// Set sampler state to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::GaussianBlurTexture() - SetSamplerState Failed: ");
	
	tempTexture->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::GaussianBlurTexture() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) tempTexture->Width(), 1.0f / (float) tempTexture->Height());
	// Pass 0 is the horizontal blur
	RenderTextureToScreen(0, source->d3dTex(), NULL, NULL, GAUSSIANBLUR_TECH, & destTexelSize);

	tempTexture->EndScene();

	source->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::GaussianBlurTexture() - Failed to clear device: ");

	destTexelSize = D3DXVECTOR2(1.0f / (float) source->Width(), 1.0f / (float) source->Height());
	// Pass 1 is the vertical blur
	RenderTextureToScreen(1, tempTexture->d3dTex(), NULL, NULL, GAUSSIANBLUR_TECH, & destTexelSize);

	source->EndScene();
	
}
void renderer::GaussianBlurTextureHorizontal(drawableTex2D * source, drawableTex2D * destTexture, D3DCOLOR clearColor, float sigma, int radius )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	SetGaussianBlurRadius(radius);
	SetGaussianBlurSigma(sigma);

	// Set sampler state to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::GaussianBlurTextureHorizontal() - SetSamplerState Failed: ");
	
	destTexture->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::GaussianBlurTextureHorizontal() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) destTexture->Width(), 1.0f / (float) destTexture->Height());
	// Pass 0 is the horizontal blur
	RenderTextureToScreen(0, source->d3dTex(), NULL, NULL, GAUSSIANBLUR_TECH, & destTexelSize);

	destTexture->EndScene();
	
}
void renderer::GaussianBlurTextureVertical(drawableTex2D * source, drawableTex2D * destTexture, D3DCOLOR clearColor, float sigma, int radius )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	SetGaussianBlurRadius(radius);
	SetGaussianBlurSigma(sigma);

	// Set sampler state to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::GaussianBlurTextureHorizontal() - SetSamplerState Failed: ");
	
	destTexture->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::GaussianBlurTextureHorizontal() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) destTexture->Width(), 1.0f / (float) destTexture->Height());
	// Pass 1 is the vertical blur
	RenderTextureToScreen(1, source->d3dTex(), NULL, NULL, GAUSSIANBLUR_TECH, & destTexelSize);

	destTexture->EndScene();
	
}

/************************************************************************/
/* Name:		DownscaleTexture										*/
/* Description:	Down scale texture to 1/16th size (width /4, height/4) 	*/
/************************************************************************/
void renderer::DownscaleTextureNxM(drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif

	// Work out how much we need to shrink the texture by in each direction:
	float xShrink = (float)source->Width() / (float)dest->Width(); 
	float yShrink = (float)source->Height() / (float)dest->Height(); 
	int numHorizontalSamples = (int) ceil(xShrink);
	int numVerticalSamples = (int) ceil(yShrink);
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gNumHorizontalSamples, &numHorizontalSamples, sizeof(int)), L"renderer::DownscaleTexture() - Set Value h_gNumHorizontalSamples Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gNumVerticalSamples, &numVerticalSamples, sizeof(int)), L"renderer::DownscaleTexture() - Set Value h_gNumVerticalSamples Failed: ");

	if(numHorizontalSamples > 4 || numVerticalSamples > 4)
		throw std::runtime_error("renderer::DownscaleTextureNxM() - Error: numHorizontalSamples > 4 || numVerticalSamples > 4");

	// Lazy coding, but since the range of # samples is small, it's OK I think.
	// Set the sample sizes in texture coordinates
	D3DXVECTOR4 horizontalSamples;
	switch(numHorizontalSamples)
	{
	case 4:
		horizontalSamples = D3DXVECTOR4(-1.5f, -0.5f, 0.5f, 1.5f);
		break;
	case 3:
		horizontalSamples = D3DXVECTOR4(-1.0f, 0.0f, 1.0f, 2.0f);
		break;
	case 2:
		horizontalSamples = D3DXVECTOR4(-0.5f, 0.5f, 1.5f, 2.5f);
		break;
	case 1:
		horizontalSamples = D3DXVECTOR4(0.0f, 1.0f, 2.0f, 3.0f);
		break;
	}
	horizontalSamples /= (float)source->Width();
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gHorizontalSamples, &horizontalSamples, sizeof(D3DXVECTOR4)), L"renderer::DownscaleTexture() - Set Value h_gHorizontalSamples Failed: ");
	D3DXVECTOR4 verticalSamples;
	switch(numVerticalSamples)
	{
	case 4:
		verticalSamples = D3DXVECTOR4(-1.5f, -0.5f, 0.5f, 1.5f);
		break;
	case 3:
		verticalSamples = D3DXVECTOR4(-1.0f, 0.0f, 1.0f, 2.0f);
		break;
	case 2:
		verticalSamples = D3DXVECTOR4(-0.5f, 0.5f, 1.5f, 2.5f);
		break;
	case 1:
		verticalSamples = D3DXVECTOR4(0.0f, 1.0f, 2.0f, 3.0f);
		break;
	}
	verticalSamples /= (float)source->Height();
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gVerticalSamples, &verticalSamples, sizeof(D3DXVECTOR4)), L"renderer::DownscaleTexture() - Set Value h_gVerticalSamples Failed: ");
		
	// Set sampler state to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::DownscaleTextureNxM() - SetSamplerState Failed: ");
	
	dest->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::DownscaleTextureNxM() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) dest->Width(), 1.0f / (float) dest->Height());
	RenderTextureToScreen(0, source->d3dTex(), NULL, NULL, DOWNSCALE_NxM_TECH, & destTexelSize);

	dest->EndScene();
	
}

/************************************************************************/
/* Name:		CalculateAverageLuminance								*/
/* Description:	Calculate luminance by downsampling until we get 1x1 tex*/
/************************************************************************/
void renderer::CalculateAverageLuminance(drawableTex2D * source, float deltaT, bool manualLuminance )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif

	// FIRSTLY CALCULATE THE PER PIXEL LUMINANCE ON THE DOWNSAMPLED TEXTURE
#ifdef _DEBUG
	if((source->Width() != m_luminanceChain[0].Width()) || (source->Height() != m_luminanceChain[0].Height()))
		throw std::runtime_error("renderer::CalculateAverageLuminance() - Error: Source and m_luminanceChain[0] textures are not the same size");
#endif
	// Set sampler states to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::DownscaleTextureNxM() - SetSamplerState Failed: ");
	m_source2Sampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource2, &curState.source2Sampler, &m_source2Sampler),L"renderer::DownscaleTextureNxM() - SetSamplerState Failed: ");
	
	m_luminanceChain[0].BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, BLACK, 0, 0),L"renderer::CalculateAverageLuminance() - Failed to clear device: ");
	D3DXVECTOR2 destTexelSize(1.0f / (float) m_luminanceChain[0].Width(), 1.0f / (float) m_luminanceChain[0].Height());
	RenderTextureToScreen(0, source->d3dTex(), NULL, NULL, CALCLUMINANCE_TECH, & destTexelSize);
	m_luminanceChain[0].EndScene();

	// Now downscale to a NxN texture, where N is some factor of 2^(2m) (ie, 1x1, 4x4, 16x16, 64x64)
	// Since the source texture aspect depends on the back-buffer aspect (probably not 1:1), use varible aspect downscaling
#ifdef _DEBUG
	if((m_luminanceChain[0].Width() > (4*m_luminanceChain[1].Width())) || (m_luminanceChain[0].Height() > (4*m_luminanceChain[0].Height())))
		throw std::runtime_error("renderer::CalculateAverageLuminance() - Error: m_luminanceChain[0] and m_luminanceChain[1] textures are > 4x in releative size");
#endif
	DownscaleTextureNxM(& m_luminanceChain[0], & m_luminanceChain[1], BLACK );

	// Now repeatadly downscale by 1/4th width and height until we get to 4x4.
	for(UINT i = 1; i < (m_luminanceChainLength-1); i ++)
		DownscaleTexture4x4(& m_luminanceChain[i], & m_luminanceChain[i+1], BLACK, false );

	// Now final downscaling:
	DownscaleTexture4x4(& m_luminanceChain[m_luminanceChainLength-1], m_currentFrameLuminance, BLACK, true );
	
	// Calculate adapted luminance only if we're not doing manual luminance
	if(!manualLuminance)
	{
		// Set some values used when calculating adapted luminance
		HR(m_FXPostProcess->SetTexture(m_FXHandlesPostProcess.h_gSource2, m_lastFrameAdaptedLuminance->d3dTex()),L"renderer::CalculateAverageLuminance() - Set Texture h_gSource2 Failed: ")
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gDeltaT, &deltaT, sizeof(float)), L"renderer::CalculateAverageLuminance() - Set Value h_gDeltaT Failed: ");
		SetLuminanceFTau(g_UI->GetSetting<float>( & var_HDRLuminanceFTau ));

		// Adapt the luminance, to simulate slowly adjusting exposure
		m_currentFrameAdaptedLuminance->BeginScene();
		HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, BLACK, 0, 0),L"renderer::CalculateAverageLuminance() - Failed to clear device: ");
		destTexelSize.x = 1.0f; destTexelSize.y = 1.0f;
		RenderTextureToScreen(0, m_currentFrameLuminance->d3dTex(), NULL, NULL, CALCADAPTEDLUMINANCE_TECH, & destTexelSize);
		m_currentFrameAdaptedLuminance->EndScene();
	}
	
}

/************************************************************************/
/* Name:		SetLuminanceFTau										*/
/* Description:	Set the FX parameter for luminance time constant		*/
/************************************************************************/
void renderer::SetLuminanceFTau(float _luminanceFTau)
{
	if(_luminanceFTau <= 0)
		throw std::runtime_error("renderer::SetLuminanceFTau() - Error: _luminanceFTau <= 0, value is too low");

	if(curState.luminanceFTau != _luminanceFTau) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gLuminanceFTau, &_luminanceFTau, sizeof(float)), L"renderer::SetLuminanceFTau() - Set Value _luminanceFTau Failed: ");
		curState.luminanceFTau = _luminanceFTau;
	}
}

/************************************************************************/
/* Name:		SetPostProcessBilinearTexelSize							*/
/* Description:	Set the FX parameters for Texel and Texture size		*/
/************************************************************************/
void renderer::SetPostProcessBilinearTexelSize(D3DXVECTOR2 * _texelSize)
{
	if(curState.postProcessBilinearTexelSize.x != _texelSize->x ||
	   curState.postProcessBilinearTexelSize.y != _texelSize->y) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBilinearTexelSize, _texelSize, sizeof(D3DXVECTOR2)), L"renderer::SetPostProcessBilinearTexelSize() - Set Value h_gBilinearTexelSize Failed: ");
		curState.postProcessBilinearTexelSize = *_texelSize;
	}
}

/************************************************************************/
/* Name:		SetPostProcessBBilinearTextureSize						*/
/* Description:	Set the FX parameters for Texel and Texture size		*/
/************************************************************************/
void renderer::SetPostProcessBilinearTextureSize(D3DXVECTOR2 * _textureSize)
{
	if(curState.postProcessBilinearTextureSize.x != _textureSize->x ||
	   curState.postProcessBilinearTextureSize.y != _textureSize->y) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBilinearTextureSize, _textureSize, sizeof(D3DXVECTOR2)), L"renderer::SetPostProcessBilinearTextureSize() - Set Value _textureSize Failed: ");
		curState.postProcessBilinearTextureSize = *_textureSize;
	}
}

/************************************************************************/
/* Name:		SetToneMapMiddleGrey									*/
/* Description:	Set the FX parameter for middle grey used in tone map	*/
/************************************************************************/
void renderer::SetToneMapMiddleGrey(float _toneMapMiddleGrey)
{
	if(curState.toneMapMiddleGrey != _toneMapMiddleGrey) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gToneMapMiddleGrey, &_toneMapMiddleGrey, sizeof(float)), L"renderer::SetToneMapMiddleGrey() - Set Value h_gToneMapMiddleGrey Failed: ");
		curState.toneMapMiddleGrey = _toneMapMiddleGrey;
	}
}

/************************************************************************/
/* Name:		SetToneMapWhiteSq										*/
/* Description:	Set the FX parameter									*/
/************************************************************************/
void renderer::SetToneMapWhiteSq(float _toneMapWhiteSq)
{
	if(curState.toneMapWhiteSq != _toneMapWhiteSq) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gToneMapWhiteSq, &_toneMapWhiteSq, sizeof(float)), L"renderer::SetToneMapWhiteSq() - Set Value h_gToneMapWhiteSq Failed: ");
		curState.toneMapWhiteSq = _toneMapWhiteSq;
	}
}

/************************************************************************/
/* Name:		SetToneMapThreshold										*/
/* Description:	Set the FX parameter for max luminance used in tone map	*/
/************************************************************************/
void renderer::SetToneMapThreshold(float _toneMapThreshold)
{
	if(curState.toneMapThreshold != _toneMapThreshold) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gToneMapThreshold, &_toneMapThreshold, sizeof(float)), L"renderer::SetToneMapThreshold() - Set Value h_gToneMapThreshold Failed: ");
		curState.toneMapThreshold = _toneMapThreshold;
	}
}

/************************************************************************/
/* Name:		SetGaussianBlurSigma									*/
/* Description:	Set the FX parameter									*/
/************************************************************************/
void renderer::SetGaussianBlurSigma(float _sigma)
{
	if(curState.gaussianBlurSigma != _sigma) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gGaussianBlurSigma, &_sigma, sizeof(float)), L"renderer::SetGaussianBlurSigma() - Set Value h_gGaussianBlurSigma Failed: ");
		curState.gaussianBlurSigma = _sigma;
	}
}

/************************************************************************/
/* Name:		SetGaussianBlurSigma									*/
/* Description:	Set the FX parameter									*/
/************************************************************************/
void renderer::SetGaussianBlurRadius(int _radius)
{
	if(curState.gaussianBlurRadius != _radius) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gGaussianBlurRadius, &_radius, sizeof(int)), L"renderer::SetGaussianBlurRadius() - Set Value h_gGaussianBlurRadius Failed: ");
		curState.gaussianBlurRadius = _radius;
	}
}

/************************************************************************/
/* Name:		SetBloomMultiplier										*/
/* Description:	Set the FX parameter									*/
/************************************************************************/
void renderer::SetBloomMultiplier(float _bloomMultiplier)
{
	if(curState.bloomMulitplier != _bloomMultiplier) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gBloomMultiplier, &_bloomMultiplier, sizeof(float)), L"renderer::SetBloomMultiplier() - Set Value h_gBloomMultiplier Failed: ");
		curState.bloomMulitplier = _bloomMultiplier;
	}
}

/************************************************************************/
/* Name:		SetMotionBlurNumSamples									*/
/* Description:	Set the FX parameter									*/
/************************************************************************/
void renderer::SetMotionBlurNumSamples(int _numSamples)
{
	if(curState.motionBlurNumSamples != _numSamples) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gMotionBlurNumSamples, &_numSamples, sizeof(int)), L"renderer::SetMotionBlurNumSamples() - Set Value h_gMotionBlurNumSamples Failed: ");
		curState.motionBlurNumSamples = _numSamples;
	}
}

/************************************************************************/
/* Name:		SetHDRManualLuminance									*/
/* Description:	Set the FX parameter									*/
/************************************************************************/
void renderer::SetHDRManualLuminance(float _HDRManualLuminance)
{
	if(curState.HDRManualLuminance != _HDRManualLuminance) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gManualLuminance, &_HDRManualLuminance, sizeof(float)), L"renderer::SetHDRManualLuminance() - Set Value h_gManualLuminance Failed: ");
		curState.HDRManualLuminance = _HDRManualLuminance;
	}
}

/************************************************************************/
/* Name:		SetMotionBlurNumSamples									*/
/* Description:	Set the FX parameter									*/
/************************************************************************/
void renderer::SetDOFBounds(D3DXVECTOR4 _DOFBounds)
{
	// First make sure that they're acceptable
	if(_DOFBounds.x < 0)
		throw std::runtime_error("renderer::SetDOFBounds(): Error DOFBounds[0] < 0, End focal point is behind the viewer?");
	if(_DOFBounds.x > _DOFBounds.y)
		throw std::runtime_error("renderer::SetDOFBounds(): Error DOFBounds[0] > DOFBounds[1], Depth of field bounds must be monotonic");
	if(_DOFBounds.y > _DOFBounds.z)
		throw std::runtime_error("renderer::SetDOFBounds(): Error DOFBounds[1] > DOFBounds[2], Depth of field bounds must be monotonic");
	if(_DOFBounds.z > _DOFBounds.w)
		throw std::runtime_error("renderer::SetDOFBounds(): Error DOFBounds[2] > DOFBounds[3], Depth of field bounds must be monotonic");

	if(curState.DOFBounds.x != _DOFBounds.x || 
	   curState.DOFBounds.y != _DOFBounds.y ||
	   curState.DOFBounds.z != _DOFBounds.z ||
	   curState.DOFBounds.w != _DOFBounds.w) // Avoid redundant set render states
	{
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gDOFBounds, &_DOFBounds, sizeof(D3DXVECTOR4)), L"renderer::SetDOFBounds() - Set Value h_gDOFBounds Failed: ");
		curState.DOFBounds = _DOFBounds;
	}
}


/************************************************************************/
/* Name:		PerformMotionBlur										*/
/* Description:	Perform motion blur on screen space textures		 	*/
/************************************************************************/
void renderer::PerformMotionBlur(drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	SetMotionBlurNumSamples(g_UI->GetSetting<int>(& var_MotionBlurNumSamples));

	// Set sampler state to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	m_source2Sampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::PerformMotionBlur() - SetSamplerState Failed: ");
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource2, &curState.source2Sampler, &m_source2Sampler),L"renderer::PerformMotionBlur() - SetSamplerState Failed: ");
	
	dest->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::PerformMotionBlur() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) dest->Width(), 1.0f / (float) dest->Height());
	RenderTextureToScreen(0, source->d3dTex(), NULL, NULL, MOTIONBLUR_TECH, & destTexelSize);

	dest->EndScene();
	
}

/************************************************************************/
/* Name:		PerformMotionBlur										*/
/* Description:	Perform motion blur on screen space textures		 	*/
/************************************************************************/
void renderer::PerformDOFBlur(drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	UINT pass;
	if(m_depthVertexTextureCap)
		pass = DOFBLUR_VERTEX_FOCUSLOOKUP_PASS; // This should be faster
	else
		pass = DOFBLUR_PIXEL_FOCUSLOOKUP_PASS;

	SetDOFBounds(g_UI->GetSetting<D3DXVECTOR4>(& var_DOFBounds ));
	
	// Set sampler state to use point filtering and clamping
	m_sourceSampler.SetPointFilterClamp();
	HR(SetSamplerState(m_FXPostProcess, &m_FXHandlesPostProcess.h_gSampSource, &curState.sourceSampler, &m_sourceSampler),L"renderer::PerformDOFBlur() - SetSamplerState Failed: ");
	
	dest->BeginScene();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::PerformDOFBlur() - Failed to clear device: ");

	D3DXVECTOR2 destTexelSize(1.0f / (float) dest->Width(), 1.0f / (float) dest->Height());
	RenderTextureToScreen(pass, source->d3dTex(), NULL, NULL, DEPTHOFFIELD_BLURDISK_TECH, & destTexelSize);

	dest->EndScene();
}

