#include "renderer\renderer_structures\drawableTexAtlas2D.h"
#include "utils_and_misc_classes\stringUtil.h"
#include "main.h"
#include "app.h"
#include "renderer\renderer.h"
#include "objectManager\objectManager.h"
#include "renderer\renderer_structures\resettableResources\resettableDrawableTexAtlas2D.h"
#include "utils_and_misc_classes\math\math_funcs.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

drawableTexAtlas2D::drawableTexAtlas2D()
{
	mTex = NULL;
	mTopSurf = NULL;
	mUnitWidth = 0;
	mUnitHeight = 0;
	mNumTextures = 0;
	mMipLevels = 0;
	mTexFormat = D3DFMT_UNKNOWN;
	mAutoGenMips = 0;
	mTextureOffsets = NULL;
	mTextureScale = D3DXVECTOR2(-1,-1);
}

void drawableTexAtlas2D::InitDrawableTexAtlas2D(UINT unitWidth, UINT unitHeight, UINT numTextures, UINT mipLevels, D3DFORMAT texFormat, bool autoGenMips)
{
	mTex = NULL;
	mTexFormat = texFormat;
	mMipLevels = mipLevels;
	mUnitWidth = unitWidth;
	mUnitHeight = unitHeight;
	mNumTextures = numTextures;
	mAutoGenMips = autoGenMips;

	if(mNumTextures < 1)
		throw std::runtime_error("drawableTexAtlas2D::InitdrawableTexAtlas2D() - Error: numTextures < 1");
	if(mMipLevels > 1)
		throw std::runtime_error("drawableTexAtlas2D::InitdrawableTexAtlas2D() - Error: mipLevels > 1, mip mapping is not yet supported");
	if(mAutoGenMips)
		throw std::runtime_error("drawableTexAtlas2D::InitdrawableTexAtlas2D() - Error: mAutoGenMips == true, mip mapping is not yet supported");

	UINT usage = D3DUSAGE_RENDERTARGET;
	if(mAutoGenMips)
		usage |= D3DUSAGE_AUTOGENMIPMAP;

	// Calculate the layout of the texture atlas:
	// This is a quick hack together.  There are much more sophisticated algorithms for minimizing wasted area.
	// Nieve solution is to pack into n * n textures
	numTexturesWide = (UINT)ceil(sqrt((float)mNumTextures));
	numTexturesHigh = numTexturesWide;
	// Now keep reduceing the number of textures in one direction until we can fit as tight as possible
	while(numTexturesWide * numTexturesHigh >= mNumTextures)
	{	numTexturesHigh -= 1; }
	
		
	// This will make us go one step too far, so add it back
	numTexturesHigh += 1;

	// Now populate the offsets and the scales (so that indexing into each one results in a [0,1 range]
	mTextureOffsets = new D3DXVECTOR2[numTextures];
	mTextureScale = D3DXVECTOR2(1.0f / (float)numTexturesWide, 
								1.0f / (float)numTexturesHigh); // Might need a pixel offset.  Not sure yet.

	UINT curMap = 0;
	for(UINT i = 0; i < numTexturesWide; i ++)
	{
		for(UINT j = 0; j < numTexturesHigh; j ++)
		{
			if(curMap < mNumTextures)
			{ 
				mTextureOffsets[curMap] = D3DXVECTOR2((float)i / (float)numTexturesWide, (float)j / (float)numTexturesHigh); 
				curMap ++;
			}
		}
	}

	defaultViewport.X = 0; defaultViewport.Y = 0; 
	defaultViewport.Width = mUnitWidth*numTexturesWide; defaultViewport.Height = mUnitHeight*numTexturesHigh;
	defaultViewport.MinZ = 0.0f; defaultViewport.MaxZ = 1.0f; 

	HR(D3DXCreateTexture(g_renderer->GetD3DDev(), mUnitWidth*numTexturesWide, mUnitHeight*numTexturesHigh, mMipLevels, usage, mTexFormat, D3DPOOL_DEFAULT, &mTex),
		L"drawableTexAtlas2D::InitDrawableTexAtlas2D() - D3DXCreateTexture failed: ");
	HR(mTex->GetSurfaceLevel(0, &mTopSurf),L"drawableTexAtlas2D::InitDrawableTexAtlas2D() - GetSurfaceLevel failed: ");

	// Add the object to the list of resettable devices for later
	resettable * newObj = new resettableDrawableTexAtlas2D(this);
	g_objectManager->AddResettableResource(newObj);
}

drawableTexAtlas2D::~drawableTexAtlas2D()
{
	if(mTex || mTopSurf)
		OnLostDevice();
	if(mTextureOffsets != NULL) 
	{ 
		if(mNumTextures > 1) 
		{ delete [] mTextureOffsets; mTextureOffsets = NULL; }
		else
		{ delete mTextureOffsets; mTextureOffsets = NULL; }
	}
}

void drawableTexAtlas2D::Release()
{
	if(mTex || mTopSurf)
		OnLostDevice();
}

void drawableTexAtlas2D::OnLostDevice()
{
	ReleaseCOM(mTex);
	ReleaseCOM(mTopSurf);
}

void drawableTexAtlas2D::OnResetDevice()
{
	UINT usage = D3DUSAGE_RENDERTARGET;
	if(mAutoGenMips)
		usage |= D3DUSAGE_AUTOGENMIPMAP;

	HR(D3DXCreateTexture(g_renderer->GetD3DDev(), mUnitWidth*numTexturesWide, mUnitHeight*numTexturesHigh, mMipLevels, usage, mTexFormat, D3DPOOL_DEFAULT, &mTex),
		L"drawableTexAtlas2D::OnResetDevice() - D3DXCreateTexture failed: ");
	HR(mTex->GetSurfaceLevel(0, &mTopSurf),L"drawableTexAtlas2D::OnResetDevice() - GetSurfaceLevel failed: ");
}

void drawableTexAtlas2D::BeginScene(UINT textureIndex)
{
	HR(g_renderer->GetD3DDev()->SetRenderTarget(0,mTopSurf),L"drawableTexAtlas2D::beginScene() - SetRenderTarget Failed: ");

	// note: viewport dimentions are in pixels.  Also, default is (0,0,renderTarget.Width,renderTarget.Height,0.0f,1.0f)
	curViewport.X = (UINT)(mTextureOffsets[textureIndex].x * (float)numTexturesWide) * mUnitWidth;
	curViewport.Y = (UINT)(mTextureOffsets[textureIndex].y * (float)numTexturesHigh) * mUnitHeight;
	curViewport.Width = mUnitWidth;
	curViewport.Height = mUnitHeight;
	curViewport.MinZ = 0.0f;
	curViewport.MaxZ = 1.0f;
	g_renderer->GetD3DDev()->SetViewport(& curViewport);

	HR(g_renderer->GetD3DDev()->BeginScene(),L"drawableTexAtlas2D::beginScene() - BeginScene failed: ");
}

void drawableTexAtlas2D::BeginSceneFullViewport()
{
	HR(g_renderer->GetD3DDev()->SetRenderTarget(0,mTopSurf),L"drawableTexAtlas2D::beginScene() - SetRenderTarget Failed: ");
	HR(g_renderer->GetD3DDev()->BeginScene(),L"drawableTexAtlas2D::beginScene() - BeginScene failed: ");
}

void drawableTexAtlas2D::SetViewport(UINT textureIndex)
{
	// note: viewport dimentions are in pixels.  Also, default is (0,0,renderTarget.Width,renderTarget.Height,0.0f,1.0f)
	curViewport.X = (UINT)(mTextureOffsets[textureIndex].x * (float)numTexturesWide) * mUnitWidth;
	curViewport.Y = (UINT)(mTextureOffsets[textureIndex].y * (float)numTexturesHigh) * mUnitHeight;
	curViewport.Width = mUnitWidth;
	curViewport.Height = mUnitHeight;
	curViewport.MinZ = 0.0f;
	curViewport.MaxZ = 1.0f;
	g_renderer->GetD3DDev()->SetViewport(& curViewport);
}

void drawableTexAtlas2D::EndScene()
{
	HR(g_renderer->GetD3DDev()->EndScene(),L"drawableTexAtlas2D::endScene() - EndScene failed: ");

	// From http://msdn.microsoft.com/en-us/library/bb174455(v=vs.85).aspx:
	// Setting a new render target will cause the viewport to be set to the full size of the new render target.
}

void drawableTexAtlas2D::GenerateTexMipSubLevels()
{
	//if(mMipLevels > 1)
	//	mTex->GenerateMipSubLevels();
}