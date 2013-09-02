// File:		obbox_FuncsBuild.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is primary OBB tree data structure to impliment the paper:
// S. Gottschalk et. al. "OBBTree: A Hierarchical Structure for Rapid Iterference Detection"

// OBBTREE Build routines for obbox class

#include "physics\obbtree_related\obbox.h"
#include "physics\obbtree_related\obboxTempVar.h"
#include "physics\obbtree_related\obboxRenderitems.h"
#include "main.h"
#include "app.h"
#include "physics\obbtree_related\obbtree.h"
#include "physics\obbtree_related\obbtreeTempVar.h"
#include "physics\objects\rbobjectMesh.h"
#include "physics\objects\rbobjectMeshData.h"
#include "ConvexHull_Libs\StanHull\hull.h"
#include "ConvexHull_Libs\CGAL\CGAL.h"
#include "physics\obbtree_related\eig3.h"
#include "utils_and_misc_classes\stringUtil.h"
#include "utils_and_misc_classes\util.h"
#include "utils_and_misc_classes\data_structures\hashset_int.h"
#include "utils_and_misc_classes\data_structures\vecA.h"
#include "utils_and_misc_classes\data_structures\vec.h"
#include "renderer\renderer_structures\vertex.h"
#include "renderer\renderer.h"
#include "physics\obbtree_related\triTriIntersect.h"
#include <UI\UI.h>
#include <UI\varNames.h>
#include "utils_and_misc_classes\math\math_funcs.h"
//#include <xnamath.h>

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#pragma warning( push )			// Edit Jonathan Tompson - 31st Jan 2011
#pragma warning( disable:4238 )	// Edit Jonathan Tompson - 31st Jan 2011

#define NULL 0

/************************************************************************/
/* Name:		BuildOBBTree											*/
/* Description: Builds one level of the OBB tree						*/
/* Gottschalk et.al. "OBBTree: A Hierarchical..." pg 3 implimented here */
/************************************************************************/
void obbox::BuildOBBTree(int curIndex, std::vector<int> * obbStack, rbobjectMeshData * meshData )
{
#ifdef _DEBUG
	if(numFaces < 1)
		throw std::runtime_error("obbox::BuildOBBTree() - Something went wrong! --> numFaces at current node < 1!!");
	if(meshData == NULL)
		throw std::runtime_error("obbox::BuildOBBTree() - Trying to build an obbtree from uninitialized mesh data");
	if(meshData->GetVertexBuffer() == NULL)
		throw std::runtime_error("obbox::BuildOBBTree() - Trying to build an obbtree from uninitialized mesh data");
#endif

	if(depth+1 <= (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
		renderitems = new obboxRenderitems;
	else
		renderitems = NULL;

	D3DXVECTOR3 * vertexBuffer = meshData->GetVertexBuffer();

	// Only find convex hull if numFaces > 1
	if(numFaces == 1)
	{
		// This will be a leaf node, just use
		t->temp->cHullIndCount = 3; 
		t->temp->cHullInd[0] = 0; // Point 1
		t->temp->cHullInd[1] = 1; // Point 2
		t->temp->cHullInd[2] = 2; // Point 3

		t->temp->cHullVertCount = 3;
		t->temp->cHullVert[0] = vertexBuffer[t->indices[indices + 0]].x; // Point 1 = p later on
		t->temp->cHullVert[1] = vertexBuffer[t->indices[indices + 0]].y;
		t->temp->cHullVert[2] = vertexBuffer[t->indices[indices + 0]].z;
		t->temp->cHullVert[3] = vertexBuffer[t->indices[indices + 1]].x; // Point 2 = q later on
		t->temp->cHullVert[4] = vertexBuffer[t->indices[indices + 1]].y;
		t->temp->cHullVert[5] = vertexBuffer[t->indices[indices + 1]].z;
		t->temp->cHullVert[6] = vertexBuffer[t->indices[indices + 2]].x; // Point 3 = r later on
		t->temp->cHullVert[7] = vertexBuffer[t->indices[indices + 2]].y;
		t->temp->cHullVert[8] = vertexBuffer[t->indices[indices + 2]].z;

		t->temp->uniqueIndexSetSize = 3;
		t->temp->uniqueIndexSet[0] = t->indices[indices + 0]; // Point 1
		t->temp->uniqueIndexSet[1] = t->indices[indices + 1]; // Point 1
		t->temp->uniqueIndexSet[2] = t->indices[indices + 2]; // Point 1
	}
	else
	{
// 1. Find an index set with non-repeating (i.e. unique) indices --> For convex hull
		FindUniqueIndexSet_HashSet(&t->temp->uniqueIndexSet, &t->temp->uniqueIndexSetSize, t->indices, indices, numFaces*3); // Hash table is O(n)
		// Collect the verticies in an array --> REQUIRED FOR INPUT TO MOST CONVEX HULL GENERATORS (groups of 3 floats)
		// NEED TO ADD SMALL DEVIATION TO EACH VERTEX --> HELPS CONVEX HULL CONVERGE
		if(g_UI->GetSetting<bool>(&var_addVectorPreturb))
			srand(1000); // Seed random number generator if we're going to use it.
		double randNum = 0; 
		double lowRand = -0.000001;  double rangeRand = 0.000002; // Gives rand [-0.000001, 0.000001)
		for(int i = 0; i < t->temp->uniqueIndexSetSize; i ++)
		{
			if(g_UI->GetSetting<bool>(&var_addVectorPreturb))
				randNum = static_cast<double>( rand() ) * rangeRand / static_cast<double>( RAND_MAX ) + lowRand ;
			t->temp->vertexSet[i*3 + 0] = vertexBuffer[t->temp->uniqueIndexSet[i]].x + randNum;
			if(g_UI->GetSetting<bool>(&var_addVectorPreturb))
				randNum = static_cast<double>( rand() ) * rangeRand / static_cast<double>( RAND_MAX ) + lowRand ;
			t->temp->vertexSet[i*3 + 1] = vertexBuffer[t->temp->uniqueIndexSet[i]].y + randNum;
			if(g_UI->GetSetting<bool>(&var_addVectorPreturb))
				randNum = static_cast<double>( rand() ) * rangeRand / static_cast<double>( RAND_MAX ) + lowRand ;
			t->temp->vertexSet[i*3 + 2] = vertexBuffer[t->temp->uniqueIndexSet[i]].z + randNum;
		}

// 2. a) Find the convex hull of the unique set
/************************************************************************/
/* SOURCES I HAVE TRIED													*/
/************************************************************************/
/* i. Stan Melax's convex hull generator								*/
/*		 --> Bad with degenerate models. but fast.						*/
/************************************************************************/
/* ii. qhull convex hull generator										*/
/*       --> Poorly written library										*/
/************************************************************************/
/* iii. Use CGAL library												*/
/*		 --> Robust but very slow.										*/
/************************************************************************/
/* iv. Just use covarience matrix of model								*/
/*		 --> Defeats the purpose of the whole algorithm (crappy OBBs)	*/
/************************************************************************/
		// Try using StanHull to create the convex hull
		int retVal;
		if(!g_UI->GetSetting<bool>(&var_noStanHull) && g_UI->GetSetting<bool>(&var_findConvexHull))
			retVal = SM_chull(t->temp->vertexSet, t->temp->uniqueIndexSetSize*3, &t->temp->cHullVert, &t->temp->cHullVertCount, &t->temp->cHullInd, &t->temp->cHullIndCount );
		else
			retVal = 0;
		if(!retVal || t->temp->cHullIndCount <= 3) // Sometimes StanHull will return one triangle convex hull!
		{
#ifdef _DEBUG_OBBBUILD_WARNINGS
			wchar_t OutputText[128]; swprintf(OutputText,127,L"     --> WARNING: StanHull failed at depth %d, numFaces %d, trying CGAL\n",depth,numFaces);
			OutputDebugString(OutputText);
#endif	
			// If StanHull Failed try CGAL
			if(!g_UI->GetSetting<bool>(&var_noCGAL) && g_UI->GetSetting<bool>(&var_findConvexHull))
				retVal = CGAL_chull(t->temp->vertexSet, t->temp->uniqueIndexSetSize*3, &t->temp->cHullVert, &t->temp->cHullVertCount, &t->temp->cHullInd, &t->temp->cHullIndCount);
			else
				retVal = 0;
			if(!retVal || t->temp->cHullIndCount <= 3)
			{
#ifdef _DEBUG_OBBBUILD_WARNINGS
				wchar_t OutputText[128]; swprintf(OutputText,127,L"     --> WARNING: CGAL failed at depth %d, numFaces %d, using original mesh\n",depth,numFaces);
				OutputDebugString(OutputText);
#endif	
				// If CGAL failed OR the model is degenerate (CH is a line segment, etc) just use the origional mesh.
				Copy_Mesh_To_CH(this, meshData, t->temp->vertexSet, t->temp->uniqueIndexSetSize*3, &t->temp->cHullVert, &t->temp->cHullVertCount, &t->temp->cHullInd, &t->temp->cHullIndCount );
			}
		}
	}

// 2. b) Initialize convex hull for rendering if we want and save it for later (to save to disk)
	if(depth+1 <= (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
	{
		// Make a copy of convex hull
		renderitems->cHullVertCount = t->temp->cHullVertCount;
		renderitems->cHullVert = new float[renderitems->cHullVertCount*3];
		for(UINT i = 0; i < renderitems->cHullVertCount*3; i ++)
			renderitems->cHullVert[i] = (float)t->temp->cHullVert[i];
		renderitems->cHullIndCount = t->temp->cHullIndCount;
		renderitems->cHullInd = new UINT[renderitems->cHullIndCount];
		for(UINT i = 0; i < renderitems->cHullIndCount; i ++)
			renderitems->cHullInd[i] = t->temp->cHullInd[i];

		// Initialize the convex hull for rendering
		initConvexHullForRendering();
	}

	// Only use covarience matrix to find OBB if numFaces > 1
	if(t->temp->cHullIndCount == 3) // One face --> MATH CHECKED BY MATLAB
	{
		// Extract the verticies
		double * p = &t->temp->cHullVert[t->temp->cHullInd[0]*3]; // Vertex 0
		double * q = &t->temp->cHullVert[t->temp->cHullInd[1]*3]; // Vertex 1
		double * r = &t->temp->cHullVert[t->temp->cHullInd[2]*3]; // Vertex 2	

		// Use an edge as the first axis (Edge1_p2q)
		double3 Edge1_p2q(q[0] - p[0],q[1] - p[1],q[2] - p[2]);
		t->temp->eigVec[0] = Edge1_p2q;
		t->temp->eigVec[0] = safenormalize(t->temp->eigVec[0]); // Normalize it

		// Use the cross product of two edges as the second axis (Edge1_p2q x Edge2_p2r)
		double3 Edge2_p2r(r[0] - p[0],r[1] - p[1],r[2] - p[2]);
		t->temp->eigVec[1] = cross(Edge1_p2q, Edge2_p2r);		
		t->temp->eigVec[1] = safenormalize(t->temp->eigVec[1]); // Normalize it

		// Use the cross product of those two edges as the third axes
		// According to Right-Hand-Rule (where index is A, middle is B and thumb is A x B), use eigVec[1] as A
		t->temp->eigVec[2] = cross(t->temp->eigVec[1], t->temp->eigVec[0]);
		t->temp->eigVec[2] = safenormalize(t->temp->eigVec[2]); // Normalize it (though should already be normalized)
	}
	else
	{
// 3. Find a) total convex hull surface area, b) the convex hull centroid, c) Covarience Matrix
	// --> Use S. Melax's double classes for better precision (I had trouble with poor precision)
	// Also, do two loops through faces --> Could be done in one, but this is easier to read and O(n) anyway.
		t->temp->A_H = 0; t->temp->A_i = 0;
		t->temp->c_H.x = 0.0; t->temp->c_H.y = 0.0; t->temp->c_H.z = 0.0; 
		t->temp->c_i.x = 0.0; t->temp->c_i.y = 0.0; t->temp->c_i.z = 0.0;
		t->temp->Cov[0][0] = 0.0; t->temp->Cov[0][1] = 0.0; t->temp->Cov[0][2] = 0.0; // Zero covarience matrix 
		t->temp->Cov[1][0] = 0.0; t->temp->Cov[1][1] = 0.0; t->temp->Cov[1][2] = 0.0;
		t->temp->Cov[2][0] = 0.0; t->temp->Cov[2][1] = 0.0; t->temp->Cov[2][2] = 0.0;

		for(UINT i = 0; i < t->temp->cHullIndCount; i += 3)
		{
			double * p = &t->temp->cHullVert[t->temp->cHullInd[i + 0]*3]; // Vertex 0
			double * q = &t->temp->cHullVert[t->temp->cHullInd[i + 1]*3]; // Vertex 1
			double * r = &t->temp->cHullVert[t->temp->cHullInd[i + 2]*3]; // Vertex 2
			AreaConvexhullTriangle(&t->temp->A_i, p, q, r);
			CentroidConvexhullTriangle(&t->temp->c_i, p, q, r);
			t->temp->c_H.x += t->temp->A_i * t->temp->c_i.x;	  t->temp->c_H.y += t->temp->A_i * t->temp->c_i.y;	  t->temp->c_H.z += t->temp->A_i * t->temp->c_i.z;
			t->temp->A_H += t->temp->A_i;
		}
		if(abs(t->temp->A_H-0.0f) < 0.000000001)
			throw std::runtime_error("obbox::BuildOBBTree() - Convex Hull Surface Area is zero!");
		t->temp->c_H.x = t->temp->c_H.x / t->temp->A_H; t->temp->c_H.y = t->temp->c_H.y / t->temp->A_H; t->temp->c_H.z = t->temp->c_H.z / t->temp->A_H; // Need a weighted average

		// Now find Covarience Matrix --> Could be included into top for loop, but this is easier to debug.
		for(UINT i = 0; i < t->temp->cHullIndCount; i +=3)
		{
			double * p = &t->temp->cHullVert[t->temp->cHullInd[i + 0]*3]; // Vertex 0
			double * q = &t->temp->cHullVert[t->temp->cHullInd[i + 1]*3]; // Vertex 
			double * r = &t->temp->cHullVert[t->temp->cHullInd[i + 2]*3]; // Vertex 2
			AreaConvexhullTriangle(&t->temp->A_i, p, q, r); // (Verified)
			CentroidConvexhullTriangle(&t->temp->c_i, p, q, r); // (Verified)
			t->temp->Cov[0][0] += (t->temp->A_i/12.0) * (9*t->temp->c_i.x*t->temp->c_i.x + p[0]*p[0] + q[0]*q[0] + r[0]*r[0]); // C_{jk}, where j = 0, k = 0		-> Or .x .x
			t->temp->Cov[0][1] += (t->temp->A_i/12.0) * (9*t->temp->c_i.x*t->temp->c_i.y + p[0]*p[1] + q[0]*q[1] + r[0]*r[1]); // C_{jk}, where j = 0, k = 1
			t->temp->Cov[0][2] += (t->temp->A_i/12.0) * (9*t->temp->c_i.x*t->temp->c_i.z + p[0]*p[2] + q[0]*q[2] + r[0]*r[2]); // C_{jk}, where j = 0, k = 2
			t->temp->Cov[1][1] += (t->temp->A_i/12.0) * (9*t->temp->c_i.y*t->temp->c_i.y + p[1]*p[1] + q[1]*q[1] + r[1]*r[1]); // C_{jk}, where j = 1, k = 1
			t->temp->Cov[1][2] += (t->temp->A_i/12.0) * (9*t->temp->c_i.y*t->temp->c_i.z + p[1]*p[2] + q[1]*q[2] + r[1]*r[2]); // C_{jk}, where j = 1, k = 2
			t->temp->Cov[2][2] += (t->temp->A_i/12.0) * (9*t->temp->c_i.z*t->temp->c_i.z + p[2]*p[2] + q[2]*q[2] + r[2]*r[2]); // C_{jk}, where j = 2, k = 2
		}
		t->temp->Cov[0][0] = (t->temp->Cov[0][0]/t->temp->A_H) - t->temp->c_H.x*t->temp->c_H.x; // C_{jk}, where j = 0, k = 0		-> Or .x .x
		t->temp->Cov[0][1] = (t->temp->Cov[0][1]/t->temp->A_H) - t->temp->c_H.x*t->temp->c_H.y; // C_{jk}, where j = 0, k = 1
		t->temp->Cov[0][2] = (t->temp->Cov[0][2]/t->temp->A_H) - t->temp->c_H.x*t->temp->c_H.z; // C_{jk}, where j = 0, k = 2
		t->temp->Cov[1][1] = (t->temp->Cov[1][1]/t->temp->A_H) - t->temp->c_H.y*t->temp->c_H.y; // C_{jk}, where j = 1, k = 1
		t->temp->Cov[1][2] = (t->temp->Cov[1][2]/t->temp->A_H) - t->temp->c_H.y*t->temp->c_H.z; // C_{jk}, where j = 1, k = 2
		t->temp->Cov[2][2] = (t->temp->Cov[2][2]/t->temp->A_H) - t->temp->c_H.z*t->temp->c_H.z; // C_{jk}, where j = 2, k = 2	

		// Add terms missed before --> Build Symmetric matrix
		t->temp->Cov[1][0] = t->temp->Cov[0][1]; 
		t->temp->Cov[2][0] = t->temp->Cov[0][2];
		t->temp->Cov[2][1] = t->temp->Cov[1][2];

// 6. Find eigen vectors of covarience matrix
	// getSymMatEigenvectorsMelax(&t->temp->eigVec[0], Cov); // Wrapper function for S. Melax code --> DOESN'T WORK!
		getSymMatEigenvectors(&t->temp->eigVec[0], t->temp->Cov); // Wrapper function for adapted java lib code

// 7. Normalize eigen vectors to form eigen basis
		t->temp->eigVec[0] = safenormalize(t->temp->eigVec[0]); // We shouldn't need to normalize, but do it anyway.
		t->temp->eigVec[1] = safenormalize(t->temp->eigVec[1]);
		t->temp->eigVec[2] = safenormalize(t->temp->eigVec[2]);
		if(!util::isBasis(&t->temp->eigVec[0],&t->temp->eigVec[1],&t->temp->eigVec[2]))
			throw std::runtime_error("obbox::BuildOBBTree() - eigenVectors do not form orthonormal basis!");
		if(!util::isRotation(&double3x3(t->temp->eigVec[0],t->temp->eigVec[1],t->temp->eigVec[2])))
			throw std::runtime_error("obbox::BuildOBBTree() - eigenVectors do not form a rotational matrix (with eigenvectors as rows)!");
	}

// 8. Find extremal vertecies along each axis (of the eigen basis) to find box dimensions
	t->temp->minVec0 = 0.0; t->temp->maxVec0 = 0.0; t->temp->minVec1 = 0.0; t->temp->maxVec1 = 0.0; t->temp->minVec2 = 0.0; t->temp->maxVec2 = 0.0;
	t->temp->curVertex.x = 0.0; t->temp->curVertex.y = 0.0; t->temp->curVertex.z = 0.0;
	t->temp->projection = 0.0; t->temp->mean0 = 0.0; t->temp->mean1 = 0.0; t->temp->mean2 = 0.0;

	// Note: Projection of a 3D point onto a line is proj = A.B / ||B||
	// http://www.euclideanspace.com/maths/geometry/elements/line/projections/index.htm 
	// (Helpful for later: http://www.euclideanspace.com/maths/geometry/elements/plane/lineOnPlane/index.htm)

	// Initialize min and max with first vertex
	t->temp->curVertex.x = vertexBuffer[t->temp->uniqueIndexSet[0]].x;
	t->temp->curVertex.y = vertexBuffer[t->temp->uniqueIndexSet[0]].y;
	t->temp->curVertex.z = vertexBuffer[t->temp->uniqueIndexSet[0]].z;
	t->temp->projection = dot(t->temp->curVertex,t->temp->eigVec[0]); t->temp->maxVec0 = t->temp->projection; t->temp->minVec0 = t->temp->projection;
	t->temp->projection = dot(t->temp->curVertex,t->temp->eigVec[1]); t->temp->maxVec1 = t->temp->projection; t->temp->minVec1 = t->temp->projection;
	t->temp->projection = dot(t->temp->curVertex,t->temp->eigVec[2]); t->temp->maxVec2 = t->temp->projection; t->temp->minVec2 = t->temp->projection;

	// Go through each vertex and find min and max O(n)
	for(int i = 1; i < t->temp->uniqueIndexSetSize; i ++)
	{
		// We have box center and vertex in object coords --> take subtraction to find vertex from box center.
		t->temp->curVertex.x = vertexBuffer[t->temp->uniqueIndexSet[i]].x;
		t->temp->curVertex.y = vertexBuffer[t->temp->uniqueIndexSet[i]].y;
		t->temp->curVertex.z = vertexBuffer[t->temp->uniqueIndexSet[i]].z;
		t->temp->projection = dot(t->temp->curVertex,t->temp->eigVec[0]);
		if(t->temp->projection > t->temp->maxVec0) { t->temp->maxVec0 = t->temp->projection; }
		if(t->temp->projection < t->temp->minVec0) { t->temp->minVec0 = t->temp->projection; }
		t->temp->projection = dot(t->temp->curVertex,t->temp->eigVec[1]);
		if(t->temp->projection > t->temp->maxVec1) { t->temp->maxVec1 = t->temp->projection; }
		if(t->temp->projection < t->temp->minVec1) { t->temp->minVec1 = t->temp->projection; }
		t->temp->projection = dot(t->temp->curVertex,t->temp->eigVec[2]);
		if(t->temp->projection > t->temp->maxVec2) { t->temp->maxVec2 = t->temp->projection; }
		if(t->temp->projection < t->temp->minVec2) { t->temp->minVec2 = t->temp->projection; }
	}
	boxDimension.x = (float)((t->temp->maxVec0 - t->temp->minVec0)/2.0);  // Half the length of a box
	boxDimension.y = (float)((t->temp->maxVec1 - t->temp->minVec1)/2.0); 
	boxDimension.z = (float)((t->temp->maxVec2 - t->temp->minVec2)/2.0);
	// Make box lengths non-zero (helps catch collisions for face-face collisions in close proximity
	if(abs(boxDimension.x - 0.0f)<EPSILON) { boxDimension.x = 0.0000001f; }
	if(abs(boxDimension.y - 0.0f)<EPSILON) { boxDimension.y = 0.0000001f; }
	if(abs(boxDimension.z - 0.0f)<EPSILON) { boxDimension.z = 0.0000001f; }
	
// 9. Find the rotation matrix and translation matrix, from what we have found.
	// RELATIVE TO OBJECT COORDINATES, NOT PARENT OBB, AS IN PAPER!!
	// change of basis from (x,y,z)->(u,v,n) has vectors as rows.  (u,v,n)->(x,y,z) has vectors as columns
	// Row's run horizontally, columns run vertically.
	t->temp->orient[0] = D3DXVECTOR3((float)t->temp->eigVec[0].x,(float)t->temp->eigVec[0].y,(float)t->temp->eigVec[0].z);
	t->temp->orient[1] = D3DXVECTOR3((float)t->temp->eigVec[1].x,(float)t->temp->eigVec[1].y,(float)t->temp->eigVec[1].z);
	t->temp->orient[2] = D3DXVECTOR3((float)t->temp->eigVec[2].x,(float)t->temp->eigVec[2].y,(float)t->temp->eigVec[2].z);
	t->temp->boxCenterBoxCoord = D3DXVECTOR3((float)(t->temp->maxVec0 + t->temp->minVec0)/2.0f,(float)(t->temp->maxVec1 + t->temp->minVec1)/2.0f,(float)(t->temp->maxVec2 + t->temp->minVec2)/2.0f); // Box center in box coordinates 
	// c = [0.5 * (l1 + u1) * v1] + [0.5 * (l2 + u2) * v2] + [0.5 * (l3 + u3) * v3]
	
	// Transform local to object coordinates, ie do a (U,V,N) to (X,Y,Z) transform with vectors as the columns
	// Modal Matrix form, eigenvectors are colomn-wise
	//orientMatrix._11 = (float)t->temp->eigVec[0].x;  orientMatrix._12 = (float)t->temp->eigVec[1].x;  orientMatrix._13 = (float)t->temp->eigVec[2].x;  orientMatrix._14 = 0.0f;
	//orientMatrix._21 = (float)t->temp->eigVec[0].y;  orientMatrix._22 = (float)t->temp->eigVec[1].y;  orientMatrix._23 = (float)t->temp->eigVec[2].y;  orientMatrix._24 = 0.0f;
	//orientMatrix._31 = (float)t->temp->eigVec[0].z;  orientMatrix._32 = (float)t->temp->eigVec[1].z;  orientMatrix._33 = (float)t->temp->eigVec[2].z;  orientMatrix._34 = 0.0f;
	//orientMatrix._41 = 0.0f;				   orientMatrix._42 = 0.0f;					  orientMatrix._43 = 0.0f;					 orientMatrix._44 = 1.0f;
	
	// Now storing row-wise --> More efficient for SIMD collision tests (fewer transpose operations)
	orientMatrix._11 = (float)t->temp->eigVec[0].x;  orientMatrix._12 = (float)t->temp->eigVec[0].y;  orientMatrix._13 = (float)t->temp->eigVec[0].z;  orientMatrix._14 = 0.0f;
	orientMatrix._21 = (float)t->temp->eigVec[1].x;  orientMatrix._22 = (float)t->temp->eigVec[1].y;  orientMatrix._23 = (float)t->temp->eigVec[1].z;  orientMatrix._24 = 0.0f;
	orientMatrix._31 = (float)t->temp->eigVec[2].x;  orientMatrix._32 = (float)t->temp->eigVec[2].y;  orientMatrix._33 = (float)t->temp->eigVec[2].z;  orientMatrix._34 = 0.0f;
	orientMatrix._41 = 0.0f;				   orientMatrix._42 = 0.0f;					  orientMatrix._43 = 0.0f;					 orientMatrix._44 = 1.0f;

	// Transform box center from local to object coordinates
	MatrixMult_ATran_B(&boxCenterObjectCoord, &orientMatrix, &t->temp->boxCenterBoxCoord);

// 9.A) Initialize obb for rendering
	if(depth+1 <= (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
	{
		if(g_UI->GetSetting<bool>(&var_drawOBBAsLines))
			initOBBForRenderingLines();
		else
			initOBBForRendering();
	}

// 10. Find mean point of verticies along largest axis (to find split point) --> If not possible, use second then third largest axes.
	// Already found mean points during 8
	OrderAxes(&boxDimension); 
	// Results will be:
	// double3	t->temp->axisOrder[3];  // [Largest, Middle, Smallest]			--> NOW USE SCRAP PAD (in obbtree)

// 11. Split axes between two children
	if(numFaces > 2 && !g_UI->GetSetting<bool>(&var_oneLevelOBBTree)) // ONE_LEVEL_OBBTREE = Stop the recursion to debug convex hull generation
	{
		t->temp->splitOK = SplitAxes(t->temp->axisOrder[0], vertexBuffer,t->temp->indices_child1,&t->temp->child1NumIndicies,t->temp->indices_child2,&t->temp->child2NumIndicies);
		if(!t->temp->splitOK)
		{
#ifdef _DEBUG_OBBBUILD_WARNINGS
			wchar_t OutputText[128]; swprintf(OutputText,127,L"     --> WARNING: splitting using second largest axis at depth %d, numFaces %d\n",depth,numFaces);
			OutputDebugString(OutputText);
#endif
			t->temp->splitOK = SplitAxes(t->temp->axisOrder[1], vertexBuffer,t->temp->indices_child1,&t->temp->child1NumIndicies,t->temp->indices_child2,&t->temp->child2NumIndicies);
			if(!t->temp->splitOK)
			{
#ifdef _DEBUG_OBBBUILD_WARNINGS
			wchar_t OutputText[128]; swprintf(OutputText,127,L"     --> WARNING: splitting using third largest axis at depth %d, numFaces %d\n",depth,numFaces);
			OutputDebugString(OutputText);
#endif
				t->temp->splitOK = SplitAxes(t->temp->axisOrder[2], vertexBuffer,t->temp->indices_child1,&t->temp->child1NumIndicies,t->temp->indices_child2,&t->temp->child2NumIndicies);
			}
		}
		
		if(!t->temp->splitOK) // If it's still not OK, just just split arbitrarily --> Only happens when a small number of triangle centers project to the same point
		{
			t->temp->splitOK = SplitAxesArbitrary(vertexBuffer,t->temp->indices_child1,&t->temp->child1NumIndicies,t->temp->indices_child2,&t->temp->child2NumIndicies);
#ifdef _DEBUG_OBBBUILD_WARNINGS
			wchar_t OutputText[128]; swprintf(OutputText,127,L"     --> WARNING: Axis was split arbitrarily at depth %d, numFaces %d\n",depth,numFaces);
			OutputDebugString(OutputText);
#endif
		}

		if(!t->temp->splitOK)
			throw std::runtime_error("obbox::BuildOBBTree() - Something went wrong, we couldn't split the faces");
	}
	else if(numFaces == 2)
	{
		t->temp->splitOK = SplitTwo(vertexBuffer,t->temp->indices_child1,&t->temp->child1NumIndicies,t->temp->indices_child2,&t->temp->child2NumIndicies);
	}
	else
		t->temp->splitOK = false;

	// Case 1: CANNOT RECURSIVELY SUBDIVIDE FURTHER OR WE'RE ON A LEAF NODE
	// "If the group of polygons cannot be partitioned along any axis by this criterion, then the group is considered indivisible"
	if(!t->temp->splitOK) 
	{
		// No work to do on childNodes.
		childNode1 = -1; childNode2 = -1;
		isLeaf = 1;
		// Don't delete the indicies, keep them until closing.
	}
	else
	{
		isLeaf = 0;
	// Case 2: CAN SUBDIVIDE FURTHER.
// 12. Initialize child nodes with new index arrays
		childNode1 = t->tree->Add_retIndex(); childNode2 = t->tree->Add_retIndex(); // Get the next avaliable node indicies and their pointers
		obbox * p_childNode1 = t->tree->GetElem(childNode1);
		obbox * p_childNode2 = t->tree->GetElem(childNode2);

		p_childNode1->numFaces	= (int)(t->temp->child1NumIndicies/3.0);
		p_childNode1->parent	= curIndex;
		p_childNode1->depth		= depth+1;
		p_childNode1->t			= t;

		p_childNode2->numFaces	= (int)(t->temp->child2NumIndicies/3.0);
		p_childNode2->parent	= curIndex;
		p_childNode2->depth		= depth+1;
		p_childNode2->t			= t;

#ifdef _DEBUG
		if( (p_childNode1->numFaces + p_childNode2->numFaces) != numFaces )
			throw std::runtime_error("obbox::BuildOBBTree() - Something went wrong.  There are more faces in child nodes than in parent!");
#endif

		// Copy children indices from temporary array into master array
		p_childNode1->indices = indices; // first child starts where parent starts
		for(int i = 0; i < t->temp->child1NumIndicies; i ++)
			t->indices[p_childNode1->indices + i] = t->temp->indices_child1[i];

		p_childNode2->indices = p_childNode1->indices + t->temp->child1NumIndicies; // second child starts where first child finishes
		for(int i = 0; i < t->temp->child2NumIndicies; i ++)
			t->indices[p_childNode2->indices + i] = t->temp->indices_child2[i];

		indices = -1; // Like deleting index array --> will throw runtime error if we try to dereference index = -1

		// Recursively build OBB Tree --> Pass references to stack on higher level
		obbStack->push_back(childNode1);
		obbStack->push_back(childNode2);
	}
}
/************************************************************************/
/* Name:		SplitAxes												*/
/* Description: Partition faces on two sides of plane formed by axes	*/
/*				and split point											*/
/* Gottschalk et. al. "OBBTree: A Hierarchical..." pg 4					*/
/************************************************************************/
bool obbox::SplitAxes(double3 axis, D3DXVECTOR3 * vertexBuffer, int * child1Index, int * child1NumIndicies, int * child2Index, int * child2NumIndicies)
{
	// Get the split point
	switch(g_UI->GetComboBoxVal(&var_OBBBuildRule))
	{
	case OBB_GEOMETRIC_SPLIT:
		t->temp->mean = CalcSplitPointSplit(axis, vertexBuffer);
		break;
	case OBB_CENTROID_MEDIAN:
		t->temp->mean = CalcSplitPointMedian(axis, vertexBuffer);
		break;
	case OBB_CENTROID_MEAN:
		t->temp->mean = CalcSplitPointMean(axis, vertexBuffer);
		break;
	default:
		throw std::runtime_error("obbox::SplitAxes() - Unrecognised build rule");
	}

	// Parameterized plane: F(x) : (x-p1).n = 0, where x is a vector, p1 is a point on the plane, n is the unit normal to the plane
	double3 planePoint = axis * t->temp->mean;	// p1
	// Go through each trangle and partition it on either side of the plane
	*child1NumIndicies = 0; *child2NumIndicies = 0;
	double p[3]; double q[3]; double r[3]; double3 curCenter;
	for(int i = 0; i < numFaces; i ++)
	{
		p[0] = (double)vertexBuffer[t->indices[indices + i*3 + 0]].x;	// Vertex 0.x
		p[1] = (double)vertexBuffer[t->indices[indices + i*3 + 0]].y;	// Vertex 0.y
		p[2] = (double)vertexBuffer[t->indices[indices + i*3 + 0]].z;	// Vertex 0.z
		q[0] = (double)vertexBuffer[t->indices[indices + i*3 + 1]].x;	// Vertex 1.x
		q[1] = (double)vertexBuffer[t->indices[indices + i*3 + 1]].y;	// Vertex 1.y
		q[2] = (double)vertexBuffer[t->indices[indices + i*3 + 1]].z;	// Vertex 1.z
		r[0] = (double)vertexBuffer[t->indices[indices + i*3 + 2]].x;	// Vertex 2.x
		r[1] = (double)vertexBuffer[t->indices[indices + i*3 + 2]].y;	// Vertex 2.y
		r[2] = (double)vertexBuffer[t->indices[indices + i*3 + 2]].z;	// Vertex 2.z
		CentroidConvexhullTriangle(&curCenter, p, q, r);
		double dotprod = dot(axis,curCenter - planePoint);
		if(dotprod >= 0.0)
		{
			child1Index[*child1NumIndicies + 0] = t->indices[indices + i*3 + 0]; // Vertex 0
			child1Index[*child1NumIndicies + 1] = t->indices[indices + i*3 + 1]; // Vertex 1
			child1Index[*child1NumIndicies + 2] = t->indices[indices + i*3 + 2]; // Vertex 2
			*child1NumIndicies += 3;
		}
		else
		{
			child2Index[*child2NumIndicies + 0] = t->indices[indices + i*3 + 0]; // Vertex 0
			child2Index[*child2NumIndicies + 1] = t->indices[indices + i*3 + 1]; // Vertex 1
			child2Index[*child2NumIndicies + 2] = t->indices[indices + i*3 + 2]; // Vertex 2
			*child2NumIndicies += 3;
		}
	}
	//if(abs(*child1NumIndicies - *child2NumIndicies) > 3)
	//	throw std::runtime_error("obbox::SplitAxes() - Uneven split by more than 1 face");
	if(*child1NumIndicies<3 || *child2NumIndicies<3) // There isn't at least one triangle in each child...
		return false; // We didn't split anything --> All indicies went on one side!
	else
		return true;
}
/************************************************************************/
/* Name:		CalcSplitPointMedian									*/
/* Description: Split point calculated by median of triangle centroids	*/
/************************************************************************/
double obbox::CalcSplitPointMedian(double3 axis, D3DXVECTOR3 * vertexBuffer)
{
	// BUILD UP A VECTOR OF TRIANGLE CENTROIDS PROJECTED ONTO EACH AXIS THEN USE STD LINEAR TIME SELECTION ALGORITHM TO FIND MEDIAN
	// I DON'T KNOW IF THIS IS OPTIMAL FOR OBBTREE DETECTION, BUT IT MAKES EVERY AXIS SPLIT EQUAL
	double p[3]; double q[3]; double r[3]; double3 curCenter;
	t->temp->centroids.clear();
	for(int i = 0; i < numFaces; i ++)
	{
		p[0] = (double)vertexBuffer[t->indices[indices + i*3 + 0]].x;	// Vertex 0.x
		p[1] = (double)vertexBuffer[t->indices[indices + i*3 + 0]].y;	// Vertex 0.y
		p[2] = (double)vertexBuffer[t->indices[indices + i*3 + 0]].z;	// Vertex 0.z
		q[0] = (double)vertexBuffer[t->indices[indices + i*3 + 1]].x;	// Vertex 1.x
		q[1] = (double)vertexBuffer[t->indices[indices + i*3 + 1]].y;	// Vertex 1.y
		q[2] = (double)vertexBuffer[t->indices[indices + i*3 + 1]].z;	// Vertex 1.z
		r[0] = (double)vertexBuffer[t->indices[indices + i*3 + 2]].x;	// Vertex 2.x
		r[1] = (double)vertexBuffer[t->indices[indices + i*3 + 2]].y;	// Vertex 2.y
		r[2] = (double)vertexBuffer[t->indices[indices + i*3 + 2]].z;	// Vertex 2.z
		CentroidConvexhullTriangle(&curCenter, p, q, r);

		// Project centroid onto axis and add it to the list
		t->temp->projection = dot(curCenter,axis);
		t->temp->centroids.push_back(t->temp->projection);
	}
	// Find the median o(n)
	std::size_t size = t->temp->centroids.end() - t->temp->centroids.begin();
	std::size_t middleIndex = size / 2;
	std::vector<double>::iterator target = t->temp->centroids.begin() + middleIndex;
	std::nth_element(t->temp->centroids.begin(), target, t->temp->centroids.end()); // "Magic" O(n) to find median.  Uses selection algorithm to get better than O(n log(n))
	
	if(size % 2 != 0)		//Odd number of elements
		return *target;
	else					//Even number of elements
	{
		double a = *target;
		std::vector<double>::iterator targetNeighbour =  target-1;
		std::nth_element(t->temp->centroids.begin(), targetNeighbour, t->temp->centroids.end());
		return (a + *targetNeighbour) / 2.0; // Average of the two values
	}
}
/************************************************************************/
/* Name:		CalcSplitPointMean										*/
/* Description: Split point calculated by mean of triangle centroids	*/
/************************************************************************/
double obbox::CalcSplitPointMean(double3 axis, D3DXVECTOR3 * vertexBuffer)
{
	double p[3]; double q[3]; double r[3]; double3 curCenter;
	double sum;
	sum = 0.0;
	for(int i = 0; i < numFaces; i ++)
	{
		p[0] = (double)vertexBuffer[t->indices[indices + i*3 + 0]].x;	// Vertex 0.x
		p[1] = (double)vertexBuffer[t->indices[indices + i*3 + 0]].y;	// Vertex 0.y
		p[2] = (double)vertexBuffer[t->indices[indices + i*3 + 0]].z;	// Vertex 0.z
		q[0] = (double)vertexBuffer[t->indices[indices + i*3 + 1]].x;	// Vertex 1.x
		q[1] = (double)vertexBuffer[t->indices[indices + i*3 + 1]].y;	// Vertex 1.y
		q[2] = (double)vertexBuffer[t->indices[indices + i*3 + 1]].z;	// Vertex 1.z
		r[0] = (double)vertexBuffer[t->indices[indices + i*3 + 2]].x;	// Vertex 2.x
		r[1] = (double)vertexBuffer[t->indices[indices + i*3 + 2]].y;	// Vertex 2.y
		r[2] = (double)vertexBuffer[t->indices[indices + i*3 + 2]].z;	// Vertex 2.z
		CentroidConvexhullTriangle(&curCenter, p, q, r);

		// Project centroid onto axis and add it to the running sum
		t->temp->projection = dot(curCenter,axis);
		sum += t->temp->projection;
	}
	// Find the mean
	return sum / (double)numFaces;
}
/************************************************************************/
/* Name:		CalcSplitPointSplit										*/
/* Description: Split point calculated by center of longest axis		*/
/************************************************************************/
double obbox::CalcSplitPointSplit(double3 axis, D3DXVECTOR3 * vertexBuffer)
{
	// Project box center onto axis abd return it
	double3 curCenter;
	curCenter.x = boxCenterObjectCoord.x;
	curCenter.y = boxCenterObjectCoord.y;
	curCenter.z = boxCenterObjectCoord.z;
	return dot(curCenter,axis);
}
/************************************************************************/
/* Name:		SplitAxesArbitrary										*/
/* Description: Partition faces arbitrarily, half to one side and half  */
/*              to the other											*/
/************************************************************************/
bool obbox::SplitAxesArbitrary(D3DXVECTOR3 * vertexBuffer, int * child1Index, int * child1NumIndicies, int * child2Index, int * child2NumIndicies)
{
	// Go through each trangle and partition it on either side of the plane
	*child1NumIndicies = 0; *child2NumIndicies = 0;
	int splitPoint = (int)floor((double)numFaces / 2.0);
	for(int i = 0; i < numFaces; i ++)
	{
		if(i < splitPoint)
		{
			child1Index[*child1NumIndicies + 0] = t->indices[indices + i*3 + 0]; // Vertex 0
			child1Index[*child1NumIndicies + 1] = t->indices[indices + i*3 + 1]; // Vertex 1
			child1Index[*child1NumIndicies + 2] = t->indices[indices + i*3 + 2]; // Vertex 2
			*child1NumIndicies += 3;
		}
		else
		{
			child2Index[*child2NumIndicies + 0] = t->indices[indices + i*3 + 0]; // Vertex 0
			child2Index[*child2NumIndicies + 1] = t->indices[indices + i*3 + 1]; // Vertex 1
			child2Index[*child2NumIndicies + 2] = t->indices[indices + i*3 + 2]; // Vertex 2
			*child2NumIndicies += 3;
		}
	}
	if(*child1NumIndicies<3 || *child2NumIndicies<3) // There isn't at least one triangle in each child...
		return false; // We didn't split anything --> All indicies went on one side!
	else
		return true;
}
/************************************************************************/
/* Name:		SplitTwo												*/
/* Description: To be used when there are only two faces, split. Put    */
/*              one on child1 and the other ond child2.					*/
/************************************************************************/
bool obbox::SplitTwo(D3DXVECTOR3 * vertexBuffer, int * child1Index, int * child1NumIndicies, int * child2Index, int * child2NumIndicies)
{
	if(numFaces != 2)
		throw std::runtime_error("obbox::SplitTwo() - Called when the number of faces isn't two!");

	// Go through each trangle and partition it on either side of the plane
	*child1NumIndicies = 0; *child2NumIndicies = 0;
	int splitPoint = 1;
	for(int i = 0; i < numFaces; i ++)
	{
		if(i < splitPoint)
		{
			child1Index[*child1NumIndicies + 0] = t->indices[indices + i*3 + 0]; // Vertex 0
			child1Index[*child1NumIndicies + 1] = t->indices[indices + i*3 + 1]; // Vertex 1
			child1Index[*child1NumIndicies + 2] = t->indices[indices + i*3 + 2]; // Vertex 2
			*child1NumIndicies += 3;
		}
		else
		{
			child2Index[*child2NumIndicies + 0] = t->indices[indices + i*3 + 0]; // Vertex 0
			child2Index[*child2NumIndicies + 1] = t->indices[indices + i*3 + 1]; // Vertex 1
			child2Index[*child2NumIndicies + 2] = t->indices[indices + i*3 + 2]; // Vertex 2
			*child2NumIndicies += 3;
		}
	}
	if(*child1NumIndicies<3 || *child2NumIndicies<3) // There isn't at least one triangle in each child...
		return false; // We didn't split anything --> All indicies went on one side!
	else
		return true;
}
/************************************************************************/
/* Name:		AreaConvexhullTriangle									*/
/* Description: Return the area of a single convex hull triangle		*/
/* Gottschalk et. al. "OBBTree: A Hierarchical..." pg 3					*/
/************************************************************************/
	// MOVED TO math_types.cpp
/************************************************************************/
/* Name:		CentroidConvexhullTriangle								*/
/* Description: Return the area of a single convex hull triangle		*/
/* Gottschalk et. al. "OBBTree: A Hierarchical..." pg 3					*/
/************************************************************************/
void obbox::CentroidConvexhullTriangle(double3 * ret, double * p, double * q, double * r)
{
	ret->x = (p[0] + q[0] + r[0]) / 3.0;
	ret->y = (p[1] + q[1] + r[1]) / 3.0;
	ret->z = (p[2] + q[2] + r[2]) / 3.0;
}
/************************************************************************/
/* Name:		FindUniqueIndexSet										*/
/* Description: Return an array of non-repeating indices - O(2n)		*/
/*		To methods listed (both order n), add values to a hash set OR   */
/*		use a boolian flag array.  Another approach could be a			*/
/*		divide-and-conquer or recursive sub-divisons and then merging??	*/
/************************************************************************/
// Solution 1 --> Use large boolian array
void obbox::FindUniqueIndexSet_BoolArray(int ** retArray, int * retSize, int * pArray, int numElements)
{
	// I used to do this using a hash_set --> But it turns out this brute force approach
	// is easier and about an order of magnitude faster.  At least this is faster than the obvious O(n^2) algorithm

	// Find largest index in the array
	int max_index = -1;
	for(int i = 0; i < numElements; i ++)
	{
		if(pArray[i] > max_index)
			max_index = pArray[i];
	}

	bool * bool_array = new bool[max_index+1]; // Very space inefficient
	if(bool_array == NULL)
		throw std::runtime_error("obbox::FindUniqueIndexSet_BoolArray() - Not enough space for new");
	
	// Inialize all elements in the boolian array to false
	ZeroMemory(bool_array, sizeof(bool)*(max_index+1)); 

	// Mark each element in the array that exists
	*retSize = 0;
	for(int i = 0; i < numElements; i ++)
	{
		if(bool_array[pArray[i]] == false)
		{
			*retSize += 1;
			bool_array[pArray[i]] = true;
		}
	}

	*retArray = new int[*retSize];
	// Go through the boolian array and see what elements we have, and add them to the output.
	int index = 0; // Could do this with a list<int>...
	for(int i = 0; i < (max_index+1); i ++)
	{
		if(bool_array[i])
		{
			(*retArray)[index] = i;
			index ++;
		}
	}
}
// Solution 2 --> Use custom hash Table
void obbox::FindUniqueIndexSet_HashSet(int ** retArray, int * retSize, int * pArray, int pArray_start, int numElements)

// Looks clean here, but is in fact slower I think...  test later
{
	hashset_int uniqueSet(numElements, 4, 2); // numBuckets, starting bucket size, maxLoad
	for(int i = pArray_start; i < (pArray_start+numElements); i ++) // Add all elements to the hash set.
		uniqueSet.AddKey(&pArray[i]);
	uniqueSet.ReturnArray(retSize, retArray);
}

/************************************************************************/
/* Name:		OrderAxes												*/
/* Description: Quick and dirty sorting of axes based on their box      */
/*			dimension lengths.											*/
/************************************************************************/
void obbox::OrderAxes(D3DXVECTOR3 * boxDimension)
{
	// Lazy way to sort box axes (O(n^2)), but only 3 values --> No point doing faster sort! Results will be:
	// double3	t->axisOrder[3];  // [Largest, Middle, Smallest]			--> NOW USE SCRAP PAD (in obbtree)
	if(boxDimension->x > boxDimension->y && boxDimension->x > boxDimension->z) // x largest
	{
		t->temp->axisOrder[0] = t->temp->eigVec[0];
		if(boxDimension->y > boxDimension->z) // y second largest
		{
			t->temp->axisOrder[1] = t->temp->eigVec[1]; t->temp->axisOrder[2] = t->temp->eigVec[2];
		}
		else // z second largest
		{
			t->temp->axisOrder[1] = t->temp->eigVec[2]; t->temp->axisOrder[2] = t->temp->eigVec[1];
		}
	}
	else if(boxDimension->y > boxDimension->z) // y largest
	{
		t->temp->axisOrder[0] = t->temp->eigVec[1];
		if(boxDimension->x > boxDimension->z) // x second largest
		{
			t->temp->axisOrder[1] = t->temp->eigVec[0]; t->temp->axisOrder[2] = t->temp->eigVec[2];
		}
		else // z second largest
		{
			t->temp->axisOrder[1] = t->temp->eigVec[2]; t->temp->axisOrder[2] = t->temp->eigVec[0];
		}
	} else // z largest
	{
		t->temp->axisOrder[0] = t->temp->eigVec[2];
		if(boxDimension->x > boxDimension->y) // x second largest
		{
			t->temp->axisOrder[1] = t->temp->eigVec[0]; t->temp->axisOrder[2] = t->temp->eigVec[1];
		}
		else // y second largest
		{
			t->temp->axisOrder[1] = t->temp->eigVec[1]; t->temp->axisOrder[2] = t->temp->eigVec[0];
		}
	}
}

#pragma warning( pop )			// Edit Jonathan Tompson - 31st Jan 2011