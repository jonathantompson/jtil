/*************************************************************
**						Object Manager						**
**************************************************************/
// File:		objectManager.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "dataAlign.h"
#include "objectManager\objectManager.h"
#include "lights\light.h"
#include "lights\lightSpot.h"
#include "lights\lightSpotSM.h"
#include "lights\lightDir.h"
#include "lights\lightPoint.h"
#include "sky\sky.h"
#include "hud\hud.h"
#include "fileIO\csvHandle.h"
#include "physics\objects\rbobjectMesh.h"
#include "utils_and_misc_classes\SIMD_helpers\dataAlign.h"
#include "utils_and_misc_classes\data_structures\vec_ptrs.h"
#include "renderer\renderer.h"
#include "utils_and_misc_classes\stringUtil.h"
#include "renderer\renderer_structures\sphere.h"
#include "renderer\renderer_structures\cone.h"

#include <new>        // Must #include this to use "placement new"

//#include "utils_and_misc_classes\new_redefine.h" // CAN'T USE THIS WITH PLACEMENT NEW

#define RBOBJECTMESH_NUM_TOKENS				18
#define RBOBJECTSPHERE_NUM_TOKENS			22
#define RBOBJECTCONE_NUM_TOKENS				22
#define LIGHTSPOTSM_NUM_TOKENS				16
#define LIGHTSPOT_NUM_TOKENS				16
#define LIGHTDIR_NUM_TOKENS					12
#define LIGHTPOINT_NUM_TOKENS				12
#define SKY_NUM_TOKENS						3

/************************************************************************/
/* Name:		ProcessToken											*/
/* Description:	Make a new object for every token. 						*/
/************************************************************************/
void objectManager::ProcessToken(vec_ptrs<wchar_t> * curToken, std::wstring * fileName )
{
	std::wstring elementHeader(curToken->GetElem(0));
	// Switch statement here would be ideal, but using wchar_t * in switch is messy (must be const case values)
	if(elementHeader == std::wstring(L"//"))
	{	} // Do nothing for comment blocks
	else if(elementHeader == std::wstring(L""))
	{	} // Do nothing for empty lines
	else if(elementHeader == std::wstring(L"rbobjectMesh"))
		ProcessTokenrbobjectMesh(curToken,fileName);
	else if(elementHeader == std::wstring(L"rbobjectSphere"))
		ProcessTokenrbobjectSphere(curToken,fileName);
	else if(elementHeader == std::wstring(L"rbobjectCone"))
		ProcessTokenrbobjectCone(curToken,fileName);
	else if(elementHeader == std::wstring(L"lightSpotSM"))
		ProcessTokenlightSpotSM(curToken,fileName);
	else if(elementHeader == std::wstring(L"lightSpot"))
		ProcessTokenLightSpot(curToken,fileName);
	else if(elementHeader == std::wstring(L"lightDir"))
		ProcessTokenLightDir(curToken,fileName);
	else if(elementHeader == std::wstring(L"lightPoint"))
		ProcessTokenLightPoint(curToken,fileName);
	else if(elementHeader == std::wstring(L"sky"))
		ProcessTokenSky(curToken,fileName);
	else
	{
		std::wstring error = L"objectManager::ProcessToken() - Unrecognized token header: '" + 
							 csvHandle::FlattenToken(curToken) + 
							 L"' in file: '" +
							 *fileName + 
							 L"'";
		throw std::runtime_error(stringUtil::toNarrowString(error.c_str(),-1));
	}
}

/************************************************************************/
/* Name:		ProcessTokenrbobjectMesh								*/
/* Description:	Create a new rbobjectMesh object with the given parameters	*/
//, rbobjectMesh,          model_name, format, immovable?,  scale,  < Xpos,  Ypos,  Zpos, >  < XrotAxis, YrotAxis, ZrotAxis, >  rotDeg,  < XLinMom, YLinMom, ZLinMom, >  < XAngMom, YAngMom, ZAngMom, >
//             0,                   1,      2,          3,      4,       5,     6,     7,             8,        9,       10,        11,         12,      13,      14,           15,      16,      17,
/************************************************************************/
void objectManager::ProcessTokenrbobjectMesh(vec_ptrs<wchar_t> * curToken, std::wstring * fileName )
{
	if(curToken->Size() != RBOBJECTMESH_NUM_TOKENS)
	{
		std::wstring error = L"objectManager::ProcessTokenrbobjectMesh() - Incorrect # tokens: '" + 
						     csvHandle::FlattenToken(curToken) + 
							 L"' in file: '" +
							 *fileName + 
							 L"'";
		throw std::runtime_error(stringUtil::toNarrowString(error.c_str(),-1));
	}

	// Make space for the new object
	rbobjectMesh * newobject = NULL;
	aligned_new_constructor(newobject,DATA_ALIGNMENT,rbobjectMesh,rbobjectMesh());

	// add the new object to the global list
	AddRBObject(newobject);
	// add the new object to the rbobjectMesh list
	h_rbobjectMeshes->Add(newobject);
	
	// Load in the model data --> Will also initialize all physics collision data
	bool moveOriginToCOM = true;
	newobject->LoadData(curToken->GetElem(1), std::wstring(curToken->GetElem(2)), g_renderer->GetD3DDev(), moveOriginToCOM);			// model_name	

	// Get RBO starting parameters from token
	float scale = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(4)));								// scale
	D3DXVECTOR3 pos = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(5))),				// Xpos
								  stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(6))),				// Ypos
								  stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(7))));			// Zpos
	D3DXVECTOR3 Raxis = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(8))),			// XrotAxis
									stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(9))),			// YrotAxis
									stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(10))));			// ZrotAxis
	float rot_rad = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(11))) * (2.0f*D3DX_PI)/360.0f;	// rotDeg
	D3DXVECTOR3 Lmom = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(12))),			// XLinMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(13))),			// YLinMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(14))));			// ZLinMom
	D3DXVECTOR3 Amom = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(15))),			// XAngMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(16))),			// YAngMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(17))));			// ZAngMom

	// Set RBO starting parameters
	newobject->SetPosition( pos );
	newobject->SetScale(scale);
	newobject->SetLinearMomentum(Lmom);
	newobject->SetAngularMomentum(Amom);
	D3DXQUATERNION temp_quat; 
	D3DXQuaternionRotationAxis(&temp_quat, &Raxis, rot_rad); // rotDeg
	newobject->SetRotation(temp_quat);

	// Set if movable or not
	if(stringUtil::NumFromString<int>(std::wstring(curToken->GetElem(3))) == 1)
		newobject->immovable = true;
	else
		newobject->immovable = false;
}

/************************************************************************/
/* Name:		ProcessTokenrbobjectSphere								*/
/* Description:	Create a new rbobjectMesh object with the given parameters	*/
//, rbobjectSphere, radius, numStacks, numSlices, < XCDiffuse, YCDiffuse, ZCDiffuse, WCDiffuse, > immovable?,  < Xpos,  Ypos,  Zpos, >  < XrotAxis, YrotAxis, ZrotAxis, >  rotDeg,  < XLinMom, YLinMom, ZLinMom, >  < XAngMom, YAngMom, ZAngMom, >
//               0,      1,         2,         3,           4,         5,         6,         7,            8,       9,    10,    11,            12,       14,       14,        15,         16,      17,      18,           19,      20,      21,
/************************************************************************/
void objectManager::ProcessTokenrbobjectSphere(vec_ptrs<wchar_t> * curToken, std::wstring * fileName )
{
	if(curToken->Size() != RBOBJECTSPHERE_NUM_TOKENS)
	{
		std::wstring error = L"objectManager::ProcessTokenrbobjectSphere() - Incorrect # tokens: '" + 
						     csvHandle::FlattenToken(curToken) + 
							 L"' in file: '" +
							 *fileName + 
							 L"'";
		throw std::runtime_error(stringUtil::toNarrowString(error.c_str(),-1));
	}

	// Make space for the new object
	rbobjectMesh * newobject = NULL;
	aligned_new_constructor(newobject,DATA_ALIGNMENT,rbobjectMesh,rbobjectMesh());

	// add the new object to the global list
	AddRBObject(newobject);
	// add the new object to the rbobjectMesh list
	h_rbobjectMeshes->Add(newobject);

	float radius = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(1)));							// radius
	UINT numStacks = stringUtil::NumFromString<UINT>(std::wstring(curToken->GetElem(2)));							// numStacks
	UINT numSlices = stringUtil::NumFromString<UINT>(std::wstring(curToken->GetElem(3)));							// numSlices
	D3DXCOLOR CDiffuse = D3DXCOLOR(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(4))),			// XCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(5))),			// YCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(6))),			// ZCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(7))));			// WCDiffuse
	sphere * newSphere = new sphere(numStacks, numSlices, radius, CDiffuse, NULL);
	
	// Load in the model data --> Will also initialize all physics collision data
	bool moveOriginToCOM = true;
	// Make the object name unique by assigning a string containing the next position in the global data
	std::wstring name = stringUtil::StringFromNum<UINT>(h_rbobjectMeshes->Size() - 1);
	std::wstring path_name = std::wstring(L"models\\sphere") + name;
	newobject->LoadDataFromBuffers(path_name.c_str(), newSphere->GetPosBuff(), newSphere->GetNormBuff(), newSphere->GetNumVert(), newSphere->GetIndBuff(), newSphere->GetNumInd(), newSphere->GetMtrl(), moveOriginToCOM);
	
	// Get RBO starting parameters from token
	float scale = 1.0f;																								// scale
	D3DXVECTOR3 pos = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(9))),				// Xpos
								  stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(10))),			// Ypos
								  stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(11))));			// Zpos
	D3DXVECTOR3 Raxis = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(12))),			// XrotAxis
									stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(13))),			// YrotAxis
									stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(14))));			// ZrotAxis
	float rot_rad = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(15))) * (2.0f*D3DX_PI)/360.0f;	// rotDeg
	D3DXVECTOR3 Lmom = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(16))),			// XLinMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(17))),			// YLinMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(18))));			// ZLinMom
	D3DXVECTOR3 Amom = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(19))),			// XAngMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(20))),			// YAngMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(21))));			// ZAngMom

	// Set RBO starting parameters
	newobject->SetPosition( pos );
	newobject->SetScale(scale);
	newobject->SetLinearMomentum(Lmom);
	newobject->SetAngularMomentum(Amom);
	D3DXQUATERNION temp_quat; 
	D3DXQuaternionRotationAxis(&temp_quat, &Raxis, rot_rad); // rotDeg
	newobject->SetRotation(temp_quat);

	// Set if movable or not
	if(stringUtil::NumFromString<int>(std::wstring(curToken->GetElem(8))) == 1)
		newobject->immovable = true;
	else
		newobject->immovable = false;

	delete newSphere;
}

/************************************************************************/
/* Name:		ProcessTokenrbobjectCone								*/
/* Description:	Create a new rbobjectMesh object with the given parameters	*/
//, rbobjectCone, radius, numVert, height, < XCDiffuse, YCDiffuse, ZCDiffuse, WCDiffuse, > immovable?,  < Xpos,  Ypos,  Zpos, >  < XrotAxis, YrotAxis, ZrotAxis, >  rotDeg,  < XLinMom, YLinMom, ZLinMom, >  < XAngMom, YAngMom, ZAngMom, >
//             0,      1,       2,      3,           4,         5,         6,         7,            8,       9,    10,    11,            12,       14,       14,        15,         16,      17,      18,           19,      20,      21,
/************************************************************************/
void objectManager::ProcessTokenrbobjectCone(vec_ptrs<wchar_t> * curToken, std::wstring * fileName )
{
	if(curToken->Size() != RBOBJECTCONE_NUM_TOKENS)
	{
		std::wstring error = L"objectManager::ProcessTokenrbobjectCone() - Incorrect # tokens: '" + 
						     csvHandle::FlattenToken(curToken) + 
							 L"' in file: '" +
							 *fileName + 
							 L"'";
		throw std::runtime_error(stringUtil::toNarrowString(error.c_str(),-1));
	}

	// Make space for the new object
	rbobjectMesh * newobject = NULL;
	aligned_new_constructor(newobject,DATA_ALIGNMENT,rbobjectMesh,rbobjectMesh());

	// add the new object to the global list
	AddRBObject(newobject);
	// add the new object to the rbobjectMesh list
	h_rbobjectMeshes->Add(newobject);

	float radius = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(1)));							// radius
	UINT numVert = stringUtil::NumFromString<UINT>(std::wstring(curToken->GetElem(2)));								// numStacks
	float height = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(3)));							// numSlices
	D3DXCOLOR CDiffuse = D3DXCOLOR(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(4))),			// XCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(5))),			// YCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(6))),			// ZCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(7))));			// WCDiffuse
	cone * newCone = new cone(numVert, height, radius, CDiffuse, NULL);
	
	// Load in the model data --> Will also initialize all physics collision data
	bool moveOriginToCOM = true;
	// Make the object name unique by assigning a string containing the next position in the global data
	std::wstring name = stringUtil::StringFromNum<UINT>(h_rbobjectMeshes->Size() - 1);
	std::wstring path_name = std::wstring(L"models\\cone") + name;
	newobject->LoadDataFromBuffers(path_name.c_str(), newCone->GetPosBuff(), newCone->GetNormBuff(), newCone->GetNumVert(), newCone->GetIndBuff(), newCone->GetNumInd(), newCone->GetMtrl(), moveOriginToCOM);
	
	// Get RBO starting parameters from token
	float scale = 1.0f;																								// scale
	D3DXVECTOR3 pos = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(9))),				// Xpos
								  stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(10))),			// Ypos
								  stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(11))));			// Zpos
	D3DXVECTOR3 Raxis = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(12))),			// XrotAxis
									stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(13))),			// YrotAxis
									stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(14))));			// ZrotAxis
	float rot_rad = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(15))) * (2.0f*D3DX_PI)/360.0f;	// rotDeg
	D3DXVECTOR3 Lmom = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(16))),			// XLinMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(17))),			// YLinMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(18))));			// ZLinMom
	D3DXVECTOR3 Amom = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(19))),			// XAngMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(20))),			// YAngMom
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(21))));			// ZAngMom

	// Set RBO starting parameters
	newobject->SetPosition( pos );
	newobject->SetScale(scale);
	newobject->SetLinearMomentum(Lmom);
	newobject->SetAngularMomentum(Amom);
	D3DXQUATERNION temp_quat; 
	D3DXQuaternionRotationAxis(&temp_quat, &Raxis, rot_rad); // rotDeg
	newobject->SetRotation(temp_quat);

	// Set if movable or not
	if(stringUtil::NumFromString<int>(std::wstring(curToken->GetElem(8))) == 1)
		newobject->immovable = true;
	else
		newobject->immovable = false;

	delete newCone;
}

/************************************************************************/
/* Name:		ProcessTokenlightSpotSM									*/
/* Description:	Create a new rbobjectMesh object with the given parameters	*/
//, lightSpotSM,  <  Xpos,   Ypos,   Zpos, >  <  Xdir,   Ydir,   Zdir, >  fov, near,   far,  lightIntensity, < XCDiffuse, YCDiffuse, ZCDiffuse, WCDiffuse, > specIntensity
//            0,        1,      2,      3,          4,      5,      6,      7,    8,     9,              10,          11,        12,        13,        14,              15
/************************************************************************/
void objectManager::ProcessTokenlightSpotSM(vec_ptrs<wchar_t> * curToken, std::wstring * fileName )
{
	if(curToken->Size() != LIGHTSPOTSM_NUM_TOKENS)
	{
		std::wstring error = L"objectManager::ProcessTokenlightSpotSM() - Incorrect # tokens: '" + 
						     csvHandle::FlattenToken(curToken) + 
							 L"' in file: '" +
							 *fileName + 
							 L"'";
		throw std::runtime_error(stringUtil::toNarrowString(error.c_str(),-1));
	}

	// Make space for the new object
	if(h_lightSpotSM != NULL)
		throw std::runtime_error("objectManager::ProcessTokenlightSpotSM(): Error: Only one shadow caster is allowed.  Check Level.csv");
	else
		h_lightSpotSM = new lightSpotSM;

	// Get Light parameters from token
	D3DXVECTOR3 posWorld = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(1))),		// Xpos
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(2))),		// Ypos
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(3))));		// Zpos
	D3DXVECTOR3 dirWorld = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(4))),		// Xdir
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(5))),		// Ydir
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(6))));		// Zdir
	float fov_rad = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(7))) * (2.0f*D3DX_PI)/360.0f;	// FOV_deg
	float light_near = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(8)));						// near
	float light_far = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(9)));							// far
	float lightIntensity = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(10)));					// power
	D3DXCOLOR CDiffuse = D3DXCOLOR(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(11))),			// XCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(12))),			// YCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(13))),			// ZCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(14))));			// WCDiffuse
	float specIntensity = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(15)));					// specIntensity

	// Set Light Parameters
	h_lightSpotSM->SetLightColor(CDiffuse, lightIntensity, light_near, light_far, specIntensity);
	h_lightSpotSM->SetLightDir(dirWorld);
	h_lightSpotSM->SetLightFOV(fov_rad);
	h_lightSpotSM->SetLightPos(posWorld);
	h_lightSpotSM->InitLightSpotSM();
}

/************************************************************************/
/* Name:		ProcessTokenLightSpot									*/
/* Description:	Create a new rbobjectMesh object with the given parameters	*/
//, lightSpot,  <  Xpos,   Ypos,   Zpos, >  <  Xdir,   Ydir,   Zdir, >  fov, near,   far,  lightIntensity, < XCDiffuse, YCDiffuse, ZCDiffuse, WCDiffuse, > specIntensity
//          0,        1,      2,      3,          4,      5,      6,      7,    8,     9,              10,          11,        12,        13,        14,              15
/************************************************************************/
void objectManager::ProcessTokenLightSpot(vec_ptrs<wchar_t> * curToken, std::wstring * fileName )
{
	if(curToken->Size() != LIGHTSPOT_NUM_TOKENS)
	{
		std::wstring error = L"objectManager::ProcessTokenLightSpot() - Incorrect # tokens: '" + 
						     csvHandle::FlattenToken(curToken) + 
							 L"' in file: '" +
							 *fileName + 
							 L"'";
		throw std::runtime_error(stringUtil::toNarrowString(error.c_str(),-1));
	}

	// Make space for the new object
	light * newObj = new lightSpot;

	// Get Light parameters from token
	D3DXVECTOR3 posWorld = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(1))),		// Xpos
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(2))),		// Ypos
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(3))));		// Zpos
	D3DXVECTOR3 dirWorld = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(4))),		// Xdir
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(5))),		// Ydir
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(6))));		// Zdir
	float fov_rad = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(7))) * (2.0f*D3DX_PI)/360.0f;	// FOV_deg
	float light_near = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(8)));						// near
	float light_far = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(9)));							// far
	float lightIntensity = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(10)));					// power
	D3DXCOLOR CDiffuse = D3DXCOLOR(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(11))),			// XCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(12))),			// YCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(13))),			// ZCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(14))));			// WCDiffuse
	float specIntensity = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(15)));					// specIntensity

	// Set Light Parameters
	newObj->SetLightColor(CDiffuse, lightIntensity, light_near, light_far, specIntensity);
	newObj->SetLightDir(dirWorld);
	newObj->SetLightFOV(fov_rad);
	newObj->SetLightPos(posWorld);

	// Finally add the new object to the global list
	AddLight(newObj);
}

/************************************************************************/
/* Name:		ProcessTokenLightDir									*/
/* Description:	Create a new rbobjectMesh object with the given parameters	*/
//, lightDir, <  Xdir,   Ydir,   Zdir, >  near,   far,  lightIntensity, < XCDiffuse, YCDiffuse, ZCDiffuse, WCDiffuse, > specIntensity
//         0,       1,      2,      3,       4,     5,               6,           7,         8,         9,        10,              11
/************************************************************************/
void objectManager::ProcessTokenLightDir(vec_ptrs<wchar_t> * curToken, std::wstring * fileName )
{
	if(curToken->Size() != LIGHTDIR_NUM_TOKENS)
	{
		std::wstring error = L"objectManager::ProcessTokenLightDir() - Incorrect # tokens: '" + 
						     csvHandle::FlattenToken(curToken) + 
							 L"' in file: '" +
							 *fileName + 
							 L"'";
		throw std::runtime_error(stringUtil::toNarrowString(error.c_str(),-1));
	}

	// Make space for the new object
	light * newObj = new lightDir;

	// Get Light parameters from token
	D3DXVECTOR3 dirWorld = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(1))),		// Xdir
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(2))),		// Ydir
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(3))));		// Zdir
	float light_near = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(4)));						// near
	float light_far = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(5)));							// far
	float lightIntensity = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(6)));					// power
	D3DXCOLOR CDiffuse = D3DXCOLOR(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(7))),			// XCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(8))),			// YCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(9))),			// ZCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(10))));			// WCDiffuse
	float specIntensity = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(11)));					// specIntensity

	// Set Light Parameters
	newObj->SetLightColor(CDiffuse, lightIntensity, light_near, light_far, specIntensity);
	newObj->SetLightDir(dirWorld);

	// Finally add the new object to the global list
	AddLight(newObj);
}

/************************************************************************/
/* Name:		ProcessTokenLightPoint									*/
/* Description:	Create a new rbobjectMesh object with the given parameters	*/
//, lightPoint, <  Xpos,   Ypos,   Zpos, >  near,   far,  lightIntensity, < XCDiffuse, YCDiffuse, ZCDiffuse, WCDiffuse, > specIntensity
//           0,       1,      2,      3,       4,     5,               6,           7,         8,         9,        10,              11
/************************************************************************/
void objectManager::ProcessTokenLightPoint(vec_ptrs<wchar_t> * curToken, std::wstring * fileName )
{
	if(curToken->Size() != LIGHTPOINT_NUM_TOKENS)
	{
		std::wstring error = L"objectManager::ProcessTokenLightPoint() - Incorrect # tokens: '" + 
						     csvHandle::FlattenToken(curToken) + 
							 L"' in file: '" +
							 *fileName + 
							 L"'";
		throw std::runtime_error(stringUtil::toNarrowString(error.c_str(),-1));
	}

	// Make space for the new object
	light * newObj = new lightPoint;

	// Get Light parameters from token
	D3DXVECTOR3 posWorld = D3DXVECTOR3(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(1))),		// Xpos
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(2))),		// Ypos
									   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(3))));		// Zpos
	float light_near = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(4)));						// near
	float light_far = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(5)));							// far
	float lightIntensity = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(6)));					// power
	D3DXCOLOR CDiffuse = D3DXCOLOR(stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(7))),			// XCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(8))),			// YCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(9))),			// ZCDiffuse
								   stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(10))));			// WCDiffuse
	float specIntensity = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(11)));					// specIntensity

	// Set Light Parameters
	newObj->SetLightColor(CDiffuse, lightIntensity, light_near, light_far, specIntensity);
	newObj->SetLightPos(posWorld);

	// Finally add the new object to the global list
	AddLight(newObj);
}

/************************************************************************/
/* Name:		ProcessTokenSky											*/
/* Description:	Create a new rbobjectMesh object with the given parameters	*/
// Token Format: sky,       sky_text_file,    radius
//                 0,                   1,         2
/************************************************************************/
void objectManager::ProcessTokenSky(vec_ptrs<wchar_t> * curToken, std::wstring * fileName )
{
	if(curToken->Size() != SKY_NUM_TOKENS)
	{
		std::wstring error = L"objectManager::ProcessTokenSky() - Incorrect # tokens: '" + 
						     csvHandle::FlattenToken(curToken) + 
							 L"' in file: '" +
							 *fileName + 
							 L"'";
		throw std::runtime_error(stringUtil::toNarrowString(error.c_str(),-1));
	}

	if(h_sky != NULL)
		throw std::runtime_error("objectManager::ProcessTokenSky() - ERROR: Trying to load more than one sky");

	// Make space for the new object
	h_sky =  new sky;

	// Get sky parameters from token
	std::wstring skyName = std::wstring(curToken->GetElem(1)) + L".dds";
	float skyRadius = stringUtil::NumFromString<float>(std::wstring(curToken->GetElem(2)));

	// Initialize the sky
	h_sky->InitSky(skyName, skyRadius);
}
