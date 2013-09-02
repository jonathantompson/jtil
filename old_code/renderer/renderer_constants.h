/*************************************************************
**					 Renderer								**
**		-> Graphics Rendering Constants, Summer 2009		**
*************************************************************/
// File:		renderer_constants.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef renderer_constants_h
#define renderer_constants_h

#define DXSCREENSPACE_MINX	-1.0f // (bound are from -1 to +1)
#define DXSCREENSPACE_MAXX	1.0f 
#define DXSCREENSPACE_MINY	-1.0f // (bound are from -1 to +1)
#define DXSCREENSPACE_MAXY	1.0f

// SHADOWMAP PARAMETERS
#define MAX_NUM_SHADOW_MAPS		9
#define SMAP_MAXLOD				2.25f
#define SMAP_CLEAR_DEPTH		1.0f
#define SMAP_CLEAR_COLOR		D3DCOLOR_COLORVALUE(1.0f, 0.0f, 0.0f, 0.0f) // Storing (D, 4*(D-D^2))
#define SMAP_OVERLAP			0.05f;
#define MAX_SHADOWMAP_WIDTH		0.2f // As a percentage of total screen space
#define SHADOWMAP_SPACING		0.1f // As a percentage of the area allocated to each shadow map
#define SMAP_SOFTNESS_MAX		0.25f
#define SMAP_SOFTNESS_STEPS		0.002f

// GBUFFER PARAMETERS
// 64bit GBUFFER Layout:
//	depth buffer:	|	 Pos.z (32bit)		|						|
//	normal buffer:	|        norm.x	        |        norm.y	        |
//	albedo buffer:	|  diff.r	|  diff.g	|  diff.b	|  specInt	|
//	misc buffer:	|  SM_split	|  specPow	|   Vel.x	|	Vel.y	|

// 32bit GBUFFER Layout:
//	depth buffer:	|	               Pos.z (32bit)				|
//	normal buffer:	|		 norm.x			|         norm.y		|
//	albedo buffer:	|  diff.r	|  diff.g	|  diff.b	|  specInt	|
//	misc buffer:	|  SM_split	|  specPow	|   Vel.x	|	Vel.y	|

// General notes: D3DFMT_A16B16G16R16F is slow on NVidia 7800 GTX I think
#define DEPTH_BUFFER_FORMAT_64B								D3DFMT_G32R32F // Must be float and at least 32bits.
#define NORMAL_BUFFER_FORMAT_64B							D3DFMT_G32R32F // Must be float and at least 16bits.  Need negative values to encode.
#define ALBEDO_BUFFER_FORMAT_64B							D3DFMT_A16B16G16R16 // Make int (faster)
#define MISC_BUFFER_FORMAT_64B								D3DFMT_A16B16G16R16 // Make int (faster), specPow has log encoding... see common_3_0_.fx)

#define DEPTH_BUFFER_FORMAT_32B								D3DFMT_R32F
#define NORMAL_BUFFER_FORMAT_32B							D3DFMT_G16R16F
#define ALBEDO_BUFFER_FORMAT_32B							D3DFMT_A8R8G8B8
#define MISC_BUFFER_FORMAT_32B								D3DFMT_A8R8G8B8

#define DEPTH_BUFFER_ID										0
#define NORMAL_BUFFER_ID									1
#define ALBEDO_BUFFER_ID									2
#define MISC_BUFFER_ID										3
#define DEPTH_BUFFER_CLEAR									D3DXCOLOR(1.0f, 0.0f, 0.0f, 0.0f)
#define NORMAL_BUFFER_CLEAR									D3DXCOLOR(1.0f, 0.0f, 0.0f, 0.0f) // When transformed (x*2 - 1), normal is 0
#define ALBEDO_BUFFER_CLEAR									D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f) // Clear color is black
#define MISC_BUFFER_CLEAR									D3DXCOLOR(0.0f, 0.0f, 0.5f, 0.5f)
#define GBUFFER_CLEAR_DEPTH									1.0f
#define GBUFFER_CLEAR_STENCIL								255
#define GBUFFER_TEXTUREDMESH_SHADING_PASS					0
#define GBUFFER_TEXTUREDMESH_NOSHADING_PASS					1
#define GBUFFER_TEXTUREDMESH_VELOCITY_SHADING_PASS			2
#define GBUFFER_TEXTUREDMESH_VELOCITY_NOSHADING_PASS		3
#define GBUFFER_TEXTUREDMESH_WIREFRAME_NOSHADING_PASS		0
#define GBUFFER_SINGLECOLORMESH_SHADING_PASS				0
#define GBUFFER_SINGLECOLORMESH_NOSHADING_PASS				1
#define GBUFFER_SINGLECOLORMESH_WIREFRAME_NOSHADING_PASS	1
#define GBUFFER_SKY_NOSHADING_PASS							0
#define GBUFFER_SKY_VELOCITY_NOSHADING_PASS					1

// LIGHTING PARAMETERS
#define SCENE_CLEAR_COLOR					D3DCOLOR_COLORVALUE(0.0f, 0.0f, 0.0f, 0.0f)
#define ACCUMULATION_CLEAR_COLOR			D3DCOLOR_COLORVALUE(0.0f, 0.0f, 0.0f, 0.0f)
#define LIGHT_VISULIZATION_COLOR			D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, 1.0f)
#define LIGHT_VISULIZATION_SIZE				0.25f
#define CONE_NUM_BASE_VERTICES				6
#define CONE_HEIGHT							1.0f
#define CONE_INSIDERADIUS					1.0f
#define CONE_COLOR							D3DCOLOR_COLORVALUE(1,1,1,1)
#define SPHERE_NUM_STACKS					6
#define SPHERE_NUM_SLICES					6
#define SPHERE_INSIDERADIUS					1.0f
#define SPHERE_COLOR						D3DCOLOR_COLORVALUE(1,1,1,1)
#define LIGHT_VOLUME_PASS					0
#define LIGHT_VOLUME_PASS_INTERSECT_NEAR	1
#define LIGHT_VOLUME_PASS_INTERSECT_FAR		2
#define LIGHT_FULLSCREEN_QUAD_PASS			3
#define COMBINELIGHT_PASS					0
#define COMBINELIGHT_AND_SMVISUAL_PASS		1

// HDR PARAMETERS
#define HDR_TONEMAP_BLOOM_ADAPTED_LUMINANCE_PASS			0
#define HDR_TONEMAP_NO_BLOOM_ADAPTED_LUMINANCE_PASS			1
#define HDR_TONEMAP_BLOOM_MANUAL_LUMINANCE_PASS				2
#define HDR_TONEMAP_NO_BLOOM_MANUAL_LUMINANCE_PASS			3
#define HDR_TONEMAP_THRESHOLD_ADAPTED_LUMINANCE_PASS		0
#define HDR_TONEMAP_THRESHOLD_MANUAL_LUMINANCE_PASS			1
#define MAXBLOOMFILTERRADIUS								12
#define MAXBLOOMFILTERSIGMA									10.0f
#define MAXLUMINANCEFTAU									0.99f
#define MAXBLOOMMULTIPLIER									5.0f
#define MAXMIDDLEGREY										2.0f
#define MAXMOTIONBLURSAMPLES								25
#define MAXMANUALLUMINANCE									5.0f
#define MINMANUALLUMINANCE									0.025f
#define STEPSIZEMANUALLUMINANCE								0.025f

// MISC AND POSTPROCESSING PARAMETERS
#define TEXTURE_ALPHA_BLEND_PASS			1
#define DOFBLUR_VERTEX_FOCUSLOOKUP_PASS		0
#define DOFBLUR_PIXEL_FOCUSLOOKUP_PASS		1
#define DOFBOUNDNUMSTEPS					25
#define DOFBOUNDMAXZ						20.0f

// SCREEN SPACE AMBIENT OCCLUSION (SSAO) PARAMETERS
#define OCCLUSION_CLEAR_COLOR	D3DCOLOR_COLORVALUE(1.0f, 1.0f, 1.0f, 1.0f)
#define MAX_SSAO_PREVIEW_WIDTH  0.15f // As a percentage of total screen space
#define SSAO_PREVIEW_SPACING    0.1f // As a percentage of the area allocated to each shadow map
#define SSAO_RADIUS_MAX			3.0f
#define SSAO_BIAS_MAX			1.0f
#define SSAO_INTENSITY_MAX		10.0f
#define SSAO_SCALE_MAX			3.0f
#define RANDOM_NORMAL_FORMAT	D3DFMT_G16R16 // switching to 32bit (and 2D vector) makes no noticable difference

#endif
