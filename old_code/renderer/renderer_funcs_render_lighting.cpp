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
#include	"renderer\renderer_structures\drawableTexAtlas2D.h"
#include	"objectManager\objectManager.h"
#include	"sky\sky.h"
#include	"hud\hud.h"
#include	"lights\light.h"
#include	"lights\lightSpotSM.h"
#include	"lights\lightSpot.h"
#include	"lights\lightDir.h"
#include	"lights\lightPoint.h"
#include	"renderer\renderer_structures\vertex.h"
#include	"renderer\renderer_structures\debugObject.h"
#include	"renderer\renderer_structures\cone.h"
#include	"physics\objects\rbobjectMeshData.h"
#include	"physics\objects\rbobjectMesh.h"
#include	"UI\varNames.h"
#include	"UI\UI.h"
#include	"camera\camera.h"
#include	"renderer\utils\d3dProfiler.h"
#include	"renderer\renderer_constants.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		InitLighting											*/
/* Description:	Initialize the Lighting pass structures					*/
/************************************************************************/
void renderer::InitLighting()
{
	//D3DFORMAT depthFormat = (D3DFORMAT)g_UI->GetComboBoxVal( & var_depthStencilFormat );
	//D3DFORMAT backBufferFormat = m_PresentParameters.BackBufferFormat;
	D3DFORMAT accumulationFormat = (D3DFORMAT)g_UI->GetComboBoxVal(& var_AccumulationFormat);
	D3DFORMAT sceneFormat = (D3DFORMAT)g_UI->GetComboBoxVal(& var_SceneFormat);

	//if(FAILED(m_pD3DObj->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, backBufferFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, accumulationFormat)))
	//	throw std::runtime_error("renderer::InitLighting() - Error: The chosen Accumulation Format is not supported!");

	// Allocate new space
	if(m_accumulationBuffer) { delete m_accumulationBuffer; }
	m_accumulationBuffer = new drawableTex2D;
	for(UINT i = 0; i < 2; i ++)
	{	
		if(m_sceneBuffer[i]) { delete m_sceneBuffer[i]; m_sceneBuffer[i] = NULL; } 
		m_sceneBuffer[i] = new drawableTex2D;
	}

	// Make new drawable textures
	UINT _height = m_PresentParameters.BackBufferHeight;
	UINT _width = m_PresentParameters.BackBufferWidth;
	UINT _mipLevels = 1;
	bool autogenMipMaps = false;
	m_accumulationBuffer->InitDrawableTex2D(_width, _height, _mipLevels, accumulationFormat, autogenMipMaps);
	for(UINT i = 0; i < 2; i ++)
		m_sceneBuffer[i]->InitDrawableTex2D(_width, _height, _mipLevels, sceneFormat, autogenMipMaps);

	// Create samplers
	m_gLightAccumulationSampler.SetFilter(D3DTEXF_POINT, D3DTADDRESS_CLAMP, 1);

	// Set the texture sampler to a valid state, otherwise the first draw call using it will throw an error:
	// Use the white texture sampler as it is always supported.
	HR(SetSamplerState(m_FXGBuffer, &m_FXHandlesGBuffer.h_gSampTex, &curState.texSampler, &m_WhiteTexSamplerState),L"Renderer::InitGBuffer() - SetSamplerState failed: ");

	SetLightingTextures();
}

/************************************************************************/
/* Name:		SetLightingTextures										*/
/* Description:	Set the textures and sampling in the lighting effect	*/
/*              that do not vary.										*/
/************************************************************************/
void renderer::SetLightingTextures()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	if(m_FXLighting)
	{
		// Light accumulation
		HR(m_FXLighting->SetTexture(m_FXHandlesLighting.h_gLightAccumulation, m_accumulationBuffer->d3dTex()),L"renderer::InitLighting() - Set Texture h_gLightAccumulation Failed: ");
		HR(SetSamplerState(m_FXLighting, &m_FXHandlesLighting.h_gSampLightAccumulation, &curState.lightAccumulationSampler, &m_gLightAccumulationSampler),L"renderer::InitLighting() - SetSamplerState Failed: ");
	
		// Set the textures and sampling in the lighting effect
		HR(m_FXLighting->SetTexture(m_FXHandlesLighting.h_gDepth, m_viewDepthMap->d3dTex()),L"renderer::InitLighting() - Set Texture h_gDepth Failed: ");
		HR(m_FXLighting->SetTexture(m_FXHandlesLighting.h_gNormal, m_viewNormalMap->d3dTex()),L"renderer::InitLighting() - Set Texture h_gNormal Failed: ");
		HR(m_FXLighting->SetTexture(m_FXHandlesLighting.h_gAlbedo, m_albedoMap->d3dTex()),L"renderer::InitLighting() - Set Texture h_gAlbedo Failed: ");
		HR(m_FXLighting->SetTexture(m_FXHandlesLighting.h_gMisc, m_miscMap->d3dTex()),L"renderer::InitLighting() - Set Texture h_gMisc Failed: ");
		HR(SetSamplerState(m_FXLighting, &m_FXHandlesLighting.h_gSampGBuffer, &curState.lightingGBufferSampler, &m_gBufferSampler),L"renderer::InitLighting() - SetSamplerState Failed: ");

		// SM textures
		SetSMTextures();

		// SSAO texture
		HR(m_FXLighting->SetTexture(m_FXHandlesLighting.h_gOcclusionBuffer, m_occlusionBuffer->d3dTex()),L"renderer::InitLighting() - Set Texture h_gOcclusionBuffer Failed: ");
		HR(SetSamplerState(m_FXLighting, &m_FXHandlesLighting.h_gSampOcclusionBuffer, &curState.occlusionBufferSampler, &m_occlusionBufferSampler),L"renderer::InitLighting() - SetSamplerState Failed: ");
	}
}

/************************************************************************/
/* Name:		RenderSceneLights										*/
/* Description: Render the scene after shadowmapping is complete		*/
/************************************************************************/
void renderer::RenderSceneLights()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	SetGlobalAmbient();

	// Avoid some multiple lookups (at the cost of putting these on the stack)
	bool _shadowsON = g_UI->GetSetting<bool>(&var_shadowsOn);
	UINT _numLights = g_objectManager->GetNumLights();
	light * curLight = NULL;
	D3DXMATRIX matWorld, matTemp;
	bool _renderLightVolumes = g_UI->GetSetting<bool>(&var_renderLightVolumes);

	// Clear the texture and send effects parameters
	ClearTexture(m_accumulationBuffer->d3dTex(),ACCUMULATION_CLEAR_COLOR);
	HR(m_FXLighting->SetMatrix(m_FXHandlesLighting.h_gCameraProj, g_objectManager->GetCamera()->GetMatProjection()),L"renderer::RenderSceneLights() - Failed to set h_gCameraProj matrix");

	m_accumulationBuffer->BeginScene();

	int _RenderGBufferPreviewFullScreen = g_UI->GetSetting<int>(&var_RenderGBufferPreviewFullScreen); // 0 is off, 1 is pos buffer, 2 is normal buffer, 3 is occlusion buffer.
	if(_RenderGBufferPreviewFullScreen == 0 || _RenderGBufferPreviewFullScreen == 7)
	{
		// HANDLE ALL LIGHT TYPES HERE -->
		if(_shadowsON)
			RenderLightSpotSM(_renderLightVolumes, & matWorld, & matTemp, g_objectManager->GetLightSpotSM()); // Render lightSpotSM, which includes shadows
		else
			RenderLightSpot(_renderLightVolumes, & matWorld, & matTemp, dynamic_cast<lightSpot *>(g_objectManager->GetLightSpotSM())); // Just render it as a normal spot light

		// Render spot lights in a block (saves effect change overhead)
		for(UINT i = 0; i < _numLights; i ++)
		{
			curLight = g_objectManager->GetLight(i);
			if(curLight->GetType() == T_SPOT)
				RenderLightSpot(_renderLightVolumes, & matWorld, & matTemp, dynamic_cast<lightSpot *>(curLight));
		}

		// Render dir lights in a block (saves effect change overhead)
		for(UINT i = 0; i < _numLights; i ++)
		{
			curLight = g_objectManager->GetLight(i);
			if(curLight->GetType() == T_DIR)
				RenderLightDir(dynamic_cast<lightDir *>(curLight));
		}

		// Render point lights in a block (saves effect change overhead)
		for(UINT i = 0; i < _numLights; i ++)
		{
			curLight = g_objectManager->GetLight(i);
			if(curLight->GetType() == T_POINT)
				RenderLightPoint(_renderLightVolumes, & matWorld, dynamic_cast<lightPoint *>(curLight));
		}
		// <-- END HANDLE ALL LIGHT TYPES HERE
	}

	m_accumulationBuffer->EndScene();

	// Combine the scene lights into the scene buffer
	if(g_UI->GetSetting<int>(&var_RenderGBufferPreviewFullScreen) == 0)
		CombineLights();
}

/************************************************************************/
/* Name:		CombineLights											*/
/* Description:	Use the accumulation buffer to render the scene			*/
/************************************************************************/
void renderer::CombineLights()
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif

	m_sceneBuffer[0]->BeginScene();

	HR(m_pD3DDev->Clear(0, 0, D3DCLEAR_TARGET, SCENE_CLEAR_COLOR, 0, 0),L"renderer::CombineLights() - Failed to clear device: ");

	// Now perform the final lighting pass, using light accumulation buffer and ambient occlusion buffer
	if(g_UI->GetSetting<bool>(&var_shadowsOn) && g_UI->GetSetting<int>(&var_SMVisualiseSplits) == 1)
		RenderFullScreenQuad(m_FXLighting, LIGHTING, COMBINEFINAL_TECH, false, 0, COMBINELIGHT_AND_SMVISUAL_PASS );
	else
		RenderFullScreenQuad(m_FXLighting, LIGHTING, COMBINEFINAL_TECH, false, 0, COMBINELIGHT_PASS );

	// Render the pixels that were marked in the stencil buffer to be unlit (eg. wireframe objects or the sky)
	RenderFullScreenQuad(m_FXLighting, LIGHTING, RENDERUNLITPIXELS_TECH, false, 0, 0 );

	m_sceneBuffer[0]->EndScene();
}

/************************************************************************/
/* Name:		SetLightingMatricies									*/
/* Description: Set the world and view projections for rendering to		*/
/*              the position and normal render targets					*/
/************************************************************************/
void renderer::SetLightingMatricies(D3DXMATRIX * matWorld)
{
	// Setup the matricies
	// We want this:
	//		a) m_hW = matWorld  --> NOT NEEDED
	//		b) m_hWV = matWorld * m_lightView --> NOT NEEDED
	//      c) m_hWVP = matWorld * m_lightView * m_lightProj

	// a) m_hW
	// HR(m_FX->SetMatrix(m_FXHandles.m_hW, matWorld), L"Render::SetLightingMatricies() - Failed to set m_hWV matrix: "); 

	// b) m_hWV
	D3DXMATRIX WV;
	D3DXMatrixMultiply(& WV, matWorld, g_objectManager->GetCamera()->GetMatView() ); // matWorld is current RBO model->world transform
	HR(m_FXLighting->SetMatrix(m_FXHandlesLighting.h_gWV, & WV), L"renderer::SetLightingMatricies() - Failed to set m_hWV matrix: "); 

	// c) m_hWVP
	D3DXMATRIX WVP;
	D3DXMatrixMultiply(& WVP, & WV, g_objectManager->GetCamera()->GetMatProjection() ); // matWorld is current RBO model->world transform
	HR(m_FXLighting->SetMatrix(m_FXHandlesLighting.h_gWVP, & WVP), L"renderer::SetLightingMatricies() - Failed to set m_hWVP matrix: "); 
}

/************************************************************************/
/* Name:		DrawLightVolume											*/
/* Description:	Draw a single mesh object in the lighting effect		*/
/************************************************************************/
void renderer::DrawLightVolume(bool intersectNear, bool intersectFar, LIGHTING_TECH tech, rbobjectMeshData * meshData, D3DXMATRIX * matWorld)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
	UINT pass;
	if(!intersectNear)
		if(!intersectFar)
			pass = LIGHT_VOLUME_PASS; // ! intersectNear && ! intersectFar
		else 
			pass = LIGHT_VOLUME_PASS; // ! intersectNear && intersectFar
	else
		if(!intersectFar)
			pass = LIGHT_VOLUME_PASS_INTERSECT_NEAR; // intersectNear && ! intersectFar
		else 
			pass = LIGHT_VOLUME_PASS_INTERSECT_FAR; // intersectNear && intersectFar

	SetLightingMatricies(matWorld);
	SetLightingTechnique(tech);

	UINT numPasses;
	HR(m_FXLighting->Begin(&numPasses, 0),L"renderer::DrawLightVolume() - m_FX->Begin Failed: ");
	HR(m_FXLighting->BeginPass(pass),L"renderer::DrawLightVolume() - m_FX->BeginPass Failed: ");

	for(UINT j = 0; j < meshData->materials->Size(); ++j)
	{
		// Don't set the material OR the texture, just render it.
        HR(m_FXLighting->CommitChanges(),L"renderer::DrawLightVolume() - CommitChanges failed: ");

		HR(meshData->pMesh->DrawSubset(j),L"renderer::DrawLightVolume() - DrawSubset failed: ");
	}

	HR(m_FXLighting->EndPass(),L"renderer::DrawLightVolume() - m_FX->EndPass Failed: ");
	HR(m_FXLighting->End(),L"renderer::DrawLightVolume() - m_FX->End Failed: ");
}

/************************************************************************/
/* Name:		RenderLightSpotSM										*/
/* Description:	Render the global light, including shadows				*/
/************************************************************************/
void renderer::RenderLightSpotSM(bool renderLightVolumes, D3DXMATRIX * matWorld, D3DXMATRIX * matTemp, lightSpotSM * light)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
#ifdef _DEBUG
	if(light == NULL)
		return;
#endif

	// Set the spot light structure in the effect
	lightSpot * curLightSpot = dynamic_cast<lightSpot *>(light);
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gSpotLight, curLightSpot->GetSpotLight(), sizeof(SpotLight)),L"renderer::RenderLightSpotSM() - Set Value h_gLight Failed: ");

	if(curState.SMShaderBilinearFiltering)
	{
		D3DXVECTOR2 _textureSize = D3DXVECTOR2((float)m_shadowMap->Width(),(float)m_shadowMap->Height());
		D3DXVECTOR2 _texelSize = D3DXVECTOR2(1.0f/_textureSize.x, 1.0f/_textureSize.y);
		SetLightingBilinearTexelSize(& _texelSize);
		SetLightingBilinearTextureSize(& _textureSize);
	}

	if(renderLightVolumes)
	{
		bool intersectNear, intersectFar;
		light->TestCameraNearFarPlanes(& intersectNear, & intersectFar);
		light->CalculateLightVolumeWorldMatrix(matWorld, matTemp, CONE_INSIDERADIUS, CONE_HEIGHT, & cone::coneForward);
		// Render the light volume
		if(curState.SMShaderBilinearFiltering)
			DrawLightVolume(intersectNear, intersectFar, SPOTLIGHT_SHADOWS_BILINEAR_TECH, g_objectManager->GetConeMesh()->meshData, matWorld);
		else
			DrawLightVolume(intersectNear, intersectFar, SPOTLIGHT_SHADOWS_TECH, g_objectManager->GetConeMesh()->meshData, matWorld);
	}
	else
		if(curState.SMShaderBilinearFiltering)
			RenderFullScreenQuad(m_FXLighting, LIGHTING, SPOTLIGHT_SHADOWS_BILINEAR_TECH, false, 0, LIGHT_FULLSCREEN_QUAD_PASS );
		else
			RenderFullScreenQuad(m_FXLighting, LIGHTING, SPOTLIGHT_SHADOWS_TECH, false, 0, LIGHT_FULLSCREEN_QUAD_PASS );
}

/************************************************************************/
/* Name:		RenderLightSpot											*/
/* Description:	Render a spot light										*/
/************************************************************************/
void renderer::RenderLightSpot(bool renderLightVolumes, D3DXMATRIX * matWorld, D3DXMATRIX * matTemp, lightSpot * light)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
#ifdef _DEBUG
	if(light == NULL)
		return;
#endif

	// Set the spot light structure in the effect
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gSpotLight, light->GetSpotLight(), sizeof(SpotLight)),L"renderer::RenderLightSpot() - Set Value h_gLight Failed: ");

	if(renderLightVolumes)
	{
		bool intersectNear, intersectFar;
		light->TestCameraNearFarPlanes(& intersectNear, & intersectFar);
		light->CalculateLightVolumeWorldMatrix(matWorld, matTemp, CONE_INSIDERADIUS, CONE_HEIGHT, & cone::coneForward);
		// Render the light volume
		DrawLightVolume(intersectNear, intersectFar, SPOTLIGHT_TECH, g_objectManager->GetConeMesh()->meshData, matWorld);
	}
	else
		RenderFullScreenQuad(m_FXLighting, LIGHTING, SPOTLIGHT_TECH, false, 0, LIGHT_FULLSCREEN_QUAD_PASS );
}

/************************************************************************/
/* Name:		RenderLightPoint										*/
/* Description:	Render a point light									*/
/************************************************************************/
void renderer::RenderLightPoint(bool renderLightVolumes, D3DXMATRIX * matWorld, lightPoint * light)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
#ifdef _DEBUG
	if(light == NULL)
		return;
#endif

	// Set the spot light structure in the effect
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gPointLight, light->GetPointLight(), sizeof(PointLight)),L"renderer::RenderLightPoint() - Set Value h_gLight Failed: ");

	if(renderLightVolumes)
	{
		bool intersectNear, intersectFar;
		light->TestCameraNearFarPlanes(& intersectNear, & intersectFar);
		light->CalculateLightVolumeWorldMatrix(matWorld, SPHERE_INSIDERADIUS);
		// Render the light volume
		DrawLightVolume(intersectNear, intersectFar, POINTLIGHT_TECH, g_objectManager->GetSphereMesh()->meshData, matWorld);
	}
	else
		RenderFullScreenQuad(m_FXLighting, LIGHTING, POINTLIGHT_TECH, false, 0, LIGHT_FULLSCREEN_QUAD_PASS );
}

/************************************************************************/
/* Name:		RenderLightDir											*/
/* Description:	Render a directional light								*/
/************************************************************************/
void renderer::RenderLightDir(lightDir * light)
{
#ifdef D3DPROFILE
	PROFILE_BLOCK
#endif
#ifdef _DEBUG
	if(light == NULL)
		return;
#endif
	// Set the spot light structure in the effect
	HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gDirLight, light->GetDirLight(), sizeof(DirLight)),L"renderer::RenderLightDir() - Set Value h_gLight Failed: ");
	
	// Render the light volume
	RenderFullScreenQuad(m_FXLighting, LIGHTING, DIRLIGHT_TECH, false, 0, 0 );
}

/************************************************************************/
/* Name:		SetLightingTechnique									*/
/* Description:	Set the desired technique								*/
/************************************************************************/
void renderer::SetLightingTechnique(LIGHTING_TECH _technique)
{
	if(curState.lighting_tech == _technique) // Avoid redundant set render states
		return; 
	switch(_technique)
	{
	case SPOTLIGHT_SHADOWS_TECH:
		HR(m_FXLighting->SetTechnique(m_FXHandlesLighting.h_SpotLight_Shadows_Tech), L"renderer::SetLightingTechnique() - Failed to set h_SpotLight_Shadows_Tech: ");
		curState.lighting_tech = SPOTLIGHT_SHADOWS_TECH;
		break;
	case SPOTLIGHT_SHADOWS_BILINEAR_TECH:
		HR(m_FXLighting->SetTechnique(m_FXHandlesLighting.h_SpotLight_Shadows_Bilinear_Tech), L"renderer::SetLightingTechnique() - Failed to set h_SpotLight_Shadows_Bilinear_Tech: ");
		curState.lighting_tech = SPOTLIGHT_SHADOWS_BILINEAR_TECH;
		break;
	case SPOTLIGHT_TECH:
		HR(m_FXLighting->SetTechnique(m_FXHandlesLighting.h_SpotLight_Tech), L"renderer::SetLightingTechnique() - Failed to set h_SpotLight_Tech: ");
		curState.lighting_tech = SPOTLIGHT_TECH;
		break;
	case DIRLIGHT_TECH:
		HR(m_FXLighting->SetTechnique(m_FXHandlesLighting.h_DirLight_Tech), L"renderer::SetLightingTechnique() - Failed to set h_DirLight_Tech: ");
		curState.lighting_tech = DIRLIGHT_TECH;
		break;
	case POINTLIGHT_TECH:
		HR(m_FXLighting->SetTechnique(m_FXHandlesLighting.h_PointLight_Tech), L"renderer::SetLightingTechnique() - Failed to set h_PointLight_Tech: ");
		curState.lighting_tech = POINTLIGHT_TECH;
		break;
	case RENDERUNLITPIXELS_TECH:
		HR(m_FXLighting->SetTechnique(m_FXHandlesLighting.h_RenderUnlitPixels_Tech), L"renderer::SetLightingTechnique() - Failed to set h_RenderUnlitPixels_Tech: ");
		curState.lighting_tech = RENDERUNLITPIXELS_TECH;
		break;
	case COMBINEFINAL_TECH:
		HR(m_FXLighting->SetTechnique(m_FXHandlesLighting.h_CombineFinal_Tech), L"renderer::SetLightingTechnique() - Failed to set h_CombineFinal_Tech: ");
		curState.lighting_tech = COMBINEFINAL_TECH;
		break;
	default:
		throw std::runtime_error("renderer::SetLightingTechnique() - Technique not recognised");
	}
}

/************************************************************************/
/* Name:		SetShadowsOn											*/
/* Description:	Set the FX parameter to turn shadows ON/OFF				*/
/************************************************************************/
void renderer::SetShadowsOn(int _shadowsOn)
{
	if(curState.shadowsOn != _shadowsOn) // Avoid redundant set render states
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gShadowsOn, &_shadowsOn, sizeof(int)), L"renderer::SetShadowsOn() - Set Value h_gShadowsOn Failed: ");
		curState.shadowsOn = _shadowsOn;
	}
}

/************************************************************************/
/* Name:		SetSSAOOn												*/
/* Description:	Set the FX parameter to turn SSAO ON/OFF				*/
/************************************************************************/
void renderer::SetSSAOOn(int _SSAOOn)
{
	if(curState.SSAOOn != _SSAOOn) // Avoid redundant set render states
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gSSAOOn, &_SSAOOn, sizeof(int)), L"renderer::SetSSAOOn() - Set Value h_gSSAOOn Failed: ");
		curState.SSAOOn = _SSAOOn;
	}
}

/************************************************************************/
/* Name:		SetLightingTexelSize									*/
/* Description:	Set the FX parameters for Texel and Texture size		*/
/************************************************************************/
void renderer::SetLightingTexelSize(D3DXVECTOR2 * _texelSize)
{
	if(curState.lightingTexelSize.x != _texelSize->x ) // Avoid redundant set render states
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gTexelSizeX, &_texelSize->x, sizeof(float)), L"renderer::SetLightingTexelSize() - Set Value h_gTexelSizeX Failed: ");
		curState.lightingTexelSize.x = _texelSize->x;
	}
	if(curState.lightingTexelSize.y != _texelSize->y ) // Avoid redundant set render states
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gTexelSizeY, &_texelSize->y, sizeof(float)), L"renderer::SetLightingTexelSize() - Set Value h_gTexelSizeY Failed: ");
		curState.lightingTexelSize.y = _texelSize->y;
	}
}

/************************************************************************/
/* Name:		SetLightingBilinearTexelSize							*/
/* Description:	Set the FX parameters for Texel and Texture size		*/
/************************************************************************/
void renderer::SetLightingBilinearTexelSize(D3DXVECTOR2 * _texelSize)
{
	if(curState.lightingBilinearTexelSize.x != _texelSize->x ||
	   curState.lightingBilinearTexelSize.y != _texelSize->y) // Avoid redundant set render states
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gBilinearTexelSize, _texelSize, sizeof(D3DXVECTOR2)), L"renderer::SetLightingBilinearTexelSize() - Set Value h_gBilinearTexelSize Failed: ");
		curState.lightingBilinearTexelSize = *_texelSize;
	}
}

/************************************************************************/
/* Name:		SetLightingBilinearTextureSize							*/
/* Description:	Set the FX parameters for Texel and Texture size		*/
/************************************************************************/
void renderer::SetLightingBilinearTextureSize(D3DXVECTOR2 * _textureSize)
{
	if(curState.lightingBilinearTextureSize.x != _textureSize->x ||
	   curState.lightingBilinearTextureSize.y != _textureSize->y) // Avoid redundant set render states
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gBilinearTextureSize, _textureSize, sizeof(D3DXVECTOR2)), L"renderer::SetLightingBilinearTextureSize() - Set Value _textureSize Failed: ");
		curState.lightingBilinearTextureSize = *_textureSize;
	}
}

/************************************************************************/
/* Name:		SetGlobalAmbient										*/
/* Description:	Set the FX parameters for global ambient color			*/
/************************************************************************/
void renderer::SetGlobalAmbient()
{
	float _globalAmbientF = g_UI->GetSetting<float>( & var_GlobalAmbient );
	D3DXVECTOR3 _globalAmbient(_globalAmbientF, _globalAmbientF, _globalAmbientF);

	if(curState.globalAmbient.x != _globalAmbient.x ||
	   curState.globalAmbient.y != _globalAmbient.y ||
	   curState.globalAmbient.z != _globalAmbient.z) // Avoid redundant set render states
	{
		HR(m_FXLighting->SetValue(m_FXHandlesLighting.h_gGlobalAmbient, &_globalAmbient, sizeof(D3DXVECTOR3)), L"renderer::SetGlobalAmbient() - Set Value h_gGlobalAmbient Failed: ");
		curState.globalAmbient = _globalAmbient;
	}
}
