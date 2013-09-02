/*************************************************************
**						sphere								**
**************************************************************/
// File:		sphere.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef _USE_MATH_DEFINES
	#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include "renderer\renderer_structures\sphere.h"
#include "renderer\renderer_structures\d3dFormats.h"
#include "renderer\renderer_structures\vertex.h"
#include "utils_and_misc_classes\stringUtil.h"
#include "main.h"
#include "renderer\renderer.h"
#include "utils_and_misc_classes\math\math_funcs.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST


/************************************************************************/
/* Name:		sphere													*/
/* Description:	Default constructor function							*/
/************************************************************************/
sphere::sphere(UINT _numStacks, UINT _numSlices, float _insideRadius, D3DCOLOR _color, float * ret_outsideRadius)
{
	numStacks = _numStacks;
	if(numStacks < 3)
		throw std::runtime_error("sphere::sphere() - Error: numStacks < 3");
	numSlices = _numSlices;
	if(numSlices < 4)
		throw std::runtime_error("sphere::sphere() - Error: numSlices < 4");
	insideRadius = _insideRadius;
	color = _color;

	numVert = max((numStacks - 2),0) * numSlices + 2; // bottom and top stacks only have 1 point each
	posBuffer = new D3DXVECTOR3[numVert];
	normalBuffer = new D3DXVECTOR3[numVert];

	numInd = (2 * numSlices * 3) + max((numStacks - 3),0) * (2 * numSlices * 3);
	indexBuffer = new DWORD[numInd];

	float sliceSeperationAngle = 2.0f * (float)M_PI / (float)numSlices;
	float sliceOutsideRadius = insideRadius / cos(sliceSeperationAngle * 0.5f); // for vertex points

	float stackSeperationAngle = 1.0f * (float)M_PI / (float)(numStacks - 1);
	float stackOutsideRadius = insideRadius / cos(stackSeperationAngle * 0.5f); // for vertex points

	float outsideRadius = max(sliceOutsideRadius, stackOutsideRadius);
	if(ret_outsideRadius != NULL)
		*ret_outsideRadius = outsideRadius;

	float phi; float theta; // classic spherical coords (theta = azimuth angle from top, phi = zenith angle along slice)
	UINT curVertex = 0;

	// top first
	phi = 0; theta = 0;
	SphericalToCartesean(& posBuffer[curVertex], outsideRadius, phi, theta); 
	D3DXVec3Normalize(& normalBuffer[curVertex], & posBuffer[curVertex]); // Just point the normal outwards
	curVertex ++;

	// Intermediate stacks
	for( UINT i = 1; i < (numStacks-1); i ++)
	{
		theta = i * stackSeperationAngle; // [0, pi]
		for( UINT j = 0; j < numSlices; j ++)
		{
			phi = j * sliceSeperationAngle; // [0, 2*pi]
			SphericalToCartesean(& posBuffer[curVertex], outsideRadius, phi, theta); 
			D3DXVec3Normalize(& normalBuffer[curVertex], & posBuffer[curVertex]); // Just point the normal outwards
			curVertex ++;
		}
	}

	// bottom
	phi = 0; theta = (float)M_PI;
	SphericalToCartesean(& posBuffer[curVertex], outsideRadius, phi, theta);
	D3DXVec3Normalize(& normalBuffer[curVertex], & posBuffer[curVertex]); // Just point the normal outwards
	curVertex ++;

	if(curVertex != numVert)
		throw std::runtime_error("sphere::sphere() - Internal Error: Didn't create enough verticies");

	// Top Indices
	UINT curIndex = 0;
	for( UINT j = 0; j < ( numSlices - 1); j ++)
	{
		indexBuffer[curIndex] = 0; // Top Vertex
		curIndex ++;
		indexBuffer[curIndex] = j + 1; // First vertex is the top, so first slice vertex starts at 1
		curIndex ++;
		indexBuffer[curIndex] = j + 2;
		curIndex ++;
	}
	indexBuffer[curIndex] = 0; // Top Vertex
	curIndex ++;
	indexBuffer[curIndex] = numSlices; // Back to the first vertex
	curIndex ++;
	indexBuffer[curIndex] = 1; 
	curIndex ++;

	// Intermediate indices
	for( UINT i = 1; i < (numStacks - 2); i ++ )
	{
		UINT curStackStartIndex = 1 + (i - 1) * numSlices;
		UINT nextStackStartIndex = 1 + i * numSlices;
		for( UINT j = 0; j < (numSlices - 1); j ++)
		{
			indexBuffer[curIndex] = curStackStartIndex + j;
			curIndex ++;
			indexBuffer[curIndex] = nextStackStartIndex + j;
			curIndex ++;
			indexBuffer[curIndex] = curStackStartIndex + j + 1;
			curIndex ++;
			indexBuffer[curIndex] = curStackStartIndex + j + 1;
			curIndex ++;
			indexBuffer[curIndex] = nextStackStartIndex + j;
			curIndex ++;
			indexBuffer[curIndex] = nextStackStartIndex + j + 1;
			curIndex ++;
		}
		indexBuffer[curIndex] = curStackStartIndex + (numSlices-1);
		curIndex ++;
		indexBuffer[curIndex] = nextStackStartIndex + (numSlices-1);
		curIndex ++;
		indexBuffer[curIndex] = curStackStartIndex; // Back to the first vertex
		curIndex ++;
		indexBuffer[curIndex] = curStackStartIndex; // Back to the first vertex
		curIndex ++;
		indexBuffer[curIndex] = nextStackStartIndex + (numSlices-1);
		curIndex ++;
		indexBuffer[curIndex] = nextStackStartIndex; // Back to the first vertex
		curIndex ++;
	}

	// Bottom Indices
	UINT curStackStartIndex = ( numStacks - 3 ) * numSlices + 1;
	for( UINT j = 0; j < ( numSlices - 1); j ++)
	{
		indexBuffer[curIndex] = curStackStartIndex + j; // First vertex is the top, so first slice vertex starts at 1
		curIndex ++;
		indexBuffer[curIndex] = numVert-1; // Bottom Vertex
		curIndex ++;
		indexBuffer[curIndex] = curStackStartIndex + j + 1;
		curIndex ++;
	}
	indexBuffer[curIndex] = curStackStartIndex + numSlices - 1;
	curIndex ++;
	indexBuffer[curIndex] = numVert-1; // Top Vertex
	curIndex ++;
	indexBuffer[curIndex] = curStackStartIndex; // Back to the first vertex
	curIndex ++;

	if(curIndex != numInd)
		throw std::runtime_error("sphere::sphere() - Internal Error: Didn't create enough indices");

	mtrl = Mtrl(color,DEFAULT_SPEC_POWER,1.0f);
}

/************************************************************************/
/* Name:		~sphere													*/
/* Description:	Default destructor function								*/
/************************************************************************/
sphere::~sphere()
{
	if(posBuffer) { delete [] posBuffer; posBuffer = NULL; }
	if(indexBuffer) { delete [] indexBuffer; indexBuffer = NULL; }
	if(normalBuffer) { delete [] normalBuffer; normalBuffer = NULL; }
}