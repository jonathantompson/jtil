// File:		obbtree.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is header obbject to store OBB tree data:
// S. Gottschalk et. al. "OBBTree: A Hierarchical Structure for Rapid Iterference Detection"

#include "physics\obbtree_related\obbtree.h"
#include "physics\obbtree_related\obbox.h" 
#include "physics\obbtree_related\obboxRenderitems.h" 
#include "physics\obbtree_related\obbtreeTempVar.h" 
#include <UI\UI.h>
#include <UI\varNames.h>
#include "utils_and_misc_classes\data_structures\vecA.h"
#include "utils_and_misc_classes\SIMD_helpers\dataAlign.h"

#include <new>        // Must #include this to use "placement new"

//#include "utils_and_misc_classes\new_redefine.h" // CAN'T USE THIS WITH PLACEMENT NEW

#pragma warning( push )			// Edit Jonathan Tompson - 31st Jan 2011
#pragma warning( disable:4238 )	// Edit Jonathan Tompson - 31st Jan 2011

//#define CHECK_TESTOBBCOLLISION_SIMD // if defined obbtree will test both collision routines and compare results.

obbtree::obbtree(DWORD _numFaces, int _numVerticies)
{
	numFaces = _numFaces;
	numVerticies = _numVerticies;
	tree = new vecA<obbox>(numFaces*2); // 2n - 1 cells in the tree --> This is a vector that can increase in size.
	indices = new int[numFaces*3]; // 3 indicies per face
	numIndices = numFaces*3;
	curOBBRender = 0;
	scaleSet = 0;
	temp = NULL;
}
obbtree::~obbtree()
{
	if(tree) 
	{ 
		for(UINT i = 0; i < tree->Size(); i ++)
		{
			if(tree->GetElem(i)->renderitems)
			{ delete tree->GetElem(i)->renderitems; tree->GetElem(i)->renderitems = NULL; }
		}
		tree->Clear(); 
		delete tree; 
		tree = NULL; 
	}
	if(indices) { delete [] indices; indices = NULL; }

	aligned_delete_destructor(temp,obbtreeTempVar);
}
/************************************************************************/
/* Name:		GetObbToRender											*/
/* Description: Just dereference the array and gets the current obb we	*/
/*              want to render											*/
/************************************************************************/
obbox * obbtree::GetObbToRender()
{
	return tree->GetElem(curOBBRender);
}
/************************************************************************/
/* Name:		MoveUp / MoveLeft / MoveRight							*/
/* Description: Functions used to traverse the curOBBRender pointer		*/
/*				along the OBBTree.										*/
/************************************************************************/
void obbtree::MoveUp()
{
	obbox * currObb = GetObbToRender();
	if(currObb->parent != -1)
		curOBBRender = currObb->parent;
}
void obbtree::MoveLeft()
{
	obbox * currObb = GetObbToRender();
	if(!currObb->isLeaf && (currObb->depth+1) < (int)(g_UI->GetComboBoxVal(&var_OBBRenderDepth)))
		curOBBRender = currObb->childNode1;
}
void obbtree::MoveRight()
{
	obbox * currObb = GetObbToRender();
	if(!currObb->isLeaf && (currObb->depth+1) < (int)(g_UI->GetComboBoxVal(&var_OBBRenderDepth)))
		curOBBRender = currObb->childNode2;
}

#pragma warning( pop )			// Edit Jonathan Tompson - 31st Jan 2011