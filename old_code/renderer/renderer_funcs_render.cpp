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
#include	"utils_and_misc_classes\stringUtil.h"
#include	"renderer\renderer_structures\drawableTex2D.h"
#include	"objectManager\objectManager.h"
#include	"sky\sky.h"
#include	"hud\hud.h"
#include	"lights\lightSpotSM.h"
#include	"lights\lightSpot.h"
#include	"lights\lightDir.h"
#include	"lights\lightPoint.h"
#include	"renderer\renderer_structures\vertex.h"
#include	"renderer\renderer_structures\debugObject.h"
#include	"physics\objects\rbobjectMeshData.h"
#include	"UI\varNames.h"
#include	"UI\UI.h"
#include	"camera\camera.h"
#include	"renderer\utils\d3dProfiler.h"
#include	"renderer\renderer_constants.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#ifndef lerp
	#define lerp(t, a, b) ( a + t * (b - a) )
#endif

/************************************************************************/
/* Name:		RenderFrame (+ helper funcitons)						*/
/* Description:	Render the scene										*/
/************************************************************************/
void renderer::RenderFrame()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif

	// Get render settings --> Avoid multiple vector table lookups
	bool _drawShadows = g_UI->GetSetting<bool>(&var_shadowsOn);
	SetShadowsOn(_drawShadows); // Set FX variable to turn shadows ON/OFF
	int _visualizeSplits = g_UI->GetSetting<int>(&var_SMVisualiseSplits);
	SetVisualizeSplits(_visualizeSplits); // Set FX variable to turn split visualization ON/OFF
	int _SSAO = g_UI->GetSetting<int>(&var_SSAOEnable);
	SetSSAOOn(_SSAO); // Set FX variable to turn SSAO rendering ON/OFF

	// Some preliminary maths
	g_objectManager->UpdateRBObjectBoundingBoxes(); // This will also update matricies
	g_objectManager->GetCamera()->UpdateView();
	g_objectManager->UpdateLights(); // Lights are described in camera view space.  Need to update camera view before hand.
	g_objectManager->GetCamera()->UpdateProj(); // Camera projection near / far is tightly fitted to world objects and light volumes.  Update lights first.
	
	// Update camera parameters in the effects
	g_renderer->SetCameraNearFar( g_objectManager->GetCamera()->GetNearFar() );
	g_renderer->SetCameraTangentFov( g_objectManager->GetCamera()->GetTangentFov() );

	if(_drawShadows && (g_UI->GetSetting<int>(&var_RenderGBufferPreviewFullScreen) == 0 || g_UI->GetSetting<int>(&var_RenderGBufferPreviewFullScreen) == 7))
		DrawAllShadowMaps(); // Populate the shadow map buffers

	DrawGBuffer(); // Draw the depth, normal, albedo and misc buffers

	if(_SSAO)
		DrawOcclusionBuffer(); // Calculate the occlusion Buffer
	else
		ClearOcclusionBuffer();

	RenderSceneLights(); // Render light contributions into the accumulation buffer

	PostProcessing(); // Perform Post Processing on the accumulation buffer and render the scene in HDR

	DisplayScene(); // Render the scene buffer into the back buffer and display it

	// Save the world matrices for next frame when generating the velocity buffer
	if(g_UI->GetSetting<bool>( & var_MotionBlurEnable ))
		g_objectManager->SaveRbobjectWorldMatrices();

}//RenderFrame

/************************************************************************/
/* Name:		DisplayScene											*/
/* Description:	 Display the scene into the backbuffer and present it	*/
/************************************************************************/
void renderer::DisplayScene()
{

#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	// Restore the render target to render into the backbuffer
	IDirect3DSurface9* pd3dBackBufferSurface;
	HR(g_renderer->GetD3DDev()->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pd3dBackBufferSurface), L"renderer::DisplayScene() - GetBackBuffer failed: ");
	HR(g_renderer->GetD3DDev()->SetRenderTarget(0, pd3dBackBufferSurface), L"renderer::DisplayScene() - SetRenderTarget(0) failed: ");
	pd3dBackBufferSurface->Release();

	HR(m_pD3DDev->BeginScene(),L"renderer::DisplayScene() - BeginScene failed: ");

	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, SCENE_CLEAR_COLOR, 0, 0),L"renderer::DisplayScene() - Failed to clear device: ");

	int _RenderGBufferPreviewFullScreen = g_UI->GetSetting<int>(&var_RenderGBufferPreviewFullScreen); // 0 is off, 1 is pos buffer, 2 is normal buffer, 3 is occlusion buffer.
	if(_RenderGBufferPreviewFullScreen != 0)
		RenderGBufferPreviewFullScreen(_RenderGBufferPreviewFullScreen);
	else
	{
		// Set the correct texel size
		D3DXVECTOR2 texelSize(1.0f / ((float)m_PresentParameters.BackBufferWidth), 1.0f / ((float)m_PresentParameters.BackBufferHeight));
		RenderTextureToScreen(0, m_sceneBuffer[0]->d3dTex(), NULL, NULL, QUADTEXTURE_TECH, & texelSize );
	}

	if(g_UI->GetSetting<bool>(&var_SMRenderPreview) && g_UI->GetSetting<bool>(&var_shadowsOn) && _RenderGBufferPreviewFullScreen == 0)
		RenderShadowMapTexturesToScreen();

	if(g_UI->GetSetting<bool>(&var_RenderGBufferPreviews))
		RenderGBufferPreviews();

	// Render the HUD and runtime UI elements --> Uses 2D Sprite
	g_objectManager->GetHud()->display();

	HR(m_pD3DDev->EndScene(),L"renderer::DisplayScene() - EndScene failed: ");
	HR(m_pD3DDev->Present(0, 0, 0, 0),L"renderer::DisplayScene() - Present failed: "); // Present the backbuffer.
}

/************************************************************************/
/* Name:		ClearTexture											*/
/* Description:	 clear given texture with given color					*/
/************************************************************************/
void renderer::ClearTexture(IDirect3DTexture9 * pd3dTexture, D3DCOLOR xColor)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	IDirect3DSurface9* pd3dSurface;
	HR(pd3dTexture->GetSurfaceLevel(0,&pd3dSurface),L"renderer::ClearTexture() - GetSurfaceLevel failed: ");
	HR(m_pD3DDev->ColorFill(pd3dSurface,NULL,xColor),L"renderer::ClearTexture() - ColorFill failed: ");
	pd3dSurface->Release();
}

/************************************************************************/
/* Name:		SetRenderTarget											*/
/* Description:	 set render target on the given level                   */
/*				(iRenderTargetIdx: 0 - 3)								*/
/************************************************************************/
void renderer::SetRenderTarget(int iRenderTargetIdx, IDirect3DTexture9* pd3dRenderTargetTexture)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
#ifdef _DEBUG
	if(iRenderTargetIdx<0 || iRenderTargetIdx>3)
		throw std::runtime_error("renderer::SetRenderTarget() - Incorrect render target ID");
#endif

	IDirect3DSurface9* pd3dSurface;
	HR(pd3dRenderTargetTexture->GetSurfaceLevel(0,&pd3dSurface),L"renderer::SetRenderTarget() - GetSurfaceLevel failed: ");
	HR(m_pD3DDev->SetRenderTarget(iRenderTargetIdx,pd3dSurface),L"renderer::SetRenderTarget() - SetRenderTarget failed: ");
	pd3dSurface->Release();
}

/************************************************************************/
/* Name:		RenderFullScreenQuad									*/
/* Description:	Render the full screen quad								*/
/************************************************************************/ 
void renderer::RenderFullScreenQuad(ID3DXEffect * m_FX, EFFECT_PASS effect, UINT Technique, bool clearBuffer, D3DCOLOR clearColor, UINT pass )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	if(effect != LIGHTING && effect != POSTPROCESS && effect != GBUFFER)
		throw std::runtime_error("renderer::RenderFullScreenQuad() - Incorrect EFFECT_PASS!  Only LIGHTING, POSTPROCESS and GBUFFER allowed");

	// Get screen width / height
	D3DVIEWPORT9 Viewport;
	HR(m_pD3DDev->GetViewport(&Viewport),L"renderer::RenderFullScreenQuad() -  GetViewport failed: "); // Get the current viewport (might be different in fullscreen vs. windowed
	D3DXVECTOR2 texelSize(1.0f / ((float)Viewport.Width), 1.0f / ((float)Viewport.Height));
	switch(effect)
	{
	case LIGHTING:
		SetLightingTexelSize(& texelSize); // Set the correct texel size
		break;
	case POSTPROCESS:
		SetPostProcessingTexelSize(& texelSize); // Set the correct texel size
		break;
	case GBUFFER:
		SetGBufferTexelSize(& texelSize); // Set the correct texel size
		break;
	}

	HR(m_pD3DDev->SetVertexDeclaration(VertexPosTex::Decl),L"renderer::RenderFullScreenQuad() -  SetVertexDeclaration failed: ");
	HR(m_pD3DDev->SetStreamSource(0, m_FSQuadVertBuff, 0, sizeof(VertexPosTex)),L"renderer::RenderFullScreenQuad() -  SetStreamSource failed: ")

	if(clearBuffer)
		HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, clearColor, 0, 0),L"renderer::RenderFullScreenQuad() -  Failed to clear device: ");
	
	// Set the FX parameters that might change from frame to frame
	switch(effect)
	{
	case LIGHTING:
		SetLightingTechnique((LIGHTING_TECH)Technique);
		break;
	case POSTPROCESS:
		SetPostProcessTechnique((POSTPROCESS_TECH)Technique);
		break;
	case GBUFFER:
		SetGBufferTechnique((GBUFFER_TECH)Technique);
		break;
	}

	UINT numPasses = 0;
	HR(m_FX->Begin(&numPasses, 0),L"renderer::RenderFullScreenQuad() - m_FXPostProcess->Begin Failed: ");
	HR(m_FX->BeginPass(pass),L"renderer::RenderFullScreenQuad() - m_FXPostProcess->BeginPass Failed: "); // Pass zero is the variable sized blur filter

	HR(m_FX->CommitChanges(),L"renderer::RenderFullScreenQuad() -  CommitChanges failed: ");

	// Now draw the Primative
	HR(m_pD3DDev->DrawPrimitive(D3DPT_TRIANGLELIST, 0, m_FSQuadVertBuffSize/3), L"renderer::RenderFullScreenQuad() -  DrawPrimitive failed: ");

	HR(m_FX->EndPass(),L"renderer::RenderFullScreenQuad() -  m_FXPostProcess->EndPass Failed: ");
	HR(m_FX->End(),L"renderer::RenderFullScreenQuad() - m_FXPostProcess->End Failed: ");

}

/************************************************************************/
/* Name:		SetSamplerState											*/
/* Description:	Set a sampler state.									*/
/************************************************************************/
HRESULT renderer::SetSamplerState(ID3DXEffect * m_FX, d3dHandlesSampler * samplerHandles, texSamplerState * curState, texSamplerState * desiredState )
{

#ifdef _DEBUG
	if(m_FX == NULL || samplerHandles == NULL || curState == NULL || desiredState == NULL)
		throw std::runtime_error("renderer::SetSamplerState() - One of the input states is null!");
#endif
	

	HRESULT retVal;
	if(curState->mipFilter != desiredState->mipFilter)
	{
		retVal = m_FX->SetValue(samplerHandles->mipFilter, &desiredState->mipFilter, sizeof(D3DTEXTUREFILTERTYPE));
		if(retVal < 0)
			return retVal;
		curState->mipFilter = desiredState->mipFilter;
	}

	if(curState->magFilter != desiredState->magFilter)
	{
		retVal = m_FX->SetValue(samplerHandles->magFilter, &desiredState->magFilter, sizeof(D3DTEXTUREFILTERTYPE));
		if(retVal < 0)
			return retVal;
		curState->magFilter = desiredState->magFilter;
	}

	if(curState->minFilter != desiredState->minFilter)
	{
		retVal = m_FX->SetValue(samplerHandles->minFilter, &desiredState->minFilter, sizeof(D3DTEXTUREFILTERTYPE));
		if(retVal < 0)
			return retVal;
		curState->minFilter = desiredState->minFilter;
	}

	if(curState->addressU != desiredState->addressU)
	{
		retVal = m_FX->SetValue(samplerHandles->addressU, &desiredState->addressU, sizeof(D3DTEXTUREADDRESS));
		if(retVal < 0)
			return retVal;
		curState->addressU = desiredState->addressU;
	}

	if(curState->addressV != desiredState->addressV)
	{
		retVal = m_FX->SetValue(samplerHandles->addressV, &desiredState->addressV, sizeof(D3DTEXTUREADDRESS));
		if(retVal < 0)
			return retVal;
		curState->addressV = desiredState->addressV;
	}

	if(curState->maxAnisotropy != desiredState->maxAnisotropy)
	{
		retVal = m_FX->SetValue(samplerHandles->maxAnisotropy, &desiredState->maxAnisotropy, sizeof(DWORD));
		if(retVal < 0)
			return retVal;
		curState->maxAnisotropy = desiredState->maxAnisotropy;
	}
	return S_OK;
}

/************************************************************************/
/* Name:		SetCameraNearFar										*/
/* Description:	Set the camera near and far bounds used by many fx		*/
/************************************************************************/
void renderer::SetCameraNearFar(D3DXVECTOR2 * nearFar)
{
	if(abs(nearFar->x - curState.cameraNearFar.x) > EPSILON || abs(nearFar->y - curState.cameraNearFar.y) > EPSILON)
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gCameraNearFar, nearFar, sizeof(D3DXVECTOR2)),L"renderer::SetCameraNearFar() - Set Value h_gCameraNearFar Failed (lighting): ");
		HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gCameraNearFar, nearFar, sizeof(D3DXVECTOR2)),L"renderer::SetCameraNearFar() - Set Value h_gCameraNearFar Failed (gbuffer): ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gCameraNearFar, nearFar, sizeof(D3DXVECTOR2)),L"renderer::SetCameraNearFar() - Set Value h_gCameraNearFar Failed (postprocess): ");
		curState.cameraNearFar = *nearFar;
	}
}

/************************************************************************/
/* Name:		SetCameraNearFar										*/
/* Description:	Set the camera near and far bounds used by many fx		*/
/************************************************************************/
void renderer::SetCameraTangentFov(D3DXVECTOR2 * tangentFov)
{
	if(abs(tangentFov->x - curState.cameraTangentFov.x) > EPSILON || abs(tangentFov->y - curState.cameraTangentFov.y) > EPSILON)
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gCameraTangentFov, tangentFov, sizeof(D3DXVECTOR2)),L"renderer::SetCameraTangentFov() - Set Value h_gCameraTangentFov Failed (lighting): ");
		HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gCameraTangentFov, tangentFov, sizeof(D3DXVECTOR2)),L"renderer::SetCameraTangentFov() - Set Value h_gCameraTangentFov Failed (postprocess): ");
		curState.cameraTangentFov = *tangentFov;
	}
}