/*************************************************************
**						obbtreeTempVar						**
**************************************************************/
// File:		obbtreeTempVar.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Scratch variables for use in building obbox elements
// --> Avoids multiple allocations on stack since obbox::BuildOBBTree is called many times.
// Some speed up seen in practice (but not too much)

#ifndef obbtreeTempVar_h
#define obbtreeTempVar_h

#include "main.h"
#include <vector>

#include "dxInclude.h"

#include "utils_and_misc_classes\math\double3.h"
#include "utils_and_misc_classes\math\double3x3.h"

typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef unsigned int * PUINT;

class obbtreeTempVar
{
public:

	friend class obbtree;
	friend class obbox;
	friend class debugObject;
	friend class app;

	// Constructor / Destructor
    obbtreeTempVar(DWORD numFaces, int numVerticies);
	~obbtreeTempVar();

private:
	//		Start 1st 16 byte block (2 * sizeof(double) = 16)
	double		A_H, A_i;
	//		Start 2nd 16 byte block (2 * 3 * sizeof(double) = 48)
	double3		c_H, c_i;
	//		Start 5th 16 byte block ((3 * 3 + 1) * sizeof(double) = 80)
	double3x3	Cov;
	double		padding_0;
	//		Start 10th 16 byte block ((3 * 3 + 1) * sizeof(double) = 80)
	double3		eigVec[3];
	double		padding_1;
	//		Start 15th 16 byte block (6 * sizeof(double) = 48)
	double		minVec0, maxVec0, minVec1, maxVec1, minVec2, maxVec2;
	//		Start 18th 16 byte block (4 * sizeof(double) = 32)
	double3		curVertex; 
	double		padding_2;
	//		Start 20th 16 byte block (4 * sizeof(double) = 32)
	double		projection, mean0, mean1, mean2;
	//		Start 22nd 16 byte block ((3 * 3 + 1) * sizeof(double) = 80)
	double3		axisOrder[3];
	double		padding_3;
	//		Start 27th 16 byte block (3 * sizeof(int) + sizeof(void *) = 16)
	int			child1NumIndicies, child2NumIndicies;
	int			splitOK;
	int *		uniqueIndexSet; // Temporary space to use when removing repeated indicies
	//		Start 28th 16 byte block (3 * sizeof(int) + sizeof(void *) = 16)
	int			uniqueIndexSetSize;
	double *	vertexSet; // Temporary space to hold input to convex hull generator
	int			vertexSetSize;
	int			padding_4;
	//		Start 29th 16 byte block (sizeof(double) + sizeof(void *) + sizeof(UINT) = 16)
	double		mean;
	double *	cHullVert; // Temporary memory space to hold convex hull
	UINT		cHullVertCount;
	//		Start 30th 16 byte block (3 * sizeof(void *) + sizeof(UINT) = 16)
	PUINT		cHullInd; // Temporary memory space to hold convex hull indicies
	UINT		cHullIndCount;
	int *		indices_child1; // Temporary memory space to use when splitting up indicies between children
	int *		indices_child2;
	//		Start 31st 16 byte block (3 * 3 * sizeof(float) + 3 * sizeof(UINT) = 48)
	D3DXVECTOR3	orient[3]; // Vectors form OBB axes (orthonormal eigen basis)
	UINT		padding_5[3];
	//		Start 34th 16 byte block (3 * sizeof(float) + sizeof(UINT) = 16)
	D3DXVECTOR3 boxCenterBoxCoord;
	UINT		padding_6;
	//		Start 35th 16 byte block (sizeof(std::vector<double>) + 3 * sizeof(UINT) = 32)
	std::vector<double> centroids;
	UINT		padding_7[3];
};

#endif