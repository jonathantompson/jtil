/*************************************************************
**					 Renderer								**
**		-> Graphics Rendering functions, Summer 2009		**
*************************************************************/
// File:		renderer.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef renderer_h
#define renderer_h

#include "dxInclude.h"

class debugObject;
class drawableTex2D;
class drawableTexAtlas2D;
class rbobjectMeshData;
class camera;
class AABBox;
class light;
class lightSpotSM;
class lightSpot;
class lightPoint;
class lightDir;
struct VertexPosTex;
class d3dHandlesSampler;

#include "renderer\renderer_structures\rendererState.h"
#include "renderer\renderer_structures\d3dFormats.h"
#include "renderer\renderer_structures\d3dHandles\d3dHandles.h"
#include "renderer\renderer_structures\texSamplerState.h"

#pragma warning( push )			
#pragma warning( disable:4324 )	// Ignore structure padding...

#ifdef _DEBUG
	#define D3DPROFILE
	#define PERFHUD_BUILD
#endif

class renderer
{
public:
	friend class hud; // It's not lazy OOP ;-) it makes sense for hud class to get access to private functions.
								renderer(); // Default Constructor
								~renderer(); // Destructor Destructor

	void						RenderFrame();
	void						OnResetDevice();
	void						OnLostDevice();

	// Some access and modifier functions
	inline LPDIRECT3DDEVICE9	GetD3DDev() { return m_pD3DDev; }
	inline LPDIRECT3D9			GetD3DObj() { return m_pD3DObj; }
	inline int					GetShaderVersion() { return m_shaderVersion; }
	inline ID3DXEffect *		GetFXLighting() { return m_FXLighting; }
	inline ID3DXEffect *		GetFXGBuffer() { return m_FXGBuffer; }
	inline d3dHandlesLighting *	GetHandlesLighting() { return & m_FXHandlesLighting; }
	inline d3dHandlesGBuffer *	GetHandlesGBuffer() { return & m_FXHandlesGBuffer; }
	inline UINT					GetAdapterToUse() { return m_AdapterToUse; }
	inline D3DDEVTYPE			GetDeviceType() { return m_DeviceType; }
	inline void					SwapDrawableTex2D(drawableTex2D ** A, drawableTex2D ** B) { drawableTex2D * temp = *A; *A = *B; *B = temp; }

	// Init and maintenance functions
	void						InitRenderer();
	void						InitD3D();
	bool						CheckObjCaps();
	bool						CheckDevCaps();
	void						toggleFullScreenMode();
	void						KillD3D();
	inline D3DPRESENT_PARAMETERS * GetAppPresentParameters() { return & m_PresentParameters; }
	inline rendererState *		GetCurState() { return & curState; }
	bool						CheckDevice();
	void						LoadWhiteTex();
	void						UpdateShadows();
	void						UpdateSSAO();
	void						LoadTexture(LPCWSTR pFile, D3DFORMAT format, UINT numMips, IDirect3DTexture9 ** tex );

	// Technique set
	void						SetLightingTechnique(LIGHTING_TECH _technique);
	void						SetPostProcessTechnique(POSTPROCESS_TECH _technique);
	void						SetSMapTechnique(SMAP_TECH _technique);
	void						SetGBufferTechnique(GBUFFER_TECH _technique);

	// G-Buffer and Shadow-map Draw Routines
	void						DrawTexturedMeshGBuffer(UINT pass, rbobjectMeshData * meshData, D3DXMATRIXA16 * matWorld, D3DXMATRIXA16 * matWorldPrevFrame, Mtrl * mtrlOverride);
	void                        DrawTexturedMeshSM(rbobjectMeshData * meshData, D3DXMATRIXA16 * matWorld);
	void						DrawTexturedMeshWireframeGBuffer(UINT pass, rbobjectMeshData * meshData, D3DXMATRIXA16 * matWorld);
	void						DrawSingleColorMeshWireframeGBuffer(UINT pass, bool drawAsLines, IDirect3DVertexBuffer9 * vertBuff, DWORD vertSize, IDirect3DIndexBuffer9 * indBuff, DWORD indSize, D3DXMATRIXA16 * matWorld);
	void						DrawSingleColorMeshGBuffer(UINT pass, IDirect3DVertexBuffer9 * vertBuff, DWORD vertSize, IDirect3DIndexBuffer9 * indBuff, DWORD indSize, D3DXMATRIXA16 * matWorld, Mtrl * matl);
    void						DrawSingleColorMeshGBuffer(UINT pass, ID3DXMesh * pMesh, D3DXMATRIXA16 * matWorld, Mtrl * matl);

	// Just draw solid triangles
	void                        DrawTrianglesColSM(IDirect3DVertexBuffer9 * vertBuff, DWORD vertSize, IDirect3DIndexBuffer9 * indBuff, DWORD indSize, D3DXMATRIXA16 * matWorld);

	texSamplerState *			GetBestTexSampler(D3DFORMAT texFormat, bool cubeTexture);
	HRESULT						SetSamplerState(ID3DXEffect * m_FX, d3dHandlesSampler * samplerHandles, texSamplerState * curState, texSamplerState * desiredState );

private:

// VARIABLES
	LPDIRECT3D9					m_pD3DObj; // the long pointer to our Direct3D interface
	LPDIRECT3DDEVICE9			m_pD3DDev; // the long pointer to the device class --> Hardware side
	IDirect3DSurface9 *			m_DepthStencilSurf;
	UINT						m_DepthStencilWidth, m_DepthStencilHeight;
	UINT						m_AdapterToUse;
	D3DDEVTYPE					m_DeviceType;

	// D3D Effects and handles
	int							m_shaderVersion;
	ID3DXEffect*				m_FXLighting;
	ID3DXEffect*				m_FXGBuffer;
	ID3DXEffect*				m_FXSMap;
	ID3DXEffect*				m_FXPostProcess;
	d3dHandlesLighting			m_FXHandlesLighting;
	d3dHandlesGBuffer			m_FXHandlesGBuffer;
	d3dHandlesSMap				m_FXHandlesSMap;
	d3dHandlesPostProcess		m_FXHandlesPostProcess;

	// White texture to use when no texture is included in model
	IDirect3DTexture9 *			m_WhiteTex;
	texSamplerState				m_WhiteTexSamplerState;
	Mtrl *						m_defaultMtrl;

	// Wrapper to hold the current renderer state information to avoid redundant setRenderState calls
	rendererState				curState;		

	D3DPRESENT_PARAMETERS		m_PresentParameters; // Hold information about the card (parameters)
	D3DCAPS9 *					m_DeviceCaps; // Hold capabilities of the card 
	D3DCAPS9 *					m_ObjectCaps; // Hold capabilities of the card
	bool						pureDevice;
	POINT						windowPreFullscreen;

	// Shadow map objects - user variables are in settings.csv, but current frame values are stored here
	drawableTexAtlas2D *		m_shadowMap;
	texSamplerState				m_SMSamplerState; // For all shadow maps
	float *						m_shadowMapSplitDepths;
	float *						m_shadowMapEffectSplitDepths;
	D3DXMATRIX *				m_matShadowMap_CamViewInv_LightViewProjs;// transform from view space to shadowmap space
	D3DXMATRIX *				m_matShadowMapProjs;
	D3DXVECTOR2 *				m_ShadowMapSplitScales;
	UINT						m_numShadowMaps;			// ... For the current frame's shadow map memory allocation
	UINT						m_SMSize;					// ... For the current frame's shadow map memory allocation
	float						m_smapTexelSize;
	UINT						m_currentShadowMapToRender;
	float						m_smapSoftness;
	float						m_MaxLOD; // Calculated from smap softness and shadowmap size
	float						m_MinFilterWidth; // Calculated from MaxLOD
	float						m_smapBlendZone;

	// GBuffer objects
	drawableTex2D *				m_viewDepthMap;		// View space depth (R = 32bit depth, G = 32bit UNUSED)
	drawableTex2D *				m_viewNormalMap;	// View space normal (R = 16bitF X, G = 16bitF Y, B = 16bitF Z, A = 16bitF spec power)
	drawableTex2D *				m_albedoMap;		// Diffuse color (R = 16bit Red, G = 16bit Green, B = 16bit Blue, A = 16bit spec intensity)
	drawableTex2D *				m_miscMap;			// Misc properties (R = View.x, G = View.y, B = Shadowmap split, A = UNUSED)
	texSamplerState				m_gBufferSampler;

	// Lighting Pass
	drawableTex2D *				m_accumulationBuffer;
	texSamplerState				m_gLightAccumulationSampler;
	drawableTex2D *				m_sceneBuffer[2]; // Need 2 one to store HDR scene and one to store pre-post processed data

	// SSAO, HDR & Post processing objects
	drawableTex2D *				m_occlusionBuffer;
	texSamplerState				m_vectorNoiseSampler;
	texSamplerState				m_occlusionBufferSampler;
	IDirect3DTexture9 *			m_vectorNoiseTexture;
	texSamplerState				m_sourceSampler;
	texSamplerState				m_source2Sampler;
	texSamplerState				m_source3Sampler;
	drawableTex2D *				m_currentFrameLuminance;
	drawableTex2D *				m_currentFrameAdaptedLuminance;
	drawableTex2D *				m_lastFrameAdaptedLuminance;
	drawableTex2D *				m_luminanceChain;
	UINT						m_luminanceChainLength;
	drawableTex2D *				m_sceneBuffer_1_4th_WL;
	drawableTex2D *				m_sceneBuffer_1_16th_WL[2]; // Need 3 to avoid having to downconvert twice (bloom and DOF use it)
	D3DXVECTOR2 *				m_DOFDiskOffsets;
	bool						m_depthVertexTextureCap;

	// Full-screen quad object used to render textures to the screen or postprocessing effects
	IDirect3DVertexBuffer9 *	m_FSQuadVertBuff;
	UINT						m_FSQuadVertBuffSize;
	void						CreateFSQuad(UINT * size, IDirect3DVertexBuffer9 ** VertBuff);

// FUNCTIONS

	// Initialization functions
	void						GetPresentParameters(D3DPRESENT_PARAMETERS * params);
	bool						CheckDisplayModeResolution(int width, int height,D3DFORMAT fmtBackbuffer);
	void						BuildFX();
	ID3DXEffect *				CompileFX(LPCWSTR fileName, DWORD compileFlages);
	void						CreateDepthStencilSurface();
	UINT						PerformResetLoop();

	// Renderer state functions modifier functions --> Mostly to avoid constant render state changes
	void						SetVisualizeSplits(int _visualizeSplits);
	void						SetSMSamplerStates(bool filtering);
	void						SetSMTextures();
	void						SetShadowsOn(int _shadowsOn);
	void						SetSSAOOn(int _SSAOOn);
	void						SetPostProcessingTexelSize(D3DXVECTOR2 * _texelSize);
	void						SetLightingTexelSize(D3DXVECTOR2 * _texelSize);
	void						SetLightingBilinearTexelSize(D3DXVECTOR2 * _texelSize);
	void						SetLightingBilinearTextureSize(D3DXVECTOR2 * _textureSize);
	void						SetPostProcessBilinearTexelSize(D3DXVECTOR2 * _texelSize);
	void						SetPostProcessBilinearTextureSize(D3DXVECTOR2 * _textureSize);
	void						SetGBufferTexelSize(D3DXVECTOR2 * _texelSize);
	void						SetCameraNearFar(D3DXVECTOR2 * nearFar);
	void						SetCameraTangentFov(D3DXVECTOR2 * tangentFov);
	void						SetLuminanceFTau(float _LuminanceFTau);
	void						SetToneMapMiddleGrey(float _toneMapMiddleGrey);
	void						SetToneMapWhiteSq(float _toneMapWhiteSq);
	void						SetToneMapThreshold(float _ToneMapThreshold);
	void						SetGaussianBlurSigma(float _sigma);
	void						SetGaussianBlurRadius(int _radius);
	void						SetBloomMultiplier(float _bloomMultiplier);
	void						SetMotionBlurNumSamples(int _numSamples);
	void						SetDOFBounds(D3DXVECTOR4 _DOFBounds);
	void						SetHDRManualLuminance(float _HDRManualLuminance);

	// SM functions
	void						CalcShadowMappingSplitDepths(float * depths, camera * _camera, float * effectDepths);
	void						InitShadowMaps();
	void						CalcShadowMappingSplitMatricies(lightSpotSM * _light, camera * _camera);
	void						GenerateSMMipSubLevels();
	void						DrawAllShadowMaps();
	void						DrawShadowMap(drawableTexAtlas2D * shadowMap, UINT shadowMapNumber);
	void						BlurShadowMaps(drawableTexAtlas2D * shadowMap);
	void						SetSMMatricies(D3DXMATRIXA16 * matWorld);

	// Lighting pass
	void						RenderLightSpotSM(bool renderLightVolumes, D3DXMATRIX * matWorld, D3DXMATRIX * matTemp, lightSpotSM * light);
	void						RenderLightSpot(bool renderLightVolumes, D3DXMATRIX * matWorld, D3DXMATRIX * matTemp, lightSpot * light);
	void						RenderLightPoint(bool renderLightVolumes, D3DXMATRIX * matWorld, lightPoint * light);
	void						RenderLightDir(lightDir * light);
	void						InitLighting();
	void						SetGlobalAmbient();
	void						SetLightingTextures();
	void						SetLightingMatricies(D3DXMATRIX * matWorld);
	void						DrawLightVolume(bool intersectNear, bool intersectFar, LIGHTING_TECH tech, rbobjectMeshData * meshData, D3DXMATRIX * matWorld);
	void						CombineLights();

	// Global rendering and misc functions
	void						RenderSceneLights();
	void						ClearTexture(IDirect3DTexture9* pd3dTexture, D3DCOLOR xColor);
	void						SetRenderTarget(int iRenderTargetIdx, IDirect3DTexture9* pd3dRenderTargetTexture);
	void						RenderFullScreenQuad(ID3DXEffect * m_FX, EFFECT_PASS effect, UINT Technique, bool clearBuffer, D3DCOLOR clearColor, UINT pass );
	void						DisplayScene();

	// GBuffer funtions
	void						InitGBuffer();
	void						DrawGBuffer();
	void						RenderGBufferPreviews();
	void						RenderGBufferPreviewFullScreen(int bufferIndex);
	void						RenderLightObjects(bool drawVelocityBuffer);
	void						renderLightVolumesWireframe();
	void						ClearGBufferInShader();
	void						SetGBufferMatricies(D3DXMATRIXA16 * matWorld);
	void						SetGBufferMatricies(D3DXMATRIXA16 * matWorld, D3DXMATRIXA16 * matWorldPrevFrame);

	// Post Processing
	void						RenderShadowMapTexturesToScreen();
	void						RenderTextureToScreen(UINT pass, LPDIRECT3DTEXTURE9 tex, D3DXVECTOR2 * translation_vec, D3DXVECTOR2 * scale_vec, POSTPROCESS_TECH tech, D3DXVECTOR2 * texelSize );
	void						InitPostProcessing();
	void						BoxBlurTexture( drawableTex2D * texture, drawableTex2D * tempTexture, D3DXVECTOR2 * filterWidth, D3DXVECTOR2 * texelSize );
	void						BoxBlurSMTextureAtlas( drawableTexAtlas2D * texture, drawableTexAtlas2D * tempTexture );
	void						PostProcessing();
	void						SetPostProcessingTextures();
	void						DownscaleTexture4x4( drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor, bool decodeLuminance );
	void						DownscaleTextureNxM( drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor );
	void						CalculateAverageLuminance( drawableTex2D * source, float deltaT, bool manualLuminance );
	void						UpscaleTexture4x4( drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor );
	void						ApplyToneMapThreshold(drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor, float threshold, bool manualLuminance );
	void						GaussianBlurTexture(drawableTex2D * source, drawableTex2D * tempTexture, D3DCOLOR clearColor, float sigma, int radius );
	void						GaussianBlurTextureHorizontal(drawableTex2D * source, drawableTex2D * destTexture, D3DCOLOR clearColor, float sigma, int radius );
	void						GaussianBlurTextureVertical(drawableTex2D * source, drawableTex2D * destTexture, D3DCOLOR clearColor, float sigma, int radius );
	void						PerformToneMapping(drawableTex2D * result, drawableTex2D * sceneBuffer, drawableTex2D * adaptedLuminance, drawableTex2D * bloomTexture_1_4th_WL, D3DCOLOR clearColor, bool bloomEnable, bool manualLuminance );
	void						PerformMotionBlur( drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor );
	void						PerformDOFBlur( drawableTex2D * source, drawableTex2D * dest, D3DCOLOR clearColor );

	// SSAO functions
	void						InitSSAO();
	void						DrawOcclusionBuffer();
	void						ClearOcclusionBuffer();

};

#pragma warning( pop )			

#endif
