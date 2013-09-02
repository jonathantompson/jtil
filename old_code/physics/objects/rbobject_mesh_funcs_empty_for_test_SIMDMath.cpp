/*************************************************************
**					Rigid Body Object						**
**			-> Store object states, Summer 2009				**
*************************************************************/
// File:		rbobjectMesh_FuncsCreate.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Create functions for rbobjectMesh class -> simplifies rbobjectMesh.cpp for compilation with other projects

#include	"physics\objects\rbobjectMesh.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		Create													*/
/* Description:	This function is taken from (and VERY heavily modified) */
/*				http://www.dhpoware.com/demos/							*/
/************************************************************************/
void rbobjectMesh::Create(LPCWSTR Filename, IDirect3DDevice9 *pDevice)
{

}

/************************************************************************/
/* Name:		ComputeMassPropertiesAndCenter							*/
/* Description:	Calculate center of mass and inertia tensor matrix. I	*/
/*		use code at: http://www.melax.com/volint.html for simplicity.	*/
/*		ASSUMES THAT GetVertexIndexData HAS BEEN CALLED BEFORE			*/
/************************************************************************/
void rbobjectMesh::ComputeMassPropertiesAndCenter(void)
{

}
/************************************************************************/
/* Name:		ComputeMassPropertiesAndCenter							*/
/* Description:	Calculate center of mass and inertia tensor matrix. I	*/
/*		use code at: http://www.melax.com/volint.html for simplicity.	*/
/*		ASSUMES THAT GetVertexIndexData HAS BEEN CALLED BEFORE			*/
/************************************************************************/
void rbobjectMesh::ComputeMassProperties(void)
{

}
/************************************************************************/
/* Name:		GetVertexIndexData										*/
/* Description:	Copy vertex and index buffer into local storage.		*/
/************************************************************************/
// Useful guide for acessing mesh data: http://www.mvps.org/directx/articles/d3dxmesh.htm
void rbobjectMesh::GetVertexIndexData(void)
{

}

/************************************************************************/
/* Name:		Render													*/
/* Description:	Render the rbobjectMesh									*/
/************************************************************************/
void rbobjectMesh::Render()
{

}//rbobjectMesh::Render

/************************************************************************/
/* Name:		Render													*/
/* Description:	Render the rbobjectMesh for the shadow map				*/
/************************************************************************/
void rbobjectMesh::RenderSM(void) // Render during shadow map
{

}//rbobjectMesh::RenderSM