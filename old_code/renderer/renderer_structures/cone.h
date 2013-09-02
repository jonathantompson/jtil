/*************************************************************
**						cone								**
**************************************************************/
// File:		cone.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef cone_h
#define cone_h

#include "dxInclude.h"
#include "renderer\renderer_structures\d3dFormats.h"

class cone
{
public:

	// Constructor / Destructor
    cone(UINT _numBaseVertices, float _height, float _baseInsideRadius, D3DCOLOR _color, float * ret_outsideRadius);
	~cone();

	inline D3DXVECTOR3 *			GetPosBuff() { return posBuffer; }
	inline UINT						GetNumVert() { return numVert; }
	inline D3DXVECTOR3 *			GetNormBuff() { return normalBuffer; }
	inline DWORD *					GetIndBuff() { return indexBuffer; }
	inline UINT						GetNumInd() { return numInd; }
	inline Mtrl *					GetMtrl(){ return & mtrl; }

	static D3DXVECTOR3				coneForward;

private:
	UINT							numBaseVertices;
	float							height;
	float							baseInsideRadius;
	float							baseOutsideRadius;

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