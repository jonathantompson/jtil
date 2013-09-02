/*************************************************************
**						Object Manager						**
**************************************************************/
// File:		objectManager.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "objectManager\objectManager.h"
#include "lights\lightSpotSM.h"
#include "lights\lightPoint.h"
#include "sky\sky.h"
#include "hud\hud.h"
#include "renderer\renderer_structures\debugObject.h"
#include "renderer\renderer_structures\cone.h"
#include "renderer\renderer_structures\sphere.h"
#include "camera\camera.h" 
#include "physics\physics.h"
#include <sstream>
#include "fileIO\csvHandleRead.h"
#include "physics\objects\rbobjectMesh.h"
#include "physics\objects\rbobjectMeshData.h"
#include "physics\obbtree_related\obbtree.h"
#include "physics\obbtree_related\obbox.h"
#include "utils_and_misc_classes\data_structures\vec_ptrs.h"
#include "utils_and_misc_classes\data_structures\vec_ptrsA.h"
#include "app.h"
#include "UI\UI.h"
#include "UI\varNames.h"
#include "renderer\renderer_structures\resettableResources\resettable.h"
#include <limits>
#include "renderer\renderer_constants.h"

#include <new>        // Must #include this to use "placement new"

//#include "utils_and_misc_classes\new_redefine.h" // CAN'T USE THIS WITH PLACEMENT NEW

#define LIGHTDEBUGOBJECTSSIZE	5.0f
#define VIEWFITNEARFARMARGIN	0.05f  // 5% extra near far (hack to overcome some bugs)

/************************************************************************/
/* Name:		objectManager											*/
/* Description:	Default constructor function							*/
/************************************************************************/
objectManager::objectManager()
{
	h_lights = NULL;
	h_lightSpotSM = NULL;
	h_rbobjects = NULL;
	h_rbobjectMeshes = NULL;
	h_sky = NULL;
	h_hud = NULL;
	h_camera = NULL;
	h_debugObjects = NULL;
	h_MeshData = NULL;
	num_lights = 0;
	num_rbobjects = 0;
	num_resettableResources = 0;
	reader = NULL;
	curToken = NULL;
	m_sphereMesh = NULL;
	m_coneMesh = NULL;
	h_resettableResources = NULL;
}

/************************************************************************/
/* Name:		~objectManager											*/
/* Description:	Default destructor function								*/
/************************************************************************/
objectManager::~objectManager()
{
	// Release all resettable resources
	if(h_resettableResources)
	{
		for(UINT i = 0; i < h_resettableResources->Size(); i ++)
			h_resettableResources->GetElem(i)->Release();
		h_resettableResources->Clear();
		delete h_resettableResources; 
		h_resettableResources = NULL;
	}

	// Delete the lights then delete the array
	if(h_lights) 
	{ 
		h_lights->Clear();
		delete h_lights; h_lights = NULL;
		num_lights = 0;
	}
	if(h_lightSpotSM) { delete h_lightSpotSM; h_lightSpotSM = NULL; }

	// Delete the mesh data in the hash_map then delete the hash_map
	if(h_MeshData)
	{
		MESHDATA_HASH_T::iterator p;
		rbobjectMeshData * curMeshData;
		for(p = h_MeshData->begin(); p != h_MeshData->end(); ++p)
		{
			// First is the key, second is the value
			curMeshData = p->second;
			delete curMeshData;
			p->second = NULL;
		} 
		delete h_MeshData; 

		// Iterate through the rbobject array and now invalidate the mesh data (otherwise it'll be freed twice)
		for(UINT i = 0; i < h_rbobjects->Size(); i ++)
		{
			if(h_rbobjects->GetElem(i)->GetType() == T_RBOBJECTMESH)
				dynamic_cast<rbobjectMesh *>(h_rbobjects->GetElem(i))->meshData = NULL;
		}
	}

	// Delete the rbobjects then delete the array
	if(h_rbobjects)
	{
		h_rbobjects->Clear();
		delete h_rbobjects; h_rbobjects = NULL;
		num_rbobjects = 0;
	}
	if(m_sphereMesh) { delete m_sphereMesh; m_sphereMesh = NULL; }
	if(m_coneMesh)		{ delete m_coneMesh; m_coneMesh = NULL; }

	// Delete sub lists
	if(h_rbobjectMeshes)
	{ 
		h_rbobjectMeshes->FreeArray(); // Objects are already deleted above, just delete the vec_ptrs itself
		delete h_rbobjectMeshes; 
		h_rbobjectMeshes = NULL; 
	} 

	if(h_sky)			{ delete h_sky; h_sky = NULL; }
	if(h_hud)			{ delete h_hud; h_hud = NULL; }
	if(h_camera)		{ delete h_camera; h_camera = NULL; }
	if(h_debugObjects)	{ delete h_debugObjects; h_debugObjects = NULL; } // Recursively delete the linked list
	if(reader)			{ reader->Close(); delete reader; reader = NULL; }
	if(curToken)		{ curToken->Clear(); delete curToken; curToken = NULL; }
}

/************************************************************************/
/* Name:		Init													*/
/* Description:	Initialize objects on startup							*/
/************************************************************************/
void objectManager::InitObjectManager()
{
	// MAKE NEW OBJECTS VECTOR OF POINTERS (empty)
	num_rbobjects = 0;
	h_rbobjects = new vec_ptrsA<rbobject>(num_rbobjects); // array of rigid body object pointers
	h_resettableResources = new vec_ptrs<resettable>();

	// INITIALIZE THE CAMERA
	h_camera = new camera;

	// INITIALIZE THE HUD DISPLAY
	h_hud = new hud;
	h_hud->InitHud();

	h_hud->RenderProgressText( L"Loading scene objects (models, lights, etc)...");
	g_app->CheckMessages();

	// MAKE NEW LIGHTS VECTOR OF POINTERS (empty)
	num_lights = 0;
	h_lights = new vec_ptrs<light>(num_lights); // array of light object pointers

	// START ALL SUB OBJECT LISTS
	h_rbobjectMeshes = new vec_ptrs<rbobjectMesh>(num_rbobjects); // array of rigid body object pointers

	// MAKE A NEW HASH_MAP TO STORE OBB AND MESH DATA
	h_MeshData = new MESHDATA_HASH_T();

	// LOAD IN THE MODELS, LIGHTS & SKY --> POPULATE THE ARRAYS
	LoadLevel(0);
	LoadLightObjects();

	if(num_rbobjects<1 || h_sky == NULL || h_lightSpotSM == NULL)
		throw std::runtime_error("objectManager::InitObjectManager() - level file needs at least 1 of each: sky, rbobjectMesh, lightSpotSM");

	// Resize the physics system now that we've populated the arrays
	g_physics->ResizePhysicsSystem(num_rbobjects);
}

/************************************************************************/
/* Name:		LoadLightObjects										*/
/* Description:	Load in the mesh required for light point visualization	*/
/************************************************************************/
void objectManager::LoadLightObjects()
{
	float outsideRadius = SPHERE_INSIDERADIUS;
	bool moveOriginToCOM = false;
	sphere * tempSphere = new sphere(SPHERE_NUM_STACKS, SPHERE_NUM_SLICES, SPHERE_INSIDERADIUS, SPHERE_COLOR, & outsideRadius);
	m_lightSphereRadiusScale =  outsideRadius / SPHERE_INSIDERADIUS;
	m_sphereMesh = new rbobjectMesh();
	m_sphereMesh->LoadDataFromBuffers(L"models\\sphere", tempSphere->GetPosBuff(), tempSphere->GetNormBuff(), tempSphere->GetNumVert(), tempSphere->GetIndBuff(), tempSphere->GetNumInd(), tempSphere->GetMtrl(), moveOriginToCOM);
	if(tempSphere) { delete tempSphere; tempSphere = NULL; }

	cone * tempCone = new cone(CONE_NUM_BASE_VERTICES, CONE_HEIGHT, CONE_INSIDERADIUS, CONE_COLOR, & outsideRadius);
	m_lightConeRadiusScale =  outsideRadius / SPHERE_INSIDERADIUS;
	m_coneMesh = new rbobjectMesh();
	m_coneMesh->LoadDataFromBuffers(L"models\\cone", tempCone->GetPosBuff(), tempCone->GetNormBuff(), tempCone->GetNumVert(), tempCone->GetIndBuff(), tempCone->GetNumInd(), tempCone->GetMtrl(), moveOriginToCOM);
	if(tempCone) { delete tempCone; tempCone = NULL; }
}

/************************************************************************/
/* Name:		AddRBObject												*/
/* Description:	Perform functions required to add a new rboobject		*/
/************************************************************************/
void objectManager::AddRBObject(rbobject * newObj)
{
	if(h_rbobjects)
	{
		h_rbobjects->Add(newObj);
		num_rbobjects += 1;
	}
	else
		throw std::runtime_error("objectManager::AddRBObject() - Error: Trying to add a light when h_rbobjects is not initilaized");
}

/************************************************************************/
/* Name:		AddLight												*/
/* Description:	Perform functions required to add a new light			*/
/************************************************************************/
void objectManager::AddLight(light * newObj)
{
	if(h_lights)
	{
		h_lights->Add(newObj);
		num_lights += 1;
	}
	else
		throw std::runtime_error("objectManager::AddLight() - Error: Trying to add a light when h_lights is not initilaized");
}

/************************************************************************/
/* Name:		AddResettableResource									*/
/* Description:	Perform functions required to add a new light			*/
/************************************************************************/
void objectManager::AddResettableResource(resettable * newObj)
{
	if(h_resettableResources)
	{
		h_resettableResources->Add(newObj);
		num_resettableResources += 1;
	}
	else
		throw std::runtime_error("objectManager::AddResettableResource() - Error: Trying to add a resettable resource when h_resettableResources is not initilaized");
}


/************************************************************************/
/* Name:		GetFileName												*/
/* Description:	Just hides away the filename generation (which might	*/
/*				differ for every project)								*/
/************************************************************************/
std::wstring objectManager::GetFileName(UINT level)
{
	std::wstringstream file;
	file << L".\\Models\\Level" << level << L".csv";
	return file.str();
}

/************************************************************************/
/* Name:		LoadLevel												*/
/* Description:	Get each token from the csvHandle class and send it off.*/
/************************************************************************/
void objectManager::LoadLevel(UINT level)
{
	curFileName = objectManager::GetFileName(level);
	reader = new csvHandleRead(curFileName);
	curToken = new vec_ptrs<wchar_t>; // Each element is a csv in the line

	g_app->CheckMessages();

	// Keep reading tokens until we're at the end
	while(!reader->CheckEOF() && !g_app->cancel)
	{
		g_app->CheckMessages();
		reader->ReadNextToken( curToken, false); // Get the next element
		if(curToken->Size() > 0)
			objectManager::ProcessToken( curToken, &curFileName); // process the object
		curToken->Clear(); // Clear the token
	}

	// If the cancel button was pressed then kill the application and exit
	if(g_app->cancel)
	{ DestroyWindow(g_app->GetWindowHandle()); app::KillApp(); exit(0); }

	// Close the file
	if(reader)		{ reader->Close(); delete reader; reader = NULL; }
	if(curToken)	{ curToken->Clear(); delete curToken; curToken = NULL; }
}

/************************************************************************/
/* Name:		MoveUpOBBTrees / Left / Right							*/
/* Description:	Choose a new obbox to render on the tree				*/
/************************************************************************/
void objectManager::MoveUpOBBTrees()
{ 
	// Find each element in the hash_table and call move up
	MESHDATA_HASH_T::iterator it;
	rbobjectMeshData * curMeshData;
	for(it = h_MeshData->begin(); it != h_MeshData->end(); ++it)
	{
		curMeshData = it->second;
		curMeshData->GetOBBTree()->MoveUp();
	} 
}
void objectManager::MoveLeftOBBTrees()
{ 
	// Find each element in the hash_table and call move Left
	MESHDATA_HASH_T::iterator it;
	rbobjectMeshData * curMeshData;
	for(it = h_MeshData->begin(); it != h_MeshData->end(); ++it)
	{
		curMeshData = it->second;
		curMeshData->GetOBBTree()->MoveLeft();
	} 
}
void objectManager::MoveRightOBBTrees()
{ 
	// Find each element in the hash_table and call move Right
	MESHDATA_HASH_T::iterator it;
	rbobjectMeshData * curMeshData;
	for(it = h_MeshData->begin(); it != h_MeshData->end(); ++it)
	{
		curMeshData = it->second;
		curMeshData->GetOBBTree()->MoveRight();
	} 
}
/************************************************************************/
/* Name:		GetMeshData()											*/
/* Description:	Find the meshdata in the hash map from the given instance*/
/************************************************************************/
rbobjectMeshData * objectManager::GetMeshData(std::wstring * instName)
{ 
	MESHDATA_HASH_T::iterator it;
	it = h_MeshData->find(*instName);
	// First is the key, second is the value
	if(it != h_MeshData->end())
		return it->second;
	else
		return NULL;
}
/************************************************************************/
/* Name:		AddMeshData()											*/
/* Description:	Add meshdata to the hash map.							*/
/************************************************************************/
void objectManager::AddMeshData(std::wstring * key, rbobjectMeshData * value)
{ 
	// First is the key, second is the value
	h_MeshData->insert(MESHDATA_HASH_T_PAIR(*key,value));
}

/************************************************************************/
/* Name:		UpdateRBObjectMatricies()								*/
/* Description:	Update each rbobject world matrix						*/
/************************************************************************/
void objectManager::UpdateRBObjectMatricies()
{ 
	for(UINT i = 0; i < h_rbobjects->Size(); i ++)
		h_rbobjects->GetElem(i)->UpdateMatricies();
}

/************************************************************************/
/* Name:		SaveRbobjectWorldMatrices()								*/
/* Description:	Save the world matrix for the next frame				*/
/************************************************************************/
void objectManager::SaveRbobjectWorldMatrices()
{ 
	for(UINT i = 0; i < h_rbobjects->Size(); i ++)
		h_rbobjects->GetElem(i)->SaveMatWorld();
}

/************************************************************************/
/* Name:		UpdateRBObjectBoundingBoxes()							*/
/* Description:	Update each rbobject boundig box						*/
/************************************************************************/
void objectManager::UpdateRBObjectBoundingBoxes()
{ 
	for(UINT i = 0; i < h_rbobjects->Size(); i++) 
		h_rbobjects->GetElem(i)->UpdateBoundingBox(); 
}

/************************************************************************/
/* Name:		FitViewMatrixNearFarToRBObjects()						*/
/* Description:	Fit the near and far parameters to the world geometry	*/
/************************************************************************/
void objectManager::FitViewMatrixNearFarToRBObjects(D3DXMATRIX * view, float * nearRtn, float * farRtn, float nearMin, float farMax, bool includeLightObjects, bool includeLightVolumes)
{ 
	// Initialize the return value;
	// This is a hack, but we need to use the keyword max (from limits library), but windows library has already defined it.
#undef max
	* nearRtn = std::numeric_limits<float>::max();
#define max(a,b)            (((a) > (b)) ? (a) : (b))
	* farRtn = 0.0f;

	// For each object, transform the world coordinates of it's AABB extents and compare Zvalues
	D3DXVECTOR3 * curWorldBounds = NULL;
	D3DXVECTOR3 curViewSpaceBound;
	for(UINT i = 0; i < h_rbobjectMeshes->Size(); i ++)
	{
		// Get the bounding box
		curWorldBounds = h_rbobjectMeshes->GetElem(i)->GetWorldBounds();
		for(UINT j = 0; j < 8; j ++)
		{
			// Transform the minimum extent into view space
			D3DXVec3TransformCoord( & curViewSpaceBound, &curWorldBounds[j], view );
			// Adjust near and far
			if(curViewSpaceBound.z < *nearRtn)
				*nearRtn = curViewSpaceBound.z;
			if(curViewSpaceBound.z > *farRtn)
				*farRtn = curViewSpaceBound.z;
		}
	}

	// Also fit to the light volumes with some margin to include the light object
	if(includeLightVolumes || includeLightObjects)
	{
		light * curLight = NULL;
		D3DXVECTOR3 * curLightPos;
		D3DXVECTOR3 * curLightDir;
		float OBBHalfLength, radius;
		D3DXVECTOR3 pos;
		for(UINT i = 0; i < num_lights; i ++)
		{
			curLight = g_objectManager->GetLight(i);
			if(curLight->GetType() == T_POINT)
			{
				lightPoint * curLightPoint = (lightPoint *)curLight;
				curLightPos = curLightPoint->GetLightPos();
				if (includeLightVolumes) // will also include the objects
				{
					radius = curLightPoint->GetLightNearFar()->y;
					radius *= 1.41421356f; // radius = radius / sin(45deg) --> For AABB to include the radius completely, we need to expand the radius.
					OBBHalfLength = radius;
				}
				else // includeLightObjects == true
					OBBHalfLength = LIGHT_VISULIZATION_SIZE * 2.0f;
				ExpandNearFarAroundPoint(curLightPos, OBBHalfLength, view, nearRtn, farRtn);
			}
			if(curLight->GetType() == T_SPOT)
			{
				lightSpot * curLightSpot = (lightSpot *)curLight;
				curLightPos = curLightSpot->GetLightPos(); // this is the peak of the triangle
				curLightDir = curLightSpot->GetLightDir();
				float height = curLightSpot->GetLightNearFar()->y;
				if (includeLightVolumes) // will also include the objects
				{
					OBBHalfLength = max(height * 0.5f, 1.41421356f * tan(curLightSpot->GetLightFOV()) * height); // The length of the obb will be dominated by the height (if angle > 26.5deg) or the cone base (if angle > 26.5deg)
					pos = *curLightPos + (*curLightDir * (height * 0.5f)); // The point should be in the center of the height (or halfway along the radius of the cone)
				}
				else // includeLightObjects == true
				{
					OBBHalfLength = LIGHT_VISULIZATION_SIZE * 2.0f;
					pos = *curLightPos;
				}
				ExpandNearFarAroundPoint(& pos, OBBHalfLength, view, nearRtn, farRtn);
			}
		}
		lightSpot * curLightSpot = (lightSpot *)h_lightSpotSM;
		curLightPos = curLightSpot->GetLightPos(); // this is the peak of the triangle
		curLightDir = curLightSpot->GetLightDir();
		float height = curLightSpot->GetLightNearFar()->y;
		if (includeLightVolumes) // will also include the objects
		{
			OBBHalfLength = max(height * 0.5f, 1.41421356f * tan(curLightSpot->GetLightFOV()) * height); // The length of the obb will be dominated by the height (if angle > 26.5deg) or the cone base (if angle > 26.5deg)
			pos = *curLightPos + (*curLightDir * (height * 0.5f)); // The point should be in the center of the height (or halfway along the radius of the cone)
		}
		else // includeLightObjects == true
		{
			OBBHalfLength = LIGHT_VISULIZATION_SIZE * 2.0f;
			pos = *curLightPos;
		}
		ExpandNearFarAroundPoint(& pos, OBBHalfLength, view, nearRtn, farRtn);
	}

	// Now add some margin
	float spread = *farRtn - *nearRtn;
	*nearRtn = *nearRtn - spread * VIEWFITNEARFARMARGIN;
	*farRtn = *farRtn + spread * VIEWFITNEARFARMARGIN;

	// Clamp the return value
	if(*nearRtn < nearMin)
		*nearRtn = nearMin;
	if(*farRtn > farMax)
		*farRtn = farMax;
}

/************************************************************************/
/* Name:		ExpandNearFarAroundPoint()								*/
/* Description:	Helper function to expand the near far to include a 	*/
/*				world space point (with some margin).					*/
/************************************************************************/
void objectManager::ExpandNearFarAroundPoint(D3DXVECTOR3 * point, float OBBHalfLength, D3DXMATRIX * view, float * nearRtn, float * farRtn)
{
	D3DXVECTOR3 curOBBVertexWorld;
	D3DXVECTOR3 curViewSpaceBound;
	float bounds[2] = {-1.0f, 1.0f};
	for( UINT x = 0; x < 2; x ++ )
	{
		for( UINT y = 0; y < 2; y ++ )
		{
			for( UINT z = 0; z < 2; z ++ )
			{
				curOBBVertexWorld.x = point->x + bounds[x]*OBBHalfLength;
				curOBBVertexWorld.y = point->y + bounds[y]*OBBHalfLength;
				curOBBVertexWorld.z = point->z + bounds[z]*OBBHalfLength;
				// Transform the minimum extent into view space
				D3DXVec3TransformCoord( & curViewSpaceBound, & curOBBVertexWorld, view );
				// Adjust near and far
				if(curViewSpaceBound.z < *nearRtn)
					*nearRtn = curViewSpaceBound.z;
				if(curViewSpaceBound.z > *farRtn)
					*farRtn = curViewSpaceBound.z;
			}
		}
	}
}

/************************************************************************/
/* Name:		RenderRBObjectMeshes()									*/
/* Description:	Render each mesh										*/
/************************************************************************/
void objectManager::RenderRBObjectMeshes()
{ 
	for(UINT i = 0; i < h_rbobjectMeshes->Size(); i ++)
		h_rbobjectMeshes->GetElem(i)->Render();
}

/************************************************************************/
/* Name:		RenderRBObjectMeshesSM()								*/
/* Description:	Render each mesh										*/
/************************************************************************/
void objectManager::RenderRBObjectMeshesSM()
{ 
	for(UINT i = 0; i < h_rbobjectMeshes->Size(); i ++)
		h_rbobjectMeshes->GetElem(i)->RenderSM();
}

/************************************************************************/
/* Name:		RenderRBObjectMeshesGBuffer()							*/
/* Description:	Render each mesh										*/
/************************************************************************/
void objectManager::RenderRBObjectMeshesGBuffer(bool drawVelocityBuffer)
{ 
	UINT pass;
	if(drawVelocityBuffer)
		pass = GBUFFER_TEXTUREDMESH_VELOCITY_SHADING_PASS;
	else
		pass = GBUFFER_TEXTUREDMESH_SHADING_PASS;

	for(UINT i = 0; i < h_rbobjectMeshes->Size(); i ++)
		h_rbobjectMeshes->GetElem(i)->RenderGBuffer(pass);
}

/************************************************************************/
/* Name:		RenderRBObjectMeshesGBufferWireframe()					*/
/* Description:	Render each mesh wireframe								*/
/************************************************************************/
void objectManager::RenderRBObjectMeshesGBufferWireframe()
{ 
	for(UINT i = 0; i < h_rbobjectMeshes->Size(); i ++)
		h_rbobjectMeshes->GetElem(i)->RenderGBufferWireframe();
}

/************************************************************************/
/* Name:		UpdateLights()											*/
/* Description:	Update light matricies									*/
/************************************************************************/
void objectManager::UpdateLights()
{ 
	for(UINT i = 0; i < h_lights->Size(); i ++)
		h_lights->GetElem(i)->Update();
	h_lightSpotSM->Update();
	h_lightSpotSM->UpdateSMMatrices();
}

/************************************************************************/
/* Name:		MoveLights()											*/
/* Description:	Update light positions									*/
/************************************************************************/
void objectManager::MoveLights(float angle)
{ 
	// Move the global light
	h_lightSpotSM->RotateLightY(angle);

	for(UINT i = 0; i < h_lights->Size(); i ++)
		h_lights->GetElem(i)->RotateLightY(angle);
}


/************************************************************************/
/* Name:		RenderRBObjectMeshConvexHulls()							*/
/* Description:	Render each meshes convex hull element					*/
/* There is some redundancy with RenderRBObjectMeshOBBs(), however		*/
/* these are debug functions and are not time critical.					*/
/************************************************************************/
void objectManager::RenderRBObjectMeshConvexHulls()
{ 
	obbox * obbToRender = NULL;
	rbobjectMesh * curObjectMesh = NULL;
	for(UINT i = 0; i < h_rbobjectMeshes->Size(); i ++)
	{
		curObjectMesh = h_rbobjectMeshes->GetElem(i);
		obbToRender = curObjectMesh->meshData->GetOBBTree()->GetObbToRender();
		if(obbToRender != NULL)
			obbToRender->DrawConvexHull( &curObjectMesh->matWorld );
	}
}

/************************************************************************/
/* Name:		RenderRBObjectMeshOBBs()								*/
/* Description:	Render each meshes OBBOX hull element					*/
/* There is some redundancy with RenderRBObjectMeshConvexHulls(),		*/
/* however these are debug functions and are not time critical.			*/
/************************************************************************/
void objectManager::RenderRBObjectMeshOBBs()
{ 
	obbox * obbToRender = NULL;
	rbobjectMesh * curObjectMesh = NULL;
	for(UINT i = 0; i < h_rbobjectMeshes->Size(); i ++)
	{
		curObjectMesh = h_rbobjectMeshes->GetElem(i);
		obbToRender = curObjectMesh->meshData->GetOBBTree()->GetObbToRender();
		if(obbToRender != NULL)
			obbToRender->DrawOBB( &curObjectMesh->matWorld );
	}
}

/************************************************************************/
/* Name:		RenderRBObjectMeshOBBsSM()								*/
/* Description:	Render each meshes OBBOX hull element					*/
/************************************************************************/
void objectManager::RenderRBObjectMeshOBBsSM()
{ 
	obbox * obbToRender = NULL;
	rbobjectMesh * curObjectMesh = NULL;
	for(UINT i = 0; i < h_rbobjectMeshes->Size(); i ++)
	{
		curObjectMesh = h_rbobjectMeshes->GetElem(i);
		obbToRender = curObjectMesh->meshData->GetOBBTree()->GetObbToRender();
		if(obbToRender != NULL)
			obbToRender->DrawOBBSM( &curObjectMesh->matWorld);
	}
}

/************************************************************************/
/* Name:		OnLostDevice()											*/
/* Description:	Free memory of resettable resources before device reset	*/
/************************************************************************/
void objectManager::OnLostDevice()
{ 
	if(h_resettableResources)
	{
		for(UINT i = 0; i < h_resettableResources->Size(); i ++)
			h_resettableResources->GetElem(i)->OnLostDevice();
	}
}

/************************************************************************/
/* Name:		OnResetDevice()											*/
/* Description:	Reload resettable resources after device reset			*/
/************************************************************************/
void objectManager::OnResetDevice()
{ 
	if(h_resettableResources)
	{
		for(UINT i = 0; i < h_resettableResources->Size(); i ++)
			h_resettableResources->GetElem(i)->OnResetDevice();
	}
}

/************************************************************************/
/* Name:		CheckResettableFormat()									*/
/* Description:	Check that resources can be reloaded after back buffer	*/
/*              format change.											*/
/************************************************************************/
void objectManager::CheckResettableFormat( D3DFORMAT backBufferFormat )
{ 
	for(UINT i = 0; i < h_resettableResources->Size(); i ++)
		h_resettableResources->GetElem(i)->CheckResettableFormat(backBufferFormat);
}

/************************************************************************/
/* Name:		UpdateTextureSamplers									*/
/* Description: functions required to turn on/off texture filters		*/
/************************************************************************/
void objectManager::UpdateTextureSamplers(void)
{
	// rbobjects mesh data
	MESHDATA_HASH_T::iterator p;
	rbobjectMeshData * curMeshData;
	for(p = h_MeshData->begin(); p != h_MeshData->end(); ++p)
	{
		// First is the key, second is the value
		curMeshData = p->second;
		curMeshData->UpdateTextureSamplers();
	} 

	// sky
	h_sky->UpdateTextureSamplers();
}

/************************************************************************/
/* Name:		Some access and modifier functions						*/
/************************************************************************/
// These are put here to avoid including many types in the .h file.
light * objectManager::GetLight(UINT index){ return h_lights->GetElem(index); }
rbobject * objectManager::GetRBObject(UINT index){ return h_rbobjects->GetElem(index); }
void objectManager::ClearDebugObjects(){ delete h_debugObjects; h_debugObjects = NULL; } // Recursive delete
UINT objectManager::GetNumRBObjects() { return h_rbobjects->Size(); }
UINT objectManager::GetNumLights() { return h_lights->Size(); }