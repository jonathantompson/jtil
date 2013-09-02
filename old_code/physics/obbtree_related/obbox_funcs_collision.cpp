// File:		obbox_FuncsCollision.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Collision Detection routines for obbox class

#include "physics\obbtree_related\obbox.h"
#include "physics\objects\rbobjectMesh.h"
#include "physics\objects\rbobjectMeshData.h"
#include "physics\objects\rbobject.h"
#include "physics\obbtree_related\obbtree.h"
#include "utils_and_misc_classes\SIMD_helpers\SIMDFuncs.h"
#include "utils_and_misc_classes\math\math_funcs.h"
#include "physics\obbtree_related\triTriIntersect.h"
#include "utils_and_misc_classes\math\double3x3.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#pragma warning( push )			// Edit Jonathan Tompson - 31st Jan 2011
#pragma warning( disable:4238 )	// Edit Jonathan Tompson - 31st Jan 2011

#define NULL 0

/************************************************************************/
/* Name:		TestOBBCollision										*/
/* Description: Test two OBB elements for collision using separating	*/
/*				axis test. Code borrowed and modified from page 104 of  */
/*				real time collision detection book.						*/
/************************************************************************/
int obbox::TestOBBCollision( obbox * a, rbobjectMesh * a_rbo, obbox * b, rbobjectMesh * b_rbo )
{
	// The following code is based on Christer Ericson's book, Real-time collision detection (pages 103-105)
	//http://books.google.com/books?id=WGpL6Sk9qNAC&lpg=PA101&ots=Pl4MjF4ciO&dq=Real-Time%20Collision%20Detection%20oriented%20bounding%20boxes&pg=PA101#v=onepage&q=Real-Time%20Collision%20Detection%20oriented%20bounding%20boxes&f=false
	// NOTE 1: RBO World matrix is defined as follows: WorldMatrix = M_scale * M_rot * M_translation
	//         OBBOX bounds need to be modified in the same way.  Also, M_rot needs to be M_rot^-1 = M_rot^t
	// NOTE 2: rbobject scale is only a scalar.  Using a vector (scaling in x, y, z directions separatly)
	//         would require updating scaling factor for all boxes --> expensive.
	// NOTE 3: the COLUMNS of orientMat are the axes of the OBB. ie, v1 = <x.x,y.x,z.x>, V2 = <x.y,y.y,z.y>, V2 = <x.z,y.z,z.z>

	// There is some matlab matlab code to verfify Matrix and Vector multiplication functions --> See: "MatlabTests.m"

	// Compute rotation matrix expressing b in a's coordinate frame.
	// Rotation composite from OBBOX to world coordinates
	// --> Both R_aOBB and R_world are [U,V,N] -> [X,Y,Z] transforms.  Therefore columns represent frame axes.
	MatrixMult(& obbox::R_aOBB_world_Tran, & a->orientMatrix, & a_rbo->R );
	MatrixMult_ThenTran(& obbox::R_bOBB_world, & b->orientMatrix, & b_rbo->R);

	// Move OBB center from object coords into world coords
	MatrixMult_ATran_B( & obbox::t_aOBB, & a_rbo->R, & (a->boxCenterObjectCoord * a_rbo->scale) ); //boxCenterObjectCoord needs to be scaled then rotated first
	MatrixMult_ATran_B( & obbox::t_bOBB, & b_rbo->R, & (b->boxCenterObjectCoord * b_rbo->scale) );

	// Add rbo position and OBB position in world coordinates
	obbox::t_a_world = a_rbo->x + obbox::t_aOBB;
	obbox::t_b_world = b_rbo->x + obbox::t_bOBB;

	// Calculate displacement vector in world coordinates
	obbox::t_world = obbox::t_b_world - obbox::t_a_world; // Vector from a to b

	// From here on is CHRISTER ERICSON'S ALGORITHM
	// Need A^Transpose * B, The following function does this in one step (saves compuation)
	MatrixMult( & obbox::R, & obbox::R_aOBB_world_Tran, & obbox::R_bOBB_world );

	// Bring translation vector into OBpB A's coordinates
	MatrixMult( & obbox::t_, & obbox::R_aOBB_world_Tran, & obbox::t_world );

	// Scale box dimensions
	obbox::a_OBBDim[0] = a->boxDimension[0] * a_rbo->scale;
	obbox::a_OBBDim[1] = a->boxDimension[1] * a_rbo->scale;
	obbox::a_OBBDim[2] = a->boxDimension[2] * a_rbo->scale;
	obbox::b_OBBDim[0] = b->boxDimension[0] * b_rbo->scale;
	obbox::b_OBBDim[1] = b->boxDimension[1] * b_rbo->scale;
	obbox::b_OBBDim[2] = b->boxDimension[2] * b_rbo->scale;

	// Compute common subexpressions. Add in an epsilon term to counteract arithmetic 
	// errors when two edges are parallel and their cross product is (near) null.
	for (int i = 0; i < 3; i ++ )
	{
		for (int j = 0; j < 3; j ++ )
		{
			// obbox::AbsR(i,j) = fabs(obbox::R(i,j)) + 0.000000000001f;
			// obbox::AbsR(i,j) = fabs_fast(obbox::R(i,j)) + 0.000000000001f;
			obbox::AbsR(i,j) = fabs_fast2(obbox::R(i,j)) + 0.000000000001f;
		}
	}

	// Test axes L = A0, L = A1, L = A2
	for (int i = 0; i < 3; i ++ )
	{
		obbox::ra = obbox::a_OBBDim[i];
		obbox::rb = (obbox::b_OBBDim[0] * obbox::AbsR(i,0)) + (obbox::b_OBBDim[1] * obbox::AbsR(i,1)) + (obbox::b_OBBDim[2] * obbox::AbsR(i,2));
		if (fabs(t_[i]) > (obbox::ra + obbox::rb)) return 0;
	}

	// Test axes L = B0, L = B1, L = B2
	for (int i = 0; i < 3; i ++ )
	{
		obbox::ra = (obbox::a_OBBDim[0] * obbox::AbsR(0,i)) + (obbox::a_OBBDim[1] * obbox::AbsR(1,i)) + (obbox::a_OBBDim[2] * obbox::AbsR(2,i));
		obbox::rb = obbox::b_OBBDim[i];
		if (fabs(obbox::t_[0] * obbox::R(0,i) + obbox::t_[1] * obbox::R(1,i) + obbox::t_[2] * obbox::R(2,i)) > (obbox::ra + obbox::rb)) return 0;
	}

	// NOTE: MANY ALGORITHMS STOP HERE SINCE THIS COVERS ~85% OF CASES (see pg 106 of real time collision detection book)
	// HOWEVER, I TESTED RETURNING HERE AND IT WAS DEFINITELY SLOWER (86FBS break early Vs. 108FPS full routine)

	// Test axis L = A0 x B0
	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,0) + obbox::a_OBBDim[2] * obbox::AbsR(1,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(0,2) + obbox::b_OBBDim[2] * obbox::AbsR(0,1);
	if (fabs(obbox::t_[2] * obbox::R(1,0) - obbox::t_[1] * obbox::R(2,0)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A0 x B1
	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,1) + obbox::a_OBBDim[2] * obbox::AbsR(1,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(0,2) + obbox::b_OBBDim[2] * obbox::AbsR(0,0);
	if (fabs(obbox::t_[2] * obbox::R(1,1) - obbox::t_[1] * obbox::R(2,1)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A0 x B2
	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,2) + obbox::a_OBBDim[2] * obbox::AbsR(1,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(0,1) + obbox::b_OBBDim[1] * obbox::AbsR(0,0);
	if (fabs(obbox::t_[2] * obbox::R(1,2) - obbox::t_[1] * obbox::R(2,2)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A1 x B0
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,0) + obbox::a_OBBDim[2] * obbox::AbsR(0,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(1,2) + obbox::b_OBBDim[2] * obbox::AbsR(1,1);
	if (fabs(obbox::t_[0] * obbox::R(2,0) - obbox::t_[2] * obbox::R(0,0)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A1 x B1
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,1) + obbox::a_OBBDim[2] * obbox::AbsR(0,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(1,2) + obbox::b_OBBDim[2] * obbox::AbsR(1,0);
	if (fabs(obbox::t_[0] * obbox::R(2,1) - obbox::t_[2] * obbox::R(0,1)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A1 x B2
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,2) + obbox::a_OBBDim[2] * obbox::AbsR(0,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(1,1) + obbox::b_OBBDim[1] * obbox::AbsR(1,0);
	if (fabs(obbox::t_[0] * obbox::R(2,2) - obbox::t_[2] * obbox::R(0,2)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A2 x B0
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,0) + obbox::a_OBBDim[1] * obbox::AbsR(0,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(2,2) + obbox::b_OBBDim[2] * obbox::AbsR(2,1);
	if (fabs(obbox::t_[1] * obbox::R(0,0) - obbox::t_[0] * obbox::R(1,0)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A2 x B1
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,1) + obbox::a_OBBDim[1] * obbox::AbsR(0,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(2,2) + obbox::b_OBBDim[2] * obbox::AbsR(2,0);
	if (fabs(obbox::t_[1] * obbox::R(0,1) - obbox::t_[0] * obbox::R(1,1)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A2 x B2
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,2) + obbox::a_OBBDim[1] * obbox::AbsR(0,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(2,1) + obbox::b_OBBDim[1] * obbox::AbsR(2,0);
	if (fabs(obbox::t_[1] * obbox::R(0,2) - obbox::t_[0] * obbox::R(1,2)) > (obbox::ra + obbox::rb)) return 0;

	// Since no separating axis is found, the OBBs must be intersecting
	return 1;
}
/************************************************************************/
/* Name:		TestOBBCollision_original -> Old version				*/
/************************************************************************/
int obbox::TestOBBCollision_original( obbox * a, rbobjectMesh * a_rbo, obbox * b, rbobjectMesh * b_rbo )
{
	// The following code is based on Christer Ericson's book, Real-time collision detection (pages 103-105)
    //http://books.google.com/books?id=WGpL6Sk9qNAC&lpg=PA101&ots=Pl4MjF4ciO&dq=Real-Time%20Collision%20Detection%20oriented%20bounding%20boxes&pg=PA101#v=onepage&q=Real-Time%20Collision%20Detection%20oriented%20bounding%20boxes&f=false
	// NOTE 1: RBO World matrix is defined as follows: WorldMatrix = M_scale * M_rot * M_translation
	//         OBBOX bounds need to be modified in the same way.  Also, M_rot needs to be M_rot^-1 = M_rot^t
    // NOTE 2: rbobject scale is only a scalar.  Using a vector (scaling in x, y, z directions separatly)
	//         would require updating scaling factor for all boxes --> expensive.
	// NOTE 3: the COLUMNS of orientMat are the axes of the OBB. ie, v1 = <x.x,y.x,z.x>, V2 = <x.y,y.y,z.y>, V2 = <x.z,y.z,z.z>

	if(a->isLeaf && b->isLeaf)
		return TestTriCollision(a, a_rbo, b, b_rbo);

	// There is some matlab matlab code to verfify Matrix and Vector multiplication functions --> See: "MatlabTests.m"

	// Compute rotation matrix expressing b in a's coordinate frame.
	// static D3DXMATRIX R, R_aOBB_world, R_bOBB_world;		--> OBBOX STATIC ELEMENTS
	// Rotation composite from OBBOX to world coordinates
	// --> Both R_aOBB and R_world are [U,V,N] -> [X,Y,Z] transforms.  Therefore columns represent frame axes.
	MatrixMult_ATran_B(& obbox::R_aOBB_world_Tran, & a_rbo->R, & a->orientMatrix); 
	MatrixMult_ATran_B(& obbox::R_bOBB_world, & b_rbo->R, & b->orientMatrix);

	// Move OBB center from object coords into world coords
	// static D3DXVECTOR3 t_aOBB, t_bOBB;					--> OBBOX STATIC ELEMENTS
	MatrixMult_ATran_B( & obbox::t_aOBB, & a_rbo->R, & (a->boxCenterObjectCoord * a_rbo->scale) ); //boxCenterObjectCoord needs to be scaled then rotated first
	MatrixMult_ATran_B( & obbox::t_bOBB, & b_rbo->R, & (b->boxCenterObjectCoord * b_rbo->scale) );

	// Add rbo position and OBB position in world coordinates
	// static D3DXVECTOR3 t_a_world, t_b_world;				--> OBBOX STATIC ELEMENTS
	obbox::t_a_world = a_rbo->x + obbox::t_aOBB;
	obbox::t_b_world = b_rbo->x + obbox::t_bOBB;

	// From here on is CHRISTER ERICSON'S ALGORITHM
	// Need A^Transpose * B, The following function does this in one step (saves compuation)
	MatrixMult_ATran_B( & obbox::R, & obbox::R_aOBB_world_Tran, & obbox::R_bOBB_world );

	// Calculate displacement vector in world coordinates
	// static D3DXVECTOR3 t_world;							--> OBBOX STATIC ELEMENTS
	obbox::t_world = obbox::t_b_world - obbox::t_a_world; // Vector from a to b

	// Bring translation vector into OBpB A's coordinates
	// static D3DXVECTOR3 t;								--> OBBOX STATIC ELEMENTS
	MatrixMult_ATran_B( & obbox::t_, & obbox::R_aOBB_world_Tran, & obbox::t_world );

	// Scale box dimensions
	// static float a_OBBDim[3];							--> OBBOX STATIC ELEMENTS
	for(int i = 0; i < 3; i ++) {obbox::a_OBBDim[i] = a->boxDimension[i] * a_rbo->scale; }
	// static float b_OBBDim[3];							--> OBBOX STATIC ELEMENTS
	for(int i = 0; i < 3; i ++) {obbox::b_OBBDim[i] = b->boxDimension[i] * b_rbo->scale; }

	// Compute common subexpressions. Add in an epsilon term to counteract arithmetic 
	// errors when two edges are parallel and their cross product is (near) null.
	// static D3DXMATRIX AbsR;								--> OBBOX STATIC ELEMENTS
	for (int i = 0; i < 3; i ++ )
		for (int j = 0; j < 3; j ++ )
		{
//			obbox::AbsR(i,j) = fabs(obbox::R(i,j)) + 0.000000000001f;
//			obbox::AbsR(i,j) = fabs_fast(obbox::R(i,j)) + 0.000000000001f;
			obbox::AbsR(i,j) = fabs_fast2(obbox::R(i,j)) + 0.000000000001f;
		}

	// static float ra, rb;									--> OBBOX STATIC ELEMENTS
	// Test axes L = A0, L = A1, L = A2
	for (int i = 0; i < 3; i ++ )
	{
		obbox::ra = obbox::a_OBBDim[i];
		obbox::rb = (obbox::b_OBBDim[0] * obbox::AbsR(i,0)) + (obbox::b_OBBDim[1] * obbox::AbsR(i,1)) + (obbox::b_OBBDim[2] * obbox::AbsR(i,2));
		if (fabs(t_[i]) > (obbox::ra + obbox::rb)) return 0;
	}

	// Test axes L = B0, L = B1, L = B2
	for (int i = 0; i < 3; i ++ )
	{
		obbox::ra = (obbox::a_OBBDim[0] * obbox::AbsR(0,i)) + (obbox::a_OBBDim[1] * obbox::AbsR(1,i)) + (obbox::a_OBBDim[2] * obbox::AbsR(2,i));
		obbox::rb = obbox::b_OBBDim[i];
		if (fabs(obbox::t_[0] * obbox::R(0,i) + obbox::t_[1] * obbox::R(1,i) + obbox::t_[2] * obbox::R(2,i)) > (obbox::ra + obbox::rb)) return 0;
	}

	// NOTE: MANY ALGORITHMS STOP HERE SINCE THIS COVERS ~85% OF CASES (see pg 106 of real time collision detection book)
	// HOWEVER, I TESTED RETURNING HERE AND IT WAS DEFINITELY SLOWER (86FBS break early Vs. 108FPS full routine)

	// Test axis L = A0 x B0
	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,0) + obbox::a_OBBDim[2] * obbox::AbsR(1,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(0,2) + obbox::b_OBBDim[2] * obbox::AbsR(0,1);
	if (fabs(obbox::t_[2] * obbox::R(1,0) - obbox::t_[1] * obbox::R(2,0)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A0 x B1
	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,1) + obbox::a_OBBDim[2] * obbox::AbsR(1,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(0,2) + obbox::b_OBBDim[2] * obbox::AbsR(0,0);
	if (fabs(obbox::t_[2] * obbox::R(1,1) - obbox::t_[1] * obbox::R(2,1)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A0 x B2
	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,2) + obbox::a_OBBDim[2] * obbox::AbsR(1,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(0,1) + obbox::b_OBBDim[1] * obbox::AbsR(0,0);
	if (fabs(obbox::t_[2] * obbox::R(1,2) - obbox::t_[1] * obbox::R(2,2)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A1 x B0
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,0) + obbox::a_OBBDim[2] * obbox::AbsR(0,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(1,2) + obbox::b_OBBDim[2] * obbox::AbsR(1,1);
	if (fabs(obbox::t_[0] * obbox::R(2,0) - obbox::t_[2] * obbox::R(0,0)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A1 x B1
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,1) + obbox::a_OBBDim[2] * obbox::AbsR(0,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(1,2) + obbox::b_OBBDim[2] * obbox::AbsR(1,0);
	if (fabs(obbox::t_[0] * obbox::R(2,1) - obbox::t_[2] * obbox::R(0,1)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A1 x B2
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,2) + obbox::a_OBBDim[2] * obbox::AbsR(0,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(1,1) + obbox::b_OBBDim[1] * obbox::AbsR(1,0);
	if (fabs(obbox::t_[0] * obbox::R(2,2) - obbox::t_[2] * obbox::R(0,2)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A2 x B0
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,0) + obbox::a_OBBDim[1] * obbox::AbsR(0,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(2,2) + obbox::b_OBBDim[2] * obbox::AbsR(2,1);
	if (fabs(obbox::t_[1] * obbox::R(0,0) - obbox::t_[0] * obbox::R(1,0)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A2 x B1
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,1) + obbox::a_OBBDim[1] * obbox::AbsR(0,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(2,2) + obbox::b_OBBDim[2] * obbox::AbsR(2,0);
	if (fabs(obbox::t_[1] * obbox::R(0,1) - obbox::t_[0] * obbox::R(1,1)) > (obbox::ra + obbox::rb)) return 0;

	// Test axis L = A2 x B2
	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,2) + obbox::a_OBBDim[1] * obbox::AbsR(0,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(2,1) + obbox::b_OBBDim[1] * obbox::AbsR(2,0);
	if (fabs(obbox::t_[1] * obbox::R(0,2) - obbox::t_[0] * obbox::R(1,2)) > (obbox::ra + obbox::rb)) return 0;

	// Since no separating axis is found, the OBBs must be intersecting
	return 1;

}

/************************************************************************/
/* Name:		TestOBBCollision_DirectX								*/
/* Description: Use only DirectX functions (slower)						*/
/************************************************************************/
int obbox::TestOBBCollision_DirectX( obbox * a, rbobjectMesh * a_rbo, obbox * b, rbobjectMesh * b_rbo )
{
	D3DXMatrixMultiply(& obbox::R_aOBB_world_Tran, & a->orientMatrix, & a_rbo->R );
	D3DXVec3TransformCoord(& obbox::t_aOBB, & (a->boxCenterObjectCoord * a_rbo->scale), & a_rbo->R );

	D3DXMatrixMultiply(& obbox::R, & b->orientMatrix, & b_rbo->R);
	D3DXMatrixTranspose(& obbox::R_bOBB_world, & obbox::R);
	D3DXVec3TransformCoord(& obbox::t_bOBB, & (b->boxCenterObjectCoord * b_rbo->scale), & b_rbo->R );

	obbox::t_a_world = a_rbo->x + obbox::t_aOBB;
	obbox::t_b_world = b_rbo->x + obbox::t_bOBB;

	obbox::t_world = obbox::t_b_world - obbox::t_a_world; // Vector from a to b

	D3DXMatrixMultiply(& obbox::R, & obbox::R_aOBB_world_Tran, & obbox::R_bOBB_world );

	D3DXMatrixTranspose(& obbox::AbsR, & obbox::R_aOBB_world_Tran);
	D3DXVec3TransformCoord( & obbox::t_, & obbox::t_world, & obbox::AbsR );

	for(int i = 0; i < 3; i ++) {obbox::a_OBBDim[i] = a->boxDimension[i] * a_rbo->scale; }
	for(int i = 0; i < 3; i ++) {obbox::b_OBBDim[i] = b->boxDimension[i] * b_rbo->scale; }

	for (int i = 0; i < 3; i ++ )
	{
		for (int j = 0; j < 3; j ++ )
		{
			obbox::AbsR(i,j) = fabs_fast2(obbox::R(i,j)) + 0.000000000001f;
		}
	}

	for (int i = 0; i < 3; i ++ )
	{
		obbox::ra = obbox::a_OBBDim[i];
		obbox::rb = (obbox::b_OBBDim[0] * obbox::AbsR(i,0)) + (obbox::b_OBBDim[1] * obbox::AbsR(i,1)) + (obbox::b_OBBDim[2] * obbox::AbsR(i,2));
		if (fabs(t_[i]) > (obbox::ra + obbox::rb)) return 0;
	}

	for (int i = 0; i < 3; i ++ )
	{
		obbox::ra = (obbox::a_OBBDim[0] * obbox::AbsR(0,i)) + (obbox::a_OBBDim[1] * obbox::AbsR(1,i)) + (obbox::a_OBBDim[2] * obbox::AbsR(2,i));
		obbox::rb = obbox::b_OBBDim[i];
		if (fabs(obbox::t_[0] * obbox::R(0,i) + obbox::t_[1] * obbox::R(1,i) + obbox::t_[2] * obbox::R(2,i)) > (obbox::ra + obbox::rb)) return 0;
	}

	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,0) + obbox::a_OBBDim[2] * obbox::AbsR(1,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(0,2) + obbox::b_OBBDim[2] * obbox::AbsR(0,1);
	if (fabs(obbox::t_[2] * obbox::R(1,0) - obbox::t_[1] * obbox::R(2,0)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,1) + obbox::a_OBBDim[2] * obbox::AbsR(1,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(0,2) + obbox::b_OBBDim[2] * obbox::AbsR(0,0);
	if (fabs(obbox::t_[2] * obbox::R(1,1) - obbox::t_[1] * obbox::R(2,1)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,2) + obbox::a_OBBDim[2] * obbox::AbsR(1,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(0,1) + obbox::b_OBBDim[1] * obbox::AbsR(0,0);
	if (fabs(obbox::t_[2] * obbox::R(1,2) - obbox::t_[1] * obbox::R(2,2)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,0) + obbox::a_OBBDim[2] * obbox::AbsR(0,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(1,2) + obbox::b_OBBDim[2] * obbox::AbsR(1,1);
	if (fabs(obbox::t_[0] * obbox::R(2,0) - obbox::t_[2] * obbox::R(0,0)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,1) + obbox::a_OBBDim[2] * obbox::AbsR(0,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(1,2) + obbox::b_OBBDim[2] * obbox::AbsR(1,0);
	if (fabs(obbox::t_[0] * obbox::R(2,1) - obbox::t_[2] * obbox::R(0,1)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,2) + obbox::a_OBBDim[2] * obbox::AbsR(0,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(1,1) + obbox::b_OBBDim[1] * obbox::AbsR(1,0);
	if (fabs(obbox::t_[0] * obbox::R(2,2) - obbox::t_[2] * obbox::R(0,2)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,0) + obbox::a_OBBDim[1] * obbox::AbsR(0,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(2,2) + obbox::b_OBBDim[2] * obbox::AbsR(2,1);
	if (fabs(obbox::t_[1] * obbox::R(0,0) - obbox::t_[0] * obbox::R(1,0)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,1) + obbox::a_OBBDim[1] * obbox::AbsR(0,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(2,2) + obbox::b_OBBDim[2] * obbox::AbsR(2,0);
	if (fabs(obbox::t_[1] * obbox::R(0,1) - obbox::t_[0] * obbox::R(1,1)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,2) + obbox::a_OBBDim[1] * obbox::AbsR(0,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(2,1) + obbox::b_OBBDim[1] * obbox::AbsR(2,0);
	if (fabs(obbox::t_[1] * obbox::R(0,2) - obbox::t_[0] * obbox::R(1,2)) > (obbox::ra + obbox::rb)) return 0;

	return 1;
}

/************************************************************************/
/* Name:		TestTriCollision										*/
/* Description: Test two OBB leaf nodes with triangle primatives for    */
/*				overlap.  Just a wrapper function for Tomas Moller's 	*/
/*				function.												*/
/************************************************************************/
int obbox::TestTriCollision( obbox * a, rbobjectMesh * a_rbo, obbox * b, rbobjectMesh * b_rbo  )
{
	float V0[3], V1[3], V2[3], U0[3], U1[3], U2[3], TEMP[3];

	D3DXVECTOR3 * vertexBuffer_a = a_rbo->meshData->GetVertexBuffer();
	D3DXVECTOR3 * vertexBuffer_b = b_rbo->meshData->GetVertexBuffer();

	for(int i = 0; i < a->numFaces; i ++) // O(n^2) loop, but OBB algorithm should make numFaces ~= 1 (or at least small)
	{
		for(int j = 0; j < b->numFaces; j ++)
		{
			// V0
			TEMP[0] = vertexBuffer_a[a->t->indices[a->indices + i*3 + 0]].x;
			TEMP[1] = vertexBuffer_a[a->t->indices[a->indices + i*3 + 0]].y;
			TEMP[2] = vertexBuffer_a[a->t->indices[a->indices + i*3 + 0]].z;
			MatrixAffineMult_ATran_B( V0, & a_rbo->matWorld, TEMP);

			// V1
			TEMP[0] = vertexBuffer_a[a->t->indices[a->indices + i*3 + 1]].x;
			TEMP[1] = vertexBuffer_a[a->t->indices[a->indices + i*3 + 1]].y;
			TEMP[2] = vertexBuffer_a[a->t->indices[a->indices + i*3 + 1]].z;
			MatrixAffineMult_ATran_B( V1, & a_rbo->matWorld, TEMP);

			// V2
			TEMP[0] = vertexBuffer_a[a->t->indices[a->indices + i*3 + 2]].x;
			TEMP[1] = vertexBuffer_a[a->t->indices[a->indices + i*3 + 2]].y;
			TEMP[2] = vertexBuffer_a[a->t->indices[a->indices + i*3 + 2]].z;
			MatrixAffineMult_ATran_B( V2, & a_rbo->matWorld, TEMP);

			// U0
			TEMP[0] = vertexBuffer_b[b->t->indices[b->indices + j*3 + 0]].x;
			TEMP[1] = vertexBuffer_b[b->t->indices[b->indices + j*3 + 0]].y;
			TEMP[2] = vertexBuffer_b[b->t->indices[b->indices + j*3 + 0]].z;
			MatrixAffineMult_ATran_B( U0, & b_rbo->matWorld, TEMP);

			// U1
			TEMP[0] = vertexBuffer_b[b->t->indices[b->indices + j*3 + 1]].x;
			TEMP[1] = vertexBuffer_b[b->t->indices[b->indices + j*3 + 1]].y;
			TEMP[2] = vertexBuffer_b[b->t->indices[b->indices + j*3 + 1]].z;
			MatrixAffineMult_ATran_B( U1, & b_rbo->matWorld, TEMP);

			// U2
			TEMP[0] = vertexBuffer_b[b->t->indices[b->indices + j*3 + 2]].x;
			TEMP[1] = vertexBuffer_b[b->t->indices[b->indices + j*3 + 2]].y;
			TEMP[2] = vertexBuffer_b[b->t->indices[b->indices + j*3 + 2]].z;
			MatrixAffineMult_ATran_B( U2, & b_rbo->matWorld, TEMP);

			// Just send verticies off to Tomas Moller's triangle-triangle intersect code.
			if(tri_tri_intersect(V0, V1, V2, U0, U1, U2))
				return 1;
		}
	}

	return 0;
}

/************************************************************************/
/* Name:		TestOBBCollision_SIMD									*/
/* Description: Test two OBB elements for collision using separating	*/
/*				axis test. Code borrowed and modified from page 104 of  */
/*				real time collision detection book.						*/
/************************************************************************/
int obbox::TestOBBCollision_SIMD( obbox * a, rbobjectMesh * a_rbo, obbox * b, rbobjectMesh * b_rbo )
{
	// Can't deference C++ class pointers in __asm --> iN OPTIMIZED RELEASE CODE, MOST OF THESE GO AWAY
	// RBO VALUES
	D3DXMATRIXA16 * a_rbo_R = &a_rbo->R;
	D3DXMATRIXA16 * b_rbo_R = &b_rbo->R;
	D3DXVECTOR3 * a_rbo_x = &a_rbo->x;
	D3DXVECTOR3 * b_rbo_x = &b_rbo->x;
	float a_rbo_scale = a_rbo->scale;
	float b_rbo_scale = b_rbo->scale;

	// OBBOX VALUES
	D3DXVECTOR3 * a_boxDimension = &a->boxDimension;
	D3DXVECTOR3 * b_boxDimension = &b->boxDimension;
	D3DXMATRIXA16 * a_orientMatrix = &a->orientMatrix;
	D3DXMATRIXA16 * b_orientMatrix = &b->orientMatrix;
	D3DXVECTOR3 * a_boxCenterObjectCoord = &a->boxCenterObjectCoord;
	D3DXVECTOR3 * b_boxCenterObjectCoord = &b->boxCenterObjectCoord;

	LOAD_MATRIXA16(a_rbo_R,xmm4,xmm5,xmm6,xmm7);
	// SCALAR CODE EQUIVILENT: D3DXMatrixMultiply(& obbox::R_aOBB_world_Tran, & a->orientMatrix, & a_rbo->R );
	MULT_MATRIXA16MATRIXA16_3x3_INSTANCE(obbox::R_aOBB_world_Tran,a_orientMatrix,xmm0,xmm1,xmm2,xmm4,xmm5,xmm6,xmm7);
	LOAD_VECTORA16(a_boxCenterObjectCoord,xmm0);
	LOAD_VECTORA16(a_boxDimension,xmm2);
	LOAD_FLOATA16(a_rbo_scale,xmm1);
	MUL_VECTORS(xmm0,xmm1);											//  xmm0 = a->boxCenterObjectCoord * a_rbo->scale
	// SCALAR CODE EQUIVILENT: for(int i = 0; i < 3; i ++) {obbox::a_OBBDim[i] = a->boxDimension[i] * a_rbo->scale; }
	MUL_VECTORS(xmm2,xmm1);											//  xmm2 = a->boxDimension * a_rbo->scale
	STORE_VECTORA16_INSTANCE(a_OBBDim,xmm2);
	// SCALAR CODE EQUIVILENT: D3DXVec3TransformCoord(& obbox::t_aOBB, & (a->boxCenterObjectCoord * a_rbo->scale), & a_rbo->R );
	MULT_VECTOR3A16MATRIXA16(xmm0,xmm1,xmm2,xmm4,xmm5,xmm6,xmm7);	//  xmm0 = a_rbo->R * (a->boxCenterObjectCoord * a_rbo->scale)
	//STORE_VECTORA16_INSTANCE(t_aOBB,xmm0);						//  Not necessary

	// SCALAR CODE EQUIVILENT: obbox::t_a_world = a_rbo->x + obbox::t_aOBB;
	LOAD_VECTORA16(a_rbo_x,xmm1);
	ADD_VECTORS(xmm0,xmm1);											//  xmm0 = a_rbo->x + obbox::tempSSE->t_aOBB
	//STORE_VECTORA16_INSTANCE(t_a_world,xmm0);						//  Not necessary

	LOAD_MATRIXA16(b_rbo_R,xmm4,xmm5,xmm6,xmm7);
	// SCALAR CODE EQUIVILENT: D3DXMatrixMultiply(& obbox::R_bOBB_world_Tran, & b->orientMatrix, & b_rbo->R );
	MULT_MATRIXA16MATRIXA16_3x3_INSTANCE(obbox::R_bOBB_world,b_orientMatrix,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7); // Needs to be traspose
	LOAD_VECTORA16(b_boxCenterObjectCoord,xmm1);
	LOAD_VECTORA16(b_boxDimension,xmm3);
	LOAD_FLOATA16(b_rbo_scale,xmm2);
	MUL_VECTORS(xmm1,xmm2);											//  xmm1 = b->boxCenterObjectCoord * b_rbo->scale
	// SCALAR CODE EQUIVILENT: for(int i = 0; i < 3; i ++) {obbox::a_OBBDim[i] = a->boxDimension[i] * b_rbo->scale; }
	MUL_VECTORS(xmm3,xmm2);											//  xmm3 = b->boxDimension * b_rbo->scale
	STORE_VECTORA16_INSTANCE(b_OBBDim,xmm3);
	// SCALAR CODE EQUIVILENT: D3DXVec3TransformCoord(& obbox::t_bOBB, & (b->boxCenterObjectCoord * b_rbo->scale), & b_rbo->R );
	MULT_VECTOR3A16MATRIXA16(xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7);	//  xmm1 = b_rbo->R * (b->boxCenterObjectCoord * b_rbo->scale)
	// STORE_VECTORA16_INSTANCE(t_bOBB,xmm1);						//  Not necessary

	// SCALAR CODE EQUIVILENT: obbox::t_b_world = b_rbo->x + obbox::t_bOBB;
	LOAD_VECTORA16(b_rbo_x,xmm2);
	ADD_VECTORS(xmm1,xmm2);											//  xmm0 = b_rbo->x + obbox::tempSSE->t_bOBB
	// STORE_VECTORA16_INSTANCE(t_b_world,xmm1);					//  Not necessary

	// SCALAR CODE EQUIVILENT: obbox::t_world = obbox::t_b_world - obbox::t_a_world; // Vector from a to b
	// LOAD_VECTORA16_INSTANCE(t_a_world,xmm0);						//  Not necessary
	SUB_VECTORS(xmm1,xmm0);
	//STORE_VECTORA16_INSTANCE(t_world,xmm1);						//  Not necessary

	LOAD_MATRIXA16_INSTANCE(obbox::R_aOBB_world_Tran,xmm4,xmm5,xmm6,xmm7);
	// SCALAR CODE EQUIVILENT: D3DXMatrixTranspose(& obbox::AbsR, & obbox::R_aOBB_world_Tran);
	TRAN_MATRIXA16(xmm4,xmm5,xmm6,xmm7,xmm3);
	// SCALAR CODE EQUIVILENT: D3DXVec3TransformCoord( & obbox::t_, & obbox::t_world, & obbox::AbsR );
	MULT_VECTOR3A16MATRIXA16(xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7);
	STORE_VECTORA16_INSTANCE(obbox::t_,xmm1);

	LOAD_MATRIXA16_INSTANCE(obbox::R_bOBB_world,xmm4,xmm5,xmm6,xmm7);
	// SCALAR CODE EQUIVILENT: D3DXMatrixTranspose(& obbox::R_bOBB_world, & obbox::R);
	TRAN_MATRIXA16(xmm4,xmm5,xmm6,xmm7,xmm3);
	// STORE_MATRIXA16_INSTANCE(obbox::R_bOBB_world,xmm4,xmm5,xmm6,xmm7); // Not Necessary
	// SCALAR CODE EQUIVILENT: D3DXMatrixMultiply(& obbox::R, & obbox::R_aOBB_world_Tran, & obbox::R_bOBB_world );
	MULT_MATRIXA16MATRIXA16_3x3_INSTANCEINSTANCE(obbox::R,obbox::R_aOBB_world_Tran,xmm0,xmm1,xmm2,xmm4,xmm5,xmm6,xmm7); // Needs to be traspose

	// SCALAR CODE EQUIVILENT:
//	for (int i = 0; i < 3; i ++ )
//		for (int j = 0; j < 3; j ++ )
//		{ obbox::AbsR(i,j) = fabs_fast2(obbox::R(i,j)) + 0.000000000001f; }

	LOAD_MATRIXA16_INSTANCE(R,xmm4,xmm5,xmm6,xmm7);
	ABS_PLUS_EPSILON_MATRIXA16(xmm4,xmm5,xmm6,xmm7,xmm0);
	STORE_MATRIXA16_INSTANCE(AbsR,xmm4,xmm5,xmm6,xmm7);

	// SCALAR CODE EQUIVILENT: 
	//for (int i = 0; i < 3; i ++ )
	//{
	//	obbox::ra = obbox::a_OBBDim[i];
	//	obbox::rb = (obbox::b_OBBDim[0] * obbox::AbsR(i,0)) + (obbox::b_OBBDim[1] * obbox::AbsR(i,1)) + (obbox::b_OBBDim[2] * obbox::AbsR(i,2));
	//	if (fabs(t_[i]) > (obbox::ra + obbox::rb)) return 0;
	//}
	LOAD_VECTORA16_INSTANCE(a_OBBDim,xmm0); // ra
	LOAD_VECTORA16_INSTANCE(b_OBBDim,xmm1);
	LOAD_MATRIXA16_INSTANCE(AbsR,xmm4,xmm5,xmm6,xmm7);
	TRAN_MATRIXA16(xmm4,xmm5,xmm6,xmm7,xmm2); // AbsR^T
	MULT_VECTOR3A16MATRIXA16(xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7); // rb = b_OBBDim * AbsR
	ADD_VECTORS(xmm0,xmm1); // ra + rb
	LOAD_VECTORA16_INSTANCE(t_,xmm2); // t_
	ABS_VECTOR(xmm2,xmm3); // fabs(t_)
	__asm {cmpltps xmm0, xmm2 }; // (ra + rb) < fabs(t_)
	GETBOOL_VECTOR3A16(comp_result,xmm0, xmm2);
	if(comp_result != 0)
		return 0;
	
//	for (int i = 0; i < 3; i ++ )
//	{
//		obbox::ra = (obbox::a_OBBDim[0] * obbox::AbsR(0,i)) + (obbox::a_OBBDim[1] * obbox::AbsR(1,i)) + (obbox::a_OBBDim[2] * obbox::AbsR(2,i));
//		obbox::rb = obbox::b_OBBDim[i];
//		if (fabs(obbox::t_[0] * obbox::R(0,i) + obbox::t_[1] * obbox::R(1,i) + obbox::t_[2] * obbox::R(2,i)) > (obbox::ra + obbox::rb)) return 0;
//	}
	LOAD_VECTORA16_INSTANCE(b_OBBDim,xmm0); // rb
	//LOAD_MATRIXA16_INSTANCE(AbsR,xmm4,xmm5,xmm6,xmm7); // Already done before, AbsR^T is in xmm4,xmm5,xmm6,xmm7
	TRAN_MATRIXA16(xmm4,xmm5,xmm6,xmm7,xmm1); // AbsR
	LOAD_VECTORA16_INSTANCE(a_OBBDim,xmm1);
	MULT_VECTOR3A16MATRIXA16(xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7); // ra
	ADD_VECTORS(xmm0,xmm1); // ra + rb
	LOAD_VECTORA16_INSTANCE(t_,xmm2); // t_
	LOAD_MATRIXA16_INSTANCE(R,xmm4,xmm5,xmm6,xmm7); // R
	MULT_VECTOR3A16MATRIXA16(xmm2,xmm1,xmm3,xmm4,xmm5,xmm6,xmm7); // t_ * R^T
	ABS_VECTOR(xmm2,xmm3); // fabs(t_ * R^T)
	__asm {cmpltps xmm0, xmm2 }; // (ra + rb) < fabs(t_ * R^T)
	GETBOOL_VECTOR3A16(comp_result,xmm0, xmm2);
	if(comp_result != 0)
		return 0;

	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,0) + obbox::a_OBBDim[2] * obbox::AbsR(1,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(0,2) + obbox::b_OBBDim[2] * obbox::AbsR(0,1);
	if (fabs(obbox::t_[2] * obbox::R(1,0) - obbox::t_[1] * obbox::R(2,0)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,1) + obbox::a_OBBDim[2] * obbox::AbsR(1,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(0,2) + obbox::b_OBBDim[2] * obbox::AbsR(0,0);
	if (fabs(obbox::t_[2] * obbox::R(1,1) - obbox::t_[1] * obbox::R(2,1)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[1] * obbox::AbsR(2,2) + obbox::a_OBBDim[2] * obbox::AbsR(1,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(0,1) + obbox::b_OBBDim[1] * obbox::AbsR(0,0);
	if (fabs(obbox::t_[2] * obbox::R(1,2) - obbox::t_[1] * obbox::R(2,2)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,0) + obbox::a_OBBDim[2] * obbox::AbsR(0,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(1,2) + obbox::b_OBBDim[2] * obbox::AbsR(1,1);
	if (fabs(obbox::t_[0] * obbox::R(2,0) - obbox::t_[2] * obbox::R(0,0)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,1) + obbox::a_OBBDim[2] * obbox::AbsR(0,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(1,2) + obbox::b_OBBDim[2] * obbox::AbsR(1,0);
	if (fabs(obbox::t_[0] * obbox::R(2,1) - obbox::t_[2] * obbox::R(0,1)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(2,2) + obbox::a_OBBDim[2] * obbox::AbsR(0,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(1,1) + obbox::b_OBBDim[1] * obbox::AbsR(1,0);
	if (fabs(obbox::t_[0] * obbox::R(2,2) - obbox::t_[2] * obbox::R(0,2)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,0) + obbox::a_OBBDim[1] * obbox::AbsR(0,0);
	obbox::rb = obbox::b_OBBDim[1] * obbox::AbsR(2,2) + obbox::b_OBBDim[2] * obbox::AbsR(2,1);
	if (fabs(obbox::t_[1] * obbox::R(0,0) - obbox::t_[0] * obbox::R(1,0)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,1) + obbox::a_OBBDim[1] * obbox::AbsR(0,1);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(2,2) + obbox::b_OBBDim[2] * obbox::AbsR(2,0);
	if (fabs(obbox::t_[1] * obbox::R(0,1) - obbox::t_[0] * obbox::R(1,1)) > (obbox::ra + obbox::rb)) return 0;

	obbox::ra = obbox::a_OBBDim[0] * obbox::AbsR(1,2) + obbox::a_OBBDim[1] * obbox::AbsR(0,2);
	obbox::rb = obbox::b_OBBDim[0] * obbox::AbsR(2,1) + obbox::b_OBBDim[1] * obbox::AbsR(2,0);
	if (fabs(obbox::t_[1] * obbox::R(0,2) - obbox::t_[0] * obbox::R(1,2)) > (obbox::ra + obbox::rb)) return 0;

	return 1;
}


#pragma warning( pop )			// Edit Jonathan Tompson - 31st Jan 2011