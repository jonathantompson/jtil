/*************************************************************
**						cone								**
**************************************************************/
// File:		cone.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef _USE_MATH_DEFINES
	#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include "renderer\renderer_structures\cone.h"
#include "renderer\renderer_structures\d3dFormats.h"
#include "renderer\renderer_structures\vertex.h"
#include "utils_and_misc_classes\stringUtil.h"
#include "main.h"
#include "renderer\renderer.h"


#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

D3DXVECTOR3 cone::coneForward = D3DXVECTOR3(0.0f, 1.0f, 0.0f);

/************************************************************************/
/* Name:		cone													*/
/* Description:	Default constructor function							*/
/************************************************************************/
cone::cone(UINT _numBaseVertices, float _height, float _baseInsideRadius, D3DCOLOR _color, float * ret_outsideRadius)
{
	numBaseVertices = _numBaseVertices;
	if(numBaseVertices < 3)
		throw std::runtime_error("cone::cone() - numBaseVertices < 3!");
	height = _height;
	baseInsideRadius = _baseInsideRadius;
	color = _color;

	numVert = numBaseVertices + 1 + // base vertices
		      numBaseVertices * 2;  // Each side needs a top vertex for the normals to be correct
													  
	posBuffer = new D3DXVECTOR3[numVert];
	normalBuffer = new D3DXVECTOR3[numVert];

	numInd = (numBaseVertices * 3) + (numBaseVertices * 3);
	indexBuffer = new DWORD[numInd];

	UINT curVertex = 0;
	float seperationAngle = 2.0f * (float)M_PI / (float)numBaseVertices;
	baseOutsideRadius = baseInsideRadius / cos(seperationAngle * 0.5f);  // for vertex points
	if(ret_outsideRadius != NULL)
		*ret_outsideRadius = baseOutsideRadius;
	
	// Define the base center vertex
	normalBuffer[curVertex] = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
	posBuffer[curVertex] = D3DXVECTOR3(0.0f, height, 0.0f);
	curVertex ++;

	// Define the base indices
	for(UINT i = 0; i < numBaseVertices; i ++)
	{
		float curAngle = seperationAngle * (float)i;
		normalBuffer[curVertex] = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
		posBuffer[curVertex] = D3DXVECTOR3(baseOutsideRadius * cos(curAngle), height, baseOutsideRadius * sin(curAngle));
		curVertex ++;
	}

	// The top is at (0,0,0)
	D3DXVECTOR3 curBasePoint, v1, v2;
	float curAngle, tangentAngle;

	// Define the cone side and top vertices
	for(UINT i = 0; i < numBaseVertices; i ++)
	{
		// base 1
		curAngle = seperationAngle * (float)i;
		curBasePoint = D3DXVECTOR3(baseOutsideRadius * cos(curAngle), height, baseOutsideRadius * sin(curAngle));
		posBuffer[curVertex] = curBasePoint;
		tangentAngle = seperationAngle * (float)i + ((float)M_PI / 2.0f); // Tangent is 90deg away
		v1 = D3DXVECTOR3(cos(tangentAngle), 0.0f, sin(tangentAngle)); // Already unit length
		D3DXVec3Normalize(& v2, & curBasePoint);
		v2 = -1.0f * v2; // vector from curPos --> Top (0,0,0)
		D3DXVec3Cross( & normalBuffer[curVertex], & v1, & v2 );
		D3DXVec3Normalize( & normalBuffer[curVertex], & normalBuffer[curVertex] );
		curVertex ++;

		// top point X, Z is half angle around
		curAngle = seperationAngle * ((float)i + 0.5f);
		curBasePoint = D3DXVECTOR3(baseOutsideRadius * cos(curAngle), height, baseOutsideRadius * sin(curAngle));
		posBuffer[curVertex] = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
		tangentAngle = seperationAngle * ((float)i + 0.5f) + ((float)M_PI / 2.0f); // Tangent is 90deg away
		v1 = D3DXVECTOR3(cos(tangentAngle), 0.0f, sin(tangentAngle)); // Already unit length
		D3DXVec3Normalize(& v2, & curBasePoint);
		v2 = -1.0f * v2; // vector from curPos --> Top (0,0,0)
		D3DXVec3Cross( & normalBuffer[curVertex], & v1, & v2 );
		D3DXVec3Normalize( & normalBuffer[curVertex], & normalBuffer[curVertex] );
		curVertex ++;
	}

	if(curVertex != numVert)
		throw std::runtime_error("cone::cone() - Internal error, didn't create enough cone vertices");
	
	// Now define the base indices
	UINT curIndex = 0;
	for(UINT i = 1; i < numBaseVertices; i ++)
	{
		indexBuffer[curIndex] = (DWORD)(i);
		curIndex ++;
		indexBuffer[curIndex] = (DWORD)(0); // Center
		curIndex ++;
		indexBuffer[curIndex] = (DWORD)(i + 1);
		curIndex ++;
	}
	// Now define the last triangle in the base
	indexBuffer[curIndex] = (DWORD)(numBaseVertices);
	curIndex ++;
	indexBuffer[curIndex] = (DWORD)(0); // Center
	curIndex ++;
	indexBuffer[curIndex] = (DWORD)(1);
	curIndex ++;

	// Define the cone side indices
	for(UINT i = 0; i < (numBaseVertices-1); i ++)
	{
		indexBuffer[curIndex] = (DWORD)(numBaseVertices + 1 + (2*i)); // base point 1
		curIndex ++;
		indexBuffer[curIndex] = (DWORD)(numBaseVertices + 1 + (2*i + 2));  // base point 2
		curIndex ++;
		indexBuffer[curIndex] = (DWORD)(numBaseVertices + 1 + (2*i + 1)); // top
		curIndex ++;
	}
	// Now define the last triangle on the side
	indexBuffer[curIndex] = (DWORD)(numBaseVertices + 1 + (2*(numBaseVertices-1))); // base point 1
	curIndex ++;
	indexBuffer[curIndex] = (DWORD)(numBaseVertices + 1); // Back to the start
	curIndex ++;
	indexBuffer[curIndex] = (DWORD)(numBaseVertices + 1 + (2*(numBaseVertices-1) + 1)); // top
	curIndex ++;

	if(curIndex != numInd)
		throw std::runtime_error("cone::cone() - Internal error, didn't create enough cone indices");

	mtrl = Mtrl(color,DEFAULT_SPEC_POWER,1.0f);
}

/************************************************************************/
/* Name:		~cone													*/
/* Description:	Default destructor function								*/
/************************************************************************/
cone::~cone()
{
	if(posBuffer) { delete [] posBuffer; posBuffer = NULL; }
	if(indexBuffer) { delete [] indexBuffer; indexBuffer = NULL; }
	if(normalBuffer) { delete [] normalBuffer; normalBuffer = NULL; }
}