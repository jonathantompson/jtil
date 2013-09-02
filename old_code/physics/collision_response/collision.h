/*************************************************************
**						Collision Class						**
*************************************************************/
// File:		collision.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef collision_h
#define collision_h

class obbox;
class rbobjectMesh;

enum CollisionType
{
	COL_UNDEFINED     = 0,
	EDGE_EDGE         = 1,
	EDGE_FACE         = 2,
};

class collision
{
public:
					collision();
					~collision();
					collision(obbox * obbox_a_in, rbobjectMesh * mesh_a, obbox * obbox_b_in, rbobjectMesh * mesh_b);
	collision &		operator = (const collision & o); // Assignment operator for vector array in physics (when resizing)

	// Pointer to the colliding objects.
	obbox *			obbox_a;
	rbobjectMesh *	mesh_a;
	obbox *			obbox_b;
	rbobjectMesh *	mesh_b;
	CollisionType	Type;

	/********************************************************************/
	/***** IF YOU ADD MEMBERS, ADD THEM TO THE ASSIGNMENT OPERATOR ******/
	/********************************************************************/

};

#endif