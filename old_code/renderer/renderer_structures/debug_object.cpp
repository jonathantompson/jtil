// File:		debugObject.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// A class to hold vertex and index buffers for rendering debug objects.  
// A vector of these is stored in renderer class and rendered each frame in wireframeCol mode.

#include "renderer\renderer_structures\debugObject.h"
#include "utils_and_misc_classes\stringUtil.h"
#include "main.h"
#include "app.h"
#include "objectManager\objectManager.h"
#include "renderer\renderer.h"
#include "renderer\renderer_structures\vertex.h"
#include "camera\camera.h"
#include "UI\UI.h"
#include "UI\varNames.h"
#include "physics\obbtree_related\obbox.h"
#include "physics\obbtree_related\obbtreeTempVar.h"
#include "physics\objects\rbobjectMesh.h"
#include "physics\objects\rbobjectMeshData.h"
#include "utils_and_misc_classes\math\double3x3.h"
#include "utils_and_misc_classes\math\double3.h"
#include "physics\obbtree_related\obbtree.h"
#include "renderer\renderer_constants.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

/************************************************************************/
/* Name:		debugObject											*/
/* Description:	Default constructor										*/
/************************************************************************/
debugObject::debugObject()
{
	vertexBuffer = NULL; indexBuffer = NULL;
	numVert = 0; numInd = 0;
	type = DO_UNDEFINED;
	next = NULL; prev = NULL;
	color =  D3DCOLOR_ARGB(255, 255, 255, 255); // Reset to black
}
/************************************************************************/
/* Name:		~debugObject											*/
/* Description:	Default destructor										*/
/************************************************************************/
debugObject::~debugObject()
{
	if(next){delete next; next = NULL;} // Recursively delete the rest of the linked list.
	ReleaseCOM(vertexBuffer);
	ReleaseCOM(indexBuffer);
}
/************************************************************************/
/* Name:		Render													*/
/* Description:	Render the primatives in wireframe mode					*/
/************************************************************************/
void debugObject::Render( void )
{
#ifdef _DEBUG
	if(type == DO_UNDEFINED || !vertexBuffer || !indexBuffer )
		throw std::runtime_error("debugObject::Render() - Trying to render an un-initialized object");
#endif
	if(type == DO_LINES)
		g_renderer->DrawSingleColorMeshWireframeGBuffer(GBUFFER_SINGLECOLORMESH_WIREFRAME_NOSHADING_PASS, true, vertexBuffer, numVert, indexBuffer, numInd , (D3DXMATRIXA16 *) & matWorld );
	else if(type == DO_TRIANGLES_WIREFRAME)
		g_renderer->DrawSingleColorMeshWireframeGBuffer(GBUFFER_SINGLECOLORMESH_WIREFRAME_NOSHADING_PASS, false, vertexBuffer, numVert, indexBuffer, numInd , (D3DXMATRIXA16 *) & matWorld );
	else if(type == DO_TRIANGLES_SOLID)
		g_renderer->DrawSingleColorMeshGBuffer(GBUFFER_SINGLECOLORMESH_SHADING_PASS, vertexBuffer, numVert, indexBuffer, numInd , (D3DXMATRIXA16 *) & matWorld, & mtrl );
	else
		throw std::runtime_error("debugObject::Render() - Unrecognised type");

	if(next)
		next->Render(); // Recursively draw the rest of the debug objects
}
/************************************************************************/
/* Name:		RenderSM												*/
/* Description:	Render the primatives in wireframe mode					*/
/************************************************************************/
void debugObject::RenderSM( void )
{
	// Only render solid triangle debugObjects for the shadow map
	if(type == DO_TRIANGLES_SOLID)
		g_renderer->DrawTrianglesColSM(vertexBuffer, numVert, indexBuffer, numInd , (D3DXMATRIXA16 *) & matWorld );

	if(next)
		next->Render(); // Recursively draw the rest of the debug objects
}
/************************************************************************/
/* Name:		InsertAtEnd												*/
/* Description:	Insert input debugObject at the end of the linked-list	*/
/************************************************************************/
void debugObject::InsertAtEnd( debugObject * input_obj )
{
#ifdef _DEBUG
	if( input_obj == NULL )
		throw std::runtime_error("debugObject::InsertAtEnd() - NULL input to function");
#endif

	debugObject * curObject = this;
	while(curObject->next != NULL) // Iterate to the end of the array
		curObject = curObject->next;

	curObject->next = input_obj;
	input_obj->prev = curObject;

}
/************************************************************************/
/* Name:		AddObboxDebugObject										*/
/* Description:	Make an exact copy of the current Obbox for rendering.	*/
/*              Second input is a D3DCOLOR value						*/
/************************************************************************/
void debugObject::AddObboxDebugObject( obbox * obbox_in, rbobjectMesh * rbo_in, D3DCOLOR color )
{
#ifdef _DEBUG
	if( obbox_in == NULL )
		throw std::runtime_error("debugObject::AddObboxDebugObject() - NULL input to function");
#endif

	// Make a new debug object on the heap
	debugObject * DO = new debugObject;

	// Add it to the linked list of objects
	debugObject * debugObjects = g_objectManager->GetDebugObjects();
	if(debugObjects) 
		debugObjects->InsertAtEnd(DO); // Insert at end of linked-list
	else
		g_objectManager->SetDebugObjects(DO); // Otherwise start a new linked-list if empty

	DO->type = DO_LINES;
	if(g_UI->GetSetting<bool>(&var_drawOBBAsLines))
	{ DO->numVert = 8; DO->numInd = 24; DO->type = DO_LINES; }		// 24 verticies in a box (8*3 for normals), 36 indicies to describe 12 triangles on 6 faces
	else
	{ DO->numVert = 24; DO->numInd = 36; DO->type = DO_TRIANGLES_SOLID; }	// 8 verticies in a box, (both sides of each triangle) 24 indicies to describe 12 edges (both sides)

	DO->matWorld = rbo_in->matWorld;
	DO->color = color;

	// **** Now setup the buffers ****

	// Obtain a pointer to a new vertex buffer.
	HR(g_renderer->GetD3DDev()->CreateVertexBuffer(DO->numVert * sizeof(VertexPosNormCol), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &DO->vertexBuffer, 0),
		L"debugObject::AddObboxDebugObject() - CreateVertexBuffer failed: ");

	// Now lock it to obtain a pointer to its internal data, and write the grid's vertex data.
	VertexPosNormCol* v = 0;
	HR(DO->vertexBuffer->Lock(0, 0, (void**)&v, 0), L"debugObject::AddObboxDebugObject() - Failed to lock vertex buffer: ");

	// Obtain a pointer to a new index buffer.
	HR(g_renderer->GetD3DDev()->CreateIndexBuffer(DO->numInd*sizeof(WORD), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &DO->indexBuffer, 0),
		L"debugObject::AddObboxDebugObject() - CreateIndexBuffer failed: ");
	
	// Now lock it to obtain a pointer to its internal data, and write the grid's index data.
	WORD* k = 0;
	HR(DO->indexBuffer->Lock(0, 0, (void**)&k, 0), L"debugObject::AddObboxDebugObject() - Failed to lock index buffer: ");

	if(g_UI->GetSetting<bool>(&var_drawOBBAsLines))
		obbox::CalcObboxVerticesIndicesLines(v, k, &obbox_in->boxCenterObjectCoord, &obbox_in->boxDimension, &obbox_in->orientMatrix, color);
	else
		obbox::CalcObboxVerticesIndices(v, k, &obbox_in->boxCenterObjectCoord, &obbox_in->boxDimension, &obbox_in->orientMatrix, color);

	DO->mtrl = Mtrl(WHITE,10.0f,1.0f);

	HR(DO->vertexBuffer->Unlock(), L"debugObject::AddObboxDebugObject() - Failed to unlock vertex buffer: ");
	HR(DO->indexBuffer->Unlock(), L"debugObject::AddObboxDebugObject() - Failed to unlock index buffer: ");

}
/************************************************************************/
/* Name:		AddObboxTriDebugObject									*/
/* Description:	Make an exact copy of the current Obbox's triangles		*/
/*              Second input is a D3DCOLOR value						*/
/************************************************************************/
void debugObject::AddObboxTriDebugObject( obbox * obbox_in, rbobjectMesh * rbo_in, D3DCOLOR color )
{
#ifdef _DEBUG
	if( obbox_in == NULL )
		throw std::runtime_error("debugObject::AddObboxTriDebugObject() - NULL input to function");
	if( !obbox_in->isLeaf )
		throw std::runtime_error("debugObject::AddObboxTriDebugObject() - Input obbox is not a leaf");
#endif

	// Make a new debug object on the heap
	debugObject * DO = new debugObject;

	// Add it to the linked list of objects
	debugObject * debugObjects = g_objectManager->GetDebugObjects();
	if(debugObjects) 
		debugObjects->InsertAtEnd(DO); // Insert at end of linked-list
	else
		g_objectManager->SetDebugObjects(DO); // Otherwise start a new linked-list if empty

	DO->type = DO_TRIANGLES_WIREFRAME;
	DO->numVert = (obbox_in->numFaces * 3); // 3 indicies per face
	DO->numInd = obbox_in->numFaces * 3 * 2; // Also store back to front indicies to render both sides
	DO->matWorld = rbo_in->matWorld;
	DO->color = color;

	// **** Now setup the buffers ****

	// Obtain a pointer to a new vertex buffer.
	HR(g_renderer->GetD3DDev()->CreateVertexBuffer(DO->numVert * sizeof(VertexPosNormCol), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &DO->vertexBuffer, 0),
		L"debugObject::AddObboxTriDebugObject() - CreateVertexBuffer failed: ");

	// Now lock it to obtain a pointer to its internal data, and write the grid's vertex data.
	VertexPosNormCol* v = 0;
	HR(DO->vertexBuffer->Lock(0, 0, (void**)&v, 0), L"debugObject::AddObboxTriDebugObject() - Failed to lock vertex buffer: ");

	// Obtain a pointer to a new index buffer.
	HR(g_renderer->GetD3DDev()->CreateIndexBuffer(DO->numInd*sizeof(WORD), D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &DO->indexBuffer, 0),
		L"debugObject::AddObboxTriDebugObject() - CreateIndexBuffer failed: ");
	
	// Now lock it to obtain a pointer to its internal data, and write the grid's index data.
	WORD* k = 0;
	HR(DO->indexBuffer->Lock(0, 0, (void**)&k, 0), L"debugObject::AddObboxTriDebugObject() - Failed to lock index buffer: ");
	
	// Copy over verticies into flat array and set indicies 0,1,2,....,n
	// Note: obbox indicies are stored in array t->indicies which is shared with all leaf nodes.
	//       The correct indicies start:  t->indices[indices]
	//							  finish: t->indicies[indicies + 3*numFaces - 1]
	D3DXVECTOR3 * vertexBuffer = rbo_in->meshData->GetVertexBuffer();
	for(UINT i = 0; i < (UINT)(obbox_in->numFaces * 3); i ++)
	{
		v[i].pos.x =  vertexBuffer[ obbox_in->t->indices[ obbox_in->indices + i]].x;	// Vertex i.x
		v[i].pos.y =  vertexBuffer[ obbox_in->t->indices[ obbox_in->indices + i]].y;	// Vertex i.y
		v[i].pos.z =  vertexBuffer[ obbox_in->t->indices[ obbox_in->indices + i]].z;	// Vertex i.z
		v[i].col = color;
		v[i].norm = D3DXVECTOR3(1.0f,0.0f,0.0f);
		k[i] = (WORD)i;
	}
	// Now render back to front
	for(UINT i = (UINT)(obbox_in->numFaces * 3); i < (UINT)((obbox_in->numFaces * 3) * 2); i ++)
	{
		k[i] = (WORD)((obbox_in->numFaces * 3) * 2 - i - 1);
	}

	DO->mtrl = Mtrl(color,0.8f,1.0f);

	HR(DO->vertexBuffer->Unlock(), L"debugObject::AddObboxTriDebugObject() - Failed to unlock vertex buffer: ");
	HR(DO->indexBuffer->Unlock(), L"debugObject::AddObboxTriDebugObject() - Failed to unlock index buffer: ");

}