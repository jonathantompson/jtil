/*************************************************************
**						AABBox								**
**************************************************************/
// File:		AABBox.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "physics\objects\AABBox.h"
#include <limits>

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		AABBox													*/
/* Description:	Default constructor function							*/
/************************************************************************/
AABBox::AABBox()
{
	min.x = std::numeric_limits<float>::infinity();
	min.y = std::numeric_limits<float>::infinity();
	min.z = std::numeric_limits<float>::infinity();
	max.x = -1.0f * std::numeric_limits<float>::infinity();
	max.y = -1.0f * std::numeric_limits<float>::infinity();
	max.z = -1.0f * std::numeric_limits<float>::infinity();
}

/************************************************************************/
/* Name:		~AABBox													*/
/* Description:	Default destructor function								*/
/************************************************************************/
AABBox::~AABBox()
{
	
}

/************************************************************************/
/* Name:		Expand													*/
/* Description: Check Min/Max against input vector and update			*/
/************************************************************************/
void AABBox::Expand(D3DXVECTOR3 * vec)
{
	if(min.x > vec->x)
		min.x = vec->x;
	if(min.y > vec->y)
		min.y = vec->y;
	if(min.z > vec->z)
		min.z = vec->z;
	if(max.x < vec->x)
		max.x = vec->x;
	if(max.y < vec->y)
		max.y = vec->y;
	if(max.z < vec->z)
		max.z = vec->z;
}