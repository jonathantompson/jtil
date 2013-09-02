#ifndef drawable_tex2d_h
#define drawable_tex2d_h

#include "dxInclude.h"

class drawableTex2D
{
public:
	drawableTex2D();
	~drawableTex2D();

	inline IDirect3DTexture9* d3dTex() {return mTex;}
	inline IDirect3DSurface9 * d3dSurf() {return mTopSurf;}
	inline UINT Width() {return mWidth;}
	inline UINT Height() {return mHeight;}
	inline D3DFORMAT GetTexFormat() {return mTexFormat;}

	void BeginScene();
	void EndScene();

	void OnLostDevice();
	void OnResetDevice();
	void Release();

	void InitDrawableTex2D(UINT width, UINT height, UINT mipLevels, D3DFORMAT texFormat, bool autoGenMips);
	void GenerateTexMipSubLevels();
	void GetRenderTargetDataPointSample(D3DXVECTOR4 * retVal, D3DXVECTOR2 * texCoord);

private:
	// This class is not designed to be copied.
	drawableTex2D(const drawableTex2D& rhs);
	drawableTex2D& operator=(const drawableTex2D& rhs);

private:
	IDirect3DTexture9*    mTex;
	IDirect3DSurface9 *	  mTopSurf;

	// These are kept in case we want to read back data from the
	// graphics device after rendering (for debug).  Normally they are NULL
	IDirect3DTexture9 *   mTexSystemMem;
	IDirect3DSurface9 *	  mTopSurfSystemMem;

	UINT         mWidth;
	UINT         mHeight;
	UINT         mMipLevels;
	D3DFORMAT    mTexFormat;
	bool         mAutoGenMips;
};

#endif // DRAWABLE_TEX2D_H