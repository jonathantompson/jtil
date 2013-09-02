// File:		rbobjectMeshData.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is header obbject to store OBB tree data, pointers to a graphics buffers for each Mesh.
// Multiple instances of the same mesh will share the same MeshData instance to avoid redundant memory use

#include	"physics\objects\rbobjectMeshData.h"
#include	"physics\obbtree_related\obbtree.h"
#include	"utils_and_misc_classes\stringUtil.h"
#include	"renderer\renderer_structures\texSamplerState.h"
#include	"renderer\renderer.h"
#include	"main.h"
#include	"renderer\renderer_structures\vertex.h"
#include	"UI\UI.h"
#include	"UI\varNames.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#define NULL 0

/************************************************************************/
/* Name:		rbobjectMeshData										*/
/* Description: Default Constructor										*/
/************************************************************************/
rbobjectMeshData::rbobjectMeshData()
{
	OBBTree = NULL;
	pMesh = NULL; pDeclaration = NULL;
	vertexBuffer = NULL; indexBuffer = NULL; numFaces = 0;
	textures =  new vec_ptrs<IDirect3DTexture9>(); 
	materials = new vec_ptrs<Mtrl>();
	textureFilters = new vec_ptrs<texSamplerState>();
}
/************************************************************************/
/* Name:		~rbobjectMeshData										*/
/* Description: Default Destructor										*/
/************************************************************************/
rbobjectMeshData::~rbobjectMeshData()
{
	if(OBBTree) { delete OBBTree; OBBTree = NULL; }

	// Release all mesh components
    ReleaseCOM(pMesh);
	ReleaseCOM(pDeclaration);
	if(vertexBuffer) { delete [] vertexBuffer; vertexBuffer = NULL; }
	if(indexBuffer) { delete [] indexBuffer; indexBuffer = NULL; }
	// Textures are released by resettable list seperately, just delete the allocated array of pointers
	if(textures) { textures->FreeArray(); delete textures; textures = NULL; }
	if(materials) { delete materials; materials = NULL; }
	if(textureFilters) { delete textureFilters; textureFilters = NULL; }
}

/************************************************************************/
/* Name:		LoadXFile												*/
/* Description: Load in the X file data.								*/
/************************************************************************/
void rbobjectMeshData::LoadXFile(const std::wstring& filename)
{
	// Step 1: Load the .x file from file into a system memory mesh.
	ID3DXMesh* meshSys      = 0;
	ID3DXBuffer* adjBuffer  = 0;
	ID3DXBuffer* mtrlBuffer = 0;
	DWORD nummaterials      = 0;
	
	// Changed to managed mesh.  Things might be messing up.
	HR(D3DXLoadMeshFromX(filename.c_str(), D3DXMESH_MANAGED , g_renderer->GetD3DDev(), 0, &mtrlBuffer, 0, &nummaterials, &meshSys),
		L"rbobjectMeshData::LoadXFile() - D3DXLoadMeshFromX failed! FileName: '" + filename + L"' ");

	// Step 2: Find out if the mesh already has normal info?
	D3DVERTEXELEMENT9 elems[MAX_FVF_DECL_SIZE];
	HR(meshSys->GetDeclaration(elems),L"rbobjectMeshData::LoadXFile() - GetDeclaration failed: ");
	
	bool hasNormals = false;
	// D3DVERTEXELEMENT9 term = D3DDECL_END();
	for(int i = 0; i < MAX_FVF_DECL_SIZE; ++i)
	{
		// Did we reach D3DDECL_END() {0xFF,0,D3DDECLTYPE_UNUSED, 0,0,0}?
		if(elems[i].Stream == 0xff )
			break;

		if( elems[i].Type == D3DDECLTYPE_FLOAT3 &&
			elems[i].Usage == D3DDECLUSAGE_NORMAL &&
			elems[i].UsageIndex == 0 )
		{
			hasNormals = true;
			break;
		}
	}

	// Step 3: Change vertex format to VertexPNT.
	D3DVERTEXELEMENT9 elements[64];
	UINT numElements = 0;
	VertexPosNormTex::Decl->GetDeclaration(elements, &numElements);

	// Are both declarations exactly the same?
	int j = 0; bool same = true;
	do
	{
		if( elems[j].Stream		!= elements[j].Stream ||
			elems[j].Offset		!= elements[j].Offset ||
			elems[j].Type		!= elements[j].Type ||
			elems[j].Method		!= elements[j].Method ||
			elems[j].Usage		!= elements[j].Usage ||
			elems[j].UsageIndex != elements[j].UsageIndex)
		{ same = false; break; }
		j += 1;
		if(j == MAX_FVF_DECL_SIZE)
			break;
	}
	while (elems[j].Stream != 0xff && elements[j].Stream != 0xff);

	if( ! same )
	{
		ID3DXMesh* temp = 0;
		HR(meshSys->CloneMesh(D3DXMESH_MANAGED, elements, g_renderer->GetD3DDev(), &temp),L"rbobjectMeshData::LoadXFile() - CloneMesh failed: ");
		ReleaseCOM(meshSys);
		meshSys = temp;
	}

	// Step 4: If the mesh did not have normals, generate them.
	if( hasNormals == false)
		HR(D3DXComputeNormals(meshSys, 0),L"rbobjectMeshData::LoadXFile() - D3DXComputeNormals failed: ");

	// Step 5: Optimize the mesh.
	HR(D3DXCreateBuffer( meshSys->GetNumFaces() * 3 * sizeof(DWORD), &adjBuffer ), L"rbobjectMeshData::LoadXFile() - Failed to create adjacency buffer: ");
	HR(meshSys->GenerateAdjacency(0.0f, (DWORD*)adjBuffer->GetBufferPointer()), L"rbobjectMeshData::LoadXFile() - Failed to create adjacency info: ");

	HR(meshSys->Optimize(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE, (DWORD*)adjBuffer->GetBufferPointer(), 0, 0, 0, &pMesh),
		L"LoadXFile: Optimize failed: ");
	ReleaseCOM(meshSys); // Done w/ system mesh.
	ReleaseCOM(adjBuffer); // Done with buffer.
	Mtrl * tempMtrl = NULL;

	// Step 6: Extract the materials and load the textures.
	if( mtrlBuffer != 0 && nummaterials != 0 )
	{
		D3DXMATERIAL* d3dxmaterials = (D3DXMATERIAL*)mtrlBuffer->GetBufferPointer();

		// We know the size of the internal arrays, so set the capacity to avoid multiple reallocs
		// Also, THIS IS REQUIRED since resettableTexture needs a double pointer to stay valid throughout runtime
		// If the array of pointers is reallocated, resettableTexture double pointers will be invalidated.
		materials->SetCapacity(nummaterials);
		textures->SetCapacity(nummaterials);
		textureFilters->SetCapacity(nummaterials);

		for(DWORD i = 0; i < nummaterials; ++i)
		{
			// Save the ith material.  Note that the MatD3D property does not have an ambient
			// value set when its loaded, so just set it to the diffuse value.
			tempMtrl = new Mtrl;
			tempMtrl->diffuse   = d3dxmaterials[i].MatD3D.Diffuse;
			// Estimate greyscale specular intensity (that is, specular intensity doesn't have color in our model)
			D3DCOLORVALUE spec = d3dxmaterials[i].MatD3D.Specular;
			float intensity = (spec.r + spec.g + spec.b)/ 3.0f; // Calculate it as the average
			tempMtrl->specIntensity = intensity;
			tempMtrl->specPower = d3dxmaterials[i].MatD3D.Power;
			materials->Add( tempMtrl );

			// Check if the ith material has an associative texture
			if( d3dxmaterials[i].pTextureFilename != 0 )
			{
				// Yes, load the texture for the ith subset - Make the format the same as the backbuffer
				textures->Add(NULL); // Add null just to incrument size
				IDirect3DTexture9 ** ppTex = textures->GetElemAddress(i); // Get the address of the element

				std::wstring path = L"models/";
				path.append(stringUtil::toWideString(d3dxmaterials[i].pTextureFilename,-1).c_str());
				D3DFORMAT backBufferFormat = g_renderer->GetAppPresentParameters()->BackBufferFormat;
				int _numMips = g_UI->GetSetting<int>(& var_textureMipLevels);
				g_renderer->LoadTexture( path.c_str(), backBufferFormat, _numMips, ppTex);
				
				// Now get the texture format and associate a filter with it
				D3DSURFACE_DESC texLevelDesc;
				texSamplerState * texFilter = NULL;
				(*ppTex)->GetLevelDesc(0, & texLevelDesc); 
				texFilter = g_renderer->GetBestTexSampler(texLevelDesc.Format, false); // Not a cube texture
				textureFilters->Add(texFilter);
			}
			else
			{
				// No texture or filter for the ith subset
				textures->Add( NULL );
				textureFilters->Add( NULL );
			}
		}
	}
	ReleaseCOM(mtrlBuffer); // done w/ buffer
}

/************************************************************************/
/* Name:		LoadXFile												*/
/* Description: Load in the X file data.								*/
/************************************************************************/
void rbobjectMeshData::CreateMeshFromBuffers(D3DXVECTOR3 * pos, D3DXVECTOR3 * norm, DWORD numVert, DWORD * indBuf, DWORD numInd, Mtrl * mtrl)
{
	ID3DXMesh* meshSys      = 0;
	ID3DXBuffer* adjBuffer  = 0;

	// Get the vertex declaration elements first
	D3DVERTEXELEMENT9 elements[64];
	UINT numElements = 0;
	VertexPosNormTex::Decl->GetDeclaration(elements, &numElements);

	// Create the mesh
	D3DXCreateMesh(numInd / 3, numVert, D3DXMESH_MANAGED, elements, g_renderer->GetD3DDev(), &meshSys);
	int meshOptions = meshSys->GetOptions();
	bool indices32Bit = (meshOptions & D3DXMESH_32BIT) == D3DXMESH_32BIT; // Do a bit mask

	// Copy over the vertex data
	VertexPosNormTex *pVertex = NULL;
	HR(meshSys->LockVertexBuffer(0, reinterpret_cast<LPVOID*>(&pVertex)),L"rbobjectMeshData::CreateMeshFromBuffers() - Failed to lock vertex buffer: ");
    for (DWORD i = 0; i < numVert; ++i)
    {
		pVertex[i].pos = pos[i];
		pVertex[i].normal = norm[i];
		pVertex[i].tex0 = D3DXVECTOR2(0.0f, 0.0f);
    }
    HR(meshSys->UnlockVertexBuffer(),L"rbobjectMeshData::CreateMeshFromBuffers() - Failed to unlock vertex buffer: ");
	
	// Copy over the index data
	BYTE * pBytesDst = NULL;
	HR(meshSys->LockIndexBuffer(0, reinterpret_cast<LPVOID*>(&pBytesDst)),L"rbobjectMeshData::CreateMeshFromBuffers() - Failed to lock index buffer: ");
	if(indices32Bit)
	{
		DWORD *pIndexDst = NULL;
		for (DWORD i = 0; i < numInd; ++i)
		{
			pIndexDst = reinterpret_cast<DWORD*>(pBytesDst);
			*pIndexDst = (DWORD)indBuf[i];
			pBytesDst += 4; // 4 bytes (32 bits)
		}
	}
	else
	{
		WORD *pIndexDst = 0;
		for (DWORD i = 0; i < numInd; ++i)
		{
			pIndexDst = reinterpret_cast<WORD*>(pBytesDst);
			*pIndexDst = (WORD)indBuf[i];
			pBytesDst += 2; // 2 bytes (16 bits)
		}
	}
    HR(meshSys->UnlockIndexBuffer(),L"rbobjectMeshData::CreateMeshFromBuffers() - Failed to unlock index buffer: ");
	
	HR(D3DXCreateBuffer( meshSys->GetNumFaces() * 3 * sizeof(DWORD), &adjBuffer ), L"rbobjectMeshData::CreateMeshFromBuffers() - Failed to create adjacency buffer: ");
	HR(meshSys->GenerateAdjacency(0.0f, (DWORD*)adjBuffer->GetBufferPointer()), L"rbobjectMeshData::CreateMeshFromBuffers() - Failed to create adjacency info: ");

	// Optimize the mesh.
	HR(meshSys->Optimize(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE, (DWORD*)adjBuffer->GetBufferPointer(), 0, 0, 0, &pMesh),
		L"LoadXFile: Optimize failed: ");
	ReleaseCOM(meshSys); // Done w/ system mesh.
	ReleaseCOM(adjBuffer); // Done with buffer.

	// Now add 1 material and 1 null texture
	Mtrl * tempMtrl = new Mtrl;
	tempMtrl->diffuse		= mtrl->diffuse;
	tempMtrl->specIntensity = mtrl->specIntensity;
	tempMtrl->specPower		= mtrl->specPower;
	materials->Add( tempMtrl );
	textures->Add(NULL);
	textureFilters->Add( NULL );
}

/************************************************************************/
/* Name:		LoadXFile												*/
/* Description: Load in the X file data.								*/
/************************************************************************/
void rbobjectMeshData::UpdateTextureSamplers()
{
	textureFilters->Clear();
	for(UINT i = 0; i < textures->Size(); i ++)
	{
		IDirect3DTexture9 * curTex = textures->GetElem(i);
		if(curTex != NULL)
		{
			// Now get the texture format and associate a filter with it
			D3DSURFACE_DESC texLevelDesc;
			texSamplerState * texFilter = NULL;
			curTex->GetLevelDesc(0, & texLevelDesc); 
			texFilter = g_renderer->GetBestTexSampler(texLevelDesc.Format, false); // Not a cube texture
			textureFilters->Add(texFilter);
		}
		else
			textureFilters->Add(NULL);
	}
}