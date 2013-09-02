/*************************************************************
**						AABBox								**
**************************************************************/
// File:		AABBox.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef AABBox_h
#define AABBox_h

#include "dxInclude.h"

#ifndef DATA_ALIGNMENT
#define DATA_ALIGNMENT 16
#endif

__declspec(align(DATA_ALIGNMENT)) class AABBox
{
public:

	// Constructor / Destructor
    AABBox();
	~AABBox();

	D3DXVECTOR3			min;
	float				padding_0;
	D3DXVECTOR3			max;
	float				padding_1;

	// Expand(), check current min/max value against input vector and set new min/max if appropriate
	void				Expand(D3DXVECTOR3 * vec);

private:


};

#endif