//=================================================================================================
// util.cpp - utility functions
//=================================================================================================

#include "utils_and_misc_classes\util.h"
#include "utils_and_misc_classes\math\double3x3.h"

#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

// Calculates correct up vector for a given look at point and eye point.
// --> Rotates up vector to be perpendicular with direction of eye.
void util::GetCorrectUp(D3DXVECTOR3 * lookAtPt, D3DXVECTOR3 * eyePt, D3DXVECTOR3 * up)
{
	D3DXVECTOR3 rotAxis, dir;
	D3DXMATRIXA16 matRot;

	dir = *lookAtPt - *eyePt;									// find direction vector
	D3DXVec3Normalize(&dir, &dir);								// normalize direction vector
	D3DXVec3Normalize(up, up);									// normalize the up vector

	float rotAngle = acos(D3DXVec3Dot(&dir, up));				// acosine of dot product will give angle between vectors if their magnitudes are 1
	// Check that up is not on the same axis as direction vector
	if(abs(rotAngle-D3DX_PI) < 0.001f || abs(rotAngle-0.0f) < 0.001f)
	{
		// If it is, preturb the up vector slightly
		up->x += 0.0001f;
		D3DXVec3Normalize(up, up);
		rotAngle = acos(D3DXVec3Dot(&dir, up));
	}
	rotAngle = rotAngle - D3DX_PI / 2.0f;						// want new up to be perpendicular to direction vector

	D3DXVec3Cross(&rotAxis, up, &dir);							// cross product of up and direction will give axis of rotation
	D3DXVec3Normalize(&rotAxis, &rotAxis);						// normalize the rotation axis

	D3DXMatrixRotationAxis(&matRot,&rotAxis, rotAngle);			// Build a rotation matrix from angle and axis
	D3DXVec3TransformCoord(up, up, &matRot);					// rotate the up vector
} //GetCorrectUp

double Determinant(const double3x3& m );
double  dot( const double3& a, const double3& b );
double3 cross( const double3& a, const double3& b );
double  magnitude( const double3& v );
double3x3 Inverse(const double3x3& a);
double3x3 Transpose( const double3x3& m );

// Tests whether a matrix is rotational or not:
// The condition for this is: (from http://en.wikipedia.org/wiki/Rotation_matrix)
// R' * R = I (or R' = R^-1)
// and
// det(R) =1
/*  NOTE: THERE IS SOME REDUNDANCY IN THESE TWO FUNCTIONS...  THERE ARE A MILLION WAYS TO TEST FOR THIS */
bool util::isRotation(double3x3 * mat)
{
	double epsilon = 0.01; // margin to allow for rounding errors
	double3x3 matInv = Inverse(*mat);
	double3x3 matTranspose = Transpose(*mat);
	if(abs(matInv.x.x - matTranspose.x.x)>epsilon) { return false; }
	if(abs(matInv.x.y - matTranspose.x.y)>epsilon) { return false; }
	if(abs(matInv.x.z - matTranspose.x.z)>epsilon) { return false; }
	if(abs(matInv.y.x - matTranspose.y.x)>epsilon) { return false; }
	if(abs(matInv.y.y - matTranspose.y.y)>epsilon) { return false; }
	if(abs(matInv.y.z - matTranspose.y.z)>epsilon) { return false; }
	if(abs(matInv.z.x - matTranspose.z.x)>epsilon) { return false; }
	if(abs(matInv.z.y - matTranspose.z.y)>epsilon) { return false; }
	if(abs(matInv.z.z - matTranspose.z.z)>epsilon) { return false; }
	return (abs(abs(Determinant(*mat))-1) < epsilon);
}
bool util::isBasis(double3 * vec0,double3 * vec1,double3 * vec2)
{
	double epsilon = 0.01; // margin to allow for rounding errors
	if (abs(magnitude(*vec0)-1) > epsilon) return false;
	if (abs(magnitude(*vec1)-1) > epsilon) return false;
	if (abs(magnitude(*vec2)-1) > epsilon) return false;
	if(abs(abs(dot(*vec2,cross(*vec0,*vec1)))-1) > epsilon) return false;
	if (abs(dot(*vec0,*vec1)) > epsilon) return false;
	if (abs(dot(*vec0,*vec2)) > epsilon) return false;
	if (abs(dot(*vec1,*vec2)) > epsilon) return false;
	return true;
}

bool util::IsWin7() {
    DWORD version = GetVersion();
    DWORD major = (DWORD) (LOBYTE(LOWORD(version)));
    DWORD minor = (DWORD) (HIBYTE(LOWORD(version)));

    return (major == 6) && (minor == 1);
}

bool util::IsVista() {
    DWORD version = GetVersion();
    DWORD major = (DWORD) (LOBYTE(LOWORD(version)));
    DWORD minor = (DWORD) (HIBYTE(LOWORD(version)));

    return (major == 6) && (minor == 0);
}