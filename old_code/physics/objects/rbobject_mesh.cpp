/*************************************************************
**					Rigid Body Object						**
**			-> Store object states, Summer 2009				**
*************************************************************/
// File:		rbobjectMesh.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include	"physics\objects\rbobjectMesh.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		rbobjectMesh											*/
/* Description:	Default constructor										*/
/************************************************************************/
rbobjectMesh::rbobjectMesh() // Remember Parent constructor is called first.
{
	meshData = NULL;
}
/************************************************************************/
/* Name:		rbobjectMesh											*/
/* Description:	Default destructor										*/
/************************************************************************/
rbobjectMesh::~rbobjectMesh()
{
	// Don't delete the mesh data, object manager will take care of this
}
