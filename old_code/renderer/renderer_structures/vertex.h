//=============================================================================
// Vertex.h by Frank Luna (C) 2005 All Rights Reserved.
//=============================================================================

#ifndef vertex_h
#define vertex_h

#include	"dxInclude.h"
#include	"main.h"

// Call in constructor and destructor, respectively, of derived application class.
void InitAllVertexDeclarations();
void DestroyAllVertexDeclarations();

//===============================================================
struct VertexPosNormTex
{
	VertexPosNormTex()
		:pos(0.0f, 0.0f, 0.0f),
		normal(0.0f, 0.0f, 0.0f),
		tex0(0.0f, 0.0f){}
	VertexPosNormTex(float x, float y, float z, 
		float nx, float ny, float nz,
		float u, float v):pos(x,y,z), normal(nx,ny,nz), tex0(u,v){}
	VertexPosNormTex(const D3DXVECTOR3& v, const D3DXVECTOR3& n, const D3DXVECTOR2& uv)
		:pos(v),normal(n), tex0(uv){}

	D3DXVECTOR3 pos;
	D3DXVECTOR3 normal;
	D3DXVECTOR2 tex0;

	static IDirect3DVertexDeclaration9* Decl;
};

struct VertexPosTex
{
	VertexPosTex()
		:pos(0.0f, 0.0f, 0.0f),
		tex0(0.0f, 0.0f){}
	VertexPosTex(float x, float y, float z, 
		float u, float v):pos(x,y,z), tex0(u,v){}
	VertexPosTex(const D3DXVECTOR3& v, const D3DXVECTOR2& uv)
		:pos(v), tex0(uv){}

	D3DXVECTOR3 pos;
	D3DXVECTOR2 tex0;

	static IDirect3DVertexDeclaration9* Decl;
};

struct VertexPos // Just position
{
	VertexPos():pos(0.0f, 0.0f, 0.0f){}
	VertexPos(float x, float y, float z):pos(x,y,z){}
	VertexPos(const D3DXVECTOR3& v):pos(v){}

	D3DXVECTOR3 pos;
	static IDirect3DVertexDeclaration9* Decl;
};

struct VertexPosCol // Position and color for wireframe rendering
{
	VertexPosCol():pos(0.0f, 0.0f, 0.0f),col(0x00000000){}
	VertexPosCol(float x, float y, float z, D3DCOLOR c):pos(x,y,z),col(c){}
	VertexPosCol(const D3DXVECTOR3& v, const D3DCOLOR& c):pos(v),col(c){}

	D3DXVECTOR3 pos;
	D3DCOLOR	col;
	static IDirect3DVertexDeclaration9* Decl;
};

struct VertexPosNormCol // Position and color for wireframe rendering
{
	VertexPosNormCol():pos(0.0f, 0.0f, 0.0f),norm(1.0f,0.0f,0.0f),col(0x00000000){}
	VertexPosNormCol(float x, float y, float z, float nx, float ny, float nz, D3DCOLOR c):pos(x,y,z),norm(nx,ny,nz),col(c){}
	VertexPosNormCol(const D3DXVECTOR3& v, const D3DXVECTOR3& n, const D3DCOLOR& c):pos(v),norm(n),col(c){}

	D3DXVECTOR3 pos;
	D3DXVECTOR3 norm;
	D3DCOLOR	col;
	static IDirect3DVertexDeclaration9* Decl;
};

#endif