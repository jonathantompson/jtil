// File:		obbox.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is primary OBB tree data structure to impliment the paper:
// S. Gottschalk et. al. "OBBTree: A Hierarchical Structure for Rapid Iterference Detection"

#include "physics\obbtree_related\obbox.h"
#include "physics\obbtree_related\obboxRenderitems.h"
#include "utils_and_misc_classes\SIMD_helpers\dataAlign.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#pragma warning( push )			// Edit Jonathan Tompson - 31st Jan 2011
#pragma warning( disable:4238 )	// Edit Jonathan Tompson - 31st Jan 2011

#define NULL 0

#ifndef DATA_ALIGNMENT
#define DATA_ALIGNMENT 16
#endif

/************************************************************************/
/* Name:		obbox													*/
/* Description: Blank Constructor										*/
/************************************************************************/
obbox::obbox()
{
	parent = -1; childNode1 = -1; childNode2 = -1; 
	indices = -1; // will throw a warning if you try and dereference it
	numFaces = 0;
	depth = 0;
	t = NULL;
	renderitems = NULL;

	// Null out the padding
	padding_0 = 0;

}
/************************************************************************/
/* Name:		obbox													*/
/* Description: Constructor, initialize with vertex length n.			*/
/************************************************************************/
obbox::obbox(int numFacesIn, int parentIn, int depthIn)
{
	t = NULL; 
	parent = parentIn;
	childNode1 = -1; childNode2 = -1;
	indices = -1; // will throw a warning if you try and dereference it
	numFaces = numFacesIn;
	renderitems = NULL;
	depth = depthIn;
}
/************************************************************************/
/* Name:		~obbox													*/
/* Description: Destructor												*/
/************************************************************************/
obbox::~obbox()
{
	if(renderitems) { delete renderitems; renderitems = NULL; }
}

/************************************************************************/
/* Name:		operator =												*/
/* Description: Assignment operator for vector array in obbtree			*/
/*				(when resizing)											*/
/************************************************************************/
obbox & obbox::operator = (const obbox & o)
{
	if (this != &o) // make sure not same object
	{
		// Perform shallow copies (same order as obbox.h if you need to check)
		parent = o.parent; 
		childNode1 = o.childNode1; 
		childNode2 = o.childNode2; 
		t = o.t;
		boxDimension = o.boxDimension;
		boxCenterObjectCoord = o.boxCenterObjectCoord;
		indices = o.indices;
		numFaces = o.numFaces; 
		isLeaf = o.isLeaf;
		depth = o.depth;
		renderitems = o.renderitems;
	}
	return *this;  // Return ref for multiple assignment
}
/************************************************************************/
/* Name:		CheckForDuplicateVertex									*/
/* Description: O(n^2) function just for debugging.  Checks that there  */
/*		aren't duplicate verticies in the index array.					*/
/************************************************************************/
bool CheckForDuplicateVertex(int numIndicies, int * indicies, D3DXVECTOR3 * verticies)
{
	for(int i = 0 ; i < (numIndicies-1); i ++)
	{
		for(int j = i+1; j < numIndicies; j++)
		{
			if(verticies[indicies[i]] == verticies[indicies[j]])
				return true;
		}
	}
	return false;
}

// Static variables
__declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		obbox::t_aOBB;
__declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		obbox::t_bOBB;
__declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		obbox::t_a_world;
__declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		obbox::t_b_world;
__declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		obbox::R;
__declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		obbox::R_aOBB_world_Tran;
__declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		obbox::R_bOBB_world;
__declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		obbox::R_aOBB_Tran;
__declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		obbox::R_bOBB_Tran;
__declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		obbox::t_world;	
__declspec(align(DATA_ALIGNMENT)) D3DXVECTOR3		obbox::t_;
__declspec(align(DATA_ALIGNMENT)) float				obbox::a_OBBDim[4]; // a_OBBDim[4]
__declspec(align(DATA_ALIGNMENT)) float				obbox::b_OBBDim[4]; // a_OBBDim[4]
__declspec(align(DATA_ALIGNMENT)) float				obbox::ra;
__declspec(align(DATA_ALIGNMENT)) float				obbox::rb;
__declspec(align(DATA_ALIGNMENT)) D3DXMATRIXA16		obbox::AbsR;
__declspec(align(DATA_ALIGNMENT)) UINT				obbox::comp_result;

#pragma warning( pop )			// Edit Jonathan Tompson - 31st Jan 2011