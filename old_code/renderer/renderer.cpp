/*************************************************************
**						 Renderer							**
**		-> Graphics Rendering functions, Summer 2009		**
*************************************************************/
// File:		renderer.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Some free models: http://artist-3d.com/free_3d_models/

#include	"renderer\renderer.h"
#include	"utils_and_misc_classes\stringUtil.h"
#include	"renderer\renderer_structures\drawableTex2D.h"
#include	"renderer\renderer_structures\drawableTexAtlas2D.h"
#include	"camera\camera.h"
#include	"physics\objects\AABbox.h"
#include	"renderer\renderer_structures\vertex.h"
#include	"physics\objects\rbobjectMeshData.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#define SHADOWMAP_RENDER_SCALE 0.8f

/************************************************************************/
/* Name:		renderer												*/
/* Description:	Default Constructor: Set render constants				*/
/************************************************************************/
renderer::renderer()
{
	// Effects
	m_FXLighting = NULL; 
	m_FXPostProcess = NULL;
	m_FXGBuffer = NULL;
	m_FXSMap = NULL;

	m_shadowMap = NULL; m_WhiteTex = NULL; 
	m_matShadowMap_CamViewInv_LightViewProjs = NULL; m_matShadowMapProjs = NULL;
	m_shadowMapSplitDepths = NULL;
	m_ShadowMapSplitScales = NULL;
	m_shadowMapEffectSplitDepths = NULL;
	m_pD3DObj = NULL;
	m_pD3DDev = NULL;
	m_DeviceCaps = new (D3DCAPS9);
	m_ObjectCaps = new (D3DCAPS9);
	pureDevice = false;

	m_MaxLOD = 1.0f;
	m_MinFilterWidth = 1.0f;

	m_FSQuadVertBuff = NULL;
	m_FSQuadVertBuffSize = 0;

	m_viewNormalMap = NULL;
	m_viewDepthMap = NULL;
	m_albedoMap = NULL;
	m_miscMap = NULL;
	m_occlusionBuffer = NULL;
	m_vectorNoiseTexture = NULL;
	m_accumulationBuffer = NULL;
	for(UINT i = 0; i < 2; i ++)
		m_sceneBuffer[i] = NULL;
	m_currentFrameLuminance = NULL;
	m_currentFrameAdaptedLuminance = NULL;
	m_lastFrameAdaptedLuminance = NULL;
	m_luminanceChain = NULL;
	m_sceneBuffer_1_4th_WL = NULL;
	for(UINT i = 0; i < 2; i ++)
		m_sceneBuffer_1_16th_WL[i] = NULL;

	m_DepthStencilSurf = NULL;
	m_DOFDiskOffsets = NULL;

	m_defaultMtrl = NULL;
}
/************************************************************************/
/* Name:		~renderer												*/
/* Description:	Default Destructor										*/
/************************************************************************/
renderer::~renderer()
{
	// Note: Resettable resources are freed when objectManager closes, but we still need to remove instances
	if(m_shadowMap) {delete [] m_shadowMap; m_shadowMap = NULL;}
	if(m_matShadowMap_CamViewInv_LightViewProjs) {delete [] m_matShadowMap_CamViewInv_LightViewProjs; m_matShadowMap_CamViewInv_LightViewProjs = NULL;}
	if(m_matShadowMapProjs) {delete [] m_matShadowMapProjs; m_matShadowMapProjs = NULL;}
	if(m_ShadowMapSplitScales) {delete [] m_ShadowMapSplitScales; m_ShadowMapSplitScales = NULL;}
	if(m_shadowMapSplitDepths) {delete [] m_shadowMapSplitDepths; m_shadowMapSplitDepths = NULL;}
	if(m_shadowMapEffectSplitDepths) {delete [] m_shadowMapEffectSplitDepths; m_shadowMapEffectSplitDepths = NULL;}
	if(m_occlusionBuffer) { delete m_occlusionBuffer; m_occlusionBuffer = NULL; }
    if(m_viewNormalMap) { delete m_viewNormalMap; m_viewNormalMap = NULL; }
    if(m_viewDepthMap) { delete m_viewDepthMap; m_viewDepthMap = NULL; }
    if(m_albedoMap) { delete m_albedoMap; m_albedoMap = NULL; }
    if(m_miscMap) { delete m_miscMap; m_miscMap = NULL; }
	if(m_accumulationBuffer) { delete m_accumulationBuffer; m_accumulationBuffer = NULL; }
	for(UINT i = 0; i < 2; i ++)
	{	if(m_sceneBuffer[i]) { delete m_sceneBuffer[i]; m_sceneBuffer[i] = NULL; } }
	if(m_currentFrameLuminance) { delete m_currentFrameLuminance; m_currentFrameLuminance = NULL; }
	if(m_currentFrameAdaptedLuminance) { delete m_currentFrameAdaptedLuminance; m_currentFrameAdaptedLuminance = NULL; }
	if(m_lastFrameAdaptedLuminance) { delete m_lastFrameAdaptedLuminance; m_lastFrameAdaptedLuminance = NULL; }
	if(m_luminanceChain) { delete [] m_luminanceChain; m_luminanceChain = NULL; }
	if(m_sceneBuffer_1_4th_WL) { delete m_sceneBuffer_1_4th_WL; m_sceneBuffer_1_4th_WL = NULL; }
	for(UINT i = 0; i < 2; i ++)
	{	if(m_sceneBuffer_1_16th_WL[i]) { delete m_sceneBuffer_1_16th_WL[i]; m_sceneBuffer_1_16th_WL[i] = NULL; } }
	if(m_defaultMtrl) { delete m_defaultMtrl; m_defaultMtrl = NULL; }
	if(m_DOFDiskOffsets) { delete [] m_DOFDiskOffsets; m_DOFDiskOffsets = NULL; }

	// Effects
	ReleaseCOM(m_FXLighting);
	ReleaseCOM(m_FXPostProcess);
	ReleaseCOM(m_FXGBuffer);
	ReleaseCOM(m_FXSMap);

	delete m_DeviceCaps; delete m_ObjectCaps; 
	ReleaseCOM(m_FSQuadVertBuff);
	ReleaseCOM(m_DepthStencilSurf);
}