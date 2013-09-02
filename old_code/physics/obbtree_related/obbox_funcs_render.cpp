// File:		obbox_FuncsRender.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is primary OBB tree data structure to impliment the paper:
// S. Gottschalk et. al. "OBBTree: A Hierarchical Structure for Rapid Iterference Detection"

// obbox render routines for obbox class

#include "physics\obbtree_related\obbox.h"
#include "physics\obbtree_related\obboxTempVar.h"
#include "physics\obbtree_related\obboxRenderitems.h"
#include "app.h"
#include "physics\obbtree_related\obbtree.h"
#include "physics\obbtree_related\obbtreeTempVar.h"
#include "utils_and_misc_classes\stringUtil.h"
#include "renderer\renderer_structures\vertex.h"
#include "renderer\renderer.h"
#include "renderer\renderer_structures\d3dFormats.h"
#include <UI\UI.h>
#include <UI\varNames.h>
#include "renderer\renderer_constants.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

#pragma warning( push )			// Edit Jonathan Tompson - 31st Jan 2011
#pragma warning( disable:4238 )	// Edit Jonathan Tompson - 31st Jan 2011

#define NULL 0

/************************************************************************/
/* Name:		initConvexHullForRendering								*/
/* Description: Sets up a D3D Mesh to render the convexHull Wireframe	*/
/************************************************************************/
void obbox::initConvexHullForRendering()
{
	renderitems->convexHullVertexBufferSize = renderitems->cHullVertCount; 
	renderitems->convexHullIndexBufferSize = 2*renderitems->cHullIndCount; // Render both sides

	// Obtain a pointer to a new vertex buffer.
	HR(g_renderer->GetD3DDev()->CreateVertexBuffer(renderitems->convexHullVertexBufferSize * sizeof(VertexPosNormCol), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &renderitems->convexHullVertexBuffer, 0),
		L"obbox::initConvexHullForRendering() - CreateVertexBuffer failed: ");

	// Now lock it to obtain a pointer to its internal data, and write the grid's vertex data.
	VertexPosNormCol* v = 0;
	HR(renderitems->convexHullVertexBuffer->Lock(0, 0, (void**)&v, 0), L"obbox::initConvexHullForRendering() - Failed to lock vertex buffer: ");
	for(UINT i = 0; i < renderitems->convexHullVertexBufferSize; ++i)
		v[i] = VertexPosNormCol(D3DXVECTOR3((float)renderitems->cHullVert[i*3 + 0],(float)renderitems->cHullVert[i*3 + 1],(float)renderitems->cHullVert[i*3 + 2]),D3DXVECTOR3(1.0f,1.0f,1.0f),BLACK);
	HR(renderitems->convexHullVertexBuffer->Unlock(), L"obbox::initConvexHullForRendering() - Failed to unlock vertex buffer: ");

	// Obtain a pointer to a new index buffer.
	// TWICE THE SIZE TO RENDER NEGATIVE AND POSITIVE NORMAL FACE SIDE.
	HR(g_renderer->GetD3DDev()->CreateIndexBuffer(2*renderitems->cHullIndCount*sizeof(WORD), D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &renderitems->convexHullIndexBuffer, 0),
		L"obbox::initConvexHullForRendering() - CreateIndexBuffer failed: ");
	
	// Now lock it to obtain a pointer to its internal data, and write the grid's index data.
	WORD* k = 0;
	HR(renderitems->convexHullIndexBuffer->Lock(0, 0, (void**)&k, 0), L"obbox::initConvexHullForRendering() - Failed to lock index buffer: ");
	for(UINT i = 0; i < renderitems->cHullIndCount; i = i+3) // positive side
	{
		k[i+0] = (WORD)renderitems->cHullInd[i+0]; // vertex 1
		k[i+1] = (WORD)renderitems->cHullInd[i+1]; // vertex 1
		k[i+2] = (WORD)renderitems->cHullInd[i+2]; // vertex 1
	}
	UINT j = 0;
	for(UINT i = renderitems->cHullIndCount; i < 2*renderitems->cHullIndCount; i += 3) // positive side
	{
		k[i+0] = (WORD)renderitems->cHullInd[j+2]; // vertex 1
		k[i+1] = (WORD)renderitems->cHullInd[j+1]; // vertex 1
		k[i+2] = (WORD)renderitems->cHullInd[j+0]; // vertex 1
		j += 3;
	}
	HR(renderitems->convexHullIndexBuffer->Unlock(), L"obbox::initConvexHullForRendering() - Failed to unlock index buffer: ");
}
/************************************************************************/
/* Name:		initOBBForRendering										*/
/* Description: Sets up D3D Mesh to render the OBB recursively			*/
/************************************************************************/
void obbox::initOBBForRendering()
{
	renderitems->OBBVertexBufferSize = 24; renderitems->OBBIndexBufferSize = 36; // 24 verticies in a box (8*3 for normals), 36 indicies to describe 12 triangles on 6 faces

	// Obtain a pointer to a new vertex buffer.
	HR(g_renderer->GetD3DDev()->CreateVertexBuffer(renderitems->OBBVertexBufferSize * sizeof(VertexPosNormCol), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &renderitems->OBBVertexBuffer, 0),
		L"obbox::initOBBForRendering() - CreateVertexBuffer failed: ");

	// Now lock it to obtain a pointer to its internal data, and write the grid's vertex data.
	VertexPosNormCol* v = 0;
	HR(renderitems->OBBVertexBuffer->Lock(0, 0, (void**)&v, 0), L"obbox::initOBBForRendering() - Failed to lock vertex buffer: ");

	// Obtain a pointer to a new index buffer.
	HR(g_renderer->GetD3DDev()->CreateIndexBuffer(renderitems->OBBIndexBufferSize*sizeof(WORD), D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &renderitems->OBBIndexBuffer, 0),
		L"obbox::initOBBForRendering() - CreateIndexBuffer failed: ");
	
	// Now lock it to obtain a pointer to its internal data, and write the grid's index data.
	WORD* k = 0;
	HR(renderitems->OBBIndexBuffer->Lock(0, 0, (void**)&k, 0), L"obbox::initOBBForRendering() - Failed to lock index buffer: ");
	
	CalcObboxVerticesIndices(v, k, &boxCenterObjectCoord, &boxDimension, &orientMatrix, CYAN);

	renderitems->mtrl = Mtrl(WHITE,10.0f,1.0f);

	HR(renderitems->OBBVertexBuffer->Unlock(), L"obbox::initOBBForRendering() - Failed to unlock vertex buffer: ");
	HR(renderitems->OBBIndexBuffer->Unlock(), L"obbox::initOBBForRendering() - Failed to unlock index buffer: ");
}
/************************************************************************/
/* Name:		initOBBForRenderingLines								*/
/* Description: Sets up D3D Mesh to render the OBB Wireframe			*/
/************************************************************************/
void obbox::initOBBForRenderingLines()
{
	renderitems->OBBVertexBufferSize = 8; renderitems->OBBIndexBufferSize = 24; // 8 verticies in a box, (both sides of each triangle) 24 indicies to describe 12 edges (both sides)

	// Obtain a pointer to a new vertex buffer.
	HR(g_renderer->GetD3DDev()->CreateVertexBuffer(renderitems->OBBVertexBufferSize * sizeof(VertexPosNormCol), 
		D3DUSAGE_WRITEONLY,	0, D3DPOOL_MANAGED, &renderitems->OBBVertexBuffer, 0),
		L"obbox::initOBBForRenderingLines() - CreateVertexBuffer failed: ");

	// Now lock it to obtain a pointer to its internal data, and write the grid's vertex data.
	VertexPosNormCol* v = 0;
	HR(renderitems->OBBVertexBuffer->Lock(0, 0, (void**)&v, 0), L"obbox::initOBBForRenderingLines() - Failed to lock vertex buffer: ");

	// Obtain a pointer to a new index buffer.
	HR(g_renderer->GetD3DDev()->CreateIndexBuffer(renderitems->OBBIndexBufferSize*sizeof(WORD), D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16, D3DPOOL_MANAGED, &renderitems->OBBIndexBuffer, 0),
		L"obbox::initOBBForRenderingLines() - CreateIndexBuffer failed: ");
	
	// Now lock it to obtain a pointer to its internal data, and write the grid's index data.
	WORD* k = 0;
	HR(renderitems->OBBIndexBuffer->Lock(0, 0, (void**)&k, 0), L"obbox::initOBBForRenderingLines() - Failed to lock index buffer: ");

	CalcObboxVerticesIndicesLines(v, k, &boxCenterObjectCoord, &boxDimension, &orientMatrix, CYAN);

	HR(renderitems->OBBVertexBuffer->Unlock(), L"obbox::initOBBForRenderingLines() - Failed to unlock vertex buffer: ");
	HR(renderitems->OBBIndexBuffer->Unlock(), L"obbox::initOBBForRenderingLines() - Failed to unlock index buffer: ");
}
/************************************************************************/
/* Name:		DrawConvexHull											*/
/* Description: Draw the convex hull									*/
/************************************************************************/
void obbox::DrawConvexHull( D3DXMATRIXA16 * matW )
{
	if(depth+1 > (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
		return;
	if(renderitems && renderitems->convexHullVertexBuffer ) // If the vertex buffer exists
		g_renderer->DrawSingleColorMeshWireframeGBuffer(GBUFFER_SINGLECOLORMESH_WIREFRAME_NOSHADING_PASS, false, renderitems->convexHullVertexBuffer, renderitems->convexHullVertexBufferSize, renderitems->convexHullIndexBuffer, renderitems->convexHullIndexBufferSize, matW );
	else
		throw std::runtime_error("obbox::DrawConvexHull() - Error: convexHullVertexBuffer not initialized");
}
/************************************************************************/
/* Name:		DrawOBB													*/
/* Description: Draw the OBB											*/
/************************************************************************/
void obbox::DrawOBB( D3DXMATRIXA16 * matW )
{
	if(depth+1 > (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
		return;
	if(renderitems && renderitems->OBBVertexBuffer) // If the vertex buffer exists
	{
		if(g_UI->GetSetting<bool>(&var_drawOBBAsLines))
			g_renderer->DrawSingleColorMeshWireframeGBuffer(GBUFFER_SINGLECOLORMESH_WIREFRAME_NOSHADING_PASS, true, renderitems->OBBVertexBuffer, renderitems->OBBVertexBufferSize, renderitems->OBBIndexBuffer, renderitems->OBBIndexBufferSize, matW );
		else
			g_renderer->DrawSingleColorMeshGBuffer(GBUFFER_SINGLECOLORMESH_SHADING_PASS, renderitems->OBBVertexBuffer, renderitems->OBBVertexBufferSize, renderitems->OBBIndexBuffer, renderitems->OBBIndexBufferSize, matW, &renderitems->mtrl);
	}
	else
		throw std::runtime_error("obbox::DrawOBB(): - Error: OBBVertexBuffer not initialized");
}
/************************************************************************/
/* Name:		DrawOBBSM												*/
/* Description: Draw the OBB											*/
/************************************************************************/
void obbox::DrawOBBSM(D3DXMATRIXA16 * matW)
{
	if(depth+1 > (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
		return;
	if(renderitems && renderitems->OBBVertexBuffer) // If the vertex buffer exists
	{
		g_renderer->DrawTrianglesColSM(renderitems->OBBVertexBuffer, renderitems->OBBVertexBufferSize, 
									   renderitems->OBBIndexBuffer, renderitems->OBBIndexBufferSize, matW);
	}
	else
		throw std::runtime_error("obbox::DrawOBBSM() - Error: OBBVertexBuffer not initialized");
}
/************************************************************************/
/* Name:		CalcObboxVerticesIndices								*/
/* Description: Generate verticies for solid shaded obbox				*/
/************************************************************************/
void obbox::CalcObboxVerticesIndices(VertexPosNormCol * v, WORD * k, D3DXVECTOR3 * boxCenterObjectCoord, D3DXVECTOR3 * boxDimension, D3DXMATRIXA16 * orientMatrix, D3DCOLOR color )
{
	// Transform box center from object to local coordinates
	D3DXVECTOR3 boxCenterBoxCoord;
	MatrixMult(&boxCenterBoxCoord, orientMatrix, boxCenterObjectCoord);

	// WORK OUT BOX CORNERS FOR VERTEX BUFFER IN OBJECT COORDINATES
	double3 boxCoord; double3 objCoord; // Some temporary variables

	// Transform the matrix normals
	D3DXVECTOR3 up(0.0,1.0,0.0);
	D3DXVECTOR3 down(0.0,-1.0,0.0);
	D3DXVECTOR3 left(-1.0,0.0,0.0);
	D3DXVECTOR3 right(1.0,0.0,0.0);
	D3DXVECTOR3 forward(0.0,0.0,1.0);
	D3DXVECTOR3 back(1.0,0.0,-1.0);
	D3DXVECTOR3 normal_temp;
	MatrixMult(&normal_temp, orientMatrix, &up);		D3DXVec3Normalize(&up, &normal_temp);		// Normal is translated by the transpose of the linear transformation
	MatrixMult(&normal_temp, orientMatrix, &down);		D3DXVec3Normalize(&down, &normal_temp);
	MatrixMult(&normal_temp, orientMatrix, &left);		D3DXVec3Normalize(&left, &normal_temp);
	MatrixMult(&normal_temp, orientMatrix, &right);		D3DXVec3Normalize(&right, &normal_temp);
	MatrixMult(&normal_temp, orientMatrix, &forward);	D3DXVec3Normalize(&forward, &normal_temp);
	MatrixMult(&normal_temp, orientMatrix, &back);		D3DXVec3Normalize(&back, &normal_temp);

	boxCoord.x = (boxCenterBoxCoord.x - boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y - boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z - boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord); // Translate box corner to object coordinates
	v[0] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),down,color);
	v[1] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),back,color);
	v[2] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),left,color);

	boxCoord.x = (boxCenterBoxCoord.x + boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y - boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z - boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[3] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),down,color);
	v[4] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),back,color);
	v[5] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),right,color);

	boxCoord.x = (boxCenterBoxCoord.x - boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y - boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z + boxDimension->z);	
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[6] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),down,color);
	v[7] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),left,color);
	v[8] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),forward,color);

	boxCoord.x = (boxCenterBoxCoord.x + boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y - boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z + boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[9] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),down,color);
	v[10] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),right,color);
	v[11] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),forward,color);

	boxCoord.x = (boxCenterBoxCoord.x - boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y + boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z - boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[12] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),back,color);
	v[13] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),left,color);
	v[14] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),up,color);

	boxCoord.x = (boxCenterBoxCoord.x + boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y + boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z - boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[15] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),back,color);
	v[16] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),right,color);
	v[17] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),up,color);

	boxCoord.x = (boxCenterBoxCoord.x - boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y + boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z + boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[18] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),left,color);
	v[19] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),forward,color);
	v[20] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),up,color);

	boxCoord.x = (boxCenterBoxCoord.x + boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y + boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z + boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[21] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),right,color);
	v[22] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),forward,color);
	v[23] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),up,color);

	k[0] = 6;  k[1] = 3;  k[2] = 9;		// face 1 (down) triange 1       
	k[3] = 6;  k[4] = 0;  k[5] = 3;		// face 1 (down) triange 2
	k[6] = 1;  k[7] = 12;  k[8] = 15;	// face 2 (front) triange 3
	k[9] = 1;  k[10] = 15; k[11] = 4;	// face 2 (front) triange 4
	k[12] = 5; k[13] = 16; k[14] = 21;	// face 3 (right) triange 5
	k[15] = 5; k[16] = 21; k[17] = 10;	// face 3 (right) triange 6
	k[24] = 7; k[25] = 18; k[26] = 13;	// face 4 (left) triange 7
	k[27] = 7; k[28] = 13; k[29] = 2;	// face 4 (left) triange 8
	k[18] = 11; k[19] = 22; k[20] = 19;	// face 5 (back) triange 9
	k[21] = 11; k[22] = 19; k[23] = 8;	// face 5 (back) triange 10
	k[30] = 14; k[31] = 23; k[32] = 17;	// face 6 (up) triange 11
	k[33] = 14; k[34] = 20; k[35] = 23;	// face 6 (up) triange 12
}

/************************************************************************/
/* Name:		CalcObboxVerticesIndicesLines							*/
/* Description: Generate verticies for wireframe shaded obbox			*/
/************************************************************************/
void obbox::CalcObboxVerticesIndicesLines(VertexPosNormCol * v, WORD * k, D3DXVECTOR3 * boxCenterObjectCoord, D3DXVECTOR3 * boxDimension, D3DXMATRIXA16 * orientMatrix, D3DCOLOR color )
{
	// Transform box center from object to local coordinates
	D3DXVECTOR3 boxCenterBoxCoord;
	MatrixMult(&boxCenterBoxCoord, orientMatrix, boxCenterObjectCoord);

	// WORK OUT BOX CORNERS FOR VERTEX BUFFER IN OBJECT COORDINATES
	double3 boxCoord; double3 objCoord; // Some temporary variables
	boxCoord.x = (boxCenterBoxCoord.x - boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y - boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z - boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord); // Translate box corner to object coordinates
	v[0] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),D3DXVECTOR3(1.0f,0.0f,0.0f),color); // normal doesn't matter

	boxCoord.x = (boxCenterBoxCoord.x + boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y - boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z - boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[1] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),D3DXVECTOR3(1.0f,0.0f,0.0f),color); // normal doesn't matter

	boxCoord.x = (boxCenterBoxCoord.x - boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y - boxDimension->y);
	boxCoord.z = (boxCenterBoxCoord.z + boxDimension->z);	
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[2] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),D3DXVECTOR3(1.0f,0.0f,0.0f),color); // normal doesn't matter

	boxCoord.x = (boxCenterBoxCoord.x + boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y - boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z + boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[3] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),D3DXVECTOR3(1.0f,0.0f,0.0f),color); // normal doesn't matter

	boxCoord.x = (boxCenterBoxCoord.x - boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y + boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z - boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[4] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),D3DXVECTOR3(1.0f,0.0f,0.0f),color); // normal doesn't matter

	boxCoord.x = (boxCenterBoxCoord.x + boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y + boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z - boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[5] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),D3DXVECTOR3(1.0f,0.0f,0.0f),color); // normal doesn't matter

	boxCoord.x = (boxCenterBoxCoord.x - boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y + boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z + boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[6] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),D3DXVECTOR3(1.0f,0.0f,0.0f),color); // normal doesn't matter

	boxCoord.x = (boxCenterBoxCoord.x + boxDimension->x); 
	boxCoord.y = (boxCenterBoxCoord.y + boxDimension->y); 
	boxCoord.z = (boxCenterBoxCoord.z + boxDimension->z);
	MatrixMult_ATran_B(&objCoord, orientMatrix, &boxCoord);
	v[7] = VertexPosNormCol(D3DXVECTOR3((float)objCoord.x, (float)objCoord.y, (float)objCoord.z),D3DXVECTOR3(1.0f,0.0f,0.0f),color); // normal doesn't matter

	k[0]  = 0; k[1]  = 1; // Line 1
	k[2]  = 1; k[3]  = 5; // Line 2
	k[4]  = 5; k[5]  = 4; // Line 3
	k[6]  = 4; k[7]  = 0; // Line 4
	k[8]  = 0; k[9]  = 2; // Line 5
	k[10] = 1; k[11] = 3; // Line 6
	k[12] = 5; k[13] = 7; // Line 7
	k[14] = 4; k[15] = 6; // Line 8
	k[16] = 2; k[17] = 3; // Line 9
	k[18] = 3; k[19] = 7; // Line 10
	k[20] = 7; k[21] = 6; // Line 11
	k[22] = 2; k[23] = 6; // Line 12
}

#pragma warning( pop )			// Edit Jonathan Tompson - 31st Jan 2011