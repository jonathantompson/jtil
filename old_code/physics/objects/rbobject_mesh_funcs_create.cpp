/*************************************************************
**					Rigid Body Object						**
**			-> Store object states, Summer 2009				**
*************************************************************/
// File:		rbobjectMesh_FuncsCreate.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Create functions for rbobjectMesh class -> simplifies rbobjectMesh.cpp for compilation with other projects

#include	"physics\objects\rbobjectMesh.h"
#include	"objectManager\objectManager.h"
#include	"physics\objects\rbobjectMeshData.h"
#include	"utils_and_misc_classes\stringUtil.h"
#include	"utils_and_misc_classes\util.h"
#include	"utils_and_misc_classes\math\math_funcs.h"
#include	"hud\hud.h"
#include	"utils_and_misc_classes\math\int3.h"
#include	"utils_and_misc_classes\math\double3.h"
#include	"physics\obbtree_related\obbtree.h"
#include	"app.h"
#include	"main.h"
#include	"UI\UI.h"
#include	"UI\varNames.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		LoadData												*/
/* Description:	Load the mesh and initlize physics data from an xfile	*/
/************************************************************************/
void rbobjectMesh::LoadData(LPCWSTR Filename, std::wstring format, IDirect3DDevice9 *pDevice, bool moveOriginToCOM)
{
	// Check that Mesh Data doesn't already exist
	std::wstring instanceName = Filename;
	meshData = g_objectManager->GetMeshData(&instanceName);
	if(meshData == NULL) // No data yet exists for this instance type
	{
		meshData = new rbobjectMeshData();
		g_objectManager->AddMeshData(&instanceName, meshData);

		// LOAD IN THE FILE DATA
		if(format == std::wstring(L"x"))
		{
			std::wstring xFilename = Filename; xFilename.append(L".x");
			meshData->LoadXFile(xFilename.c_str());
		}
		else
			throw std::runtime_error("rbobjectMesh::LoadData() - Unrecognised file format.  Valid value is only 'x'.");
		
		ProcessMeshData(Filename, moveOriginToCOM);
	}
	else
	{
		ComputeMassProperties(); // Just get Itensor and bounding box data
	}

	// Add verticies and triangles to statistics
	g_objectManager->GetHud()->addVertices(meshData->pMesh->GetNumVertices());
	g_objectManager->GetHud()->addTriangles(meshData->pMesh->GetNumFaces());

}

/************************************************************************/
/* Name:		LoadDataFromBuffers										*/
/* Description:	Load the mesh and initlize physics data from vertex		*/
/*				and index data.											*/
/************************************************************************/
void rbobjectMesh::LoadDataFromBuffers(LPCWSTR meshName, D3DXVECTOR3 * pos, D3DXVECTOR3 * norm, DWORD numVert, DWORD * indBuf, DWORD numInd, Mtrl * mtrl, bool moveOriginToCOM)
{
	// Check that Mesh Data doesn't already exist
	std::wstring instanceName = meshName;
	meshData = g_objectManager->GetMeshData(&instanceName);
	if(meshData == NULL) // No data yet exists for this instance type
	{
		meshData = new rbobjectMeshData();
		g_objectManager->AddMeshData(&instanceName, meshData);

		// CREATE A NEW MESH FROM THE INPUT BUFFERS
		meshData->CreateMeshFromBuffers(pos, norm, numVert, indBuf, numInd, mtrl);
		
		ProcessMeshData(meshName, moveOriginToCOM);
	}
	else
	{
		ComputeMassProperties(); // Just get Itensor and bounding box data
	}

	// Add verticies and triangles to statistics
	g_objectManager->GetHud()->addVertices(meshData->pMesh->GetNumVertices());
	g_objectManager->GetHud()->addTriangles(meshData->pMesh->GetNumFaces());

}

/************************************************************************/
/* Name:		ProcessMeshData											*/
/* Description:	Perform post processing on the new mesh					*/
/************************************************************************/
void rbobjectMesh::ProcessMeshData(LPCWSTR Filename, bool moveOriginToCOM)
{
	// COMPUTE MASS PROPERTIES
	// Normalize();
	GetVertexIndexData(); // Not centered about COM
	if(moveOriginToCOM)
	{
		ComputeMassPropertiesAndCenter();
		GetVertexIndexData(); // Centered about COM --> Do it again
	}
	else
		ComputeMassProperties();

	// Check that obbtree data doesn't already exist
	obbtree * tree = meshData->GetOBBTree();
	if(tree == NULL && !g_UI->GetSetting<bool>(& var_forceNoOBB)) // Tree doesn't exist
	{
		tree = new obbtree(meshData->numFaces, meshData->numVerticies);
		tree->InitOBBTree(this, Filename); // CREATE THE OBB TREE FROM DISK OR FROM SCRATCH
		// Now set the mesh data tree
		meshData->SetOBBTree(tree);
	}
}

/************************************************************************/
/* Name:		ComputeMassPropertiesAndCenter							*/
/* Description:	Calculate center of mass and inertia tensor matrix. I	*/
/*		use code at: http://www.melax.com/volint.html for simplicity.	*/
/*		ASSUMES THAT GetVertexIndexData HAS BEEN CALLED BEFORE			*/
/************************************************************************/
void rbobjectMesh::ComputeMassPropertiesAndCenter()
{
	// CONVERT VERTEX BUFFER TO TYPE USED BY S. MELAX SO WE CAN USE HIS CODE - O(n): but done on startup
	double3 * vert = new double3[meshData->numVerticies];
	for(DWORD i = 0; i < meshData->numVerticies; i ++)
	{
		vert[i].x =  meshData->vertexBuffer[i].x; 
		vert[i].y =  meshData->vertexBuffer[i].y; 
		vert[i].z =  meshData->vertexBuffer[i].z; 
	}

	// CONVERT INDEX BUFFER TO TYPE USED BY S. MELAX SO WE CAN USE HIS CODE - O(n): but done on startup
	int3 * ind = new int3[meshData->numFaces];
	for(DWORD i = 0; i < meshData->numFaces; i ++)
	{
		ind[i].x =  meshData->indexBuffer[3*i+0]; 
		ind[i].y =  meshData->indexBuffer[3*i+1];
		ind[i].z =  meshData->indexBuffer[3*i+2];
	}	

	// CALCULATE THE CENTER OF MASS
	D3DXVECTOR3 COM; 
	CenterOfMass(& COM, meshData->vertexBuffer, meshData->indexBuffer, meshData->numFaces);
	// Now shift all verticies by this amount
	ZeroCOM(vert, & COM);

	// Make sure COM now is zero!  If not try again
	CenterOfMass(& COM, meshData->vertexBuffer, meshData->indexBuffer, meshData->numFaces);
	if(abs(COM.x) > 0.01f || abs(COM.y) > 0.01f || abs(COM.z) > 0.01f)
	{
		// Now shift all verticies by this amount
		ZeroCOM(vert, & COM);
		CenterOfMass(& COM, meshData->vertexBuffer, meshData->indexBuffer, meshData->numFaces);
		if(abs(COM.x) > 0.01f || abs(COM.y) > 0.01f || abs(COM.z) > 0.01f)
			throw std::runtime_error("rbobjectMesh::ComputeMassPropertiesAndCenter() - Couldn't initialize object coordinate origion to center of mass");
	}

	// GET BOUNDING BOX
	BYTE * pBytes = 0;
	HR(meshData->pMesh->LockVertexBuffer(D3DLOCK_READONLY, (LPVOID*)&pBytes), L"rbobjectMesh::ComputeMassPropertiesAndCenter() - Failed to lock vertex buffer: ");
	D3DXVECTOR3 minBounds,maxBounds;
	D3DXComputeBoundingBox((D3DXVECTOR3*)pBytes, meshData->pMesh->GetNumVertices(), D3DXGetFVFVertexSize(meshData->pMesh->GetFVF()), &minBounds, &maxBounds);
	HR(meshData->pMesh->UnlockVertexBuffer(), L"rbobjectMesh::ComputeMassPropertiesAndCenter() - Failed to unlock vertex buffer: ");

	// We have min and max values, use these to get the 8 corners of the bounding box in object space
	m_objectBounds[0] = D3DXVECTOR3( minBounds.x, minBounds.y, minBounds.z ); // xyz
	m_objectBounds[1] = D3DXVECTOR3( maxBounds.x, minBounds.y, minBounds.z ); // Xyz
	m_objectBounds[2] = D3DXVECTOR3( minBounds.x, maxBounds.y, minBounds.z ); // xYz
	m_objectBounds[3] = D3DXVECTOR3( maxBounds.x, maxBounds.y, minBounds.z ); // XYz
	m_objectBounds[4] = D3DXVECTOR3( minBounds.x, minBounds.y, maxBounds.z ); // xyZ
	m_objectBounds[5] = D3DXVECTOR3( maxBounds.x, minBounds.y, maxBounds.z ); // XyZ
	m_objectBounds[6] = D3DXVECTOR3( minBounds.x, maxBounds.y, maxBounds.z ); // xYZ
	m_objectBounds[7] = D3DXVECTOR3( maxBounds.x, maxBounds.y, maxBounds.z ); // XYZ

	GetItensor(vert, ind, meshData->numFaces, &Ibody, &Ibody_inv);
/*
	// Test GetItensor with a cube (from <-1,-1,-1> --> <+1,+1,+1>)
	double3 * vertTest = new double3[8];
	int3 * indTest = new int3[12];
	vertTest[0].x = -2.25; vertTest[0].y = -2.25; vertTest[0].z = -2.25; 
	vertTest[1].x = +2.25; vertTest[1].y = -2.25; vertTest[1].z = -2.25;
	vertTest[2].x = +2.25; vertTest[2].y = +2.25; vertTest[2].z = -2.25;
	vertTest[3].x = -2.25; vertTest[3].y = +2.25; vertTest[3].z = -2.25;
	vertTest[4].x = -2.25; vertTest[4].y = -2.25; vertTest[4].z = +2.25;
	vertTest[5].x = +2.25; vertTest[5].y = -2.25; vertTest[5].z = +2.25;
	vertTest[6].x = +2.25; vertTest[6].y = +2.25; vertTest[6].z = +2.25;
	vertTest[7].x = -2.25; vertTest[7].y = +2.25; vertTest[7].z = +2.25;
	indTest[0].x = 0; indTest[0].y = 1; indTest[0].z = 3;
	indTest[1].x = 3; indTest[1].y = 1; indTest[1].z = 2;
	indTest[2].x = 0; indTest[2].y = 4; indTest[2].z = 1;
	indTest[3].x = 4; indTest[3].y = 5; indTest[3].z = 1;
	indTest[4].x = 1; indTest[4].y = 5; indTest[4].z = 2;
	indTest[5].x = 2; indTest[5].y = 5; indTest[5].z = 6;
	indTest[6].x = 6; indTest[6].y = 3; indTest[6].z = 2;
	indTest[7].x = 6; indTest[7].y = 7; indTest[7].z = 3;
	indTest[8].x = 7; indTest[8].y = 4; indTest[8].z = 3;
	indTest[9].x = 4; indTest[9].y = 0; indTest[9].z = 3;
	indTest[10].x = 7; indTest[10].y = 6; indTest[10].z = 5;
	indTest[11].x = 7; indTest[11].y = 5; indTest[11].z = 4;
	double3 COMTest = CenterOfMass(vertTest, indTest, 12);
	D3DXMATRIXA16 IbodyTest; D3DXMATRIXA16 IbodyTest_inv; 
	GetItensor(vertTest, indTest, 12, &IbodyTest, &IbodyTest_inv);
	// COM should be 0,0,0 --> Intertia Tensor should be diagonal with entries for a AxAxA box: 2*A^2/12
*/

	UpdateBoundingBox(); // Works out axis-aligned BB values --> Makes culling faster.

	// Clean up since we're done
	delete [] vert;
	delete [] ind;
}

/************************************************************************/
/* Name:		ZeroCOM													*/
/* Description:	Shift verticies so that COM is at 0,0,0					*/
/************************************************************************/
void rbobjectMesh::ZeroCOM(double3 * vert, D3DXVECTOR3 * COM)
{
	BYTE *pBytes = NULL;
	HR(meshData->pMesh->LockVertexBuffer(D3DLOCK_READONLY, reinterpret_cast<LPVOID*>(&pBytes)), L"rbobjectMesh::ZeroCOM() - Failed to lock vertex buffer: ");
	D3DXVECTOR3 *pVector = 0;
	DWORD dwVertexSize = D3DXGetFVFVertexSize(meshData->pMesh->GetFVF());
	for(DWORD i = 0; i < meshData->numVerticies; i ++)
	{
		pVector = reinterpret_cast<D3DXVECTOR3*>(pBytes);
		pVector->x -= COM->x; pVector->y -= COM->y; pVector->z -= COM->z;
		// Also shift vertex buffer (so it is up to date)
		meshData->vertexBuffer[i].x -= COM->x; meshData->vertexBuffer[i].y -= COM->y; meshData->vertexBuffer[i].z -= COM->z; 
		vert[i].x -= (double)COM->x; vert[i].y -= (double)COM->y; vert[i].z -= (double)COM->z;
		pBytes += dwVertexSize;
	}
	HR(meshData->pMesh->UnlockVertexBuffer(), L"rbobjectMesh::ZeroCOM() - Failed to unlock vertex buffer: ");
}

/************************************************************************/
/* Name:		ComputeMassPropertiesAndCenter							*/
/* Description:	Calculate center of mass and inertia tensor matrix. I	*/
/*		use code at: http://www.melax.com/volint.html for simplicity.	*/
/*		ASSUMES THAT GetVertexIndexData HAS BEEN CALLED BEFORE			*/
/************************************************************************/
void rbobjectMesh::ComputeMassProperties()
{
	// CONVERT VERTEX BUFFER TO TYPE USED BY S. MELAX SO WE CAN USE HIS CODE - O(n): but done on startup
	// S. MELAX code used for center of mass and inertia tensor calculations.
	double3 * vert = new double3[meshData->numVerticies];
	for(DWORD i = 0; i < meshData->numVerticies; i ++)
	{
		vert[i].x =  meshData->vertexBuffer[i].x; 
		vert[i].y =  meshData->vertexBuffer[i].y; 
		vert[i].z =  meshData->vertexBuffer[i].z; 
	}

	// CONVERT INDEX BUFFER TO TYPE USED BY S. MELAX SO WE CAN USE HIS CODE - O(n): but done on startup
	int3 * ind = new int3[meshData->numFaces];
	for(DWORD i = 0; i < meshData->numFaces; i ++)
	{
		ind[i].x =  meshData->indexBuffer[3*i+0]; 
		ind[i].y =  meshData->indexBuffer[3*i+1];
		ind[i].z =  meshData->indexBuffer[3*i+2];
	}	

	D3DXVECTOR3 COM; 
	CenterOfMass(& COM, meshData->vertexBuffer, meshData->indexBuffer, meshData->numFaces);

	// GET BOUNDING BOX
	BYTE *pBytes = NULL;
	HR(meshData->pMesh->LockVertexBuffer(D3DLOCK_READONLY, (LPVOID*)&pBytes), L"rbobjectMesh::ComputeMassProperties() - Failed to lock vertex buffer: ");
	D3DXVECTOR3 minBounds,maxBounds;
	D3DXComputeBoundingBox((D3DXVECTOR3*)pBytes, meshData->pMesh->GetNumVertices(), D3DXGetFVFVertexSize(meshData->pMesh->GetFVF()), &minBounds, &maxBounds);
	HR(meshData->pMesh->UnlockVertexBuffer(), L"rbobjectMesh::ComputeMassProperties() - Failed to unlock vertex buffer: ");

	// We have min and max values, use these to get the 8 corners of the bounding box in object space
	m_objectBounds[0] = D3DXVECTOR3( minBounds.x, minBounds.y, minBounds.z ); // xyz
	m_objectBounds[1] = D3DXVECTOR3( maxBounds.x, minBounds.y, minBounds.z ); // Xyz
	m_objectBounds[2] = D3DXVECTOR3( minBounds.x, maxBounds.y, minBounds.z ); // xYz
	m_objectBounds[3] = D3DXVECTOR3( maxBounds.x, maxBounds.y, minBounds.z ); // XYz
	m_objectBounds[4] = D3DXVECTOR3( minBounds.x, minBounds.y, maxBounds.z ); // xyZ
	m_objectBounds[5] = D3DXVECTOR3( maxBounds.x, minBounds.y, maxBounds.z ); // XyZ
	m_objectBounds[6] = D3DXVECTOR3( minBounds.x, maxBounds.y, maxBounds.z ); // xYZ
	m_objectBounds[7] = D3DXVECTOR3( maxBounds.x, maxBounds.y, maxBounds.z ); // XYZ

	GetItensor(vert, ind, meshData->numFaces, &Ibody, &Ibody_inv);

	UpdateBoundingBox(); // Works out axis-aligned BB values --> Makes culling faster.

	// Clean up since we're done
	delete [] vert;
	delete [] ind;
}
/************************************************************************/
/* Name:		GetVertexIndexData										*/
/* Description:	Copy vertex and index buffer into local storage.		*/
/************************************************************************/
// Useful guide for acessing mesh data: http://www.mvps.org/directx/articles/d3dxmesh.htm
void rbobjectMesh::GetVertexIndexData()
{
    meshData->numVerticies = meshData->pMesh->GetNumVertices();
	meshData->numFaces = meshData->pMesh->GetNumFaces();
    //DWORD dwFVF = meshData->pMesh->GetFVF();
    DWORD dwVertexSize = D3DXGetFVFVertexSize(meshData->pMesh->GetFVF());
	int meshOptions = meshData->pMesh->GetOptions();
	bool indices32Bit = (meshOptions & D3DXMESH_32BIT) == D3DXMESH_32BIT; // Do a bit mask

	// Iterate through and copy mesh's vertex buffer into local data structure
	if(meshData->vertexBuffer != NULL)
		delete [] meshData->vertexBuffer; // Number of faces may have changed
	meshData->vertexBuffer = new D3DXVECTOR3[meshData->numVerticies];
	BYTE *pBytes = NULL;
	HR(meshData->pMesh->LockVertexBuffer(D3DLOCK_READONLY, reinterpret_cast<LPVOID*>(&pBytes)),L"rbobjectMesh::GetVertexIndexData() - Failed to lock vertex buffer: ");
    D3DXVECTOR3 *pVector = 0;
    for (DWORD i = 0; i < meshData->numVerticies; ++i)
    {
        pVector = reinterpret_cast<D3DXVECTOR3*>(pBytes);
		meshData->vertexBuffer[i].x = pVector->x;
		meshData->vertexBuffer[i].y = pVector->y;
		meshData->vertexBuffer[i].z = pVector->z;
        pBytes += dwVertexSize;
    }
    HR(meshData->pMesh->UnlockVertexBuffer(),L"rbobjectMesh::GetVertexIndexData() - Failed to unlock vertex buffer: ");

	// Iterate through and copy mesh's index buffer into local data structure
	if(meshData->indexBuffer != NULL)
		delete [] meshData->indexBuffer; // Number of faces may have changed
	meshData->indexBuffer = new int[meshData->numFaces*3]; 
	pBytes = NULL;
	HR(meshData->pMesh->LockIndexBuffer(D3DLOCK_READONLY, reinterpret_cast<LPVOID*>(&pBytes)),L"rbobjectMesh::GetVertexIndexData() - Failed to lock index buffer: ");
	if(indices32Bit)
	{
		DWORD *pIndex = 0;
		for (DWORD i = 0; i < (meshData->numFaces*3); ++i)
		{
			pIndex = reinterpret_cast<DWORD*>(pBytes);
			meshData->indexBuffer[i] = (int)*pIndex;
			pBytes += 4; // 4 bytes (32 bits)
		}
	}
	else
	{
		WORD *pIndex = 0;
		for (DWORD i = 0; i < (meshData->numFaces*3); ++i)
		{
			pIndex = reinterpret_cast<WORD*>(pBytes);
			meshData->indexBuffer[i] = (int)*pIndex;
			pBytes += 2; // 2 bytes (16 bits)
		}
	}
    HR(meshData->pMesh->UnlockIndexBuffer(),L"rbobjectMesh::GetVertexIndexData() - Failed to unlock index buffer: ");
}