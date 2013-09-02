/*************************************************************
**						Object Manager						**
**************************************************************/
// File:		objectManager.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef objectManager_h
#define objectManager_h

#include "dxInclude.h"

class debugObject;
class camera;
class light;
class hud;
class camera;
class sky;
class rbobjectMeshData;
class rbobjectMesh;
class rbobject;
class csvHandleRead;
class lightSpotSM;
class cone;
class resettable;
template <typename T> class vec_ptrsA;
template <typename T> class vec_ptrs;
template <typename T> class vecA;

// The hash table types we're going to use
#include "utils_and_misc_classes\stringHasher.h"
typedef stdext::hash_map<std::wstring, rbobjectMeshData *, stringHasher> MESHDATA_HASH_T;
typedef stdext::hash_map<std::wstring, rbobjectMeshData *, stringHasher>::value_type MESHDATA_HASH_T_PAIR;

#ifndef UINT
typedef unsigned int        UINT;
#endif

class objectManager
{
public:
	friend class app;

	// Constructor / Destructor
    objectManager();
	~objectManager();

	// Access and Modifier Functions
	light *							GetLight(UINT index);
	inline lightSpotSM *			GetLightSpotSM() { return h_lightSpotSM; }
	inline sky *					GetSky(){ return h_sky; }
	inline hud *					GetHud(){ return h_hud; }
	inline camera *					GetCamera(){ return h_camera; }
	inline debugObject *			GetDebugObjects(){ return h_debugObjects; }
	rbobject *						GetRBObject(UINT index);
	inline vec_ptrsA<rbobject> *	GetRBObjects(){ return h_rbobjects; }
	void							ClearDebugObjects(); // Recursive delete
	inline void						SetDebugObjects(debugObject * in){ h_debugObjects = in; }
	UINT							GetNumRBObjects();
	UINT							GetNumLights();
	inline rbobjectMesh *			GetSphereMesh() { return m_sphereMesh; }
	inline rbobjectMesh *			GetConeMesh() { return m_coneMesh; }
	inline vec_ptrs<resettable> *	GetResettableResources() { return h_resettableResources; }
	void							CheckResettableFormat( D3DFORMAT backBufferFormat );
	inline float					GetLightSphereRadiusScale() { return m_lightSphereRadiusScale; }
	inline float					GetLightConeRadiusScale() { return m_lightConeRadiusScale; }

	// Render functions
	void						UpdateRBObjectMatricies();
	void						UpdateRBObjectBoundingBoxes();
	void						RenderRBObjectMeshes();
	void						RenderRBObjectMeshesSM();
	void						RenderRBObjectMeshesGBuffer(bool drawVelocityBuffer);
	void						RenderRBObjectMeshesGBufferWireframe();
	void						RenderRBObjectMeshOBBs();
	void						RenderRBObjectMeshOBBsSM();
	void						RenderRBObjectMeshConvexHulls();
	void						FitViewMatrixNearFarToRBObjects(D3DXMATRIX * view, float * nearRtn, float * farRtn, float nearMin, float farMax, bool includeLightObjects, bool includeLightVolumes);
	static void					ExpandNearFarAroundPoint(D3DXVECTOR3 * point, float OBBHalfLength, D3DXMATRIX * view, float * nearRtn, float * farRtn);
	void						UpdateTextureSamplers();
	void						UpdateLights();
	void						SaveRbobjectWorldMatrices();

	// Hash table referencing
	rbobjectMeshData *			GetMeshData(std::wstring * instName);
	void						AddMeshData(std::wstring * key, rbobjectMeshData * value);

	void						InitObjectManager();
	void						AddRBObject(rbobject * newObj);
	void						AddLight(light * newObj);
	void						AddResettableResource(resettable * newObj);
	void						OnLostDevice();
	void						OnResetDevice();

	void						MoveLights(float angle);

	// functions used to render OBB Tree --> Debug functions
	void						MoveUpOBBTrees();
	void						MoveLeftOBBTrees();
	void						MoveRightOBBTrees();

private:

	//object data
	vec_ptrs<light>	*			h_lights;	
	lightSpotSM *				h_lightSpotSM; // Only one shadow casting light is allowed for now
	vec_ptrsA<rbobject> *		h_rbobjects;		// Global list over every object
	vec_ptrs<rbobjectMesh> *	h_rbobjectMeshes;	// List of only the mesh objects (used to group render techniques)
	vec_ptrs<resettable> *		h_resettableResources; // Vector of the elements to reload on lost and reset device
	UINT						num_lights;
	UINT						num_rbobjects;
	UINT						num_resettableResources;

	// Store static object data --> Potentially shared accross objects.  Avoids storing the same data twice
	MESHDATA_HASH_T *			h_MeshData;

	sky *						h_sky;
	hud *						h_hud;
	camera *					h_camera;
	debugObject *				h_debugObjects;		// Doubly Linked-list of debug objects
	rbobjectMesh *				m_sphereMesh; // For visualizing point light position
	rbobjectMesh *				m_coneMesh; // For visualizing spot light position
	float						m_lightSphereRadiusScale; // Since sphere volume is an approximation, outside radius will be larger that real sphere
	float						m_lightConeRadiusScale; // Since cone volume is an approximation, outside radius will be larger that real cone

	// private functions
	static std::wstring			GetFileName(UINT level);
	void						LoadLevel(UINT level);
	void						ProcessToken(vec_ptrs<wchar_t> * curToken, std::wstring * fileName );
	void						ProcessTokenrbobjectMesh(vec_ptrs<wchar_t> * curToken, std::wstring * fileName );
	void						ProcessTokenrbobjectSphere(vec_ptrs<wchar_t> * curToken, std::wstring * fileName );
	void						ProcessTokenrbobjectCone(vec_ptrs<wchar_t> * curToken, std::wstring * fileName );
	void						ProcessTokenlightSpotSM(vec_ptrs<wchar_t> * curToken, std::wstring * fileName );
	void						ProcessTokenLightSpot(vec_ptrs<wchar_t> * curToken, std::wstring * fileName );
	void						ProcessTokenLightDir(vec_ptrs<wchar_t> * curToken, std::wstring * fileName );
	void						ProcessTokenLightPoint(vec_ptrs<wchar_t> * curToken, std::wstring * fileName );
	void						ProcessTokenSky(vec_ptrs<wchar_t> * curToken, std::wstring * fileName );
	void						LoadLightObjects();

	// Temporary data for rendering
	D3DXMATRIX					m_temp_mat;

	// Temporary data for loading levels
	csvHandleRead *				reader;
	vec_ptrs<wchar_t> *			curToken;
	std::wstring				curFileName;

};

#endif