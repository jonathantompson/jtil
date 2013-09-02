/*************************************************************
**					Rigid Body Object						**
**			-> Store object states, Summer 2009				**
*************************************************************/
// File:		rbobject.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// http://eta.physics.uoguelph.ca/tutorials/torque/Q.torque.inertia.html
// & http://www.gamedev.net/community/forums/topic.asp?topic_id=57001

#include	"physics\objects\rbobject.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		rbobject												*/
/* Description: Default Constructor										*/
/************************************************************************/
rbobject::rbobject()
{
	// Zero all vectors and positions.  Make matricies identity matrix
	mass = 1.0f;
	D3DXMatrixIdentity(&Ibody);
	D3DXMatrixIdentity(&matWorldPrevFrame);
	D3DXMatrixIdentity(&Ibody_inv);
	x.x = 0; x.y = 0; x.z = 0; x_t1.x = 0; x_t1.y = 0; x_t1.z = 0; 
	P.x = 0; P.y = 0; P.z = 0; P_t1.x = 0; P_t1.y = 0; P_t1.z = 0;
	L.x = 0; L.y = 0; L.z = 0; L_t1.x = 0; L_t1.y = 0; L_t1.z = 0;
	
	//q.w = 0; q.x = 0; q.y = 1; q.z = 0;
	q.w = 1; q.x = 0; q.y = 0; q.z = 0;   // --> Force all objects to start with R = indentity matrix

	q_t1.w = 0; q_t1.x = 0; q_t1.y = 0; q_t1.z = 0;
	D3DXMatrixIdentity(&Iinv);
	scale = 1.0f;
	v.x = 0; v.y = 0; v.z = 0;
	omega.x = 0; omega.y = 0; omega.z = 0;
	force.x = 0; force.y = 0; force.z = 0;
	torque.x = 0; torque.y = 0; torque.z = 0;
	dirtyScaleMatrix = true; dirtyRotMatrix = true; dirtyTransMatrix = true;
	immovable = false;
	UpdateMatricies();
}

/************************************************************************/
/* Name:		UpdateMatricies											*/
/* Description:	Update matricies if required							*/
/************************************************************************/
void rbobject::UpdateMatricies()
{
	bool dirtyWorldMatrix = false;
	if(dirtyScaleMatrix)
	{
		D3DXMatrixScaling( &scaleMat, scale, scale, scale );
		dirtyScaleMatrix = false;
		dirtyWorldMatrix = true;
	}
	if(dirtyRotMatrix)
	{
		D3DXMatrixRotationQuaternion( &R , &q );
		dirtyRotMatrix = false;
		dirtyWorldMatrix = true;
	}
	if(dirtyTransMatrix)
	{
		D3DXMatrixTranslation( &Trans, x.x, x.y, x.z );
		dirtyTransMatrix = false;
		dirtyWorldMatrix = true;
	}
	if(dirtyWorldMatrix)
	{
		matWorld = scaleMat * R * Trans;
	}
}

/************************************************************************/
/* Name:		RotateObjectAxisAngle									*/
/* Description: Force a rotation mostly used when debugging				*/
/************************************************************************/
void rbobject::RotateObjectAxisAngle( D3DXVECTOR3 axis, float rotAngle )
{
	D3DXVec3Normalize(& axis, & axis);

	// First represent the axis angle rotation as a quaternion
	D3DXQUATERNION qrot;
	D3DXQuaternionRotationAxis(& qrot, & axis, rotAngle);

	// Now multiply the current orientation quaternion 
	D3DXQuaternionMultiply(& q, & q, & qrot);

	// Now make sure the world transform gets updated
	dirtyRotMatrix = true;
}
