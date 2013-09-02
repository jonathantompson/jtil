/*************************************************************
**					Rigid Body Objects						**
**			-> Store object states, Summer 2009				**
**************************************************************
File:		rbobject.h
Author:		Jonathan Tompson
e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

D. Baraff and A. Witkin's "Bypically Based Modeling: Principles and Practice
was very useful.  I used their code as a starting point.

want to do this eventually!
Some codes use hierarchical trees of proxy geometry, e.g., build a capsule (cylinder 
with spherical endcaps) for each limb of an articulated character. There are algorithms 
for building these proxy geometry hierarchies in an automated way. For example, do a 
search on OBBTree (oriented bounding box tree), and you should find papers from the 
University of North Carolina @ Chapel Hill on a technique to automatically build a 
hierarchical tree that uses rectanguloid-shaped boxes for the proxy geometry. */

#ifndef rbobject_h
#define rbobject_h

#include	"main.h"
#include	<vector>
#include	"physics\objects\AABbox.h"
#include	"dataAlign.h"

#define STATESIZE13		// size of a rbobject (x, quat, P and L) in array form
 
// ******************************
// Rigid body object parent class
// ******************************
class int3; 
class double3;
class obbtree;
struct CUSTOMVERTEX;

enum RBOBJECT_TYPE {
	T_RBOBJECT,
    T_RBOBJECTMESH,
};

#ifndef _MSC_VER
#pragma pack(push)
#pragma pack(16)
#endif

__declspec(align(DATA_ALIGNMENT)) class rbobject
{
public:
	// LAZY OOP --> MAYBE CLEAN UP LATER
	friend class rbobjectMesh;
	friend class obbox;
	friend class renderer;
	friend class debugObject;
	friend class physics;
	friend class obbtree;
	friend class objectManager;
	friend class app;
	friend int main ();

							rbobject();		// default Constructor

	inline void				SetScale(float newscale) { scale = newscale; dirtyScaleMatrix = true; }
	inline void				SetPosition(D3DXVECTOR3 newpos) { x = newpos; dirtyTransMatrix = true; }
	inline void				SetRotation(D3DXQUATERNION newrot) { q = newrot; dirtyRotMatrix = true; }
	inline void				SetLinearMomentum(D3DXVECTOR3 newP) { P = newP; }
	inline void				SetAngularMomentum(D3DXVECTOR3 newL) { L = newL; }
	inline void				SetAngularVelocity(D3DXVECTOR3 newOmega) { omega = newOmega; }
	void					UpdateBoundingBox(); // Transform BB into world space
	void					UpdateMatricies();
	void					GetItensorBox(D3DXVECTOR3 * maxBound, D3DXMATRIXA16 *Ib, D3DXMATRIXA16 *Ib_inv);
	void					GetItensor(double3 * verts, int3 * ind, DWORD numFaces, D3DXMATRIXA16 *Ib, D3DXMATRIXA16 *Ib_inv);
	inline AABBox *			GetAABox() { return & AABBox; }
	inline D3DXVECTOR3 *	GetWorldBounds() { return  m_worldBounds; }
	inline void				SaveMatWorld() { matWorldPrevFrame = matWorld; }
	void					RotateObjectAxisAngle( D3DXVECTOR3 axis, float rotAngle );

	static void				StateToArray(rbobject * rb, float *y);
	static void				ArrayToState(rbobject * rb, float *y);
	static void				ObjectsToState(float *yout);	// Store the state for all the rigid bodies into y[]
	static void				StateToObjects(float *yin);	// Load the state for all the rigid bodies from y[]

	// virtual members
	inline virtual void		Create(LPCTSTR Filename, IDirect3DDevice9 *pDevice) {}
	inline virtual void		Create(CUSTOMVERTEX vertices[], short numVert, short indices[], short numindices) {}
	inline virtual void		Render() {}
	inline virtual void		RenderSM() {}
	inline virtual void		Normalize(float scaleTo = 1.0f, bool center = true) {}
	inline virtual			~rbobject(){}	// default Destructor
	inline virtual void		GetVertexIndexData(){}

	inline virtual RBOBJECT_TYPE GetType() { return T_RBOBJECT; }

private: 
	// Constant Values
	//			Start 1st 16 byte block (4 * sizeof(float) = 16)
    // Note: OBBOX lengths for enitre OBB Tree are updated everytime the rboject scale factor is 
	//       updated.  This will take a long time --> Scale is only to be updated once at startup, 
	//       it was never meant to be dynamic.
	__declspec(align(DATA_ALIGNMENT)) float	scale;				// Scaling factor
	float					mass;				// Object Mass
	float					padding_0[2];
	//			Start 2nd 16 byte block (8 * 3 * sizeof(float) = 96)
	D3DXVECTOR3				m_objectBounds[8];	// Bounding Box data in MODEL SPACE
	//			Start 8th 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 			scaleMat;			// Scaling translation
	//			Start 12th 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 			Ibody;				// Body Space Inertia Tensor --> Calculated on runtime
	//			Start 16th 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 			Ibody_inv;			// Inverse Body Space Inertia Tensor

	// State Variables
	//			Start 20th 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				x;					// Position
	float					padding_1;
	//			Start 21st 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				x_t1;
	float					padding_2;
	//			Start 22nd 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				P;					// Linear Momentum
	float					padding_3;
	//			Start 23rd 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				P_t1;
	float					padding_4;
	//			Start 24th 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				L;					// Angular Momentum
	float					padding_5;
	//			Start 25th 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				L_t1;
	float					padding_6;
	//			Start 26th 16 byte block (4 * sizeof(float) = 16)
	D3DXQUATERNION			q;					// Rotational state
	//			Start 27th 16 byte block (4 * sizeof(float) = 16)
	D3DXQUATERNION			q_t1;
	// (use D3DXquaternionRotationMatrix and D3DXMATRIXA16 Rotationquaternion to transform: quaternion <--> matrix)

	// Derived Quantities (from state vectors)
	//			Start 28th 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 			Iinv;				// Inverse World Space Inertia Tensor
	//			Start 32nd 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 			R;					// Rotation matrix (derived from quaternions)
	//			Start 36th 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 			R_t;				// Transpose Rotation matrix
	//			Start 40th 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 			Trans;				// Translation matrix (updated when rendered)
	//			Start 44th 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 			matWorld;			// MODEL SPACE --> WORLD SPACE TRANSFORM
	//			Start 48th 16 byte block (16 * sizeof(float) = 64)
	D3DXMATRIXA16 			matWorldPrevFrame;	// MODEL SPACE --> WORLD SPACE TRANSFORM for the last rendered frame
	//			Start 49th 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				v;					// Velocity
	float					padding_7;
	//			Start 50th 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				omega;				// Angular Velocity
	float					padding_8;
	//			Start 51th 16 byte block (8 * 3 * sizeof(float) = 96)
	D3DXVECTOR3				m_worldBounds[8];	// Bounding Box data in WORLD SPACE (not axis aligned)
	//			Start 57th 16 byte block (8 * sizeof(float) = 16)
	AABBox					AABBox;
	//			Start 59th 16 byte block (4 * sizeof(int) = 16)
	int						dirtyScaleMatrix;
	int						dirtyRotMatrix;
	int						dirtyTransMatrix;
	int						immovable;
	
	// Computed quantities
	//			Start 59th 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				force;				// Total force on the object
	float					padding_11;
	//			Start 60th 16 byte block (4 * sizeof(float) = 16)
	D3DXVECTOR3				torque;				// Total torque on the object
	float					padding_12;

};

#ifndef _MSC_VER
#pragma pack(pop)
#endif

#endif