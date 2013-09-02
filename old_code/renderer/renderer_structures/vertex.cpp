//=============================================================================
// Vertex.h by Frank Luna (C) 2005 All Rights Reserved.
//=============================================================================

#include "renderer\renderer_structures\vertex.h"
#include "app.h"
#include "utils_and_misc_classes\stringUtil.h"
#include "renderer\renderer.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

// Initialize static variables.
IDirect3DVertexDeclaration9* VertexPosNormTex::Decl = 0;
IDirect3DVertexDeclaration9* VertexPosTex::Decl = 0;
IDirect3DVertexDeclaration9* VertexPos::Decl = 0;
IDirect3DVertexDeclaration9* VertexPosCol::Decl = 0;
IDirect3DVertexDeclaration9* VertexPosNormCol::Decl = 0;

void InitAllVertexDeclarations()
{

	//===============================================================
	// VertexPosNormTex

	D3DVERTEXELEMENT9 VertexPosNormTexElements[] = 
	{
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
		{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};	
	HR(g_renderer->GetD3DDev()->CreateVertexDeclaration(VertexPosNormTexElements, &VertexPosNormTex::Decl),
		L"InitAllVertexDeclarations() - CreateVertexDeclaration for PosNormTex elements failed: ");

	//===============================================================
	// VertexPosTex

	D3DVERTEXELEMENT9 VertexPosTexElements[] = 
	{
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END()
	};	
	HR(g_renderer->GetD3DDev()->CreateVertexDeclaration(VertexPosTexElements, &VertexPosTex::Decl),
		L"InitAllVertexDeclarations() - CreateVertexDeclaration for PosTex elements failed: ");

	//===============================================================
	// VertexPos

	D3DVERTEXELEMENT9 VertexPosElements[] = 
	{
		{0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		D3DDECL_END()
	};	
	HR(g_renderer->GetD3DDev()->CreateVertexDeclaration(VertexPosElements, &VertexPos::Decl),
		L"InitAllVertexDeclarations() - CreateVertexDeclaration for Pos elements failed: ");

	//===============================================================
	// VertexPosCol

	D3DVERTEXELEMENT9 VertexPosColElements[] =
	{
		{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
		D3DDECL_END()
	};
	HR(g_renderer->GetD3DDev()->CreateVertexDeclaration(VertexPosColElements, &VertexPosCol::Decl),
		L"InitAllVertexDeclarations() - CreateVertexDeclaration for PosCol elements failed: ");

	//===============================================================
	// VertexPosColNorm

	D3DVERTEXELEMENT9 VertexPosNormColElements[] =
	{
		{0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
		{0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
		D3DDECL_END()
	};
	HR(g_renderer->GetD3DDev()->CreateVertexDeclaration(VertexPosNormColElements, &VertexPosNormCol::Decl),
		L"InitAllVertexDeclarations() - CreateVertexDeclaration for PosColNorm elements failed: ");

}

void DestroyAllVertexDeclarations()
{
	ReleaseCOM(VertexPosNormTex::Decl);
	ReleaseCOM(VertexPosTex::Decl);
	ReleaseCOM(VertexPos::Decl);
	ReleaseCOM(VertexPosCol::Decl);
	ReleaseCOM(VertexPosNormCol::Decl);
}