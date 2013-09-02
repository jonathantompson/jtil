/*************************************************************
**						sphere								**
**************************************************************/
// File:		sphere.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef sphere_h
#define sphere_h

#include "dxInclude.h"
#include "renderer\renderer_structures\d3dFormats.h"

class sphere
{
public:

	// Constructor / Destructor
    sphere(UINT _numStacks, UINT _numSlices, float _insideRadius, D3DCOLOR _color, float * ret_outsideRadius);
	~sphere();

	inline D3DXVECTOR3 *			GetPosBuff() { return posBuffer; }
	inline UINT						GetNumVert() { return numVert; }
	inline D3DXVECTOR3 *			GetNormBuff() { return normalBuffer; }
	inline DWORD *					GetIndBuff() { return indexBuffer; }
	inline UINT						GetNumInd() { return numInd; }
	inline Mtrl *					GetMtrl(){ return & mtrl; }

private:
	UINT							numStacks;
	UINT							numSlices;
	float							insideRadius;

	UINT							numVert;
	UINT							numInd;
	D3DCOLOR						color;
	Mtrl							mtrl;

	// Affine transformation, scale * rotation * translation
	D3DXMATRIX 						matWorld;

	// Buffers
	D3DXVECTOR3 *					posBuffer;
	D3DXVECTOR3 *					normalBuffer;
	DWORD *							indexBuffer;
};

#endif