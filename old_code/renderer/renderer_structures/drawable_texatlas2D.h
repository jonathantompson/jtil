#ifndef drawableTexAtlas2d_h
#define drawableTexAtlas2d_h

#include "dxInclude.h"

class drawableTexAtlas2D
{
public:
	drawableTexAtlas2D();
	~drawableTexAtlas2D();

	inline IDirect3DTexture9* d3dTex() {return mTex;}
	inline IDirect3DSurface9 * d3dSurf() {return mTopSurf;}
	inline UINT UnitWidth() {return mUnitWidth;}
	inline UINT UnitHeight() {return mUnitHeight;}
	inline UINT Width() {return mUnitWidth * numTexturesWide;}
	inline UINT Height() {return mUnitHeight * numTexturesHigh;}
	inline D3DFORMAT GetTexFormat() {return mTexFormat;}
	inline D3DXVECTOR2 * GetTextureScale() {return & mTextureScale;}
	inline D3DXVECTOR2 * GetTextureOffsets() {return mTextureOffsets;}

	void BeginScene(UINT textureIndex);
	void BeginSceneFullViewport();
	void SetViewport(UINT textureIndex);
	void EndScene();

	void OnLostDevice();
	void OnResetDevice();
	void Release();

	void InitDrawableTexAtlas2D(UINT unitWidth, UINT unitHeight, UINT numTexutres, UINT mipLevels, D3DFORMAT texFormat, bool autoGenMips);
	void GenerateTexMipSubLevels();

private:
	// This class is not designed to be copied.
	drawableTexAtlas2D(const drawableTexAtlas2D& rhs);
	drawableTexAtlas2D& operator=(const drawableTexAtlas2D& rhs);

private:
	IDirect3DTexture9*    mTex;
	IDirect3DSurface9 *	  mTopSurf;

	UINT			mUnitWidth;
	UINT			mUnitHeight;
	UINT			mMipLevels;
	D3DFORMAT		mTexFormat;
	bool			mAutoGenMips;
	UINT			mNumTextures;
	D3DXVECTOR2 *	mTextureOffsets;
	D3DXVECTOR2		mTextureScale;
	UINT			numTexturesWide;
	UINT			numTexturesHigh;
	D3DVIEWPORT9	curViewport;
	D3DVIEWPORT9	defaultViewport;
	
};

#endif // DRAWABLE_TEX2D_H