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
#include	"renderer\renderer_structures\cone.h"
#include	"objectManager\objectManager.h"
#include	"sky\sky.h"
#include	"hud\hud.h"
#include	"lights\light.h"
#include	"lights\lightSpotSM.h"
#include	"lights\lightSpot.h"
#include	"lights\lightPoint.h"
#include	"renderer\renderer_structures\vertex.h"
#include	"renderer\renderer_structures\debugObject.h"
#include	"physics\objects\rbobjectMeshData.h"
#include	"physics\objects\rbobjectMesh.h"
#include	"UI\varNames.h"
#include	"UI\UI.h"
#include	"camera\camera.h"
#include	"renderer\utils\d3dProfiler.h"
#include	"renderer\renderer_constants.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		InitGBuffer												*/
/* Description:	Initialize the G Buffer structures						*/
/************************************************************************/
void renderer::InitGBuffer()
{
	// D3DFORMAT depthFormat = (D3DFORMAT)g_UI->GetComboBoxVal( & var_depthStencilFormat );
	D3DFORMAT backBufferFormat = m_PresentParameters.BackBufferFormat;

	D3DFORMAT depthFormat, normalFormat, albedoFormat, miscFormat;

	switch(g_UI->GetComboBoxVal(& var_gBufferFormat))
	{
	case GBUFFER_32BIT:
		depthFormat = DEPTH_BUFFER_FORMAT_32B;
		normalFormat = NORMAL_BUFFER_FORMAT_32B;
		albedoFormat = ALBEDO_BUFFER_FORMAT_32B;
		miscFormat = MISC_BUFFER_FORMAT_32B;
		break;
	case GBUFFER_64BIT:
		depthFormat = DEPTH_BUFFER_FORMAT_64B;
		normalFormat = NORMAL_BUFFER_FORMAT_64B;
		albedoFormat = ALBEDO_BUFFER_FORMAT_64B;
		miscFormat = MISC_BUFFER_FORMAT_64B;
		break;
	default:
		throw std::runtime_error("renderer::InitGBuffer() - Error: Gbuffer format option not recognized");
	}

	if(FAILED(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), backBufferFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, depthFormat)))
		throw std::runtime_error("renderer::InitGBuffer() - Error: DEPTH_BUFFER_FORMAT is not supported!");
	if(FAILED(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), backBufferFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, normalFormat)))
		throw std::runtime_error("renderer::InitGBuffer() - Error: NORMAL_BUFFER_FORMAT is not supported!");
	if(FAILED(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), backBufferFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, albedoFormat)))
		throw std::runtime_error("renderer::InitGBuffer() - Error: ALBEDO_BUFFER_FORMAT is not supported!");
	if(FAILED(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), backBufferFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, miscFormat)))
		throw std::runtime_error("renderer::InitGBuffer() - Error: MISC_BUFFER_FORMAT is not supported!");

	// Check if we can support hardware vertex texture sampling on the current depth buffer
	//   --> This is for an optimization for the depth of field effect. 
	//   --> Otherwise we can do extra work in the pixel shader as a work around:
	if(FAILED(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), backBufferFormat, D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, depthFormat)))
		m_depthVertexTextureCap = false;
	else
		m_depthVertexTextureCap = true;

	// Allocate new space
	if(m_viewNormalMap) { delete m_viewNormalMap; }
	m_viewNormalMap = new drawableTex2D;
	if(m_viewDepthMap) { delete m_viewDepthMap; }
	m_viewDepthMap = new drawableTex2D;
	if(m_albedoMap) { delete m_albedoMap; }
	m_albedoMap = new drawableTex2D;
	if(m_miscMap) { delete m_miscMap; }
	m_miscMap = new drawableTex2D;

	// Create samplers
	m_gBufferSampler.SetFilter(D3DTEXF_POINT, D3DTADDRESS_CLAMP, 1);

	// Make new drawable textures
	UINT _height = m_PresentParameters.BackBufferHeight;
	UINT _width = m_PresentParameters.BackBufferWidth;
	UINT _mipLevels = 1;

	bool autogenMipMaps = false;
	m_viewNormalMap->InitDrawableTex2D(_width, _height, _mipLevels, normalFormat, autogenMipMaps);
	m_viewDepthMap->InitDrawableTex2D(_width, _height, _mipLevels, depthFormat, autogenMipMaps);
	m_albedoMap->InitDrawableTex2D(_width, _height, _mipLevels, albedoFormat, autogenMipMaps);
	m_miscMap->InitDrawableTex2D(_width, _height, _mipLevels, miscFormat, autogenMipMaps);
	
	// Set the clear colors in the GBuffer effect
	D3DXVECTOR4 _depthClear = ColorToRGBAVector(DEPTH_BUFFER_CLEAR);
	D3DXVECTOR4 _normalClear = ColorToRGBAVector(NORMAL_BUFFER_CLEAR);
	D3DXVECTOR4 _albedoClear = ColorToRGBAVector(ALBEDO_BUFFER_CLEAR);
	D3DXVECTOR4 _miscClear = ColorToRGBAVector(MISC_BUFFER_CLEAR);
	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gDepthClearColor, &_depthClear, sizeof(D3DXVECTOR4)),L"renderer::InitGBuffer() - Set Value h_gDepthClearColor Failed: ");
	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gNormalClearColor, &_normalClear, sizeof(D3DXVECTOR4)),L"renderer::InitGBuffer() - Set Value h_gNormalClearColor Failed: ");
	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gAlbedoClearColor, &_albedoClear, sizeof(D3DXVECTOR4)),L"renderer::InitGBuffer() - Set Value h_gAlbedoClearColor Failed: ");
	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gMiscClearColor, &_miscClear, sizeof(D3DXVECTOR4)),L"renderer::InitGBuffer() - Set Value h_gMiscClearColor Failed: ");
	
	// Environment map (sky texture)
	HR(m_FXGBuffer->SetTexture(m_FXHandlesGBuffer.h_gEnvMap, g_objectManager->GetSky()->mEnvMap),L"renderer::InitGBuffer() - Failed to set sky texture: ");
	HR(g_renderer->SetSamplerState(m_FXGBuffer, &m_FXHandlesGBuffer.h_gSampEnvMap, &curState.skyTexSampler, g_objectManager->GetSky()->texFilter),L"renderer::InitGBuffer() - SetSamplerState failed: ");

}

/************************************************************************/
/* Name:		DrawGBuffer												*/
/* Description: Draw scene geometry information into the render targets	*/
/************************************************************************/
void renderer::DrawGBuffer()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	// Prefetch some settings
	UINT numPasses = 0;
	bool _drawMeshObjects = g_UI->GetSetting<bool>(&var_drawModels);
	bool _drawLightObjects = g_UI->GetSetting<bool>(&var_renderLightObjects);
	bool _drawLightVolumes = g_UI->GetSetting<bool>(&var_renderLightVolumesWireframe);
	bool _renderObjectsWireframe = g_UI->GetSetting<bool>(&var_renderObjectsWireframe);
	bool _drawVelocityBuffer = g_UI->GetSetting<bool>(&var_MotionBlurEnable);
	
	// clear render targets from previous content
	if(!g_UI->GetSetting<bool>(& var_clearGBufferInShader))
	{
		ClearTexture(m_viewDepthMap->d3dTex(),DEPTH_BUFFER_CLEAR);
		ClearTexture(m_viewNormalMap->d3dTex(),NORMAL_BUFFER_CLEAR);
		ClearTexture(m_albedoMap->d3dTex(),ALBEDO_BUFFER_CLEAR);
		ClearTexture(m_miscMap->d3dTex(),MISC_BUFFER_CLEAR);
	}

	HR(m_pD3DDev->SetRenderTarget(DEPTH_BUFFER_ID,m_viewDepthMap->d3dSurf()), L"renderer::DrawGBuffer() - SetRenderTarget m_viewDepthMap Failed :");
	HR(m_pD3DDev->SetRenderTarget(NORMAL_BUFFER_ID,m_viewNormalMap->d3dSurf()), L"renderer::DrawGBuffer() - SetRenderTarget m_viewNormalMap Failed :");
	HR(m_pD3DDev->SetRenderTarget(ALBEDO_BUFFER_ID,m_albedoMap->d3dSurf()), L"renderer::DrawGBuffer() - SetRenderTarget m_albedoMap Failed :");
	HR(m_pD3DDev->SetRenderTarget(MISC_BUFFER_ID,m_miscMap->d3dSurf()), L"renderer::DrawGBuffer() - SetRenderTarget m_miscMap Failed :");

	HR(m_pD3DDev->BeginScene(),L"renderer::DrawGBuffer() - BeginScene failed: ");
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, GBUFFER_CLEAR_DEPTH, GBUFFER_CLEAR_STENCIL),L"renderer::DrawGBuffer() - Failed to clear device: ");

	if(g_UI->GetSetting<bool>(& var_clearGBufferInShader))
		ClearGBufferInShader();

	// DRAW ALL OBJECTS HERE
		if(g_UI->GetSetting<bool>(&var_drawSky))
			g_objectManager->GetSky()->draw(); // Draw the sky first

		 // Render the mesh elements
		if( (_drawMeshObjects && ! _renderObjectsWireframe) || _drawLightObjects)
		{
			// Set the technique and begin the effect to avoid redundant set render state overhead
			SetGBufferTechnique(TEXTUREDMESH_TECH);
			HR(m_FXGBuffer->Begin(&numPasses, 0),L"renderer::DrawGBuffer() - m_FX->Begin Failed: ");

			// Draw the objects
			if(_drawMeshObjects && ! _renderObjectsWireframe)
				g_objectManager->RenderRBObjectMeshesGBuffer(_drawVelocityBuffer);

			// Render the light visualizations
			if(_drawLightObjects)
				RenderLightObjects(_drawVelocityBuffer);

			// Finish the technique: effect framework will save all effect values
			HR(m_FXGBuffer->End(),L"renderer::DrawGBuffer() - m_FX->End Failed: ");
		}

		if( (_drawMeshObjects && _renderObjectsWireframe))
		{
			g_objectManager->RenderRBObjectMeshesGBufferWireframe();
		}

		// Render the obbox debug elements -> Wireframe or solid rendering
		if(g_UI->GetSetting<bool>(&var_drawOBBTree))
			g_objectManager->RenderRBObjectMeshOBBs();

		// Render the obbox convex hull debug elements -> Wireframe
        if(g_UI->GetSetting<bool>(&var_drawConvexHull))
            g_objectManager->RenderRBObjectMeshConvexHulls();

		// Draw the debug objects -> Wireframe or solid rendering
		if(g_objectManager->GetDebugObjects())
			g_objectManager->GetDebugObjects()->Render(); // Recursively draw the debugObjects linked-list.

		// Draw the debug objects -> Wireframe or solid rendering
		if(_drawLightVolumes)
			renderLightVolumesWireframe();

	// FINISH DRAWING OBJECTS HERE

	HR(m_pD3DDev->EndScene(),L"renderer::DrawGBuffer() - EndScene failed: ");

	// disable render targets
	// HR(m_pD3DDev->SetRenderTarget(0, NULL), L"renderer::DrawGBuffer() - SetRenderTarget(0) failed: ");
	HR(m_pD3DDev->SetRenderTarget(1, NULL), L"renderer::DrawGBuffer() - SetRenderTarget(1) failed: ");
	HR(m_pD3DDev->SetRenderTarget(2, NULL), L"renderer::DrawGBuffer() - SetRenderTarget(2) failed: ");
	HR(m_pD3DDev->SetRenderTarget(3, NULL), L"renderer::DrawGBuffer() - SetRenderTarget(3) failed: ");

}

/************************************************************************/
/* Name:		ClearGBufferInShader									*/
/* Description: Clear the GBuffer in the pixel shader					*/
/************************************************************************/
void renderer::ClearGBufferInShader()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	RenderFullScreenQuad(m_FXGBuffer, GBUFFER, CLEARGBUFFER_TECH, false, BLACK, 0);
}

/************************************************************************/
/* Name:		SetGBufferMatricies										*/
/* Description: Set the world and view projections for rendering to		*/
/*              the position and normal render targets					*/
/************************************************************************/
void renderer::SetGBufferMatricies(D3DXMATRIXA16 * matWorld)
{
	// Setup the matricies
	// We want this:
	//		a) m_hW = matWorld   --> NOT NEEDED
	//		b) m_hWV = matWorld * m_lightView
	//      c) m_hWVP = matWorld * m_lightView * m_lightProj

	// a) m_hW
	// HR(m_FX->SetMatrix(m_FXHandles.m_hW, matWorld), L"Render::SetGBufferMatricies() - Failed to set m_hWV matrix: "); 

	// b) m_hWV
	D3DXMATRIX WV;
	D3DXMatrixMultiply(& WV, matWorld, g_objectManager->GetCamera()->GetMatView() ); // matWorld is current RBO model->world transform
	HR(m_FXGBuffer->SetMatrix(m_FXHandlesGBuffer.h_gWV, & WV), L"renderer::SetGBufferMatricies() - Failed to set m_hWV matrix: "); 

	// c) m_hWVP
	D3DXMATRIX WVP;
	D3DXMatrixMultiply(& WVP, & WV, g_objectManager->GetCamera()->GetMatProjection() ); // matWorld is current RBO model->world transform
	HR(m_FXGBuffer->SetMatrix(m_FXHandlesGBuffer.h_gWVP, & WVP), L"renderer::SetGBufferMatricies() - Failed to set m_hWVP matrix: "); 
}

void renderer::SetGBufferMatricies(D3DXMATRIXA16 * matWorld, D3DXMATRIXA16 * matWorldPrevFrame )
{
	SetGBufferMatricies(matWorld);
	if(g_UI->GetSetting<bool>(& var_MotionBlurEnable ))
	{
		D3DXMATRIX WV;
		D3DXMatrixMultiply(& WV, matWorldPrevFrame, g_objectManager->GetCamera()->GetMatViewPrevFrame() );
		D3DXMATRIX WVP;
		D3DXMatrixMultiply(& WVP, & WV, g_objectManager->GetCamera()->GetMatProjectionPrevFrame() );
		HR(m_FXGBuffer->SetMatrix(m_FXHandlesGBuffer.h_gWVP_prevFrame, & WVP), L"renderer::SetGBufferMatricies() - Failed to set h_gWVP_prevFrame matrix: "); 
	}


}

/************************************************************************/
/* Name:		DrawTexturedMeshGBuffer									*/
/* Description:	Draw a textured mesh object								*/
/************************************************************************/
void renderer::DrawTexturedMeshGBuffer(UINT pass, rbobjectMeshData * meshData, D3DXMATRIXA16 * matWorld, D3DXMATRIXA16 * matWorldPrevFrame, Mtrl * mtrlOverride)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	SetGBufferMatricies(matWorld, matWorldPrevFrame);

	HR(m_FXGBuffer->BeginPass(pass),L"renderer::DrawTexturedMeshGBuffer() - m_FX->BeginPass Failed: ");

	for(UINT j = 0; j < meshData->materials->Size(); ++j)
	{
		if(mtrlOverride == NULL)
		{	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gMtrl, meshData->materials->GetElem(j), sizeof(Mtrl)),L"renderer::DrawTexturedMeshGBuffer() - Failed to set value h_gMtrl: "); }
		else
		{	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gMtrl, mtrlOverride, sizeof(Mtrl)),L"renderer::DrawTexturedMeshGBuffer() - Failed to set value h_gMtrl: "); }

        if(meshData->textures->GetElem(j) != 0) {
                HR(m_FXGBuffer->SetTexture(m_FXHandlesGBuffer.h_gTex, meshData->textures->GetElem(j)),L"renderer::DrawTexturedMeshGBuffer() - Failed to set a texture: ");
				HR(SetSamplerState(m_FXGBuffer, &m_FXHandlesGBuffer.h_gSampTex, &curState.texSampler, meshData->textureFilters->GetElem(j)),L"renderer::DrawTexturedMeshGBuffer() - SetSamplerState failed: ");
        } else {
                HR(m_FXGBuffer->SetTexture(m_FXHandlesGBuffer.h_gTex, m_WhiteTex),L"renderer::DrawTexturedMeshGBuffer() - Failed to set the white texture: ");
				HR(SetSamplerState(m_FXGBuffer, &m_FXHandlesGBuffer.h_gSampTex, &curState.texSampler, &m_WhiteTexSamplerState),L"renderer::DrawTexturedMeshGBuffer() - SetSamplerState failed: ");
        }

        HR(m_FXGBuffer->CommitChanges(),L"renderer::DrawTexturedMeshGBuffer() - CommitChanges failed: ");

		HR(meshData->pMesh->DrawSubset(j),L"renderer::DrawTexturedMeshGBuffer() - DrawSubset failed: ");
	}

	HR(m_FXGBuffer->EndPass(),L"renderer::DrawTexturedMeshGBuffer() - m_FX->EndPass Failed: ");
}

/************************************************************************/
/* Name:		DrawSingleColorMeshWireframeGBuffer						*/
/* Description:	Draw a single colored mesh object in wireframe mode		*/
/*				From Vertex and Index Buffers							*/
/************************************************************************/
void renderer::DrawSingleColorMeshWireframeGBuffer(UINT pass, bool drawAsLines, IDirect3DVertexBuffer9 * vertBuff, DWORD vertSize, IDirect3DIndexBuffer9 * indBuff, DWORD indSize, D3DXMATRIXA16 * matWorld)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
#ifdef _DEBUG
	if(pass != 0)
		throw std::runtime_error("renderer::DrawSingleColorMeshWireframeGBuffer() - Error: pass can only be 1");
#endif
	
	SetGBufferMatricies(matWorld);
	SetGBufferTechnique(SINGLECOLORMESH_WIREFRAME_TECH);

	HR(m_pD3DDev->SetStreamSource(0, vertBuff, 0, sizeof(VertexPosNormCol)),L"renderer::DrawSingleColorMeshWireframeGBuffer() - SetStreamSource failed: ");
    HR(m_pD3DDev->SetIndices(indBuff),L"renderer::DrawSingleColorMeshWireframeGBuffer() - SetIndices failed: ");
    HR(m_pD3DDev->SetVertexDeclaration(VertexPosNormCol::Decl),L"renderer::DrawSingleColorMeshWireframeGBuffer() - SetVertexDeclaration failed: ");

	UINT numPasses = 0;
	HR(m_FXGBuffer->Begin(&numPasses, 0),L"renderer::DrawSingleColorMeshWireframeGBuffer() - m_FX->Begin Failed: ");
	HR(m_FXGBuffer->BeginPass(pass),L"renderer::DrawSingleColorMeshWireframeGBuffer() - m_FX->BeginPass Failed: ");

    HR(m_FXGBuffer->CommitChanges(),L"renderer::DrawSingleColorMeshWireframeGBuffer() - CommitChanges failed: ");

	if(drawAsLines)
	{	HR(m_pD3DDev->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, vertSize, 0, indSize/2), L"renderer::DrawSingleColorMeshWireframeGBuffer() - DrawIndexedPrimitive failed: "); }
	else
		HR(m_pD3DDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vertSize, 0, indSize/3), L"renderer::DrawSingleColorMeshWireframeGBuffer() - DrawIndexedPrimitive failed: ");

	HR(m_FXGBuffer->EndPass(),L"renderer::DrawSingleColorMeshWireframeGBuffer() - m_FX->EndPass Failed: ");
	HR(m_FXGBuffer->End(),L"renderer::DrawSingleColorMeshWireframeGBuffer() - m_FX->End Failed: ");
}

/************************************************************************/
/* Name:		DrawTexturedMeshWireframeGBuffer						*/
/* Description:	Draw a single mesh object in wireframe mode				*/
/************************************************************************/
void renderer::DrawTexturedMeshWireframeGBuffer(UINT pass, rbobjectMeshData * meshData, D3DXMATRIXA16 * matWorld)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
#ifdef _DEBUG
	if(pass != 0)
		throw std::runtime_error("renderer::DrawTexturedMeshWireframeGBuffer() - Error: pass can only be 1");
#endif

	SetGBufferMatricies(matWorld);
	SetGBufferTechnique(TEXTUREDMESH_WIREFRAME_TECH);

	UINT numPasses;
	HR(m_FXGBuffer->Begin(&numPasses, 0),L"renderer::DrawTexturedMeshWireframeGBuffer() - m_FX->Begin Failed: ");
	HR(m_FXGBuffer->BeginPass(pass),L"renderer::DrawTexturedMeshWireframeGBuffer() - m_FX->BeginPass Failed: ");

	for(UINT j = 0; j < meshData->materials->Size(); ++j)
	{
		HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gMtrl, meshData->materials->GetElem(j), sizeof(Mtrl)),L"renderer::DrawTexturedMeshWireframeGBuffer() - Failed to set value h_gMtrl: ");

        if(meshData->textures->GetElem(j) != 0) {
                HR(m_FXGBuffer->SetTexture(m_FXHandlesGBuffer.h_gTex, meshData->textures->GetElem(j)),L"renderer::DrawTexturedMeshWireframeGBuffer() - Failed to set a texture: ");
				HR(SetSamplerState(m_FXGBuffer, &m_FXHandlesGBuffer.h_gSampTex, &curState.texSampler, meshData->textureFilters->GetElem(j)),L"renderer::DrawTexturedMeshWireframeGBuffer() - SetSamplerState failed: ");
        } else {
                HR(m_FXGBuffer->SetTexture(m_FXHandlesGBuffer.h_gTex, m_WhiteTex),L"renderer::DrawTexturedMeshWireframeGBuffer() - Failed to set the white texture: ");
				HR(SetSamplerState(m_FXGBuffer, &m_FXHandlesGBuffer.h_gSampTex, &curState.texSampler, &m_WhiteTexSamplerState),L"renderer::DrawTexturedMeshWireframeGBuffer() - SetSamplerState failed: ");
        }

        HR(m_FXGBuffer->CommitChanges(),L"renderer::DrawTexturedMeshWireframeGBuffer() - CommitChanges failed: ");

		HR(meshData->pMesh->DrawSubset(j),L"renderer::DrawTexturedMeshWireframeGBuffer() - DrawSubset failed: ");
	}

	HR(m_FXGBuffer->EndPass(),L"renderer::DrawTexturedMeshWireframeGBuffer() - m_FX->EndPass Failed: ");
	HR(m_FXGBuffer->End(),L"renderer::DrawTexturedMeshWireframeGBuffer() - m_FX->End Failed: ");
}

/************************************************************************/
/* Name:		DrawSingleColorMeshGBuffer								*/
/* Description:	Draw a single colored mesh object						*/
/************************************************************************/
void renderer::DrawSingleColorMeshGBuffer(UINT pass, IDirect3DVertexBuffer9 * vertBuff, DWORD vertSize, IDirect3DIndexBuffer9 * indBuff, DWORD indSize, D3DXMATRIXA16 * matWorld, Mtrl * matl)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	
	SetGBufferMatricies(matWorld);
	SetGBufferTechnique(SINGLECOLORMESH_TECH);

	HR(m_pD3DDev->SetStreamSource(0, vertBuff, 0, sizeof(VertexPosNormCol)),L"renderer::DrawSingleColorMeshGBuffer() - SetStreamSource failed: ");
    HR(m_pD3DDev->SetIndices(indBuff),L"renderer::DrawSingleColorMeshGBuffer() - SetIndices failed: ");
    HR(m_pD3DDev->SetVertexDeclaration(VertexPosNormCol::Decl),L"renderer::DrawSingleColorMeshGBuffer() - SetVertexDeclaration failed: ");

	UINT numPasses = 0;
	HR(m_FXGBuffer->Begin(&numPasses, 0),L"renderer::DrawSingleColorMeshGBuffer() - m_FX->Begin Failed: ");
	HR(m_FXGBuffer->BeginPass(pass),L"renderer::DrawSingleColorMeshGBuffer() - m_FX->BeginPass Failed: ");

	
	if(matl == NULL)
	{	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gMtrl, m_defaultMtrl, sizeof(Mtrl)),L"renderer::DrawSingleColorMeshGBuffer() - Failed to set value m_hMtrl: "); }
	else
	{	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gMtrl, matl, sizeof(Mtrl)),L"renderer::DrawSingleColorMeshGBuffer() - Failed to set value m_hMtrl: "); }
    HR(m_FXGBuffer->CommitChanges(),L"renderer::DrawSingleColorMeshGBuffer() - CommitChanges failed: ");

	HR(m_pD3DDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vertSize, 0, indSize/3), L"renderer::DrawSingleColorMeshGBuffer() - DrawIndexedPrimitive failed: ");

	HR(m_FXGBuffer->EndPass(),L"renderer::DrawSingleColorMeshGBuffer() - m_FX->EndPass Failed: ");
	HR(m_FXGBuffer->End(),L"renderer::DrawSingleColorMeshGBuffer() - m_FX->End Failed: ");
}

/************************************************************************/
/* Name:		DrawSingleColorMeshGBuffer								*/
/* Description:	Draw a single colored mesh object						*/
/************************************************************************/
void renderer::DrawSingleColorMeshGBuffer(UINT pass, ID3DXMesh * pMesh, D3DXMATRIXA16 * matWorld, Mtrl * matl)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	
	SetGBufferTechnique(SINGLECOLORMESH_TECH);

	UINT numPasses = 0;
	HR(m_FXGBuffer->Begin(&numPasses, 0),L"renderer::DrawSingleColorMeshGBuffer() - m_FX->Begin Failed: ");
	HR(m_FXGBuffer->BeginPass(pass),L"renderer::DrawSingleColorMeshGBuffer() - m_FX->BeginPass Failed: ");

	//SetGBufferMatricies(matWorld);
	if(matl == NULL)
	{	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gMtrl, m_defaultMtrl, sizeof(Mtrl)),L"renderer::DrawSingleColorMeshGBuffer() - Failed to set value m_hMtrl: "); }
	else
	{	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gMtrl, matl, sizeof(Mtrl)),L"renderer::DrawSingleColorMeshGBuffer() - Failed to set value m_hMtrl: "); }
    HR(m_FXGBuffer->CommitChanges(),L"renderer::DrawSingleColorMeshGBuffer() - CommitChanges failed: ");

	HR(pMesh->DrawSubset(0),L"renderer::DrawSingleColorMeshGBuffer() - DrawSubset failed: ");

	HR(m_FXGBuffer->EndPass(),L"renderer::DrawSingleColorMeshGBuffer() - m_FX->EndPass Failed: ");
	HR(m_FXGBuffer->End(),L"renderer::DrawSingleColorMeshGBuffer() - m_FX->End Failed: ");
}

/************************************************************************/
/* Name:		SetGBufferTechnique										*/
/* Description:	Set the desired technique								*/
/************************************************************************/
void renderer::SetGBufferTechnique(GBUFFER_TECH _technique)
{
	if(curState.gBuffer_tech == _technique) // Avoid redundant set render states
		return; 
	switch(_technique)
	{
	case CLEARGBUFFER_TECH:
		HR(m_FXGBuffer->SetTechnique(m_FXHandlesGBuffer.h_ClearGBuffer_Tech), L"renderer::SetGBufferTechnique() - Failed to set h_ClearGBuffer_Tech: ");
		curState.gBuffer_tech = CLEARGBUFFER_TECH;
		break;
	case TEXTUREDMESH_TECH:
		HR(m_FXGBuffer->SetTechnique(m_FXHandlesGBuffer.h_TexturedMesh_Tech), L"renderer::SetGBufferTechnique() - Failed to set h_TexturedMesh_Tech: ");
		curState.gBuffer_tech = TEXTUREDMESH_TECH;
		break;
	case TEXTUREDMESH_WIREFRAME_TECH:
		HR(m_FXGBuffer->SetTechnique(m_FXHandlesGBuffer.h_TexturedMesh_Wireframe_Tech), L"renderer::SetGBufferTechnique() - Failed to set h_TexturedMesh_Wireframe_Tech: ");
		curState.gBuffer_tech = TEXTUREDMESH_WIREFRAME_TECH;
		break;
	case SINGLECOLORMESH_WIREFRAME_TECH:
		HR(m_FXGBuffer->SetTechnique(m_FXHandlesGBuffer.h_SingleColorMesh_Wireframe_Tech), L"renderer::SetGBufferTechnique() - Failed to set h_SingleColorMesh_Wireframe_Tech: ");
		curState.gBuffer_tech = SINGLECOLORMESH_WIREFRAME_TECH;
		break;
	case SINGLECOLORMESH_TECH:
		HR(m_FXGBuffer->SetTechnique(m_FXHandlesGBuffer.h_SingleColorMesh_Tech), L"renderer::SetGBufferTechnique() - Failed to set h_SingleColorMesh_Tech: ");
		curState.gBuffer_tech = SINGLECOLORMESH_TECH;
		break;
	case SKYBOX_TECH:
		HR(m_FXGBuffer->SetTechnique(m_FXHandlesGBuffer.h_SkyBox_Tech), L"renderer::SetGBufferTechnique() - Failed to set h_SkyBox_Tech: ");
		curState.gBuffer_tech = SKYBOX_TECH;
		break;
	default:
		throw std::runtime_error("renderer::SetGBufferTechnique() - Technique not recognised");
	}
}

/************************************************************************/
/* Name:		SetPostProcessingTexelSize								*/
/* Description:	Set the FX parameters for Texel and Texture size		*/
/************************************************************************/
void renderer::SetGBufferTexelSize(D3DXVECTOR2 * _texelSize)
{
	if(curState.gBufferTexelSize.x != _texelSize->x ) // Avoid redundant set render states
	{
		HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gTexelSizeX, &_texelSize->x, sizeof(float)), L"renderer::SetGBufferTexelSize() - Set Value h_gTexelSizeX Failed: ");
		curState.gBufferTexelSize.x = _texelSize->x;
	}
	if(curState.gBufferTexelSize.y != _texelSize->y ) // Avoid redundant set render states
	{
		HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gTexelSizeY, &_texelSize->y, sizeof(float)), L"renderer::SetGBufferTexelSize() - Set Value h_gTexelSizeY Failed: ");
		curState.gBufferTexelSize.y = _texelSize->y;
	}
}

/************************************************************************/
/* Name:		RenderLightDegubObjects									*/
/* Description:	Render spheres and cones where the lights are.			*/
/************************************************************************/
void renderer::RenderLightObjects(bool drawVelocityBuffer)
{
	UINT pass;
	if(drawVelocityBuffer)
		pass = GBUFFER_TEXTUREDMESH_VELOCITY_NOSHADING_PASS;
	else
		pass = GBUFFER_TEXTUREDMESH_NOSHADING_PASS;

	// The world matrix, view matrix and position
	Mtrl mtrl; light * curLight = NULL;
	
	// Render the global light first
	lightSpotSM * globalLight = g_objectManager->GetLightSpotSM();
	globalLight->CalculateLightObjectWorldMatrix( CONE_INSIDERADIUS, CONE_HEIGHT, & cone::coneForward, LIGHT_VISULIZATION_SIZE);
	globalLight->GetLightMtrl(& mtrl);
	DrawTexturedMeshGBuffer(pass, g_objectManager->GetConeMesh()->meshData, 
		                    (D3DXMATRIXA16 *) globalLight->GetLightMatWorld(), (D3DXMATRIXA16 *) globalLight->GetLightMatWorldPrevFrame(), & mtrl);

	// Now render the spot and the point lights
	for(UINT i = 0; i < g_objectManager->GetNumLights(); i ++)
	{
		curLight = g_objectManager->GetLight(i);
		if(curLight->GetType() == T_POINT)
		{
			((lightPoint*)curLight)->CalculateLightObjectWorldMatrix(SPHERE_INSIDERADIUS, LIGHT_VISULIZATION_SIZE);
			curLight->GetLightMtrl(& mtrl);
			DrawTexturedMeshGBuffer(pass, g_objectManager->GetSphereMesh()->meshData, 
				                    (D3DXMATRIXA16 *) ((lightPoint*)curLight)->GetLightMatWorld(), (D3DXMATRIXA16 *) ((lightPoint*)curLight)->GetLightMatWorldPrevFrame(), & mtrl);
		}
		else if(curLight->GetType() == T_SPOT)
		{
			((lightSpot*)curLight)->CalculateLightObjectWorldMatrix(CONE_INSIDERADIUS, CONE_HEIGHT, & cone::coneForward, LIGHT_VISULIZATION_SIZE);
			curLight->GetLightMtrl(& mtrl);
			DrawTexturedMeshGBuffer(pass, g_objectManager->GetConeMesh()->meshData, 
									(D3DXMATRIXA16 *) ((lightSpot*)curLight)->GetLightMatWorld(), (D3DXMATRIXA16 *) ((lightSpot*)curLight)->GetLightMatWorldPrevFrame(), & mtrl);
		}
	}
}

/************************************************************************/
/* Name:		RenderLightDegubObjects									*/
/* Description:	Render spheres and cones where the lights are.			*/
/************************************************************************/
void renderer::renderLightVolumesWireframe()
{
	// Note: cone point is @ (0,0,0) and the base is a circle of radius 1 in the X,Y plane centered at (0,1,0)
	// Therefore, default fov = 45 deg (defined as the half angle).
	
	// The world matrix for the current light
	D3DXMATRIXA16 matWorld; D3DXMATRIXA16 tempMat;

	// Render the global light first
	g_objectManager->GetLightSpotSM()->CalculateLightVolumeWorldMatrix(& matWorld, & tempMat, CONE_INSIDERADIUS, CONE_HEIGHT, & cone::coneForward);
	DrawTexturedMeshWireframeGBuffer(GBUFFER_TEXTUREDMESH_WIREFRAME_NOSHADING_PASS, g_objectManager->GetConeMesh()->meshData, &matWorld); // Render on pass 0 so no lighting calculations are performed

	// Now render the spot and the point lights
	light * curLight = NULL;
	for(UINT i = 0; i < g_objectManager->GetNumLights(); i ++)
	{
		curLight = g_objectManager->GetLight(i);
		if(curLight->GetType() == T_POINT)
		{
			((lightPoint*)curLight)->CalculateLightVolumeWorldMatrix(& matWorld, SPHERE_INSIDERADIUS);
			DrawTexturedMeshWireframeGBuffer(GBUFFER_TEXTUREDMESH_WIREFRAME_NOSHADING_PASS, g_objectManager->GetSphereMesh()->meshData, &matWorld);
		}
		else if(curLight->GetType() == T_SPOT)
		{
			((lightSpot*)curLight)->CalculateLightVolumeWorldMatrix(& matWorld, & tempMat, CONE_INSIDERADIUS, CONE_HEIGHT, & cone::coneForward);
			DrawTexturedMeshWireframeGBuffer(GBUFFER_TEXTUREDMESH_WIREFRAME_NOSHADING_PASS, g_objectManager->GetConeMesh()->meshData, &matWorld);
		}
	}
}