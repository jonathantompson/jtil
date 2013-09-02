/*************************************************************
**						rendererState						**
**************************************************************/
// File:		rendererState.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef rendererState_h
#define rendererState_h

enum LIGHTING_TECH {
	LIGHTING_UNDEFINED_TECH,
	SPOTLIGHT_SHADOWS_TECH,
	SPOTLIGHT_SHADOWS_BILINEAR_TECH,
	SPOTLIGHT_TECH,
    POINTLIGHT_TECH,
	DIRLIGHT_TECH,
	RENDERUNLITPIXELS_TECH,
	COMBINEFINAL_TECH,
};

enum POSTPROCESS_TECH {
	POSTPROCESS_UNDEFINED_TECH,
	QUADTEXTURE_TECH,
	QUADPOSITIONTEXTURE_TECH,
	QUADNORMALTEXTURE_TECH,
	QUADVELOCITYTEXTURE_TECH,
	TRANSFORMEDTEXTURE_TECH,
	TRANSFORMED1DTEXTURE_TECH,
	TRANSFORMED2DTEXTURE_TECH,
	TRANSFORMED3DTEXTURE_TECH,
	OCCLUSIONMAP_TECH,
	BOXBLUR_TECH,
	BOXBLUR_SMATLAS_TECH,
	DOWNSCALE_4x4_TECH,
	DOWNSCALE_4x4_DECODELUMINANCE_TECH,
	DOWNSCALE_NxM_TECH,
	UPSCALE_TECH,
	CALCLUMINANCE_TECH,
	CALCADAPTEDLUMINANCE_TECH,
	TONEMAP_TECH,
	APPLYTONEMAPTHRESHOLD_TECH,
	GAUSSIANBLUR_TECH,
	MOTIONBLUR_TECH,
	DEPTHOFFIELD_BLURDISK_TECH,
};

enum SMAP_TECH {
	SMAP_UNDEFINED_TECH,
	BUILDSHADOWMAP_TECH,
};

enum GBUFFER_TECH {
	GBUFFER_UNDEFINED_TECH,
	CLEARGBUFFER_TECH,
	TEXTUREDMESH_TECH,
	TEXTUREDMESH_WIREFRAME_TECH,
	SINGLECOLORMESH_WIREFRAME_TECH,
	SINGLECOLORMESH_TECH,
	SKYBOX_TECH,
};

enum EFFECT_PASS {
	LIGHTING,
	GBUFFER,
	POSTPROCESS,
	SMAP,
};

#include "renderer\renderer_structures\texSamplerState.h"

class rendererState
{
public:
	friend class renderer;

	// Constructor / Destructor
    rendererState();
	~rendererState();

	LIGHTING_TECH				lighting_tech;
	POSTPROCESS_TECH			postProcess_tech;
	SMAP_TECH					SMap_tech;
	GBUFFER_TECH				gBuffer_tech;
	
	int							visualizeSplits;
	bool						SMShaderBilinearFiltering;
	int							SMShaderBilinearFilteringFxState;
	texSamplerState				SMTexSampler;  // Sampler state for shadow map textures
	texSamplerState				texSampler;  // Sampler state for mesh textures
	texSamplerState				occlusionBufferSampler;  // Sampler state for the occlusion buffer
	texSamplerState				sourceSampler; // Sampler state for post processing textures
	texSamplerState				source2Sampler; // Sampler state for post processing textures
	texSamplerState				source3Sampler; // Sampler state for post processing textures
	texSamplerState				randomNormalSampler; // Sampler state for post processing random normal
	texSamplerState				postProcessingGBufferSampler;
	texSamplerState				lightingGBufferSampler;
	texSamplerState				lightAccumulationSampler;
	texSamplerState				vectorNoiseSampler;
	texSamplerState				skyTexSampler;  // Sampler state for mesh textures
	float						lightingSMtexelSize; // 1/texture_size for 2D bilinear texture lookup
	float						lightingSMtextureSize; // for bilinear texture lookup
	int							shadowsOn;
	int							SSAOOn;
	D3DXVECTOR2					postProcessingTexelSize;
	D3DXVECTOR2					lightingTexelSize;
	D3DXVECTOR2					lightingBilinearTexelSize;
	D3DXVECTOR2					lightingBilinearTextureSize;
	D3DXVECTOR2					postProcessBilinearTexelSize;
	D3DXVECTOR2					postProcessBilinearTextureSize;
	D3DXVECTOR3					globalAmbient;
	D3DXVECTOR2					gBufferTexelSize;
	bool						occlusionCleared;
	D3DXVECTOR2					cameraNearFar;
	D3DXVECTOR2					cameraTangentFov;
	float						luminanceFTau;
	float						toneMapMiddleGrey;
	float						toneMapWhiteSq;
	float						toneMapThreshold;
	float						gaussianBlurSigma;
	int							gaussianBlurRadius;
	float						bloomMulitplier;
	int							motionBlurNumSamples;
	D3DXVECTOR4					DOFBounds;
	float						HDRManualLuminance;

private:

};

#endif