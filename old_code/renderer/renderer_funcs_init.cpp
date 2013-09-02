/*************************************************************
**						 Renderer							**
**		-> Graphics Rendering functions, Summer 2009		**
*************************************************************/
// File:		renderer.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Some free models: http://artist-3d.com/free_3d_models/

#include	"renderer\renderer.h"
#include	"renderer\renderer_structures\drawableTex2D.h"
#include	"renderer\renderer_structures\drawableTexAtlas2D.h"
#include	"renderer\renderer_structures\resettableResources\resettableTexture.h"
#include	"physics\physics.h"
#include	"main.h"
#include	"UI\UI.h"
#include	"UI\varNames.h"
#include	"objectManager\objectManager.h"
#include	"hud\hud.h"
#include	"sky\sky.h"
#include	"utils_and_misc_classes\SIMD_helpers\cpuid.h"
#include	"renderer\renderer_structures\vertex.h"
#include	"camera\camera.h"
#include	"physics\objects\AABbox.h"
#include	"app.h"
#include	"utils_and_misc_classes\data_structures\vec.h"
#include	"physics\objects\rbobjectMeshData.h"
#include	"renderer\renderer_constants.h"
#include	"utils_and_misc_classes\util.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		InitRenderer											*/
/* Description:	set projection matrix, render states and texture states	*/
/************************************************************************/
void renderer::InitRenderer()
{
	g_objectManager->GetHud()->RenderProgressText( L"Compiling and loading effect files...");
	g_app->CheckMessages();

	// LOAD IN THE MAIN EFFECTS FILE
	BuildFX();

	g_objectManager->GetHud()->RenderProgressText( L"Initializing rendering system...");
	g_app->CheckMessages();

	// MAKE WHITETEX AND IT'S ASSOCIATED TEXTURE FILTER: REQUIRED FOR MODELS WITHOUT TEXTURES
	LoadWhiteTex();

	// INITIALIZE THE GBUFFER
	InitGBuffer();

	// INITIALIZE THE POST PROCESSING EFFECTS
	InitPostProcessing();

	// INITIALIZE THE SSAO RENDER TARGETS (Must be done AFTER the post processing)
	InitSSAO();

	// INITIALIZE THE SHADOW MAPS
	InitShadowMaps();

	// INITIALIZE THE LIGHTING STAGE
	InitLighting();

	// Create a fullscreen quad (in screen-space) to render textures to the screen
	CreateFSQuad(& m_FSQuadVertBuffSize, &m_FSQuadVertBuff);

	// INITIALIZE PHYSICS ENGINE WITH NEW MODELS
	g_physics->InitPhysics();

}//InitScene

/************************************************************************/
/* Name:		BuildFX													*/
/* Description:	Create the FX from a .fx file.							*/
/************************************************************************/
void renderer::BuildFX()
{
	 // Get the FX filenames
	LPCWSTR shaderFile_Lighting;
	LPCWSTR shaderFile_SMap;
	LPCWSTR shaderFile_GBuffer;
	LPCWSTR shaderFile_PostProcess;
	switch(m_shaderVersion)
	{
	case 3: 
		shaderFile_Lighting		= L"Shaders/lighting_3_0.fx"; 
		shaderFile_SMap			= L"Shaders/sMap_3_0.fx"; 
		shaderFile_GBuffer		= L"Shaders/gBuffer_3_0.fx"; 
		shaderFile_PostProcess	= L"Shaders/postProcess_3_0.fx"; 
		break;
	default: 
		throw std::runtime_error("renderer::BuildFX() - m_shaderVersion must be 3."); 
		break;
	}
	
#ifdef D3DPROFILE
	DWORD compileFlages = D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION;
#else
	DWORD compileFlages = D3DXSHADER_OPTIMIZATION_LEVEL3;
#endif

#ifdef _DEBUG
	if(compileFlages == D3DXSHADER_OPTIMIZATION_LEVEL3)
		OutputDebugString(L"\n --> Using fully optimized shaders.\n\n");
	else
		OutputDebugString(L"\n --> Using debug shaders.\n\n");
#endif

	// Compile the FX files
	m_FXLighting	= CompileFX(shaderFile_Lighting, compileFlages);
	m_FXSMap		= CompileFX(shaderFile_SMap, compileFlages);
	m_FXGBuffer		= CompileFX(shaderFile_GBuffer, compileFlages);
	m_FXPostProcess = CompileFX(shaderFile_PostProcess, compileFlages);

	// Grab all the FX handles
	m_FXHandlesLighting.GetHandles(m_FXLighting);
	m_FXHandlesGBuffer.GetHandles(m_FXGBuffer);
	m_FXHandlesSMap.GetHandles(m_FXSMap);
	m_FXHandlesPostProcess.GetHandles(m_FXPostProcess);
}

/************************************************************************/
/* Name:		CompileFX												*/
/* Description:	Compile a FX file and return the pointer				*/
/************************************************************************/
ID3DXEffect * renderer::CompileFX(LPCWSTR fileName, DWORD compileFlages)
{
	ID3DXEffect * retVal = NULL;
	ID3DXBuffer * errors = 0;
	if(FAILED(D3DXCreateEffectFromFile(m_pD3DDev, fileName, 0, 0, compileFlages, 0, &retVal, &errors)) || errors)
	{
		if( errors )
			MessageBox(0, stringUtil::toWideString((char*)errors->GetBufferPointer(),-1).c_str(), 0, 0);
		throw std::runtime_error("renderer::CompileFX() - D3DXCreateEffectFromFile failed");
	}
	return retVal;
}

/************************************************************************/
/* Name:		OnLostDevice & OnResetDevice							*/
/* Description:	When device goes out of focus -->  Fix it				*/
/************************************************************************/
void renderer::OnLostDevice()
{
#ifdef _DEBUG
	OutputDebugString(L"renderer::OnLostDevice() - Lost Device called...  Releasing resources...\n");
#endif
	if(g_objectManager)
	{
		g_objectManager->OnLostDevice();
		if(g_objectManager->GetHud()) { g_objectManager->GetHud()->OnLostDevice(); }
		if(g_objectManager->GetSky()) { g_objectManager->GetSky()->OnLostDevice(); }
	}
	if(m_FXLighting)
		HR(m_FXLighting->OnLostDevice(),L"renderer::OnLostDevice() - m_FXLighting->OnLostDevice() failed: ");
	if(m_FXGBuffer)
		HR(m_FXGBuffer->OnLostDevice(),L"renderer::OnLostDevice() - m_FXGBuffer->OnLostDevice() failed: ");
	if(m_FXSMap)
		HR(m_FXSMap->OnLostDevice(),L"renderer::OnLostDevice() - m_FXSMap->OnLostDevice() failed: ");
	if(m_FXPostProcess)
		HR(m_FXPostProcess->OnLostDevice(),L"renderer::OnLostDevice() - m_FXPostProcess->OnLostDevice() failed: ");
	HR(m_pD3DDev->SetDepthStencilSurface(NULL),L"renderer::OnLostDevice() - SetDepthStencilSurface failed: ");
	ReleaseCOM(m_DepthStencilSurf);
	// For MANAGED textures, on lost device is NOT needed
}

void renderer::OnResetDevice()
{
#ifdef _DEBUG
	OutputDebugString(L"renderer::OnResetDevice() - Reset Device called...  Reloading resources...\n");
#endif
	if(g_objectManager)
	{
		if(g_objectManager->GetHud()) { g_objectManager->GetHud()->OnResetDevice(); }
		if(g_objectManager->GetSky()) { g_objectManager->GetSky()->OnResetDevice(); }
		g_objectManager->OnResetDevice();
	}
	if(m_FXLighting)
		HR(m_FXLighting->OnResetDevice(),L"renderer::OnResetDevice() - m_FXLighting->OnResetDevice() failed: ");
	if(m_FXGBuffer)
		HR(m_FXGBuffer->OnResetDevice(),L"renderer::OnResetDevice() - m_FXGBuffer->OnResetDevice() failed: ");
	if(m_FXSMap)
		HR(m_FXSMap->OnResetDevice(),L"renderer::OnResetDevice() - m_FXSMap->OnResetDevice() failed: ");
	if(m_FXPostProcess)
		HR(m_FXPostProcess->OnResetDevice(),L"renderer::OnResetDevice() - m_FXPostProcess->OnResetDevice() failed: ");

	curState.occlusionCleared = false; // It'll need clearing again now it's new.
	CreateDepthStencilSurface();

	// Now, all the effects files are pointing to the wrong textures, fix this:
	SetPostProcessingTextures();
	SetLightingTextures();
}

/************************************************************************/
/* Name:		InitD3D													*/
/* Description:	create Direct3D object and device						*/
/************************************************************************/
void renderer::InitD3D()
{
	HWND m_hWindow = g_app->GetWindowHandle();

	if(g_UI->GetSetting<bool>(& var_useSSESIMD))
	{
		// Check for SSE extensions
		cpuid::_p_info info;
		cpuid::get_cpuid(&info);
		#ifdef _DEBUG
			if(g_UI->GetSetting<bool>(& var_printSSEInfo ))
				cpuid::print_cpuid_info(&info);
		#endif
		if(!(cpuid::check_sse(&info) && check_sse2(&info)))
		{
			std::wstring err = L"renderer::InitD3D() - CPU does not support SSE & SSE2-SIMD instructions! \nPlease run on Pentium III CPU or newer. \nSIMD will be disabled at performance cost.";
			MessageBox(NULL, err.c_str(), L"Warning", MB_OK | MB_ICONERROR);
			g_UI->SetSetting<bool>(& var_useSSESIMD, false);
		}
	}

	//create Direct3D interface object, D3D_SDK_VERSION V9c = 32 
	m_pD3DObj = Direct3DCreate9(D3D_SDK_VERSION);
	ER_CHECK(m_pD3DObj==0, L"renderer::InitD3D() - Direct3DCreate9() failed. Error: ");

	m_AdapterToUse = D3DADAPTER_DEFAULT;
	m_DeviceType = D3DDEVTYPE_HAL;
#ifdef PERFHUD_BUILD
	// Look for 'NVIDIA PerfHUD' adapter
	// If it is present, override default settings
	for (UINT Adapter=0;Adapter<m_pD3DObj->GetAdapterCount();Adapter++)
	{
		D3DADAPTER_IDENTIFIER9 Identifier;
		HRESULT Res;
		Res = m_pD3DObj->GetAdapterIdentifier(Adapter,0,&Identifier);
		if (strstr(Identifier.Description,"PerfHUD") != 0)
		{
			m_AdapterToUse=Adapter;
			m_DeviceType=D3DDEVTYPE_REF;
			break;
		}
	}
#endif

	WINDOWPLACEMENT placement;
	GetWindowPlacement(g_app->GetWindowHandle(),&placement);

	// Change window style to remove border in window mode
	DWORD dwStyle;
	int m_dwWidth; int m_dwHeight; g_UI->GetWidthHeight(& m_dwWidth, & m_dwHeight);
	if(g_UI->GetComboBoxVal(& var_fullscreen) != 1)
	{
		dwStyle = (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_THICKFRAME;
		SetWindowLongPtr(m_hWindow, GWL_STYLE, dwStyle);
		// Make sure the changes are flushed and change window size anyway
		RECT DesktopSize; SIZE sizeWindow; RECT rc;
		rc.top = rc.left = 0; rc.right = m_dwWidth; rc.bottom = m_dwHeight;
		AdjustWindowRect(&rc, dwStyle, 0); // in window mode the border can hide some pixels
		sizeWindow.cx = rc.right - rc.left;
		sizeWindow.cy = rc.bottom - rc.top;
		GetClientRect(GetDesktopWindow(),&DesktopSize); //Get desktop resolution
		ER_CHECK(!SetWindowPos(m_hWindow, NULL, (DesktopSize.right - m_dwWidth) / 2, (DesktopSize.bottom - m_dwHeight) / 2, sizeWindow.cx, 
			sizeWindow.cy, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW), L"renderer::InitD3D() - Failed to resize window!!");
		// Actually, AdjustWindowRect wont take into account the menu-bar!  Need to do this manually ourselves.
		g_app->SetClientSize( m_hWindow, m_dwWidth, m_dwHeight );
	}
	else
	{
		// Change the window style to a more fullscreen friendly style.
		SetWindowLongPtr(m_hWindow, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		// SetWindowLongPtr(m_hWindow, GWL_STYLE, WS_POPUP | WS_SYSMENU);
		ER_CHECK(SetWindowPos(m_hWindow, NULL, 0, 0, m_dwWidth, m_dwHeight, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW)==0, L"renderer::InitD3D() - Failed to resize window!!")
	}

		// check that we can run the desired settings
	GetPresentParameters(& m_PresentParameters);
	CheckObjCaps();

	DWORD behaviourFlags = g_UI->GetVertexProcessing();
	if(pureDevice)
		behaviourFlags |= D3DCREATE_PUREDEVICE;

	bool er = FAILED(m_pD3DObj->CreateDevice(m_AdapterToUse, // Which video card to use
											 m_DeviceType, // DeviceType: _HAL = hardware abstraction layer (may use software if no hardware support).
											 m_hWindow,
											 behaviourFlags, // Where 3D calculations are done (software, etc)
											 &m_PresentParameters, // our desired parameters
											 &m_pD3DDev)); // graphics device interface
	if(er && g_UI->GetComboBoxVal(& var_fullscreen)) // Get out of fullscreen
		ShowWindow(m_hWindow,SW_HIDE);
	ER_CHECK(er,L"renderer::InitD3D() - CreateDevice() failed -> Possibly your adapter doesn't support the desired resolution.  Try 800x600 first.");

	CheckDevCaps();

	// Fix a bug on windows 7.  CheckDevice returns OK even though internal device is reset.
	if(util::IsWin7() || util::IsVista())
	{
		if(PerformResetLoop() == 0)
			throw std::runtime_error("renderer::InitD3D() - Could not reset device after changing window size");
	}

	CheckDevice();
	
	g_app->m_bAppPaused = 0;
	windowPreFullscreen.x = 0; windowPreFullscreen.y = 0;
	
}//InitD3D

/************************************************************************/
/* Name:		PerformResetLoop										*/
/* Description:	Release resources, reset then reload resources			*/
/************************************************************************/
UINT renderer::PerformResetLoop()
{
	OnLostDevice();
	if(FAILED(m_pD3DDev->Reset(&m_PresentParameters))) 
	{
		MessageBox(g_app->GetWindowHandle(),L"renderer::PerformResetLoop() - Reset() failed!",L"CheckDevice()",MB_OK);
		return 0;
	}
	OnResetDevice();
	return 1;
}

/************************************************************************/
/* Name:		CheckObjCaps & CheckDevCaps								*/
/* Description:	Create our own managed depth stencil surface.			*/
/************************************************************************/
void renderer::CreateDepthStencilSurface()
{
	if(m_DepthStencilSurf == NULL)
	{
		if(m_shadowMap != NULL) // Don't make it yet.  The SM texture isn't initialized.  Happens on Windows 7 fullscreen startup hack (where a reset loop is called)
		{
			// Make a new depth stencil surface
			int m_dwWidth; int m_dwHeight; g_UI->GetWidthHeight(& m_dwWidth, & m_dwHeight);
			m_DepthStencilWidth = max((UINT)m_dwWidth, m_shadowMap->Width());
			m_DepthStencilHeight = max((UINT)m_dwHeight, m_shadowMap->Height());
			HR(m_pD3DDev->CreateDepthStencilSurface(m_DepthStencilWidth, m_DepthStencilHeight,						// Width and Height
													(D3DFORMAT)g_UI->GetComboBoxVal( & var_depthStencilFormat ),	// Format
													(D3DMULTISAMPLE_TYPE)g_UI->GetComboBoxVal(& var_multiSampling),	// Multisample
													0,																// Multisample quality
													false,															// Discard on Present or SetDepthStencilSurface
													& m_DepthStencilSurf,											// depth-stencil resource
													NULL), L"renderer::CreateDepthStencilSurface() - CreateDepthStencilSurface failed: ");  // reserved parameter
			HR(m_pD3DDev->SetDepthStencilSurface(m_DepthStencilSurf),L"renderer::CreateDepthStencilSurface() - SetDepthStencilSurface failed: ");
		}
	}
}

/************************************************************************/
/* Name:		CheckObjCaps & CheckDevCaps								*/
/* Description:	check for necessary device caps							*/
/* --> If we're full screen, we can choose whatever as long as card		*/
/*     supports what we want.  If windowed we're need to check for 		*/
/*     format conversion (F. Luna, pg 84)								*/
/************************************************************************/
bool renderer::CheckObjCaps()
{
	D3DFORMAT fmtBackbuffer;
	D3DDISPLAYMODE mode;
	HRESULT hResult;

	// Grab the current desktop format
	HR(m_pD3DObj->GetAdapterDisplayMode(g_renderer->GetAdapterToUse(), &mode),L"renderer::CheckObjCaps() - GetAdapterDisplayMode() failed. Error: ");

	int m_dwWidth; int m_dwHeight; g_UI->GetWidthHeight(& m_dwWidth, & m_dwHeight);
	if(g_UI->GetComboBoxVal(& var_fullscreen) != 1) 
	{
		// Check for conversion between desired backbuffer and desktop
		hResult = m_pD3DObj->CheckDeviceFormatConversion(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), (D3DFORMAT)g_UI->GetComboBoxVal(& var_backbufferFormat), mode.Format);
		if(!FAILED(hResult))
			hResult = m_pD3DObj->CheckDeviceFormatConversion(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), mode.Format, (D3DFORMAT)g_UI->GetComboBoxVal(& var_backbufferFormat) );
		if(FAILED(hResult))
			throw std::runtime_error("renderer::CheckObjCaps() - A conversion between the current backbuffer format to the desktop format is not supported.");
		fmtBackbuffer = (D3DFORMAT)g_UI->GetComboBoxVal(& var_backbufferFormat);
	} else {
		// Format is the adapter format...
		fmtBackbuffer = mode.Format;
		mode.Height = m_dwHeight;
		mode.Width = m_dwWidth; 
		mode.RefreshRate = 0; // Adapter default
	}

	// Need to see if this format is ok as a backbuffer format in this adapter mode
	HR(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), mode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, fmtBackbuffer),
		L"renderer::CheckObjCaps() - Unable to initialize your chosen backbuffer format!");

	HR(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), mode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, (D3DFORMAT)g_UI->GetComboBoxVal(& var_depthStencilFormat)),
		L"renderer::CheckObjCaps() - Unable to initialize your chosen Depth Stencil format!");

	DWORD retQualityLevels;
	HR(m_pD3DObj->CheckDeviceMultiSampleType(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), mode.Format, (g_UI->GetComboBoxVal(& var_fullscreen) != 1), (D3DMULTISAMPLE_TYPE)g_UI->GetComboBoxVal(& var_multiSampling), &retQualityLevels),
		L"renderer::CheckObjCaps() - Unable to initialize your chosen Multi Sampling Format!");

	// If in fullscreen make sure adaptor + monitor can display this resolution
	if(g_UI->GetComboBoxVal(& var_fullscreen) &&
	   CheckDisplayModeResolution(m_dwWidth, m_dwHeight, fmtBackbuffer) == false)
		throw std::runtime_error("renderer::CheckObjCaps() - Fullscreen at this resolution is not supported by your monitor.  Try windowed (Fullscreen off)");

	// Get capabilities for this device --> HANDY TO HAVE
	HR(m_pD3DObj->GetDeviceCaps(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), m_ObjectCaps),
		L"renderer::CheckObjCaps() - GetDeviceCaps() failed. Error: ");;

	// check for hardware vertex processing if we've chosen it.
	DWORD behaviourFlags = g_UI->GetVertexProcessing();
	if((behaviourFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) != 0)
	{
		if(!(m_ObjectCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT))
			throw std::runtime_error("renderer::CheckObjCaps() - Device does not support hardware vertex processing!");
	}

	if(g_UI->GetSetting<bool>( &var_pureDevice ))
	{
		// If pure device and HW T&L supported --> Make a pure device
		if( m_ObjectCaps->DevCaps & D3DDEVCAPS_PUREDEVICE &&
			behaviourFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING)
			pureDevice = true;
	}

	return 1; // Everything is OK with the settings!
}//CheckDeviceCaps

/************************************************************************/
/* Name:		CheckDevCaps											*/
/* Description:	check for necessary device caps							*/
/************************************************************************/
bool renderer::CheckDevCaps()
{
	// Get capabilities for this device --> HANDY TO HAVE
	if(m_pD3DDev == NULL)
		throw std::runtime_error("renderer::CheckDevCaps() - m_pD3DDev is NULL, trying to get caps of an uninitialized device");
	HRESULT hResult = m_pD3DDev->GetDeviceCaps(m_DeviceCaps);
	if(FAILED(hResult))
		m_pD3DObj->Release();
	HR(hResult,L"renderer::CheckDevCaps() - GetDeviceCaps() failed. Error: ");

	// Check for vertex shader version 2.0 support.
	if( (m_DeviceCaps->VertexShaderVersion < D3DVS_VERSION(3, 0)) || (m_DeviceCaps->PixelShaderVersion < D3DPS_VERSION(3, 0)))
	{
		throw std::runtime_error("renderer::CheckDevCaps() - Only shader model 3.0 is supported.  Please run applicaiton on a newer system."); 
	} else
		m_shaderVersion = 3;

	return 1; // Everything is OK with the settings!
}

/************************************************************************/
/* Name:		CheckDevice												*/
/* Description:	check for lost device --> ALT-TAB BUG!					*/
/************************************************************************/
bool renderer::CheckDevice()
{
	if(m_pD3DDev)
	{
		switch(m_pD3DDev->TestCooperativeLevel()) // a) lost, b) restorable, c) everying ok
		{
		case D3DERR_DEVICELOST: 
			{
				OnLostDevice();
				Sleep(20); // Pause to give other processes CPU
				return 0; // Stop the 3D rendering (we've lost focus)
			}
		case D3DERR_DRIVERINTERNALERROR:
			{
				throw std::runtime_error("renderer::CheckDevice() - Internal Driver Error...Exiting");
			}
		case D3DERR_DEVICENOTRESET: // Attempt to restart the rendering
			{
				return (PerformResetLoop() == 1);
			}
		case D3D_OK: // Everything is ok
			{
				return 1;
		}	
		default: 
			throw std::runtime_error("renderer::CheckDevice() - Unrecognized return value from TestCooperativeLevel()");
		}
	}
	else
		return 1;
}//CheckDevice

/************************************************************************/
/* Name:		KillD3D													*/
/* Description:	release memory for Direct3D device and Direct3D object	*/
/************************************************************************/
void renderer::KillD3D()
{
	DestroyAllVertexDeclarations();
	ReleaseCOM(m_pD3DDev);
	ReleaseCOM(m_pD3DObj);

}//KillD3D

/************************************************************************/
/* Name:		toggleFullScreenMode									*/
/* Description:	Toggle between fullscreen mode when user presses F1		*/
/************************************************************************/
void renderer::toggleFullScreenMode()
{
	DWORD dwStyle;

	int m_dwWidth; int m_dwHeight; g_UI->GetWidthHeight(& m_dwWidth, & m_dwHeight);

	// ***************************************
	// Toggle from WINDOW --> FULLSCREEN
	// ***************************************
	if( g_UI->GetComboBoxVal(& var_fullscreen) != 1 ) 
	{
		// Save the window position before going to full screen
		RECT rcWnd;
		GetWindowRect(g_app->GetWindowHandle(), &rcWnd);
		windowPreFullscreen.x = rcWnd.left;
		windowPreFullscreen.y = rcWnd.top;

		if( !m_PresentParameters.Windowed )  // Are we already in fullscreen mode?
			return; // do nothing
		g_UI->SetComboBoxVal(& var_fullscreen, 1);

		// Grab the current desktop format and make sure BackBufferFormat match
		D3DDISPLAYMODE mode;
		HR(m_pD3DObj->GetAdapterDisplayMode(g_renderer->GetAdapterToUse(), &mode),L"renderer::toggleFullScreenMode() - GetAdapterDisplayMode() failed. Error: ");
		m_PresentParameters.BackBufferFormat = mode.Format;

		// Check if the fullscreen resolution is supported
		if(CheckDisplayModeResolution(m_dwWidth, m_dwHeight, m_PresentParameters.BackBufferFormat) == false)
		{
			MessageBox(g_app->GetWindowHandle(),L"renderer::toggleFullScreenMode() - Cannot toggle fullscreen ON/OFF.  Adapter doesn't support this resolution",L"CheckDevice()",MB_OK);
			g_UI->SetComboBoxVal(& var_fullscreen, 0);
			GetPresentParameters(& m_PresentParameters);
			return;
		}
	}
	// ***************************************
	// Toggle from FULLSCREEN --> WINDOW
	// ***************************************
	else
	{
		if( m_PresentParameters.Windowed )  // Are we already in windowed mode?
			return; // do nothing
		g_UI->SetComboBoxVal(& var_fullscreen, 0);
	}

	// Check the format of resettable objects in this new backbuffer format
	GetPresentParameters(& m_PresentParameters);
	g_objectManager->CheckResettableFormat( m_PresentParameters.BackBufferFormat );
	curState.occlusionCleared = false;

	PerformResetLoop();

	// ***************************************
	// Toggle from WINDOW --> FULLSCREEN
	// ***************************************
	if( g_UI->GetComboBoxVal(& var_fullscreen)) 
	{
		// Change the window style to a more fullscreen friendly style.
		SetWindowLongPtr(g_app->GetWindowHandle(), GWL_STYLE, WS_POPUP | WS_VISIBLE);
		ER_CHECK(SetWindowPos(g_app->GetWindowHandle(), NULL, 0, 0, m_dwWidth, m_dwHeight, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW)==0, L"Failed to resize window!!")
		g_UI->HideRuntimeMenu();
	}
	// ***************************************
	// Toggle from FULLSCREEN --> WINDOW
	// ***************************************
	else
	{
		dwStyle = (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_THICKFRAME;
		SetWindowLongPtr(g_app->GetWindowHandle(), GWL_STYLE, dwStyle);
		// Make sure the changes are flushed and change window size anyway
		RECT DesktopSize; SIZE sizeWindow; RECT rc;
		rc.top = rc.left = 0; rc.right = m_dwWidth; rc.bottom = m_dwHeight;
		AdjustWindowRect(&rc, dwStyle, 0); // in window mode the border can hide some pixels
		sizeWindow.cx = rc.right - rc.left;
		sizeWindow.cy = rc.bottom - rc.top;
		GetClientRect(GetDesktopWindow(),&DesktopSize); //Get desktop resolution
		if(windowPreFullscreen.x > 0 && windowPreFullscreen.y > 0)
		{
			ER_CHECK(!SetWindowPos(g_app->GetWindowHandle(), NULL, windowPreFullscreen.x, windowPreFullscreen.y, sizeWindow.cx, 
				sizeWindow.cy, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW), L"renderer::toggleFullScreenMode() - Failed to resize window!!");
		}
		else
		{
			ER_CHECK(!SetWindowPos(g_app->GetWindowHandle(), NULL, (DesktopSize.right - m_dwWidth) / 2, (DesktopSize.bottom - m_dwHeight) / 2, sizeWindow.cx, 
				sizeWindow.cy, SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW), L"renderer::toggleFullScreenMode() - Failed to resize window!!");
		}

		g_UI->ShowRuntimeMenu();

		// Actually, AdjustWindowRect wont take into account the menu-bar!  Need to do this manually ourselves.
		g_app->SetClientSize( g_app->GetWindowHandle(), m_dwWidth, m_dwHeight );
	}

	PerformResetLoop();

	// It may take time for windows to peform the above actions.  Spin wait until everything is ok.
	while(!CheckDevice())
		Sleep(20);
}

/************************************************************************/
/* Name:		CheckDisplayModeResolution					    		*/
/* Description:	Check that the current adapter and display can output   */
/*              the desired resolution.									*/
/************************************************************************/
bool renderer::CheckDisplayModeResolution(int width, int height,D3DFORMAT fmtBackbuffer)
{
	D3DDISPLAYMODE mode;
	HRESULT hResult = m_pD3DObj->GetAdapterDisplayMode(g_renderer->GetAdapterToUse(), &mode);

	// Check that this backbuffer size is a supported fullscreen resolution too.
	// Get the number of display modes supported by this adapter in this bit depth
	UINT nAdapterModes = m_pD3DObj->GetAdapterModeCount(g_renderer->GetAdapterToUse(), fmtBackbuffer);

	// Go through each of the modes and get details about it
	bool bResolutionFound = false;
	for(UINT i=0; i<nAdapterModes; ++i)
	{
		// Grab this display mode
		hResult = m_pD3DObj->EnumAdapterModes(g_renderer->GetAdapterToUse(), fmtBackbuffer, i, & mode);
		if(FAILED(hResult))
		{
			// We failed to get this mode. Just continue to the next mode
			continue;
		}

		// Is is a match?
		if(mode.Width == (UINT)width && mode.Height == (UINT)height)
		{
			bResolutionFound = true;
			break;
		}
	}

	// Did we find the mode?
	if(!bResolutionFound)
		return false;
	else
		return true;
}

/************************************************************************/
/* Name:		GetPresetParameters										*/
/* Description:	 Get the correct preset parameters based off user input */
/************************************************************************/
void renderer::GetPresentParameters(D3DPRESENT_PARAMETERS * params)
{
	int m_dwWidth; int m_dwHeight; g_UI->GetWidthHeight(& m_dwWidth, & m_dwHeight);

	// store device (hardware) information
	ZeroMemory(params,sizeof(*params));
	params->Windowed					= g_UI->GetComboBoxVal(& var_fullscreen) != 1; // Program not windowed
	params->SwapEffect					= D3DSWAPEFFECT_DISCARD; // Type of swap chain (for back buffers), discard old frame.
	params->EnableAutoDepthStencil		= false; // Manage our own depth stencil
	params->hDeviceWindow				= g_app->GetWindowHandle(); // the window to be used by D3D
	params->BackBufferWidth				= m_dwWidth; // BB width
	params->BackBufferHeight			= m_dwHeight; // BB hight
	params->BackBufferFormat			= (D3DFORMAT)g_UI->GetComboBoxVal(& var_backbufferFormat);
	params->BackBufferCount				= 1;
	params->MultiSampleType				= (D3DMULTISAMPLE_TYPE)g_UI->GetComboBoxVal(& var_multiSampling); // 0-4 multi-samples (anti-aliasing)
	params->MultiSampleQuality			= 0; // NOT SURE IF YOU NEED THIS!!
	params->FullScreen_RefreshRateInHz	= D3DPRESENT_RATE_DEFAULT;
	params->PresentationInterval		= D3DPRESENT_INTERVAL_IMMEDIATE;
	params->Flags						= 0;
	
}

/************************************************************************/
/* Name:		CreateFSQuad											*/
/* Description:	Create a full-screen quad in screen space				*/
/************************************************************************/
void renderer::CreateFSQuad(UINT * size, IDirect3DVertexBuffer9 ** VertBuff)
{
	// SEE "quad.png" For coordinates
	*size = 6;
	HR(m_pD3DDev->CreateVertexBuffer(*size * sizeof(VertexPosTex), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, VertBuff, 0), L"renderer::CreateFSQuad() -  CreateVertexBuffer failed: ");
	// Now lock it to obtain a pointer to its internal data, and write the grid's vertex data.
	VertexPosTex* data = 0;
	HR((*VertBuff)->Lock(0, 0, (void**)&data, 0), L"renderer::CreateFSQuad() - Failed to lock vertex buffer: ");
	data[0] = VertexPosTex(D3DXVECTOR3(DXSCREENSPACE_MINX,DXSCREENSPACE_MINY,0.0f),D3DXVECTOR2(0.0f,1.0f)); // V1
	data[1] = VertexPosTex(D3DXVECTOR3(DXSCREENSPACE_MAXX,DXSCREENSPACE_MINY,0.0f),D3DXVECTOR2(1.0f,1.0f)); // V2
	data[2] = VertexPosTex(D3DXVECTOR3(DXSCREENSPACE_MINX,DXSCREENSPACE_MAXY,0.0f),D3DXVECTOR2(0.0f,0.0f)); // V4
	data[3] = VertexPosTex(D3DXVECTOR3(DXSCREENSPACE_MINX,DXSCREENSPACE_MAXY,0.0f),D3DXVECTOR2(0.0f,0.0f)); // V4
	data[4] = VertexPosTex(D3DXVECTOR3(DXSCREENSPACE_MAXX,DXSCREENSPACE_MINY,0.0f),D3DXVECTOR2(1.0f,1.0f)); // V2
	data[5] = VertexPosTex(D3DXVECTOR3(DXSCREENSPACE_MAXX,DXSCREENSPACE_MAXY,0.0f),D3DXVECTOR2(1.0f,0.0f)); // V3
	HR((*VertBuff)->Unlock(), L"renderer::CreateFSQuad() - Failed to unlock vertex buffer: ");
}

/************************************************************************/
/* Name:		GetBestTexSampler										*/
/* Description:	Get the best texture sampler that the hardware supports	*/
/************************************************************************/
texSamplerState * renderer::GetBestTexSampler(D3DFORMAT texFormat, bool cubeTexture)
{
	texSamplerState * retVal = new texSamplerState();
	retVal->maxAnisotropy = 1;
	retVal->addressU = D3DTADDRESS_WRAP;
	retVal->addressV = D3DTADDRESS_WRAP;

	DWORD caps = 0;
	D3DRESOURCETYPE usage = (D3DRESOURCETYPE)0;
	if(cubeTexture)
	{
		usage = D3DRTYPE_CUBETEXTURE;
		caps = m_DeviceCaps->CubeTextureFilterCaps;
	}
	else
	{
		usage = D3DRTYPE_TEXTURE;
		caps = m_DeviceCaps->TextureFilterCaps;
	}

	D3DFORMAT backBufferFormat = g_renderer->GetAppPresentParameters()->BackBufferFormat;
	// Check for hardware support if we're filtering textures:
	if( ! g_UI->GetSetting<bool>(& var_textureFiltering) ||
		FAILED(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), backBufferFormat, D3DUSAGE_AUTOGENMIPMAP, usage, texFormat)) ||
		FAILED(m_pD3DObj->CheckDeviceFormat(g_renderer->GetAdapterToUse(), g_renderer->GetDeviceType(), backBufferFormat, D3DUSAGE_QUERY_FILTER, usage, texFormat)))
	{
		// No hardware support
		retVal->magFilter = D3DTEXF_POINT; // These are always supported
		retVal->mipFilter = D3DTEXF_POINT;
		retVal->minFilter = D3DTEXF_POINT;

		if(g_UI->GetSetting<bool>(& var_textureFiltering))
			throw std::runtime_error("renderer::GetBestTexSampler() - Hardware doesn't support texture filtering!");
	}
	else
	{
		// Get the best mag filter
		if(caps & D3DPTFILTERCAPS_MAGFANISOTROPIC)
		{ retVal->magFilter = D3DTEXF_ANISOTROPIC; retVal->maxAnisotropy = 16; }
		else if(caps & D3DPTFILTERCAPS_MAGFLINEAR)
			retVal->magFilter = D3DTEXF_LINEAR;
		else
			retVal->magFilter = D3DTEXF_POINT;

		// Get the best min filter
		if(caps & D3DPTFILTERCAPS_MINFANISOTROPIC)
		{ retVal->minFilter = D3DTEXF_ANISOTROPIC; retVal->maxAnisotropy = 16; }
		else if(caps & D3DPTFILTERCAPS_MINFLINEAR)
			retVal->minFilter = D3DTEXF_LINEAR;
		else
			retVal->minFilter = D3DTEXF_POINT;

		// Get the best mip filter --> There is no ANISOTROPIC mip filter cap
		if(caps & D3DPTFILTERCAPS_MIPFLINEAR)
			retVal->mipFilter = D3DTEXF_LINEAR;
		else
			retVal->mipFilter = D3DTEXF_POINT;
	}

	return retVal;
}

/************************************************************************/
/* Name:		LoadWhiteTex											*/
/* Description:	Load the white texture used when mesh objects don't		*/
/*              have textures.											*/
/************************************************************************/
void renderer::LoadWhiteTex()
{
	D3DFORMAT backBufferFormat = g_renderer->GetAppPresentParameters()->BackBufferFormat;
	UINT mipLevels = 1;
	LoadTexture(L"models/whitetex.dds",backBufferFormat,mipLevels, &m_WhiteTex);
	m_WhiteTexSamplerState.SetPointFilterWrap();

	m_defaultMtrl = new Mtrl();
	m_defaultMtrl->diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	m_defaultMtrl->specIntensity = 1.0f;
	m_defaultMtrl->specPower = 8.0f;
}

/************************************************************************/
/* Name:		LoadTexture												*/
/* Description:	Wrapper to D3DXCreateTextureFromFileEx()				*/
/************************************************************************/
void renderer::LoadTexture(LPCWSTR pFile, D3DFORMAT format, UINT numMips, IDirect3DTexture9 ** tex )
{
	// Add the texture to the global list
	resettable * newObj = new resettableTexture(pFile, format, numMips, tex);
	newObj->OnResetDevice(); // Load in the texture by calling on reset device
	g_objectManager->AddResettableResource(newObj);
}