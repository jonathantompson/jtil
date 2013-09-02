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
#include	"renderer\renderer_structures\drawableTexAtlas2D.h"
#include	"objectManager\objectManager.h"
#include	"lights\lightSpotSM.h"
#include	"renderer\renderer_structures\vertex.h"
#include	"renderer\renderer_structures\debugObject.h"
#include	"physics\objects\rbobjectMeshData.h"
#include	"UI\varNames.h"
#include	"UI\UI.h"
#include	"camera\camera.h"
#include	"renderer\utils\d3dProfiler.h"
#include	"renderer\renderer_structures\resettableResources\resettableDrawableTexAtlas2D.h"
#include    "utils_and_misc_classes\data_structures\vec_ptrs.h"
#include	<limits>
#include	"renderer\renderer_constants.h"
#include	"utils_and_misc_classes\math\math_funcs.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#ifndef lerp
	#define lerp(t, a, b) ( a + t * (b - a) )
#endif

/************************************************************************/
/* Name:		InitShadowMaps											*/
/* Description:	Initialize the shadow map structures					*/
/************************************************************************/
void renderer::InitShadowMaps()
{
	// If the shadow maps alredy exist then delete them
	if(m_shadowMap) 
	{ 
		// Find the resettable object and delete it from the global list
		if(g_objectManager)
		{
			vec_ptrs<resettable> *	resettableResources = g_objectManager->GetResettableResources();
			if(resettableResources)
			{
				bool resourceFound = false;
				for(UINT i = 0; i < resettableResources->Size(); i ++)
				{
					resettable * curResource = resettableResources->GetElem(i);
					if(curResource->GetType() == T_RESETTABLE_DRAWABLETEXATLAS2D)
					{
						if(reinterpret_cast<resettableDrawableTexAtlas2D *>(curResource)->GetTex() == m_shadowMap)
						{
							reinterpret_cast<resettableDrawableTexAtlas2D *>(curResource)->Release();
							resettableResources->RemoveElement(i);
							resourceFound = true;
							break;
						}
					}
				}
				if(resourceFound == false)
					throw std::runtime_error("renderer::InitShadowMaps() - Trying to clear SM array but couldn't find a resettable shadow map resource!");
			}
			else
				throw std::runtime_error("renderer::InitShadowMaps() - Trying to clear SM array but resettableResources array doesn't exist!");
		}
		else
			throw std::runtime_error("renderer::InitShadowMaps() - Trying to clear SM array but object manager doesn't exist!");
		delete [] m_shadowMap; m_shadowMap = NULL;
	}

	D3DFORMAT smapFormat = (D3DFORMAT)g_UI->GetComboBoxVal( & var_SMFormat ); // Default is typically G32R32F
	curState.SMShaderBilinearFiltering = false;
	if(g_UI->GetSetting<bool>(&var_SMFilter))
	{
		// Try and find a SMAP format that supports auto mip map generation and 32bit floating point filtering
		D3DFORMAT backBufferFormat = g_renderer->GetAppPresentParameters()->BackBufferFormat;
		D3DFORMAT formats[2];
		switch(smapFormat)
		{
		case D3DFMT_G32R32F:
			formats[0] = D3DFMT_G32R32F; formats[1] = D3DFMT_A32B32G32R32F;
			break;
		case D3DFMT_G16R16F:
			formats[0] = D3DFMT_G16R16F; formats[1] = D3DFMT_A16B16G16R16F;
			break;
		case D3DFMT_G16R16:
			formats[0] = D3DFMT_G16R16; formats[1] = D3DFMT_A16B16G16R16;
			break;
		default:
			throw std::runtime_error("renderer::InitShadowMaps() - Error: input smapFormat is not supported.");
		}
		int i = 0;
		for(; i < 2; i ++)
		{
			smapFormat = formats[i];
			if(!FAILED(m_pD3DObj->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFormat, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, smapFormat)) &&
			   !FAILED(m_pD3DObj->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL , backBufferFormat, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, smapFormat)))
			   break;
		}
		if( i == 2 ) // Nothing supports filtering...
		{
			smapFormat = (D3DFORMAT)g_UI->GetComboBoxVal( & var_SMFormat ); // Back to default if nothing supports it!
			curState.SMShaderBilinearFiltering = true;
#ifdef _DEBUG
			OutputDebugString(L"\nrenderer::InitShadowMaps() - WARNING: Hardware support for filtering FP textures is not avaliable!\nUsing manual filtering instead.  Expect poor performance.\n\n");
#endif
		}
	}

	// Get settings
	m_SMSize = g_UI->GetSetting<int>(&var_SMSize);
	m_smapTexelSize = 1.0f / (float)m_SMSize;
	m_smapSoftness = g_UI->GetSetting<float>(&var_SMSoftness);
	if(m_smapSoftness > SMAP_SOFTNESS_MAX)
		throw std::runtime_error("renderer::InitShadowMaps() - var_SMSoftness is larger than SMAP_SOFTNESS_MAX (0.5f default)");
	m_numShadowMaps = g_UI->GetSetting<int>(&var_SMCascadeCount);
	int _smapMipLevels = g_UI->GetSetting<int>(&var_SMMipLevels);
	m_smapBlendZone = g_UI->GetSetting<float>(&var_SMBlendZone);

	// Compute the maximum LOD from the softness setting
    // Note that this effectively makes it a logarithmic slider of filter width,
    // which is convenient and intuitive.
    float LogSMDim = log((float)m_SMSize) / log(2.0f);
    m_MaxLOD = m_smapSoftness * LogSMDim;
	m_MinFilterWidth = pow(2.0f, m_MaxLOD);

	// Check the settings
	if(m_numShadowMaps > MAX_NUM_SHADOW_MAPS || m_numShadowMaps < 0)
		throw std::runtime_error("renderer::InitShadowMaps() - Incorrect number of shadow maps (var_SMCascadeCount)");
	if( m_SMSize != 128 && m_SMSize != 256 && m_SMSize != 512 && m_SMSize != 1024 && m_SMSize != 2048)
		throw std::runtime_error("renderer::InitShadowMaps() - var_SMSize must be a factor of 2 between (and including) 128 and 2048");

	// Allocate space for each shadowmap
	m_shadowMap = new drawableTexAtlas2D[2]; // Need an extra texture for ping-pong blur filter

	if(m_shadowMapSplitDepths) { delete [] m_shadowMapSplitDepths; }
	m_shadowMapSplitDepths = new float[m_numShadowMaps + 1];

	if(m_shadowMapEffectSplitDepths) { delete [] m_shadowMapEffectSplitDepths; }
	m_shadowMapEffectSplitDepths = new float[MAX_NUM_SHADOW_MAPS - 1];
	
	if(m_matShadowMap_CamViewInv_LightViewProjs) { delete [] m_matShadowMap_CamViewInv_LightViewProjs; }
	m_matShadowMap_CamViewInv_LightViewProjs = new D3DXMATRIX[m_numShadowMaps];

	if(m_matShadowMapProjs) { delete [] m_matShadowMapProjs; }
	m_matShadowMapProjs = new D3DXMATRIX[m_numShadowMaps];

	if(m_ShadowMapSplitScales) { delete [] m_ShadowMapSplitScales; }
	m_ShadowMapSplitScales = new D3DXVECTOR2[m_numShadowMaps];
		
	// Make new shadowmap drawable textures
	// NOTE: THERE IS ONE TEXTURE EXTRA FOR POST-PROCESSING --> WHEN BLURING THE SHADOW MAPS IN SCREEN SPACE.
	bool autogenMipMaps = false;
 	m_shadowMap[0].InitDrawableTexAtlas2D(m_SMSize, m_SMSize, m_numShadowMaps, _smapMipLevels, smapFormat, autogenMipMaps);
	m_shadowMap[1].InitDrawableTexAtlas2D(m_SMSize, m_SMSize, m_numShadowMaps, _smapMipLevels, smapFormat, autogenMipMaps);

	// Compute the maximum LOD from the softness setting
    // Note that this effectively makes it a logarithmic slider of filter width,
    // which is convenient and intuitive.
	float MaxDim = (float)(max(m_SMSize, m_SMSize)); // Max of width and height
    float LogMaxDim = log(MaxDim) / log(2.0f);
    m_MaxLOD = g_UI->GetSetting<float>(&var_SMSoftness) * LogMaxDim;
	m_MinFilterWidth = pow(2.0f, m_MaxLOD);

	// Calculate split depths and matricies (this will also be done once per frame)
	camera * sceneCamera = g_objectManager->GetCamera();
	lightSpotSM * sceneLight = g_objectManager->GetLightSpotSM();
	CalcShadowMappingSplitDepths(m_shadowMapSplitDepths, sceneCamera, m_shadowMapEffectSplitDepths);
	CalcShadowMappingSplitMatricies(sceneLight, sceneCamera);

	// Send some constants to the shaders
	float _minVariance = g_UI->GetSetting<float>(&var_SMMinVariance);
	float _depthEpsilon = g_UI->GetSetting<float>(&var_SMDepthEpsilon);
	float _LBR = g_UI->GetSetting<float>(&var_SMLightBleedingReduction);
	int m_numShadowMaps_int = (int)m_numShadowMaps;
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gVSMMinVariance, &_minVariance, sizeof(float)), L"renderer::InitShadowMaps() - Set Value h_gVSMMinVariance Failed: ");
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gLBRAmount, &_LBR, sizeof(float)), L"renderer::InitShadowMaps() - Set Value h_gLBRAmount Failed: ");
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gNumShadowMaps, &m_numShadowMaps_int, sizeof(int)),L"renderer::InitShadowMaps() - Set Value h_gNumShadowMaps Failed: ");
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gShadowMapBlendZone, &m_smapBlendZone, sizeof(float)),L"renderer::InitShadowMaps() - Set Value h_gShadowMapBlendZone Failed: ");
	HR(m_FXSMap->SetValue(m_FXHandlesSMap.h_gVSMDepthEpsilon, &_depthEpsilon, sizeof(float)), L"renderer::InitShadowMaps() - Set Value h_gVSMDepthEpsilon Failed: ");

	// Values used for indexing texture atlas
	D3DXVECTOR2 textureOffsets[MAX_NUM_SHADOW_MAPS];
	for(UINT i = 0; i < m_numShadowMaps; i ++)
		textureOffsets[i] = m_shadowMap->GetTextureOffsets()[i];
	for(UINT i = m_numShadowMaps; i < MAX_NUM_SHADOW_MAPS; i ++)
		textureOffsets[i] = D3DXVECTOR2(1.0f, 1.0f); // Clamp the rest at shadowmap border
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gSMTextureAtlasOffsets, & textureOffsets, sizeof(D3DXVECTOR2)*MAX_NUM_SHADOW_MAPS), L"renderer::InitShadowMaps() - Set Value h_gSMTextureAtlasOffsets Failed: ");
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gSMTextureAtlasScale,  m_shadowMap->GetTextureScale(), sizeof(D3DXVECTOR2)), L"renderer::InitShadowMaps() - Set Value h_gSMTextureAtlasScale Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSMTextureAtlasOffsets, & textureOffsets, sizeof(D3DXVECTOR2)*MAX_NUM_SHADOW_MAPS), L"renderer::InitShadowMaps() - Set Value h_gSMTextureAtlasOffsets Failed: ");
	HR(m_FXPostProcess->SetValue(m_FXHandlesPostProcess.h_gSMTextureAtlasScale,  m_shadowMap->GetTextureScale(), sizeof(D3DXVECTOR2)), L"renderer::InitShadowMaps() - Set Value h_gSMTextureAtlasScale Failed: ");

	// Make sure the SM texture sampler states are correct
	bool hardwareFilter = g_UI->GetSetting<bool>(&var_SMFilter) && !curState.SMShaderBilinearFiltering;
	SetSMSamplerStates(hardwareFilter);

	// Check that the depth stencil surface is large enough and if not, make a new one
	if(m_DepthStencilSurf) { ReleaseCOM(m_DepthStencilSurf); }
	g_renderer->CreateDepthStencilSurface();
}

/************************************************************************/
/* Name:		UpdateShadows											*/
/* Description:	Call when shadow map preferences have changed.			*/
/************************************************************************/
void renderer::UpdateShadows()
{
	InitShadowMaps(); // Just reinitialize the shadowmaps with the new settings
	SetSMTextures();
}

/************************************************************************/
/* Name:		DrawAllShadowMaps										*/
/* Description: Draw all the cascaded shadow maps						*/
/************************************************************************/
void renderer::DrawAllShadowMaps()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	
	// Set the effect light structure
	lightSpotSM * SMLight = g_objectManager->GetLightSpotSM();
	HR(m_FXSMap->SetValue(m_FXHandlesSMap.h_gLight, SMLight->GetSpotLightWorldCoords(), sizeof(SpotLight)),L"renderer::DrawAllShadowMaps() - Set Value h_gLight Failed: ");

	// Calculate shadow map transforms
	camera * sceneCamera = g_objectManager->GetCamera();
	lightSpotSM * sceneLight = g_objectManager->GetLightSpotSM();
	CalcShadowMappingSplitDepths(m_shadowMapSplitDepths, sceneCamera, m_shadowMapEffectSplitDepths);
	CalcShadowMappingSplitMatricies(sceneLight, sceneCamera);

	// Start the render pass
	m_shadowMap->BeginSceneFullViewport();
	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, SMAP_CLEAR_COLOR, SMAP_CLEAR_DEPTH, 0),L"renderer::DrawAllShadowMaps() - Failed to clear device: ");
	SetSMapTechnique(BUILDSHADOWMAP_TECH);
	UINT numPasses = 0;
	HR(m_FXSMap->Begin(&numPasses, 0),L"renderer::DrawAllShadowMaps() - m_FX->Begin Failed: ");

	// Render each shadowmap
	for(UINT i = 0; i < m_numShadowMaps; i ++)
	{
		m_currentShadowMapToRender = i;
		m_shadowMap->SetViewport(i);
		// TO DO: Perform frustrum culling here...
		DrawShadowMap(m_shadowMap, i);
	}
	HR(m_FXSMap->End(),L"renderer::DrawAllShadowMaps() - m_FX->End Failed: ");
	m_shadowMap->EndScene();

	// Blur the shadow maps
	if(g_UI->GetSetting<bool>(& var_SMBlur))
		BlurShadowMaps(m_shadowMap);

	GenerateSMMipSubLevels();
}//drawShadowMap

/************************************************************************/
/* Name:		DrawShadowMap											*/
/* Description:	Render scene from the light perspective as shadow map	*/
/************************************************************************/
void renderer::DrawShadowMap(drawableTexAtlas2D * shadowMap, UINT shadowMapNumber)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	// DRAW ALL OBJECTS HERE
	if(g_UI->GetSetting<bool>(&var_drawModels))
		g_objectManager->RenderRBObjectMeshesSM();

	// Render solid triangles if user has selected the option to do so
	if(!g_UI->GetSetting<bool>(&var_drawOBBAsLines))
	{
		if(g_UI->GetSetting<bool>(&var_drawOBBTree))
			g_objectManager->RenderRBObjectMeshOBBsSM();
	}

	if(g_objectManager->GetDebugObjects())
			g_objectManager->GetDebugObjects()->RenderSM(); // Recursively draw the debugObjects linked-list.

	// FINISH DRAWING OBJECTS HERE
}//drawShadowMap

/************************************************************************/
/* Name:		BlurShadowMap											*/
/* Description:	Blur the shadow map using a BoxBlur						*/
/************************************************************************/
void renderer::BlurShadowMaps(drawableTexAtlas2D * shadowMap)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	
	// Use the last array slice (which is extra) for temporary space
	BoxBlurSMTextureAtlas(& shadowMap[0], & shadowMap[1] );
} //drawShadowMap

/************************************************************************/
/* Name:		DrawTrianglesColSM										*/
/* Description: Draw flat triangles with color for rendering shadowmap	*/
/************************************************************************/
void renderer::DrawTrianglesColSM(IDirect3DVertexBuffer9 * vertBuff, DWORD vertSize, IDirect3DIndexBuffer9 * indBuff, DWORD indSize, D3DXMATRIXA16 * matWorld )
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	HR(m_FXSMap->BeginPass(0),L"renderer::DrawTrianglesColSM() - m_FX->BeginPass Failed: ");

	HR(m_pD3DDev->SetStreamSource(0, vertBuff, 0, sizeof(VertexPosNormCol)),L"renderer::DrawTrianglesColSM() - SetStreamSource failed: ");
	HR(m_pD3DDev->SetIndices(indBuff),L"renderer::DrawTrianglesColSM() - SetIndices failed: ");
	HR(m_pD3DDev->SetVertexDeclaration(VertexPosNormCol::Decl),L"renderer::DrawTrianglesColSM() - SetVertexDeclaration failed: ");

	SetSMMatricies(matWorld);

	HR(m_FXSMap->CommitChanges(),L"renderer::DrawTrianglesColSM() - CommitChanges failed: ");

	// Now draw the mesh
	HR(m_pD3DDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vertSize, 0, indSize/3), L"renderer::DrawTrianglesColSM() - DrawIndexedPrimitive failed: ");

	HR(m_FXSMap->EndPass(),L"renderer::DrawTrianglesColSM() - m_FX->EndPass Failed: ");
}


/************************************************************************/
/* Name:		SetSMMatricies											*/
/* Description: Set the world and view projections for rendering to SM	*/
/************************************************************************/
void renderer::SetSMMatricies(D3DXMATRIXA16 * matWorld)
{
	// Setup the matricies
	// We want this:
	//		a) m_hW = matWorld   --> NOT NEEDED
	//		b) m_hWV = matWorld * m_lightView
	//      c) m_hWVP = matWorld * m_lightView * m_lightProj

	// A) m_hW
	//HR(m_FX->SetMatrix(m_FXHandles.m_hW, matWorld), L"Render::SetSMMatricies() - Failed to set m_hWV matrix: "); 

	// b) m_hWV
	D3DXMATRIX WV;
	D3DXMatrixMultiply(& WV, matWorld, & g_objectManager->GetLightSpotSM()->m_lightView); // matWorld is current RBO model->world transform
	HR(m_FXSMap->SetMatrix(m_FXHandlesSMap.h_gWV, & WV), L"renderer::SetSMMatricies() - Failed to set h_gWV matrix: "); 

	// c) m_hWVP
	D3DXMATRIX WVP;
	D3DXMatrixMultiply(& WVP, & WV, & m_matShadowMapProjs[m_currentShadowMapToRender]); // matWorld is current RBO model->world transform
	HR(m_FXSMap->SetMatrix(m_FXHandlesSMap.h_gWVP, & WVP), L"renderer::SetSMMatricies() - Failed to set h_gWVP matrix: "); 
}

/************************************************************************/
/* Name:		DrawTexturedMeshSM										*/
/* Description:	Draw a textured mesh object to the shadow map			*/
/************************************************************************/
void renderer::DrawTexturedMeshSM(rbobjectMeshData * meshData, D3DXMATRIXA16 * matWorld)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	HR(m_FXSMap->BeginPass(0),L"renderer::DrawTexturedMeshSM() - m_FX->BeginPass Failed: ");

	SetSMMatricies(matWorld);
	
	HR(m_FXSMap->CommitChanges(),L"renderer::DrawTexturedMeshSM() - CommitChanges failed: ");

	for(UINT j = 0; j < meshData->materials->Size(); ++j)
	{
		HR(meshData->pMesh->DrawSubset(j),L"renderer::DrawTexturedMeshSM() - DrawSubset failed: ");
	}

	HR(m_FXSMap->EndPass(),L"renderer::DrawTexturedMeshSM() - m_FX->EndPass Failed: ");
}

/************************************************************************/
/* Name:		SetVisualizeSplits										*/
/* Description:	Set the shader boolian visualizeSplits					*/
/************************************************************************/
void renderer::SetVisualizeSplits(int _visualizeSplits)
{
	if(curState.visualizeSplits == _visualizeSplits) // Avoid redundant set render states
		return; 
	else
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gVisualizeSplits, &_visualizeSplits, sizeof(int)), L"renderer::SetVisualizeSplits() - Set Value h_gVisualizeSplits Failed: ");
		curState.visualizeSplits = _visualizeSplits;
	}
}

/************************************************************************/
/* Name:		CalcShadowMappingSplitDepths							*/
/* Description:	Calculate the cascaded shadowmap split depths according */
/*				to paper.												*/
/************************************************************************/
void renderer::CalcShadowMappingSplitDepths(float * depths, camera * _camera, float * effectDepths)
{
	D3DXVECTOR2 * nearFar = _camera->GetNearFar();
	float lambda = g_UI->GetSetting<float>(&var_SMCascadeSplitLogFactor);

	// Implements the "practical" split scheme from the PSSM paper
	float SplitRange = nearFar->y - nearFar->x;
	float SplitRatio = nearFar->y / nearFar->x;

	for (unsigned int i = 0; i < m_numShadowMaps; ++i ) 
	{
		float p			   = (float)i / (float)m_numShadowMaps;
		float LogSplit     = nearFar->x * pow(SplitRatio, p);
		float UniformSplit = nearFar->x + SplitRange * p;
		// Lerp between the two schemes
		float Split        = lambda * (LogSplit - UniformSplit) + UniformSplit;
		depths[i] = Split;
	}

	// Just for simplicty later, push the camera far plane as the last "split"
	depths[m_numShadowMaps] = nearFar->y;

	// Update the effect variable
	// NOTE: Single 4-tuple, so maximum of five splits right now
	float CameraSplitRatio = nearFar->y / (nearFar->y - nearFar->x);

	unsigned int effectDepthIndex;
	for (effectDepthIndex = 1; effectDepthIndex < m_numShadowMaps; ++effectDepthIndex) {
			// NOTE: Have to rescale the splits. The effect expects them to be in post-projection,
			// but pre-homogenious divide [0,...,far] range, relative to the REAL camera near
			// and far.
			float Value = (depths[effectDepthIndex] - nearFar->x) * CameraSplitRatio;
			effectDepths[effectDepthIndex-1] = Value;
	}
	// Fill in the rest with "a large number"
	for (; effectDepthIndex <= (MAX_NUM_SHADOW_MAPS-1); ++effectDepthIndex) {
// This is a hack, but we need to use the keyword max (from limits library), but windows library has already defined it.
#undef max
		effectDepths[effectDepthIndex-1] = std::numeric_limits<float>::max();
#define max(a,b)            (((a) > (b)) ? (a) : (b))
	}

	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gSplitDistances, effectDepths, sizeof(float) * (MAX_NUM_SHADOW_MAPS-1)),L"renderer::CalcShadowMappingSplitDepths() - Set Value h_gSplitDistances Failed: ");
	HR(m_FXGBuffer->SetValue(m_FXHandlesGBuffer.h_gSplitDistances, effectDepths, sizeof(float) * (MAX_NUM_SHADOW_MAPS-1)),L"renderer::CalcShadowMappingSplitDepths() - Set Value h_gSplitDistances Failed: ");

}

/************************************************************************/
/* Name:		CalcShadowMapMatricies									*/
/* Description:	Calculate the matricies for the current shadow map		*/
/* Some of this code is taken from chapter 8 sample code of GPU Gems 3  */
/************************************************************************/
void renderer::CalcShadowMappingSplitMatricies(lightSpotSM * _light, camera * _camera)
{
	// Compute the minimum filter width in normalized device coordinates ([-1, 1])
	// NOTE: We compute *half* of this width here for modifying a region later.
	D3DXVECTOR2 HalfMinFilterWidthNDC;
	HalfMinFilterWidthNDC.x = m_MinFilterWidth / m_SMSize; // m_Width = ShadowMap Size
	HalfMinFilterWidthNDC.y = m_MinFilterWidth / m_SMSize;

	// Extract some useful camera-related information
	D3DXMATRIX * CameraViewInv = _camera->GetMatViewInv();

	float XScaleInv = 1.0f / _camera->GetMatProjection()->_11;
	float YScaleInv = 1.0f / _camera->GetMatProjection()->_22;

	// Construct a matrix that transforms from camera view space into projected light space
	// D3DXMATRIX LightViewProj = _light->m_lightView * _light->m_lightProj; // Should already be updated
	D3DXMATRIX ViewToProjLightSpace = *CameraViewInv * _light->m_LightVP;

	D3DXVECTOR3 Corners[8];
	D3DXVECTOR4 CornersProj[8];
	for(UINT i = 0; i < m_numShadowMaps; ++i) 
	{
		// Compute corners for this frustum
		// Need to overlap min into blend zone for all splits OTHER than the first
		float Near;
		if(i > 0)
		{
			float lastSplitDistance = m_shadowMapSplitDepths[i] - m_shadowMapSplitDepths[i-1]; // distance of the split BEFORE this one
			Near = m_shadowMapSplitDepths[i] - (lastSplitDistance * m_smapBlendZone);
		}
		else
			Near = m_shadowMapSplitDepths[i];
		float Far  = m_shadowMapSplitDepths[i+1];

		float splitDistance = Far - Near;
		Near = Near - splitDistance * SMAP_OVERLAP;
		Far = Far + splitDistance * SMAP_OVERLAP;

		// Near corners (in view space)
		float NX = XScaleInv * Near;
		float NY = YScaleInv * Near;
		Corners[0] = D3DXVECTOR3(-NX,  NY, Near);
		Corners[1] = D3DXVECTOR3( NX,  NY, Near);
		Corners[2] = D3DXVECTOR3(-NX, -NY, Near);
		Corners[3] = D3DXVECTOR3( NX, -NY, Near);
		// Far corners (in view space)
		float FX = XScaleInv * Far;
		float FY = YScaleInv * Far;
		Corners[4] = D3DXVECTOR3(-FX,  FY, Far);
		Corners[5] = D3DXVECTOR3( FX,  FY, Far);
		Corners[6] = D3DXVECTOR3(-FX, -FY, Far);
		Corners[7] = D3DXVECTOR3( FX, -FY, Far);

		// Transform corners into projected light space
		D3DXVec3TransformArray(CornersProj, sizeof(D3DXVECTOR4), Corners, sizeof(D3DXVECTOR3), &ViewToProjLightSpace, 8);

		// TODO: Adjust Near/Far and corresponding depth scaling
		D3DXVECTOR2 Min( 1,  1);
		D3DXVECTOR2 Max(-1, -1);
		for (unsigned int c = 0; c < 8; ++c) {
			// Homogenious divide x and y
			const D3DXVECTOR4& p = CornersProj[c];

			if (p.z < 0.0f) {
				// In front of near clipping plane! Be conservative...
				Min = D3DXVECTOR2(-1, -1);
				Max = D3DXVECTOR2( 1,  1);
				break;
			} else {
				D3DXVECTOR2 v(p.x, p.y);
				v *= 1.0f / p.w;
				// Update boundaries
				D3DXVec2Minimize(&Min, &Min, &v);
				D3DXVec2Maximize(&Max, &Max, &v);
			}
		}

		// Degenerate slice?
		D3DXVECTOR2 Dim = Max - Min;
		if (Max.x <= -1.0f || Max.y <= -1.0f || Min.x >= 1.0f || Min.y >= 1.0f ||
			Dim.x <= 0.0f || Dim.y <= 0.0f) {
				// TODO: Something better... (skip this slice)
				Min = D3DXVECTOR2(-1, -1);
				Max = D3DXVECTOR2( 1,  1);
		}

		// TODO: Clamp extreme magnifications, since they will cause gigantic blurs.
		// Not an issue if we were using PSSAVSM though (i.e. summed-area tables)

		// Expand region by minimum filter width in each dimension to make sure that
		// we can blur properly and get adjacent geometry. Arguably mipmapping will
		// still be wrong for extreme minifications, but that won't be noticable.
		Min -= HalfMinFilterWidthNDC;
		Max += HalfMinFilterWidthNDC;

		// Clamp to valid range
		Min.x = min(1.0f, max(-1.0f, Min.x));
		Min.y = min(1.0f, max(-1.0f, Min.y));
		Max.x = min(1.0f, max(-1.0f, Max.x));
		Max.y = min(1.0f, max(-1.0f, Max.y));

		// Compute scale and offset
		D3DXVECTOR2 Scale;
		Scale.x = 2.0f / (Max.x - Min.x);
		Scale.y = 2.0f / (Max.y - Min.y);
		D3DXVECTOR2 Offset;
		Offset.x = -0.5f * (Max.x + Min.x) * Scale.x;
		Offset.y = -0.5f * (Max.y + Min.y) * Scale.y;

		// Store scale factors for later use when blurring
		m_ShadowMapSplitScales[i].x = Scale.x; m_ShadowMapSplitScales[i].y = Scale.y;

		// Adjust projection matrix to "zoom in" on the target region
		D3DXMATRIX Zoom( Scale.x,   0.0f,		0.0f,   0.0f,
						 0.0f,		Scale.y,	0.0f,   0.0f,
						 0.0f,		0.0f,		1.0f,   0.0f,
						 Offset.x,	Offset.y,	0.0f,   1.0f);

		// Compute new composite matrices and store
		m_matShadowMapProjs[i] = _light->m_lightProj * Zoom;

		/*
		// EDIT: Jonathan - To prevent swimming we want to project a constant point into the SMAP and 
		// adjust the translation so that it fits into a consant pixel.
		D3DXMATRIX lightViewProj = _light->m_lightView * m_matShadowMapProjs[i];
		const D3DXVECTOR3 worldOrigin(0.0f, 0.0f, 0.0f);
		D3DXVECTOR3 transformedOrigin;
		D3DXVec3TransformCoord(& transformedOrigin, & worldOrigin, & lightViewProj);

		// Find nearest shadow map texel. The 0.5f is because x,y are in the 
		// range -1 .. 1 and we need them in the range 0 .. 1
		float texCoordX = 0.5f * transformedOrigin.x * (float)m_SMSize;
		float texCoordY = 0.5f * transformedOrigin.y * (float)m_SMSize;

		float dx = Round(texCoordX) - texCoordX;
		float dy = Round(texCoordY) - texCoordY;

		// Transform back into homogeneous light space [-1,1]
		dx /= (float)m_SMSize * -0.5f;
		dy /= (float)m_SMSize * -0.5f;

		// Use this to create a rounding matrix with which to alter the current shadow map
		D3DXMATRIX xRounding;
		D3DXMatrixTranslation(&xRounding, dx, dy, 0); 
		m_matShadowMapProjs[i] = m_matShadowMapProjs[i] * xRounding;
		*/

		// Update update to include inverse camera as well
		m_matShadowMap_CamViewInv_LightViewProjs[i] = (*CameraViewInv) * _light->m_lightView * m_matShadowMapProjs[i];
	} // for(UINT i = 0; i < m_numShadowMaps; ++i) 

	// Tighly fit the light bounds to scene geometry (done earlier)

	// Update matrices in shader
	HR(m_FXLighting->SetMatrixArray(m_FXHandlesLighting.h_gSplitVcaminvVlightPlight_Matrices, m_matShadowMap_CamViewInv_LightViewProjs, m_numShadowMaps),L"renderer::CalcShadowMappingSplitMatricies() -  Cannot set h_gSplitVPMatrices: ");
}

/************************************************************************/
/* Name:		LoadSMTextures											*/
/* Description:	Load in the textures to the shaders						*/
/************************************************************************/
void renderer::SetSMTextures()
{
	// Set the scenes shadowmap textures
	HR(m_FXLighting->SetTexture(m_FXHandlesLighting.h_gShadowMap, m_shadowMap->d3dTex()),L"renderer::SetSMTextures() - Set Value h_gShadowMap Failed: ");
}

/************************************************************************/
/* Name:		GenerateSMMipSubLevels									*/
/* Description:	Call D3DX to generate mip sublevels						*/
/************************************************************************/
void renderer::GenerateSMMipSubLevels()
{
	// Set the scenes shadowmap textures (this is messy) TO DO: put into texture atlas
	if(m_shadowMap)
		m_shadowMap->GenerateTexMipSubLevels();
}

/************************************************************************/
/* Name:		SetSMapTechnique										*/
/* Description:	Set the desired technique								*/
/************************************************************************/
void renderer::SetSMapTechnique(SMAP_TECH _technique)
{
	if(curState.SMap_tech == _technique) // Avoid redundant set render states
		return; 
	switch(_technique)
	{
	case BUILDSHADOWMAP_TECH:
		HR(m_FXSMap->SetTechnique(m_FXHandlesSMap.h_BuildShadowMap_Tech), L"renderer::SetSMapTechnique() - Failed to set h_BuildShadowMap_Tech: ");
		curState.SMap_tech = BUILDSHADOWMAP_TECH;
		break;
	default:
		throw std::runtime_error("renderer::SetSMapTechnique() - Technique not recognised");
	}
}

/************************************************************************/
/* Name:		SetSMSamplerStates										*/
/* Description: Set the correct sampler render states					*/
/************************************************************************/
void renderer::SetSMSamplerStates(bool filtering)
{
	m_SMSamplerState.addressU = D3DTADDRESS_CLAMP;
	m_SMSamplerState.addressV = D3DTADDRESS_CLAMP;
	m_SMSamplerState.magFilter = D3DTEXF_POINT; // No need to check for hardware support.  POINT and NONE are always supported
	m_SMSamplerState.minFilter = D3DTEXF_POINT;
	m_SMSamplerState.mipFilter = D3DTEXF_NONE;
	m_SMSamplerState.maxAnisotropy = 1;
	if(filtering)
	{
		D3DFORMAT texFormat = m_shadowMap->GetTexFormat();
		D3DFORMAT backBufferFormat = g_renderer->GetAppPresentParameters()->BackBufferFormat;
		// Check for hardware support:
		if(FAILED(m_pD3DObj->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFormat, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DRTYPE_TEXTURE, texFormat)) ||
			FAILED(m_pD3DObj->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL , backBufferFormat, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, texFormat)))
			throw std::runtime_error("renderer::SetSMSamplerStates() - Error, trying to enable hardware filtering on a SM texture format that doesn't support it!");

		// Get the best mag filter
		if(m_DeviceCaps->TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
		{ m_SMSamplerState.magFilter = D3DTEXF_ANISOTROPIC; m_SMSamplerState.maxAnisotropy = 16; }
		else if(m_DeviceCaps->TextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR)
			m_SMSamplerState.magFilter = D3DTEXF_LINEAR;
		else
			m_SMSamplerState.magFilter = D3DTEXF_POINT;

		// Get the best min filter
		if(m_DeviceCaps->TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC)
		{ m_SMSamplerState.minFilter = D3DTEXF_ANISOTROPIC; m_SMSamplerState.maxAnisotropy = 16; }
		else if(m_DeviceCaps->TextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR)
			m_SMSamplerState.minFilter = D3DTEXF_LINEAR;
		else
			m_SMSamplerState.minFilter = D3DTEXF_POINT;

		// Get the best mip filter --> There is no ANISOTROPIC mip filter cap
		//if(m_DeviceCaps->TextureFilterCaps & D3DPTFILTERCAPS_MIPFLINEAR)
		//	m_SMSamplerState.mipFilter = D3DTEXF_LINEAR;
		//else
		//	m_SMSamplerState.mipFilter = D3DTEXF_POINT;
		// EDIT: DON'T USE MIP FILTER --> RESULTED IN SOME WEIRD ERRORS
		m_SMSamplerState.mipFilter = D3DTEXF_NONE;
		 m_SMSamplerState.maxAnisotropy = 1;
	}

	HR(SetSamplerState(m_FXLighting, &m_FXHandlesLighting.h_gSampShadow, &curState.SMTexSampler, &m_SMSamplerState),L"renderer::SetSMSamplerStates() - SetSamplerState Failed: ");

}