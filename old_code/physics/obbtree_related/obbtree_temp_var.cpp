/*************************************************************
**						obbtreeTempVar						**
**************************************************************/
// File:		obbtreeTempVar.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "physics\obbtree_related\obbtreeTempVar.h"
#include <vector>

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		obbtreeTempVar											*/
/* Description:	Default constructor function							*/
/************************************************************************/
obbtreeTempVar::obbtreeTempVar(DWORD numFaces, int numVerticies)
{
	uniqueIndexSet = new int[numFaces*3]; // Temporary memory space to use when removing repeated indicies
	vertexSet = new double[numVerticies*3]; // Temporary memory space to hold input to convex hull generator
	cHullVert = new double[numVerticies*3]; // Temporary memory space to hold convex hull
	// TOMPSON HACK: Origionally it was numVertices only, but I was hitting out of range exceptions...
	cHullInd = new UINT[max(((int)numFaces)*3,numVerticies)]; // Temporary memory space to hold convex hull indicies
	uniqueIndexSetSize = 0;
	vertexSetSize = 0;
	cHullVertCount = 0;
	cHullIndCount = 0;
    indices_child1 = new int[numFaces*3]; // Temporary memory space to use when splitting up indicies between children
	indices_child2 = new int[numFaces*3];
}

/************************************************************************/
/* Name:		~obbtreeTempVar											*/
/* Description:	Default destructor function								*/
/************************************************************************/
obbtreeTempVar::~obbtreeTempVar()
{
	if(uniqueIndexSet) { delete [] uniqueIndexSet; uniqueIndexSet = NULL; }
	if(vertexSet) { delete [] vertexSet; vertexSet = NULL; }
	if(cHullVert) { delete [] cHullVert; cHullVert = NULL; }
	if(cHullInd) { delete [] cHullInd; cHullInd = NULL; }
	if(indices_child1) { delete [] indices_child1; indices_child1 = NULL; }
	if(indices_child2) { delete [] indices_child2; indices_child2 = NULL; }
}