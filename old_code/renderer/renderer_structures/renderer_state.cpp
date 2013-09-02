/*************************************************************
**						rendererState						**
**************************************************************/
// File:		rendererState.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "renderer\renderer_structures\rendererState.h"
#include "renderer\renderer.h"
#include "main.h"
#include "utils_and_misc_classes\stringUtil.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		rendererState											*/
/* Description:	Default constructor function							*/
/************************************************************************/
rendererState::rendererState()
{
	lighting_tech = LIGHTING_UNDEFINED_TECH;
	postProcess_tech = POSTPROCESS_UNDEFINED_TECH;
	SMap_tech = SMAP_UNDEFINED_TECH;
	gBuffer_tech = GBUFFER_UNDEFINED_TECH;

	visualizeSplits = -1;
	SMShaderBilinearFiltering = false;
	SMShaderBilinearFilteringFxState = -1;
	lightingSMtexelSize = -1.0f;
	lightingSMtextureSize = -1.0f;
	shadowsOn = -1;
	SSAOOn = -1;
	postProcessingTexelSize.x = -1; postProcessingTexelSize.y = -1;
	lightingTexelSize.x = -1; lightingTexelSize.y = -1;
	lightingBilinearTexelSize.x = -1; lightingBilinearTexelSize.y = -1;
	lightingBilinearTextureSize.x = -1; lightingBilinearTextureSize.y = -1;
	postProcessBilinearTexelSize.x = -1; postProcessBilinearTexelSize.y = -1;
	postProcessBilinearTextureSize.x = -1; postProcessBilinearTextureSize.y = -1;
	globalAmbient.x = -1; globalAmbient.y = -1; globalAmbient.z = -1;
	cameraNearFar.x = -1; cameraNearFar.y = -1; // impossible value
	cameraTangentFov.x = -1000; cameraTangentFov.y = -1000; // impossible value
	occlusionCleared = false;
	luminanceFTau = -1;
	toneMapMiddleGrey = -1;
	toneMapWhiteSq = -1;
	toneMapThreshold = -1;
	gaussianBlurSigma = -1;
	gaussianBlurRadius = -1;
	bloomMulitplier = -1;
	motionBlurNumSamples = -1;
	DOFBounds.x = -1; DOFBounds.y = -1; DOFBounds.z = -1; DOFBounds.w = -1;
	HDRManualLuminance = -1;
}

/************************************************************************/
/* Name:		~rendererState											*/
/* Description:	Default destructor function								*/
/************************************************************************/
rendererState::~rendererState()
{
	
}
