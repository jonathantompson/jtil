/*************************************************************
**					Rigid Body Objects						**
**			-> Store object states, Summer 2009				**
**************************************************************
File:		rbobjectMesh.h
Author:		Jonathan Tompson
e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com
*/

#ifndef rbobjectMesh_h
#define rbobjectMesh_h

#include	"physics\objects\rbobject.h"

class rbobject;
class rbobjectMeshData;
struct Mtrl;

#ifndef DATA_ALIGNMENT
#define DATA_ALIGNMENT 16
#endif

#ifndef _MSC_VER
#pragma pack(push)
#pragma pack(16)
#endif

// ****************************
// Rigid body object Mesh class
// ****************************
__declspec(align(DATA_ALIGNMENT)) class rbobjectMesh : public rbobject
{
public:

	friend class rbobjectMeshData;

									rbobjectMesh();
									~rbobjectMesh();

	void							LoadData(LPCTSTR Filename, std::wstring format, IDirect3DDevice9 *pDevice, bool moveOriginToCOM);
	void							LoadDataFromBuffers(LPCWSTR meshName, D3DXVECTOR3 * pos, D3DXVECTOR3 * norm, DWORD numVert, DWORD * indBuf, DWORD numInd, Mtrl * mtrl, bool moveOriginToCOM);
	void							RenderSM();
	void							RenderGBuffer(UINT pass);
	void							RenderGBufferWireframe();
	void							GetVertexIndexData();

	//			Start 1st 16 byte block (sizeof(void *) + 3 * sizeof(UINT) = 16)
	rbobjectMeshData *				meshData;		// Pointer to static mesh data (might be sharded accross different instances of the same mesh)
	UINT							padding_0[3];


	inline virtual RBOBJECT_TYPE GetType() { return T_RBOBJECTMESH; }

private:

	void							ComputeMassPropertiesAndCenter(); // Computer bounding box and Itensor AND shift verticies to COM
	void							ComputeMassProperties(); // Just compute bounding box and Itensor
	void							ZeroCOM(double3 * vert, D3DXVECTOR3 * COM);
	void							ProcessMeshData(LPCWSTR Filename, bool moveOriginToCOM);
};

#ifndef _MSC_VER
#pragma pack(pop)
#endif

#endif