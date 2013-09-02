/*************************************************************
**					Physics engine							**
**				Summer 2009									**
*************************************************************/
// File:		physics.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com
// Collision detection: Bounding box culling with fine detection for non-convex meshes.
// Collision reaction: Impulse based, frictionless collisions, with minimum impulse responce.
// ODE solver: Choice of Euler integrator or Runge Kutta 4th order integrator

#ifndef physics_h
#define physics_h

#include	"dxInclude.h"
#include	<list>
#include	"utils_and_misc_classes\data_structures\vec.h"

struct AABBOverlap {
	bool								xAxisOverlap,   // xAxisOverlaps
										yAxisOverlap,
										zAxisOverlap;
};
AABBOverlap *							GetOverlapStatus(AABBOverlap * AABBOverlapStatus, int a, int b); // O(1)
void									SetOverlapStatusXaxis(AABBOverlap * AABBOverlapStatus, int a, int b, bool setval);
void									SetOverlapStatusYaxis(AABBOverlap * AABBOverlapStatus, int a, int b, bool setval);
void									SetOverlapStatusZaxis(AABBOverlap * AABBOverlapStatus, int a, int b, bool setval);

class physics;
class collision;
class rbobject;
class rbobjectMesh;
template <typename T> class vec_ptrs;
template <typename T> class vec_ptrsA;
class intPair;

// FUNCTION POINTERS
typedef void (physics::*dydt_func)(float t, float y[], float ydot[]); // Function pointer which takes the ODE step
typedef float (physics::*select_func)(vec_ptrsA<rbobject> * rb_list, int index); // Function pointer used in InsertionSortAABBs.  Grabs an rbobject from global list and grabs correct vertex info.
typedef float (*set_func)(AABBOverlap * AABBOverlapStatus, int a, int b, bool value); // Function pointer used in InsertionSortAABBs.  Sets the appropriate overlap status in AABBOverlap array.
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

class physics
{
public:
	friend class obbtree;  // Bad OOP, but it's easier this way
										physics();				// Default Constructor
										~physics();				// Default Destructor

	// Rigid Body Object functions
	void								calcStep(bool pausePhysics);					// Main entry point
	void								ode(float y0[], float yend[], int len, float t0, float t1, dydt_func dydt);
	void								dydt(float t, float y[], float ydot[]);
	void								ComputeForceAndTorque(float t, rbobject * rb);
	void								ddtStateToArray(rbobject * rb, float * ydot);
	void								InitPhysics();
	void								InterpolateFrame(float alphaTime);

	// Coarse Collision Detection
	void								InitAABBs(); // Set up data structures and perform intial sort and sweep, likely O(n^2)
	void								CoarseCollisionDetection(); // Perform per-step sort and sweep, probably O(n)
	void								InsertionSortAABBs( int * pArray, int arraySize, select_func MinSel );			 // Sort for coarse collision det.
	void								SweepAxisList( int * pArray, int arraySize, select_func MinSel, select_func MaxSel, set_func Set ); // Sweep for coarse collision det.
	inline float						physics::xAxisMinSel(vec_ptrsA<rbobject> * rb_list, UINT index);
	inline float 						physics::yAxisMinSel(vec_ptrsA<rbobject> * rb_list, UINT index);
	inline float 						physics::zAxisMinSel(vec_ptrsA<rbobject> * rb_list, UINT index);
	inline float 						physics::xAxisMaxSel(vec_ptrsA<rbobject> * rb_list, UINT index);
	inline float 						physics::yAxisMaxSel(vec_ptrsA<rbobject> * rb_list, UINT index);
	inline float 						physics::zAxisMaxSel(vec_ptrsA<rbobject> * rb_list, UINT index);

	// Interface with other blocks
	void								ResizePhysicsSystem(UINT numObjects);
	
	// These used to be private, but it's just easier this way
	vec<float> *						y;							// STATE: y[]
	vec<float> *						yfinal;						// STATE: yfinal[]
	vec<float> *						ydot;						// STATE: dydt[]
	vec<float> *						ygraphics;					// STATE: ygraphics[]

	vec<collision> *					OBBCollisions; 

private:
	// Derivatives for RK4 integrator and a temporary variable
	vec<float> *						k1, * k2, * k3, * k4, * ytemp;	

	// Coarse Collision detection
	vec<int> *							AABBXaxis;   // Sorted indices of h_rbobjects --> xaxis values
	vec<int> *							AABBYaxis;
	vec<int> *							AABBZaxis;
	vec<int> *							AABBActiveList; // Used to be a linked list of active values, now using vector and index pointers
	vec<AABBOverlap> *					AABBOverlapStatus; // Holds the overlap status for all rigid body object pairs --> Triangular array

	// Fine Collision Detection
	rbobjectMesh *						ObbDebugObjectA; // Pointer to the objects currently being rendered in OBBDebugMode
	rbobjectMesh *						ObbDebugObjectB;
	vec<intPair> *						obbox_pairs;

	// Constants - (constant per physics frame)
	D3DXVECTOR3							GRAVITY;  // This is updated once per physics iteration from the g_UI setting
};

#endif