// File:		obbox.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is primary OBB tree data structure to impliment the paper:
// S. Gottschalk et. al. "OBBTree: A Hierarchical Structure for Rapid Iterference Detection"

#ifndef obbox_h
#define obbox_h

#include    "dataAlign.h"
#include	"dxInclude.h"
#include	<vector>
#include	<stdlib.h>

class rbobject;
class rbobjectMesh;
class rbobjectMeshData;
class obbtree;
class obboxRenderitems;
class double3x3;
class double3;
struct VertexPosNormCol;

__declspec(align(DATA_ALIGNMENT)) class obbox 
{
public:
									obbox();
									obbox(int numFacesIn,  int parentIn, int depthIn);
									~obbox();

	obbox & operator = (const obbox & o); // Assignment operator

	friend class rbobject; // This is lazy I know!
	friend class rbmesh;
	friend class debugObject;
	friend class obbtree;
	friend class app;
	friend int main ();

	// CGAL function:
	friend void Copy_Mesh_To_CH(obbox * curobbox, rbobjectMeshData * meshData, double * vertexSet, int vertexSetSize, double ** cHullVert, UINT * cHullVertCount, PUINT * cHullInd, UINT * cHullIndCount );

	void							BuildOBBTree(int curIndex, std::vector<int> * obbStack, rbobjectMeshData * meshData); // build one level of the OBB tree

	// Helper functions to build OBB Tree
	static void						CentroidConvexhullTriangle(double3 * ret, double * p, double * q, double * r);
	bool							SplitAxes(double3 axis, D3DXVECTOR3 * vertexBuffer, int * child1Index, int * child1NumIndicies, int * child2Index, int * child2NumIndicies);
	double							CalcSplitPointMedian(double3 axis, D3DXVECTOR3 * vertexBuffer);
	double							CalcSplitPointMean(double3 axis, D3DXVECTOR3 * vertexBuffer);
	double							CalcSplitPointSplit(double3 axis, D3DXVECTOR3 * vertexBuffer);
	bool							SplitAxesArbitrary(D3DXVECTOR3 * vertexBuffer, int * child1Index, int * child1NumIndicies, int * child2Index, int * child2NumIndicies);
	bool							SplitTwo(D3DXVECTOR3 * vertexBuffer, int * child1Index, int * child1NumIndicies, int * child2Index, int * child2NumIndicies);
	void							OrderAxes(D3DXVECTOR3 * boxDimension);

	// Convex Hull Generator --> OLD VERSION OF CODE: MOVED TO:   hull.cpp::SM_chull()
	//	void							GetConvexHull_SMelax(double * vertexSet, int uniqueIndexSetSize, HullDesc * desc, PHullResult * hr);


	// Two versions of the same thing...  Find out later which is fater
	static void						FindUniqueIndexSet_BoolArray(int ** retArray, int * retSize, int * pArray, int numElements);
	static void						FindUniqueIndexSet_HashSet(int ** retArray, int * retSize, int * pArray, int pArray_start, int numElements); // More memory efficient I think

	// Rendering (mostly for debugging)
	void							initConvexHullForRendering();
	void							initOBBForRendering();
	void							initOBBForRenderingLines();
	void							DrawConvexHull(D3DXMATRIXA16 * matW);
	void							DrawOBB(D3DXMATRIXA16 * matW);
	void							DrawOBBSM(D3DXMATRIXA16 * matW);
	static void						CalcObboxVerticesIndices(VertexPosNormCol * v, WORD * indexBuf, D3DXVECTOR3 * boxCenterObjectCoord, D3DXVECTOR3 * boxDimension, D3DXMATRIXA16 * orientMatrix, D3DCOLOR color );
	static void						CalcObboxVerticesIndicesLines(VertexPosNormCol * v, WORD * indexBuf, D3DXVECTOR3 * boxCenterObjectCoord, D3DXVECTOR3 * boxDimension, D3DXMATRIXA16 * orientMatrix, D3DCOLOR color );

	// Collision tests
	static int						TestOBBCollision( obbox * a, rbobjectMesh * a_rbo, obbox * b, rbobjectMesh * b_rbo );
	static int						TestOBBCollision_DirectX( obbox * a, rbobjectMesh * a_rbo, obbox * b, rbobjectMesh * b_rbo );
	static int						TestOBBCollision_SIMD( obbox * a, rbobjectMesh * a_rbo, obbox * b, rbobjectMesh * b_rbo );
	static int						TestTriCollision( obbox * OBB1, rbobjectMesh * a_rbo, obbox * OBB2, rbobjectMesh * b_rbo  );

	// Temporary, until issues are resolved
	static int						TestOBBCollision_original( obbox * a, rbobjectMesh * a_rbo, obbox * b, rbobjectMesh * b_rbo );

private:
	//		start 1st 16 byte block (3 * sizeof(int) + sizeof(void *) = 16)
	int								parent, childNode1, childNode2;
	obbtree *						t;								
	//		start 2nd 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 					orientMatrix;			// This is is the (U,V,N)->(X,Y,Z) local to world space transform.  Axes run in ROWS (horizontally)
	//		start 6th 16 byte block (3 * sizeof(float) + sizeof(int) = 16)
	D3DXVECTOR3						boxDimension;			// 3 Box dimensions (along axes) -> length / 2
	int								depth;
	//		start 7th 16 byte block (3 * sizeof(float) + sizeof(int) = 16)
	D3DXVECTOR3						boxCenterObjectCoord;	// The boxes center in object coordinates
	int								isLeaf;

	// List of triangle vertex indices (groups of three indices to a triangle)
	// Match indices to verticies through rbo object --> Only for leaf nodes
	//		start 8th 16 byte block (3 * sizeof(int) + sizeof(void *) = 16)
	int								indices; // This is a starting index in the obbtree "indices" array.  Avoid dynamic memory for every cell.
	int								numFaces; // numindices = numFaces * 3
	obboxRenderitems *				renderitems; // Render items + saved convex hull now moved to obboxRenderitems class to save space in obbox array
	int								padding_0;

	// temporary variables for collision detection routine...
	// If you add more they need to be initialized in "InitTempVars()" and appened to obbox_FuncsCollision.cpp list
	static __declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		t_aOBB;
	static __declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		t_bOBB;
	static __declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		t_a_world;
	static __declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		t_b_world;
	static __declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		R;
	static __declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		R_aOBB_world_Tran;
	static __declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		R_bOBB_world;
	static __declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		R_aOBB_Tran;
	static __declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		R_bOBB_Tran;
	static __declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		t_world;	
	static __declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		t_;
	static __declspec(align(DATA_ALIGNMENT)) float				a_OBBDim[4]; // a_OBBDim[4]
	static __declspec(align(DATA_ALIGNMENT)) float				b_OBBDim[4]; // a_OBBDim[4]
	static __declspec(align(DATA_ALIGNMENT)) float				ra;
	static __declspec(align(DATA_ALIGNMENT)) float				rb;
	static __declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		AbsR;
	static __declspec(align(DATA_ALIGNMENT)) UINT				comp_result;
};

bool CheckForDuplicateVertex(int numIndicies, int * indicies, D3DXVECTOR3 * verticies);

#endif