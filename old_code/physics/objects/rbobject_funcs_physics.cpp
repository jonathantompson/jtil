/*************************************************************
**					Rigid Body Object						**
**			-> Store object states, Summer 2009				**
*************************************************************/
// File:		rbobject_FuncsPhysics.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Physics functions for rbobject class -> simplifies rbobject.cpp for compilation with other projects

#include	"physics\objects\rbobject.h"
#include	"objectManager\objectManager.h"
#include	"utils_and_misc_classes\math\double3x3.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#define EPSILON						0.00000001 // A small value (with some margin)

/************************************************************************/
/* Name:		UpdateBoundingBox										*/
/* Description:	Update bounding box bounds for changing world conditions*/
/************************************************************************/
void rbobject::UpdateBoundingBox()
{
	//http://www.toymaker.info/Games/html/collisions.html
	UpdateMatricies();
	// Get bounding box in world coordinates
	for( int i = 0; i < 8; i++ )
		D3DXVec3TransformCoord( &m_worldBounds[i], &m_objectBounds[i], &matWorld );

	// Now get bounding box in axis aligned world coordinates --> Box area will grow.
	AABBox.min.x = m_worldBounds[0].x; AABBox.min.y = m_worldBounds[0].y; AABBox.min.z = m_worldBounds[0].z;
	AABBox.max.x = m_worldBounds[0].x; AABBox.max.y = m_worldBounds[0].y; AABBox.max.z = m_worldBounds[0].z;
	// Due to rotations, don't know which verticies will give minimum and maximum
	for (int i=1;i<8;i++)
		AABBox.Expand(&m_worldBounds[i]);
}
/************************************************************************/
/* Name:		GetItensorBox	OLD CODE!!!								*/
/* Description:	Calculate tensor matrix from bouning box coordinates	*/
/*		Function assumes that box is symmetric about axes (ie, that 	*/
/*		model and bounding boxes have been normalized to center of mass)*/
/************************************************************************/
void rbobject::GetItensorBox(D3DXVECTOR3 *maxBound, D3DXMATRIXA16 *Ib, D3DXMATRIXA16 *Ib_inv)
{
	// Page D26 of Baraff and Witkin: Physically Based Modelling Notes
	float massScale = mass / 12;
	// Ixx = M/12 * (y0^2 + z0^2) --> where y0 and z0 are length of the cube sides
	Ib->_11 = massScale * 4*(maxBound->y)*(maxBound->y) + 4*(maxBound->z)*(maxBound->z);
	// Iyy = M/12 * (x0^2 + z0^2)
	Ib->_22 = massScale * 4*(maxBound->x)*(maxBound->x) + 4*(maxBound->z)*(maxBound->z);
	// Izz = M/12 * (x0^2 + y0^2)
	Ib->_33 = massScale * 4*(maxBound->x)*(maxBound->x) + 4*(maxBound->y)*(maxBound->y);
	Ib->_12 = 0; Ib->_13 = 0; Ib->_14 = 0;
	Ib->_21 = 0; Ib->_23 = 0; Ib->_24 = 0;
	Ib->_31 = 0; Ib->_32 = 0; Ib->_34 = 0;
	Ib->_41 = 0; Ib->_42 = 0; Ib->_43 = 0; Ib->_44 = 1;

	// Inverse of a diagonal matrix is just inverse of diagonal elements
	*Ib_inv = *Ib;
	Ib_inv->_11 =  1.0f/Ib_inv->_11; Ib_inv->_22 =  1.0f/Ib_inv->_22; Ib_inv->_33 =  1.0f/Ib_inv->_33;
}
/************************************************************************/
/* Name:		GetItensor												*/
/* Description:	Wrapper function to use Stan Melax's code.				*/
/*		Assume verticies normalized so that 0,0,0 is center of mass.	*/
/************************************************************************/
void rbobject::GetItensor(double3 * verts, int3 * ind, DWORD numFaces, D3DXMATRIXA16 *Ib, D3DXMATRIXA16 *Ib_inv)
{
	D3DXMatrixIdentity(Ib); D3DXMatrixIdentity(Ib_inv); 
	//float3x3 Inertia(const float3 *vertices, const int3 *tris, consts int count, const float3& com=float3(0,0,0))  
	double3x3 _Ib = Inertia(verts, ind, numFaces); // center of mass input is default (ie, <0,0,0>)
	// S Melax: "All you have to do is multiply the resulting matrix by your objects mass to scale it appropriately."
	_Ib.x.x *=  mass; _Ib.x.y *=  mass; _Ib.x.z *=  mass;
	_Ib.y.x *=  mass; _Ib.y.y *=  mass; _Ib.y.z *=  mass;
	_Ib.z.x *=  mass; _Ib.z.y *=  mass; _Ib.z.z *=  mass;
	// Find inverse of matrix
	double3x3 _Ib_inv = Inverse(_Ib);
	// set the input matricies to the correct values
	Ib->_11 = (float)_Ib.x.x; Ib->_12 = (float)_Ib.x.y;  Ib->_13 = (float)_Ib.x.z; 
	Ib->_21 = (float)_Ib.y.x; Ib->_22 = (float)_Ib.y.y;  Ib->_23 = (float)_Ib.y.z; 
	Ib->_31 = (float)_Ib.z.x; Ib->_32 = (float)_Ib.z.y;  Ib->_33 = (float)_Ib.z.z; 
	Ib_inv->_11 = (float)_Ib_inv.x.x; Ib_inv->_12 = (float)_Ib_inv.x.y;  Ib_inv->_13 = (float)_Ib_inv.x.z; 
	Ib_inv->_21 = (float)_Ib_inv.y.x; Ib_inv->_22 = (float)_Ib_inv.y.y;  Ib_inv->_23 = (float)_Ib_inv.y.z; 
	Ib_inv->_31 = (float)_Ib_inv.z.x; Ib_inv->_32 = (float)_Ib_inv.z.y;  Ib_inv->_33 = (float)_Ib_inv.z.z; 
}

/************************************************************************/
/* Name:		StateToArray											*/
/* Description:	Given a rbobject, copy state into array y				*/
/************************************************************************/
void rbobject::StateToArray(rbobject * rb, float *y)
{
	// Assumes memory has been allocated in y!!!
	// STORE POSITION
	*y++ = rb->x.x; *y++ = rb->x.y; *y++ = rb->x.z;
	// STORE QUATERNION
	*y++ = rb->q.w; *y++ = rb->q.x; *y++ = rb->q.y; *y++ = rb->q.z;
	// STORE LINEAR MOMENTUM
	*y++ = rb->P.x; *y++ = rb->P.y; *y++ = rb->P.z;
	// STORE ANGULAR MOMENTUM
	*y++ = rb->L.x; *y++ = rb->L.y; *y++ = rb->L.z;
}
/************************************************************************/
/* Name:		ArrayToState											*/
/* Description:	Given an array y, copy state into rbobject				*/
/************************************************************************/
void rbobject::ArrayToState(rbobject * rb, float *y)
{
	// LOAD POSITION
	rb->x.x = *y++; rb->x.y = *y++; rb->x.z = *y++;	
	// LOAD QUATERNION
	rb->q.w = *y++; rb->q.x = *y++; rb->q.y = *y++; rb->q.z = *y++;	
	// LOAD LINEAR MOMENTUM
	rb->P.x = *y++; rb->P.y = *y++; rb->P.z = *y++;	
	// LOAD ANGULAR MOMENTUM
	rb->L.x = *y++; rb->L.y = *y++; rb->L.z = *y++;	
	
	// Compute other variables
	rb->v = rb->P / rb->mass;

	if(abs(D3DXQuaternionLength(&rb->q))>EPSILON)
		D3DXQuaternionNormalize(&rb->q, &rb->q);
	D3DXMatrixRotationQuaternion(&rb->R, &rb->q);
	D3DXMatrixTranspose(&rb->R_t, &rb->R);
	rb->Iinv = rb->R * rb->Ibody_inv * rb->R_t;
	D3DXVec3TransformCoord(&rb->omega, &rb->L, &rb->Iinv); 
}
/************************************************************************/
/* Name:		ArrayToState											*/
/* Description:	Copy all rbobject states into array yout				*/
/************************************************************************/
void rbobject::ObjectsToState(float *yout) // Bodies_to_Array in Baraff and Witkin
{
	for(int i = 0; i < (int)g_objectManager->GetNumRBObjects(); i ++)
		StateToArray(g_objectManager->GetRBObject(i), &yout[i * STATESIZE]);
}
/************************************************************************/
/* Name:		StateToObjects											*/
/* Description:	Copy all rbobject states from array yout				*/
/************************************************************************/
void rbobject::StateToObjects(float *yin) // Array_to_Bodies in Baraff and Witkin
{
	for(int i = 0; i < (int)g_objectManager->GetNumRBObjects(); i ++)
		ArrayToState(g_objectManager->GetRBObject(i), &yin[i * STATESIZE]);
}