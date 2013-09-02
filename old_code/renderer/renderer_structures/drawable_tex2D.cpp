#include "renderer\renderer_structures\drawableTex2D.h"
#include "utils_and_misc_classes\stringUtil.h"
#include "utils_and_misc_classes\math\math_funcs.h"
#include "main.h"
#include "app.h"
#include "renderer\renderer.h"
#include "objectManager\objectManager.h"
#include "renderer\renderer_structures\resettableResources\resettableDrawableTex2D.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

drawableTex2D::drawableTex2D()
{
	mTex = NULL;
	mTopSurf = NULL;
	mWidth = 0;
	mHeight = 0;
	mMipLevels = 0;
	mTexFormat = D3DFMT_UNKNOWN;
	mAutoGenMips = 0;
	mTexSystemMem = NULL;
	mTopSurfSystemMem = NULL;
}

void drawableTex2D::InitDrawableTex2D(UINT width, UINT height, UINT mipLevels, D3DFORMAT texFormat, bool autoGenMips)
{
	mTex = NULL;
	mTexFormat = texFormat;
	mMipLevels = mipLevels;
	mWidth = width;
	mHeight = height;
	mAutoGenMips = autoGenMips;

	UINT usage = D3DUSAGE_RENDERTARGET;
	if(mAutoGenMips)
		usage |= D3DUSAGE_AUTOGENMIPMAP;

	HR(D3DXCreateTexture(g_renderer->GetD3DDev(), mWidth, mHeight, mMipLevels, usage, mTexFormat, D3DPOOL_DEFAULT, &mTex),
		L"drawableTex2D::InitDrawableTex2D() - D3DXCreateTexture failed: ");
	HR(mTex->GetSurfaceLevel(0, &mTopSurf),L"drawableTex2D::InitDrawableTex2D() - GetSurfaceLevel failed: ");

	// Add the object to the list of resettable devices for later
	resettable * newObj = new resettableDrawableTex2D(this);
	g_objectManager->AddResettableResource(newObj);
}

drawableTex2D::~drawableTex2D()
{
	if(mTex || mTopSurf)
		OnLostDevice();
}

void drawableTex2D::Release()
{
	if(mTex || mTopSurf)
		OnLostDevice();
}

void drawableTex2D::OnLostDevice()
{
	ReleaseCOM(mTex);
	ReleaseCOM(mTopSurf);
}

void drawableTex2D::OnResetDevice()
{
	UINT usage = D3DUSAGE_RENDERTARGET;
	if(mAutoGenMips)
		usage |= D3DUSAGE_AUTOGENMIPMAP;

	HR(D3DXCreateTexture(g_renderer->GetD3DDev(), mWidth, mHeight, mMipLevels, usage, mTexFormat, D3DPOOL_DEFAULT, &mTex),
		L"drawableTex2D::OnResetDevice() - D3DXCreateTexture failed: ");
	HR(mTex->GetSurfaceLevel(0, &mTopSurf),L"drawableTex2D::OnResetDevice() - GetSurfaceLevel failed: ");
}

void drawableTex2D::BeginScene()
{
	HR(g_renderer->GetD3DDev()->SetRenderTarget(0,mTopSurf),L"drawableTex2D::beginScene() - SetRenderTarget Failed: ");

	HR(g_renderer->GetD3DDev()->BeginScene(),L"drawableTex2D::beginScene() - BeginScene failed: ");
}

void drawableTex2D::EndScene()
{
	HR(g_renderer->GetD3DDev()->EndScene(),L"drawableTex2D::endScene() - EndScene failed: ");
	// EDIT, NO LONGER PUTTING RENDER TARGET TO BACK BUFFER!
}

void drawableTex2D::GenerateTexMipSubLevels()
{
	if(mMipLevels > 1)
		mTex->GenerateMipSubLevels();
}


void drawableTex2D::GetRenderTargetDataPointSample(D3DXVECTOR4 * retVal, D3DXVECTOR2 * texCoord)
{
	// Create a surface in SYSTEMMEM to load the data into (if it doesn't already exist):
	if(mTexSystemMem == NULL)
		HR(D3DXCreateTexture(g_renderer->GetD3DDev(), mWidth, mHeight, mMipLevels, 0, mTexFormat, D3DPOOL_SYSTEMMEM, &mTexSystemMem), L"drawableTex2D::GetRenderTargetData() - D3DXCreateTexture failed: ");
	if(mTopSurfSystemMem == NULL)		
		HR(mTexSystemMem->GetSurfaceLevel(0, &mTopSurfSystemMem),L"drawableTex2D::GetRenderTargetData() - GetSurfaceLevel failed: ");

	// Copy over the data using D3D function
	HR(g_renderer->GetD3DDev()->GetRenderTargetData(mTopSurf, mTopSurfSystemMem), L"drawableTex2D::GetRenderTargetData() - GetRenderTargetData failed: ");

	// Now lock the surface and read the pixel at the tex coord:
	D3DLOCKED_RECT lr;
	RECT rect; // Lock the entire structure (no slower than just locking one pixel according to gamedev) --> But this is a debug function anyway so who cares!
	rect.left = 0;		rect.right = mWidth;
	rect.top = 0;		rect.bottom = mHeight;
	HR(mTopSurfSystemMem->LockRect( &lr, &rect, D3DLOCK_READONLY ),L"drawableTex2D::GetRenderTargetData() - LockRect failed: ");
	// Get the pixel from the texcoord
	BYTE* pData = (BYTE*)lr.pBits;
	D3DXVECTOR4 bytesPerChannel;
	GetFormatByteSizePerChannelRGBA(mTexFormat, & bytesPerChannel);
	int BytesPerTexel = (int)(bytesPerChannel.x + bytesPerChannel.y + bytesPerChannel.z + bytesPerChannel.w);
	// lr.Pitch = Number of bytes in one row of the surface, which is the horizontal data
	// UINT TexelsPerRow = (UINT)floor((float)lr.Pitch / (float)BytesPerTexel); // The end of the row may have padding

	// Get the nearest pixel to the texCoord input
	int X = (int)Round(texCoord->x * mWidth);
	int Y = (int)Round(texCoord->y * mHeight);

	BYTE*  pPixelAddress = ( (unsigned char*)pData ) + X * BytesPerTexel + Y * lr.Pitch;
	DecodePixelData(mTexFormat, pPixelAddress, retVal);

	HR(mTopSurfSystemMem->UnlockRect(),L"drawableTex2D::GetRenderTargetData() - LockRect failed: ");

}