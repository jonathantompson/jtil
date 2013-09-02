//=================================================================================================
// util.h - utility functions
//=================================================================================================
// This is borrowed from http://thetavern.servebeer.com/?p=articles&a=D3DTutorial1
// I liked their implimentation of error handling during initialization
//
// There is other stuff thrown in here that doesn't fit anywhere good.

#ifndef util_H_INCLUDED
#define util_H_INCLUDED

#include	"dxInclude.h"
#ifndef EPSILON
	#define EPSILON						0.00000001 // A small value (with some margin)
#endif
#include	<stdexcept>
#include	<stdlib.h>
#include	<string.h>
#include	<vector>
#include	<sstream>

class double3x3;
class double3;

struct Mtrl;
template <typename T> class vec_ptrs;
class texSamplerState;

namespace util
{
	// My own code to test whether or not a matrix is a rotation matrix
	bool isRotation(double3x3 * mat);
	bool isBasis(double3 * vec0,double3 * vec1,double3 * vec2);

	void GetCorrectUp(D3DXVECTOR3 * lookAtPt, D3DXVECTOR3 * eyePt, D3DXVECTOR3 * up);

	bool IsWin7();
	bool IsVista();
};

#endif // util_H_INCLUDED