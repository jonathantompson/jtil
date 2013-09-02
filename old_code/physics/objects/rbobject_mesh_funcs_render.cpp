/*************************************************************
**					Rigid Body Object						**
**			-> Store object states, Summer 2009				**
*************************************************************/
// File:		rbobjectMesh_FuncsCreate.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// Create functions for rbobjectMesh class -> simplifies rbobjectMesh.cpp for compilation with other projects

#include	"physics\objects\rbobjectMesh.h"
#include	"objectManager\objectManager.h"
#include	"physics\objects\rbobjectMeshData.h"
#include	"UI\UI.h"
#include	"renderer\renderer.h"
#include	"UI\varNames.h"
#include	"lights\light.h"
#include	"camera\camera.h"
#include	"renderer\renderer_constants.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		RenderSM												*/
/* Description:	Render the rbobjectMesh for the shadow map				*/
/************************************************************************/
void rbobjectMesh::RenderSM() // Render during shadow map
{
	g_renderer->DrawTexturedMeshSM(meshData,&matWorld);
}

/************************************************************************/
/* Name:		RenderGBuffer											*/
/* Description:	Render the rbobjectMesh into the G Buffer				*/
/************************************************************************/
void rbobjectMesh::RenderGBuffer(UINT pass) // Render during depth map
{
	g_renderer->DrawTexturedMeshGBuffer(pass, meshData,&matWorld, &matWorldPrevFrame, NULL);
}

/************************************************************************/
/* Name:		Render													*/
/* Description:	Render the rbobjectMesh into the G Buffer in wireframe	*/
/************************************************************************/
void rbobjectMesh::RenderGBufferWireframe() // Render during depth map
{
	g_renderer->DrawTexturedMeshWireframeGBuffer(GBUFFER_TEXTUREDMESH_WIREFRAME_NOSHADING_PASS, meshData,&matWorld);
}
