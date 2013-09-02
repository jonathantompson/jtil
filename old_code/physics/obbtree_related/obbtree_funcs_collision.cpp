// File:		obbtree.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is header obbject to store OBB tree data:
// S. Gottschalk et. al. "OBBTree: A Hierarchical Structure for Rapid Iterference Detection"

#include "physics\obbtree_related\obbtree.h"
#include "physics\obbtree_related\obbox.h" 
#include "app.h"
#include "physics\objects\rbobjectMesh.h"
#include "physics\objects\rbobjectMeshData.h"
#include "objectManager\objectManager.h"
#include "physics\collision_response\collision.h"
#include "physics\physics.h"
#include "utils_and_misc_classes\data_structures\intPair.h"
#include "utils_and_misc_classes\data_structures\vecA.h"
#include "renderer\renderer_structures\debugObject.h"

#include <new>        // Must #include this to use "placement new"

//#include "utils_and_misc_classes\new_redefine.h" // CAN'T USE THIS WITH PLACEMENT NEW

#pragma warning( push )			// Edit Jonathan Tompson - 31st Jan 2011
#pragma warning( disable:4238 )	// Edit Jonathan Tompson - 31st Jan 2011

//#define CHECK_TESTOBBCOLLISION_SIMD // if defined obbtree will test both collision routines and compare results.

/************************************************************************/
/* Name:		TestOBBTreeCollision									*/
/* Description: Test two OBB trees for collision using separating		*/
/*				axis tests.	return number of collisions added to vector	*/
/************************************************************************/
void obbtree::TestOBBTreeCollision(rbobjectMesh * a, rbobjectMesh * b, bool useSSESIMD)
{
	if(!g_physics->obbox_pairs->IsEmpty())
		throw std::runtime_error("obbtree::TestOBBTreeCollision() - obbox_pairs should be empty!");

	if(a == NULL || b == NULL)
		throw std::runtime_error("obbtree::TestOBBTreeCollision() - Input is null");

	// We will process obbox entries according to the rule: DESCEND LARGEST AREA
	// Note: DESCEND LARGEST LENGTH might work just as well (and not require 6 multiplications to work out).
	// The pairs to process will be thrown on a stack (LIFO) stored in obbox_pairs (static to obbtree class).

	// Put the roots on the stack.
	intPair * tmpPair = NULL;
	tmpPair = g_physics->obbox_pairs->Add_retPtr(); 
	tmpPair->first = 0; tmpPair->second = 0; // Roots have index 0
	collision curCollision;
	obbox * curNodeA = NULL;
	obbox * curNodeB = NULL;
	obbtree * obbtree_a = a->meshData->GetOBBTree();
	obbtree * obbtree_b = b->meshData->GetOBBTree();

	while(!g_physics->obbox_pairs->IsEmpty())
	{
		intPair curPair = *g_physics->obbox_pairs->PopBack(); // Get the next index and remove it from the stack
		curNodeA = obbtree_a->tree->GetElem(curPair.first);
		curNodeB = obbtree_b->tree->GetElem(curPair.second);

		int colResult = 0;
		if(curNodeA->isLeaf && curNodeB->isLeaf)
			colResult = obbox::TestTriCollision(curNodeA, a, curNodeB, b);
		else
		{
#ifdef CHECK_TESTOBBCOLLISION_SIMD
			colResult = obbox::TestOBBCollision_SIMD(curNodeA, a, curNodeB, b);
			if((colResult != obbox::TestOBBCollision(curNodeA, a, curNodeB, b)) ||
			   (colResult != obbox::TestOBBCollision_DirectX(curNodeA, a, curNodeB, b)))
			{
#ifdef VISUAL_STUDIO
				DebugBreak();
#endif
				colResult = obbox::TestOBBCollision_SIMD(curNodeA, a, curNodeB, b);
				colResult = obbox::TestOBBCollision(curNodeA, a, curNodeB, b);
				colResult = obbox::TestOBBCollision_DirectX(curNodeA, a, curNodeB, b);
				throw std::runtime_error("obbtree::TestOBBTreeCollision() - TestOBBCollision_SIMD and TestOBBCollision didn't give the same result");
			}
#else
			if(useSSESIMD)
				colResult = obbox::TestOBBCollision_SIMD(curNodeA, a, curNodeB, b);
			else
				colResult = obbox::TestOBBCollision(curNodeA, a, curNodeB, b);
#endif
		}


		// Test collision at the current level --> Either stop here or add more pairs to the stack.
		if(colResult)
		{
			// If neither A or B are leaf nodes --> Most common, put this first
			if(!curNodeA->isLeaf && !curNodeB->isLeaf)
			{
				// Descend the box of largest area --> Optimal stratergy for fewest iterations
				// Not necessarily fastest since 6 mults are needed.
				// TAKES INTO ACCOUNT CONSTANT SCALING FROM OBJECT TO WORLD COORDINATES.
				if(((curNodeA->boxDimension.x * curNodeA->boxDimension.y * curNodeA->boxDimension.z) * (a->scale * a->scale * a->scale)) > 
				   ((curNodeB->boxDimension.x * curNodeB->boxDimension.y * curNodeB->boxDimension.z) * (b->scale * b->scale * b->scale)) )
				{
					tmpPair = g_physics->obbox_pairs->Add_retPtr(); 
					tmpPair->first = curNodeA->childNode1;		tmpPair->second = curPair.second;
					tmpPair = g_physics->obbox_pairs->Add_retPtr(); 
					tmpPair->first = curNodeA->childNode2;		tmpPair->second = curPair.second;
				}
				else
				{
					tmpPair = g_physics->obbox_pairs->Add_retPtr(); 
					tmpPair->first = curPair.first;				tmpPair->second = curNodeB->childNode1;
					tmpPair = g_physics->obbox_pairs->Add_retPtr(); 
					tmpPair->first = curPair.first;				tmpPair->second = curNodeB->childNode2;
				}
			}
			// If only a is a leaf
			else if (curNodeA->isLeaf && !curNodeB->isLeaf)
			{
				// Descend b's children
				tmpPair = g_physics->obbox_pairs->Add_retPtr(); 
				tmpPair->first = curPair.first;					tmpPair->second = curNodeB->childNode1;
				tmpPair = g_physics->obbox_pairs->Add_retPtr(); 
				tmpPair->first = curPair.first;					tmpPair->second = curNodeB->childNode2;
			}
			// If only b is a leaf
			else if (!curNodeA->isLeaf && curNodeB->isLeaf)
			{
				// Descend a's children
				tmpPair = g_physics->obbox_pairs->Add_retPtr(); 
				tmpPair->first = curNodeA->childNode1;			tmpPair->second = curPair.second;
				tmpPair = g_physics->obbox_pairs->Add_retPtr(); 
				tmpPair->first = curNodeA->childNode2;			tmpPair->second = curPair.second;
			}
			// If both are leaf nodes --> Least common, it goes last
			else
			{
				// Add the nodes to the return collisions array.
				curCollision.obbox_a = curNodeA;
				curCollision.obbox_b = curNodeB;
				curCollision.mesh_a = a;
				curCollision.mesh_b = b;
				curCollision.Type = COL_UNDEFINED;
				g_physics->OBBCollisions->Add(&curCollision);
			}
		}
	}
}
/************************************************************************/
/* Name:		TestOBBTreeCollisionDebug								*/
/* Description: Test one obbox at a time, rendering each result.		*/
/************************************************************************/
void obbtree::TestOBBTreeCollisionDebug(rbobjectMesh * a, rbobjectMesh * b, bool useSSESIMD)
{
	if(a == NULL || b == NULL)
		throw std::runtime_error("obbtree::TestOBBTreeCollisionDebug() - TestOBBTreeCollision: Input is null");
	a->UpdateMatricies(); // Update Rotation and Translation matricies (should have been done already, but O(1) anyway)
	b->UpdateMatricies(); // Update Rotation and Translation matricies (should have been done already, but O(1) anyway)

	g_app->m_EnterObbDebugMode = true;
	g_objectManager->ClearDebugObjects(); // Recursively clear the rest of the blue or red OBBes

	if(g_physics->obbox_pairs->IsEmpty()) // First time we're running through --> Start with root
		// Put the roots on the stack.
		g_physics->obbox_pairs->Add(&intPair(0,0));

	// We will process obbox entries according to the rule: DESCEND LARGEST AREA
	// Note: DESCEND LARGEST LENGTH might work just as well (and not require 6 multiplications to work out).
	// The pairs to process will be thrown on a stack (LIFO) stored in obbox_pairs (static to obbtree class).

	collision curCollision;
	obbtree * obbtree_a = a->meshData->GetOBBTree();
	obbtree * obbtree_b = b->meshData->GetOBBTree();

	obbox * curNodeA = NULL;
	obbox * curNodeB = NULL;
	intPair curPair = *g_physics->obbox_pairs->PopBack(); // Get the next index and remove it from the stack
	curNodeA = obbtree_a->tree->GetElem(curPair.first);
	curNodeB = obbtree_b->tree->GetElem(curPair.second);

	int colResult = 0;
	if(curNodeA->isLeaf && curNodeB->isLeaf)
		colResult = obbox::TestTriCollision(curNodeA, a, curNodeB, b);
	else
	{
		if(useSSESIMD)
			colResult = obbox::TestOBBCollision_SIMD(curNodeA, a, curNodeB, b);
		else
			colResult = obbox::TestOBBCollision(curNodeA, a, curNodeB, b);
	}

	// Test collision at the current level --> Either stop here or add more pairs to the stack.
	if(colResult)
	{
		// If both are leaf nodes
		if(curNodeA->isLeaf && curNodeB->isLeaf)
		{
			debugObject::AddObboxTriDebugObject(curNodeA, a, RED); // Make a render object of the OBB leaf triangles
			debugObject::AddObboxTriDebugObject(curNodeB, b, RED);

			// Add the nodes to the return collisions array.
			curCollision.obbox_a = curNodeA;
			curCollision.obbox_b = curNodeB;
			curCollision.Type = COL_UNDEFINED;
			g_physics->OBBCollisions->Add(&curCollision);
		}
		
		else
		{
			debugObject::AddObboxDebugObject(curNodeA, a, RED); // Make a render object of the OBB boxes
			debugObject::AddObboxDebugObject(curNodeB, b, RED);

			// If only a is a leaf
			if (curNodeA->isLeaf)
			{
				// Descend b's children
				g_physics->obbox_pairs->Add( &intPair( curPair.first, curNodeB->childNode1 ) );
				g_physics->obbox_pairs->Add( &intPair( curPair.first, curNodeB->childNode2 ) );
			}
			// If only b is a leaf
			else if (curNodeB->isLeaf)
			{
				// Descend a's children
				g_physics->obbox_pairs->Add( &intPair( curNodeA->childNode1 , curPair.second ) );
				g_physics->obbox_pairs->Add( &intPair( curNodeA->childNode2 , curPair.second ) );
			}
			// Descend either a or b
			else
			{
				// Descend the box of largest area --> Optimal stratergy for fewest iterations
				// Not necessarily fastest since 6 mults are needed.
				// TAKES INTO ACCOUNT CONSTANT SCALING FROM OBJECT TO WORLD COORDINATES.
				if(((curNodeA->boxDimension.x * curNodeA->boxDimension.y * curNodeA->boxDimension.z) * (a->scale * a->scale * a->scale)) > 
				   ((curNodeB->boxDimension.x * curNodeB->boxDimension.y * curNodeB->boxDimension.z) * (b->scale * b->scale * b->scale)) )
				{
					g_physics->obbox_pairs->Add( &intPair( curNodeA->childNode1 , curPair.second ) );
					g_physics->obbox_pairs->Add( &intPair( curNodeA->childNode2 , curPair.second ) );
				}
				else
				{
					g_physics->obbox_pairs->Add( &intPair( curPair.first, curNodeB->childNode1 ) );
					g_physics->obbox_pairs->Add( &intPair( curPair.first, curNodeB->childNode2 ) );
				}
			}
		}
	}
	else // Not in contact
	{
		if(curNodeA->isLeaf && curNodeB->isLeaf)
		{
			debugObject::AddObboxTriDebugObject(curNodeA, a, BLUE);
			debugObject::AddObboxTriDebugObject(curNodeB, b, BLUE);
		}
		else
		{
			debugObject::AddObboxDebugObject(curNodeA, a, BLUE);
			debugObject::AddObboxDebugObject(curNodeB, b, BLUE);
		}
	}
}

#pragma warning( pop )			// Edit Jonathan Tompson - 31st Jan 2011