// File:		debugObject.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// A class to hold vertex and index buffers for rendering debug objects, stored in a linked list.
// The header of these is stored in the "renderer" class and rendered each frame in wireframe mode.

#ifndef debugObject_h
#define debugObject_h

enum debugObject_type 
{
	DO_UNDEFINED = 0,
	DO_LINES = 1,
	DO_TRIANGLES_WIREFRAME = 2,
	DO_TRIANGLES_SOLID = 3,
};

class obbox;
class rbobjectMesh;

#include "dxInclude.h"
#include "renderer\renderer_structures\d3dFormats.h"

class debugObject 
{
public:
									debugObject( void );
									~debugObject( void ); // Recursively delete list

	void							Render( void );
	void							RenderSM( void );
	void							InsertAtEnd( debugObject * input_obj );
	static void						AddObboxDebugObject( obbox * obbox_in, rbobjectMesh * rbo_in, D3DCOLOR color  );
	static void						AddObboxTriDebugObject( obbox * obbox_in, rbobjectMesh * rbo_in, D3DCOLOR color  );

	// Object type and raw buffer data
	debugObject_type                type;
	UINT							numVert;
	UINT							numInd;
	D3DCOLOR						color;
	Mtrl							mtrl;

	// Affine transformation, scale * rotation * translation
	D3DXMATRIX 						matWorld;

	// D3D Buffers
	IDirect3DVertexBuffer9 *		vertexBuffer;
	IDirect3DIndexBuffer9 *			indexBuffer;

	// Linked list pointers
	debugObject *					next;
	debugObject *					prev;

};

#endif