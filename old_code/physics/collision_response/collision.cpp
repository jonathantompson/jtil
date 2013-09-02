/*************************************************************
**						Collision Class						**
*************************************************************/
// File:		collision.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "physics\collision_response\collision.h"
#define NULL 0 

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		collision												*/
/* Description:	Default constructor.									*/
/************************************************************************/
collision::collision()
{
	Type = COL_UNDEFINED;
	obbox_a = NULL;
	obbox_b = NULL;
	mesh_a = NULL;
	mesh_b = NULL;
}
/************************************************************************/
/* Name:		collision												*/
/* Description:	Default constructor.									*/
/************************************************************************/
collision::collision(obbox * obbox_a_in, rbobjectMesh * mesh_a_in, obbox * obbox_b_in, rbobjectMesh * mesh_b_in)
{
	Type = COL_UNDEFINED;
	obbox_a = obbox_a_in;
	obbox_b = obbox_b_in;
	mesh_a = mesh_a_in;
	mesh_b = mesh_b_in;
}
/************************************************************************/
/* Name:		~collision												*/
/* Description:	Default destructor.										*/
/************************************************************************/
collision::~collision()
{
	// Nothing to do.
}
/************************************************************************/
/* Name:		operator =												*/
/* Description: Assignment operator for vector array in physics			*/
/*				(when resizing)											*/
/************************************************************************/
collision & collision::operator = (const collision & o)
{
	if (this != &o) // make sure not same object
	{
		// Perform shallow copies (same order as collision.h if you need to check)
		obbox_a = o.obbox_a;
		obbox_b = o.obbox_b;
		mesh_a = o.mesh_a;
		mesh_b = o.mesh_b;
		Type = o.Type;
	}
	return *this;  // Return ref for multiple assignment
}