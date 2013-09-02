// File:		rbobjectMeshData.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is header obbject to store OBB tree data, pointers to a graphics buffers for each Mesh.
// Multiple instances of the same mesh will share the same MeshData instance to avoid redundant memory use

#ifndef rbobjectMeshData_h
#define rbobjectMeshData_h

#include	"dxInclude.h"
#include	"utils_and_misc_classes\data_structures\vec_ptrs.h"
#include	<string>

#include	"renderer\renderer_structures\d3dFormats.h"

class obbtree;
class texSamplerState;

class rbobjectMeshData 
{
public:
	
	friend class rbobjectMesh;
	friend class renderer;

									rbobjectMeshData();
									~rbobjectMeshData();

	// Selector and modifier functions
	inline obbtree *				GetOBBTree() { return OBBTree; }
	inline void						SetOBBTree(obbtree * inTree) { OBBTree = inTree; }
	inline DWORD					GetNumVerticies() { return numVerticies; }
	inline DWORD					GetNumFaces(){ return numFaces; }
	inline D3DXVECTOR3 *			GetVertexBuffer(){ return vertexBuffer; }
	inline int *					GetIndexBuffer(){ return indexBuffer; }

	void							LoadXFile(const std::wstring& filename);
	void							CreateMeshFromBuffers(D3DXVECTOR3 * pos, D3DXVECTOR3 * norm, DWORD numVert, DWORD * indBuf, DWORD numInd, Mtrl * mtrl);

	void							UpdateTextureSamplers();
private:
	obbtree	*						OBBTree;

	// Raw vertex and index buffer data.  Mainly used to build the obbtree.  Keep around if needed.
	DWORD							numFaces;
	DWORD							numVerticies;
	D3DXVECTOR3 *					vertexBuffer;		// IN OBJECT COORDINATES! For OBB calculation
	int *							indexBuffer;		// IN OBJECT COORDINATES!

	ID3DXMesh *						pMesh;
    IDirect3DVertexDeclaration9 *	pDeclaration;
    vec_ptrs<IDirect3DTexture9> *	textures;		// Vector array of textures
	vec_ptrs<texSamplerState> *		textureFilters;	// Vector array of texture filter states
    vec_ptrs<Mtrl> *				materials;		// Vector array of materials
	IDirect3DVertexDeclaration9*	Decl;			// Vertex declaration
	
};

#endif