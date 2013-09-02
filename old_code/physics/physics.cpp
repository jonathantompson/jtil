/*************************************************************
**						Physics engine						**
**				Summer 2009									**
*************************************************************/
// File:		physics.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com
// Collision detection: Bounding box culling with fine detection for non-convex meshes.
// Collision reaction: Impulse based, frictionless collisions, with minimum impulse responce.
// ODE solver: Choice of Euler integrator or Runge Kutta 4th order integrator

#include	"physics\physics.h"
#include	"physics\objects\rbobject.h"
#include	"physics\objects\rbobjectMesh.h"
#include	"main.h"
#include	"app.h"
#include	"objectManager\objectManager.h"
#include	"physics\collision_response\collision.h"
#include	<list>					// Doubly-linked-list
#include	"UI\UI.h"
#include	"UI\varNames.h"
#include    "utils_and_misc_classes\data_structures\intPair.h"
#include	"clk\clk.h"
#include	"renderer\renderer_structures\d3dFormats.h"
#include	"utils_and_misc_classes\data_structures\vec_ptrsA.h"
#include	"physics\obbtree_related\obbtree.h"
#include	"renderer\renderer_structures\debugObject.h"
#include	"lights\lightSpotSM.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

inline float physics::xAxisMinSel(vec_ptrsA<rbobject> * rb_list, UINT index) { return rb_list->GetElem(index)->AABBox.min.x; }
inline float physics::yAxisMinSel(vec_ptrsA<rbobject> * rb_list, UINT index) { return rb_list->GetElem(index)->AABBox.min.y; }
inline float physics::zAxisMinSel(vec_ptrsA<rbobject> * rb_list, UINT index) { return rb_list->GetElem(index)->AABBox.min.z; }
inline float physics::xAxisMaxSel(vec_ptrsA<rbobject> * rb_list, UINT index) { return rb_list->GetElem(index)->AABBox.max.x; }
inline float physics::yAxisMaxSel(vec_ptrsA<rbobject> * rb_list, UINT index) { return rb_list->GetElem(index)->AABBox.max.y; }
inline float physics::zAxisMaxSel(vec_ptrsA<rbobject> * rb_list, UINT index) { return rb_list->GetElem(index)->AABBox.max.z; }

/************************************************************************/
/* Name:		physics													*/
/* Description:	Default Constructor: Set ODE solver variables			*/
/************************************************************************/
physics::physics()
{
	// Rigid body simulator (RK4)
	y = NULL;
	ydot = NULL;
	yfinal = NULL;
	ygraphics = NULL;
	k1 = NULL; 
	k2 = NULL;
	k3 = NULL; 
	k4 = NULL;
	ytemp = NULL;

	// Collision detection
	AABBXaxis = NULL;
	AABBYaxis = NULL;
	AABBZaxis = NULL;
	AABBActiveList = NULL;

	// initialize AABB overlap array: Size is (NO_RBOBJECTS * NO_RBOBJECTS) --> 2n wasted!  But easier to index
	AABBOverlapStatus = NULL;
	OBBCollisions = NULL;

	// Fine collision detection
	obbox_pairs = new vec<intPair>(1);

	GRAVITY = g_UI->GetSetting<D3DXVECTOR3>( &var_startingGravity );
}
/************************************************************************/
/* Name:		~physics												*/
/* Description:	Default Destructor										*/
/************************************************************************/
physics::~physics()
{
	if(y) { delete y; y = NULL; }
	if(ydot) { delete ydot; ydot = NULL; }
	if(yfinal) { delete yfinal; yfinal = NULL; }
	if(ygraphics) { delete ygraphics; ygraphics = NULL; }
	if(k1) { delete k1; k1 = NULL; }
	if(k2) { delete k2; k2 = NULL; }
	if(k3) { delete k3; k3 = NULL; }
	if(k4) { delete k4; k4 = NULL; }
	if(ytemp) { delete ytemp; ytemp = NULL; }
	if(AABBXaxis) { delete AABBXaxis; AABBXaxis = NULL; }
	if(AABBYaxis) { delete AABBYaxis; AABBYaxis = NULL; }
	if(AABBZaxis) { delete AABBZaxis; AABBZaxis = NULL; }
	if(AABBActiveList) { delete AABBActiveList; AABBActiveList = NULL; }
	if(AABBOverlapStatus) { delete AABBOverlapStatus; AABBOverlapStatus = NULL; }
	if(OBBCollisions) { delete OBBCollisions; OBBCollisions = NULL; }
	if(obbox_pairs) { delete obbox_pairs; obbox_pairs = NULL; }

}
/************************************************************************/
/* Name:		calcStep												*/
/* Description:	Main Entry point for physics solver						*/
/************************************************************************/
void physics::calcStep(bool pausePhysics) // PHYSICS_STEP
{
	if(!pausePhysics && !g_app->m_EnterObbDebugMode )
	{
#ifdef _DEBUG
		if(y->Size() != g_objectManager->GetNumRBObjects() * STATESIZE) // This would indicate that nothing has been initialized
			throw std::runtime_error("physics::calcStep() - Physics system has not been initialized - Need to call ResizePhysicsSystem()");
#endif

		// Check if we're using SSE instructions
		bool useSSESIMD = g_UI->GetSetting<bool>(& var_useSSESIMD);

		// Make y = yfinal
		for(int i = 0; i < (STATESIZE * (int)g_objectManager->GetNumRBObjects()); i++)
			y->GetArray()[i] = yfinal->GetArray()[i];

		do
		{
		
			// Take a physics step and store it in yfinal
			//dydt_func dydt = &physics::dydt;
			ode(y->GetArray(), yfinal->GetArray(), STATESIZE * g_objectManager->GetNumRBObjects(), (float)g_clk->m_currentTimePhysics, 
				(float)g_clk->m_currentTimePhysics + g_app->GetPhysicsStep(), &physics::dydt);

			// Update rigid-body-objects for collision detection
			rbobject::StateToObjects(yfinal->GetArray());

			// Do a coarse collision test --> THIS CAN BE FASTER. USE INSERTION SORT SWAPPING TO KEEP TRACK OF OVERLAPS (AS PER BARAFF & WITKIN)
			CoarseCollisionDetection();

			// Do a coarse collision test to fill up the collision array.  The sweep and prune is O(n).
			// --> But then it checks a big boolian array in O(n^2).  Consider another implimentation!
			int numOverlaps = 0;
			for(int i = 0; i < ((int)g_objectManager->GetNumRBObjects()-1); i ++)
			{
				for(int j = i+1; j < ((int)g_objectManager->GetNumRBObjects()); j ++)
				{
					AABBOverlap * curOverlap = GetOverlapStatus(AABBOverlapStatus->GetArray(), i, j);
					if(curOverlap->xAxisOverlap && curOverlap->yAxisOverlap && curOverlap->zAxisOverlap)
					{
						// Objects potentially overlap
						numOverlaps ++;

						if(!g_app->m_EnterObbDebugMode)
						{
							//  Get the collision pairs
							if(g_UI->GetComboBoxVal(&var_OBBDebugMode) == 1)
							{
#ifdef _DEBUG
								if(g_objectManager->GetRBObject(i)->GetType() != T_RBOBJECTMESH)
									throw std::exception("physics::calcStep() - ObbDebugObjectA is not an rbobjectMesh (possibly rbobject)");
								if(g_objectManager->GetRBObject(j)->GetType() != T_RBOBJECTMESH)
									throw std::exception("physics::calcStep() - ObbDebugObjectB is not an rbobjectMesh (possibly rbobject)");
#endif
								ObbDebugObjectA = dynamic_cast<rbobjectMesh*>(g_objectManager->GetRBObject(i));
								ObbDebugObjectB = dynamic_cast<rbobjectMesh*>(g_objectManager->GetRBObject(j));
								obbtree::TestOBBTreeCollisionDebug( ObbDebugObjectA, ObbDebugObjectB, useSSESIMD);
							} 
							else
							{
#ifdef _DEBUG
								if(g_objectManager->GetRBObject(i)->GetType() != T_RBOBJECTMESH)
									throw std::exception("physics::calcStep() - ObbObjectA is not an rbobjectMesh (possibly rbobject)");
								if(g_objectManager->GetRBObject(j)->GetType() != T_RBOBJECTMESH)
									throw std::exception("physics::calcStep() - ObbObjectB is not an rbobjectMesh (possibly rbobject)");
#endif
								obbtree::TestOBBTreeCollision( dynamic_cast<rbobjectMesh*>(g_objectManager->GetRBObject(i)), dynamic_cast<rbobjectMesh*>(g_objectManager->GetRBObject(j)), useSSESIMD );
							}
						} //if(!g_app->m_EnterObbDebugMode)
					} //if(curOverlap->xAxisOverlap && curOverlap->yAxisOverlap && curOverlap->zAxisOverlap)
				} //for(int j = i+1; j < ((int)g_objectManager->GetNumRBObjects()); j ++)
			} //for(int i = 0; i < ((int)g_objectManager->GetNumRBObjects()-1); i ++)

			if(!g_app->m_EnterObbDebugMode)
			{
				// Render obbox leaf collisions
				if(g_UI->GetSetting<bool>( &var_pauseOnOBBLeafCollision ) && OBBCollisions->Size()>0)
					g_UI->SetSetting<bool>( &var_pausePhysics, true );
				if(g_UI->GetSetting<bool>( &var_renderOBBLeafCollisionBoxes ) && OBBCollisions->Size()>0)
				{
					// Render the boxes that are colliding.
					for(UINT i = 0; i <  OBBCollisions->Size(); i ++)
					{
						collision * curCollision = OBBCollisions->GetElem(i);
						debugObject::AddObboxDebugObject(curCollision->obbox_a, curCollision->mesh_a, BLACK);
						debugObject::AddObboxDebugObject(curCollision->obbox_b, curCollision->mesh_b, BLACK);
					}
				}
				if(g_UI->GetSetting<bool>( &var_renderOBBLeafCollisionTriangles ) && OBBCollisions->Size()>0)
				{
					// Render the triangles that are colliding.
					for(UINT i = 0; i <  OBBCollisions->Size(); i ++)
					{
						collision * curCollision = OBBCollisions->GetElem(i);
						debugObject::AddObboxTriDebugObject(curCollision->obbox_a, curCollision->mesh_a, BLACK);
						debugObject::AddObboxTriDebugObject(curCollision->obbox_b, curCollision->mesh_b, BLACK);
					}
				}
			}
			// Recover from collisions
		} 
		while ( false /* While there are collisions to process */ );
	}

	if(g_app->m_EnterObbDebugMode)
	{
		if(g_app->m_ObbDebugModeNext)
		{
			if(obbox_pairs->IsEmpty())
			{
				g_app->m_EnterObbDebugMode = false; // Allow physics system to proceed as normal
				g_app->m_ObbDebugModeNext = false;
				g_objectManager->ClearDebugObjects(); // Recursively delete the list
			}
			else
			{
				g_app->m_ObbDebugModeNext = false;	
				obbtree::TestOBBTreeCollisionDebug( ObbDebugObjectA, ObbDebugObjectB, g_UI->GetSetting<bool>(& var_useSSESIMD) );
			}
		}
	}

	// DEBUG: rotate the shadow casting light
	if(g_UI->GetSetting<bool>(&var_rotateLight))
		g_objectManager->MoveLights(g_app->GetPhysicsStep() * 0.8f);

}
/************************************************************************/
/* Name:		ResizePhysicsSystem										*/
/* Description:	Initializes the physics system to a new number of		*/
/*              objects. O(n) --> Try to call only once on startup		*/
/************************************************************************/
void physics::ResizePhysicsSystem(UINT numObjects)
{
	// Rigid body simulator (RK4)
	if(y)
	{ y->Clear(); y->SetCapacity(numObjects); }
	else
		y = new vec<float>(STATESIZE * numObjects);
	y->ForceSize(STATESIZE * numObjects);
	
	if(yfinal)
	{ yfinal->Clear(); yfinal->SetCapacity(numObjects); }
	else
		yfinal = new vec<float>(STATESIZE * numObjects);
	yfinal->ForceSize(STATESIZE * numObjects);
	
	if(ygraphics)
	{ ygraphics->Clear(); ygraphics->SetCapacity(numObjects); }
	else
		ygraphics = new vec<float>(STATESIZE * numObjects);
	ygraphics->ForceSize(STATESIZE * numObjects);
	
	if(k1)
	{ k1->Clear(); k1->SetCapacity(numObjects); }
	else
		k1 = new vec<float>(STATESIZE * numObjects);
	k1->ForceSize(STATESIZE * numObjects);
	
	if(k2)
	{ k2->Clear(); k2->SetCapacity(numObjects); }
	else
		k2 = new vec<float>(STATESIZE * numObjects);
	k2->ForceSize(STATESIZE * numObjects);
	
	if(k3)
	{ k3->Clear(); k3->SetCapacity(numObjects); }
	else
		k3 = new vec<float>(STATESIZE * numObjects);
	k3->ForceSize(STATESIZE * numObjects);
	
	if(k4)
	{ k4->Clear(); k4->SetCapacity(numObjects); }
	else
		k4 = new vec<float>(STATESIZE * numObjects);
	k4->ForceSize(STATESIZE * numObjects);
	
	if(ytemp)
	{ ytemp->Clear(); ytemp->SetCapacity(numObjects); }
	else
		ytemp = new vec<float>(STATESIZE * numObjects);
	ytemp->ForceSize(STATESIZE * numObjects);
	
	// Collision detection
	if(AABBXaxis)
	{ AABBXaxis->Clear(); AABBXaxis->SetCapacity(numObjects); }
	else
		AABBXaxis = new vec<int>(numObjects);
	
	if(AABBYaxis)
	{ AABBYaxis->Clear(); AABBYaxis->SetCapacity(numObjects); }
	else
		AABBYaxis = new vec<int>(numObjects);
	
	if(AABBZaxis)
	{ AABBZaxis->Clear(); AABBZaxis->SetCapacity(numObjects); }
	else
		AABBZaxis = new vec<int>(numObjects);
	
	if(AABBActiveList)
	{ AABBActiveList->Clear(); AABBActiveList->SetCapacity(numObjects); }
	else
		AABBActiveList = new vec<int>(numObjects);
	
	// initialize AABB overlap array: Size is (NO_RBOBJECTS * NO_RBOBJECTS) --> 2n wasted!  But easier to index
	if(AABBOverlapStatus)
	{ AABBOverlapStatus->Clear(); AABBOverlapStatus->SetCapacity(numObjects); }
	else
		AABBOverlapStatus = new vec<AABBOverlap>(numObjects * numObjects);
	
	if(OBBCollisions)
	{ OBBCollisions->Clear(); OBBCollisions->SetCapacity(g_UI->GetSetting<int>( &var_collisionVectorSize )); }
	else
		OBBCollisions = new vec<collision>(g_UI->GetSetting<int>( &var_collisionVectorSize ));
	
	InitPhysics(); // fill the array's with data
}
/************************************************************************/
/* Name:		InterpolateFrame										*/
/* Description:	Linearly interpolate graphics state between steps		*/
/************************************************************************/
void physics::InterpolateFrame(float alphaTime)
{
	// These functions (although poor OOP practice, help avoid large numbers of inline calls
	float * ygraph = ygraphics->GetArray();
	float * yfin = yfinal->GetArray();
	float * y0 = y->GetArray();

	// Prefetch some settings
	bool physicsPaused = g_UI->GetSetting<bool>( &var_pausePhysics );
	int numRBObjects = (int)g_objectManager->GetNumRBObjects();

	for(int i = 0; i < (STATESIZE * numRBObjects); i++)
	{
		if( !physicsPaused && !g_app->m_EnterObbDebugMode) // Interpolate only if physics is not paused AND we're not in OBB debug mode
			ygraph[i] = yfin[i]*alphaTime + y0[i]*(1.0f - alphaTime);
		else
			ygraph[i] = yfin[i];
	}
	rbobject::StateToObjects(ygraph);
	rbobject * curRBObject = NULL;
	for(int i = 0; i < numRBObjects; i++)
	{
		curRBObject = g_objectManager->GetRBObject(i);
		curRBObject->dirtyScaleMatrix = true;
		curRBObject->dirtyRotMatrix = true;
		curRBObject->dirtyTransMatrix = true;
	}
}
/************************************************************************/
/* Name:		InitPhysics												*/
/* Description:	Initialization of array and any other bookkeeping		*/
/************************************************************************/
void physics::InitPhysics()
{
	if(y)
		rbobject::ObjectsToState(y->GetArray());
	if(yfinal)
		rbobject::ObjectsToState(yfinal->GetArray());

	InitAABBs();
}
/************************************************************************/
/* Name:		GetOverlapStatus										*/
/* Description:	Get correct index in AABBOverlapStatus array. Makes		*/
/*				higher level code easier to read.						*/
/************************************************************************/
AABBOverlap * GetOverlapStatus(AABBOverlap * AABBOverlapStatus, int a, int b)
{
	int NUM_RBOBJECTS = (int)g_objectManager->GetNumRBObjects();
#ifdef _DEBUG
	if(a>=NUM_RBOBJECTS || b>=NUM_RBOBJECTS)
		throw std::runtime_error("GetOverlapStatus: object a or object b are out of bounds!");
	if(a == b)
		throw std::runtime_error("GetOverlapStatus: object a = object b!");
#endif
	if(a < b)
		return & AABBOverlapStatus[a * NUM_RBOBJECTS + b];
	else
		return & AABBOverlapStatus[b * NUM_RBOBJECTS + a];
}
void SetOverlapStatusXaxis(AABBOverlap * AABBOverlapStatus, int a, int b, bool setval)
{
	int NUM_RBOBJECTS = (int)g_objectManager->GetNumRBObjects();
#ifdef _DEBUG
	if(a>=NUM_RBOBJECTS || b>=NUM_RBOBJECTS)
		throw std::runtime_error("GetOverlapStatus: object a or object b are out of bounds!");
	if(a == b)
		throw std::runtime_error("GetOverlapStatus: object a = object b!");
#endif
	if(a < b)
		AABBOverlapStatus[a * NUM_RBOBJECTS + b].xAxisOverlap = setval;
	else
		AABBOverlapStatus[b * NUM_RBOBJECTS + a].xAxisOverlap = setval;
}
void SetOverlapStatusYaxis(AABBOverlap * AABBOverlapStatus, int a, int b, bool setval)
{
	int NUM_RBOBJECTS = (int)g_objectManager->GetNumRBObjects();
#ifdef _DEBUG
	if(a>=NUM_RBOBJECTS || b>=NUM_RBOBJECTS)
		throw std::runtime_error("GetOverlapStatus: object a or object b are out of bounds!");
	if(a == b)
		throw std::runtime_error("GetOverlapStatus: object a = object b!");
#endif
	if(a < b)
		AABBOverlapStatus[a * NUM_RBOBJECTS + b].yAxisOverlap = setval;
	else
		AABBOverlapStatus[b * NUM_RBOBJECTS + a].yAxisOverlap = setval;
}
void SetOverlapStatusZaxis(AABBOverlap * AABBOverlapStatus, int a, int b, bool setval)
{
	int NUM_RBOBJECTS = (int)g_objectManager->GetNumRBObjects();
#ifdef _DEBUG
	if(a>=NUM_RBOBJECTS || b>=NUM_RBOBJECTS)
		throw std::runtime_error("GetOverlapStatus: object a or object b are out of bounds!");
	if(a == b)
		throw std::runtime_error("GetOverlapStatus: object a = object b!");
#endif
	if(a < b)
		AABBOverlapStatus[a * NUM_RBOBJECTS + b].zAxisOverlap = setval;
	else
		AABBOverlapStatus[b * NUM_RBOBJECTS + a].zAxisOverlap = setval;
}
/************************************************************************/
/* Name:		InitAABBs												*/
/* Description:	Initialize axis aligned bounding boxes and perform first*/
/*				Sort and sweep, expected O(n^2).						*/
/************************************************************************/
void physics::InitAABBs()
{
	int NO_RBOBJECTS = (int)g_objectManager->GetNumRBObjects();

	// Reset all object overlap statuses.  This array wastes space --> 2x larger than necessary.  But faster indexing this way.
	// AABBOverlapStatus->Clear(); // NOT NECESSARY --> THIS IS DONE EARLIER (AND SIZE IS SET)
	AABBOverlap falseOverlap;
	falseOverlap.xAxisOverlap = false; falseOverlap.yAxisOverlap = false; falseOverlap.zAxisOverlap = false;

	for(int i = 0; i < (NO_RBOBJECTS * NO_RBOBJECTS - 1); i++)
		AABBOverlapStatus->Add(&falseOverlap);

	// Initialize axis lists as just the RBOBJECTS (arbitrary order)
	// AABBXaxis->Clear(); AABBYaxis->Clear(); AABBZaxis->Clear(); // NOT NECESSARY --> THIS IS DONE EARLIER (AND SIZE IS SET)
	for(int i = 0; i < (NO_RBOBJECTS); i++)
	{
		AABBXaxis->Add(&i);  AABBYaxis->Add(&i);  AABBZaxis->Add(&i);
	}

	// Perform a first time sort and sweep --> Expected slow due to insertion sort on unsorted list.
	CoarseCollisionDetection();
}
/************************************************************************/
/* Name:		InsertionSortAABBs										*/
/* Description:	Takes a list of rbobjects indices, and sorts them using*/
/*				the value derived from the selection function.			*/
/************************************************************************/
void physics::InsertionSortAABBs( int * pArray, int arraySize, select_func MinSel )
{
	if(arraySize<1 || pArray == NULL)
		throw std::runtime_error("physics::InsertionSortAABBs() - Array is empty, nothing to sort!");

	int rbobjectToSort; float curVal, valueToSort;
	for(int i=1; i<arraySize;i++) {  // Place the next value
		rbobjectToSort = pArray[i];
		valueToSort = CALL_MEMBER_FN(*this,MinSel)(g_objectManager->GetRBObjects(), rbobjectToSort);
		for(int j=0; j<=i;j++) {
			curVal = CALL_MEMBER_FN(*this,MinSel)(g_objectManager->GetRBObjects(), pArray[j]);
			if(curVal > valueToSort) {
				// push the other value forward to insert
				for(int m = i; m>j; m--) {
					pArray[m] = pArray[m-1];
				}
				pArray[j] = rbobjectToSort;
				break;
			}
		}
	}
}
/************************************************************************/
/* Name:		SweepAxisList											*/
/* Description:	Takes a list of rbobjects, performs a sweep determining */
/*				all overlaps, and sets the appropriate values in the	*/
/*				overlap list (using a set_func). pArray must already be	*/
/*				ordered by minimum AABB vertex.							*/
/************************************************************************/
void physics::SweepAxisList( int * pArray, int arraySize, select_func MinSel, select_func MaxSel, set_func Set )
{
	int curArrayIndex = 0; 
	AABBActiveList->ForceSize(0); // This clears the vector without deallocating the memory

	while(curArrayIndex < arraySize)
	{

		// Check the current object against objects on the active list
		for(int i = 0; i < (int)AABBActiveList->Size() ; /*Nothing*/)
		{
			int * curActiveListObject = AABBActiveList->GetElem(i);
			// If the new object start is after the active list end, then: REMOVE INDEX FROM ACTIVE LIST
			if(CALL_MEMBER_FN(*this,MaxSel)(g_objectManager->GetRBObjects(), * curActiveListObject) < CALL_MEMBER_FN(*this,MinSel)(g_objectManager->GetRBObjects(), pArray[curArrayIndex]))
			{
				(*Set)(AABBOverlapStatus->GetArray(), * curActiveListObject, pArray[curArrayIndex], false);

				// To remove the object from the active list, swap it with the last element and reduce the size by 1
				if(AABBActiveList->Size()>1)
				{
					UINT lastIndex = AABBActiveList->Size()-1;
					int * lastValue = AABBActiveList->GetElem(lastIndex);
					AABBActiveList->SetElem(lastValue, i); // Put the last element in the current index
				}

				AABBActiveList->ForceSize(AABBActiveList->Size()-1); // Reduce the size by 1
				
			}
			else // OTHERWISE THERE IS OVERLAP BETWEEN THESE OBJECTS, so set overlap status
			{
				(*Set)(AABBOverlapStatus->GetArray(), * curActiveListObject, pArray[curArrayIndex], true);
				i += 1;
			}
		}

		// Put the current rbobject index on the active list
		AABBActiveList->Add(&pArray[curArrayIndex]);
		
		curArrayIndex ++;
	}
}
/************************************************************************/
/* Name:		CoarseCollisionDetection								*/
/* Description:	Use Axis aligned bounding boxes to find all sets of		*/
/*				potential collisions. Expected O(n+k+c)					*/
/************************************************************************/
void physics::CoarseCollisionDetection()
{
	int NO_RBOBJECTS = (int)g_objectManager->GetNumRBObjects();
	// Update Bounding boxes
	g_objectManager->UpdateRBObjectBoundingBoxes();
	
	// Do an insertion sort on each axis list to order objects, by minimum BB vertex
	// Insertion sort is O(n) when almost sorted, therefore best for slowly moving objects
	InsertionSortAABBs(AABBXaxis->GetArray(), NO_RBOBJECTS, (select_func) &physics::xAxisMinSel);
	InsertionSortAABBs(AABBYaxis->GetArray(), NO_RBOBJECTS, (select_func) &physics::yAxisMinSel);
	InsertionSortAABBs(AABBZaxis->GetArray(), NO_RBOBJECTS, (select_func) &physics::zAxisMinSel);

	// Now Find all overlaps by doing sweep of each axis lists.
	SweepAxisList(AABBXaxis->GetArray(), NO_RBOBJECTS, (select_func) &physics::xAxisMinSel, (select_func) &physics::xAxisMaxSel, (set_func) &SetOverlapStatusXaxis);
	SweepAxisList(AABBYaxis->GetArray(), NO_RBOBJECTS, (select_func) &physics::yAxisMinSel, (select_func) &physics::yAxisMaxSel, (set_func) &SetOverlapStatusYaxis);
	SweepAxisList(AABBZaxis->GetArray(), NO_RBOBJECTS, (select_func) &physics::zAxisMinSel, (select_func) &physics::zAxisMaxSel, (set_func) &SetOverlapStatusZaxis);
}
/************************************************************************/
/* Name:		ode														*/
/* Description:	RK4 integrator ODE core, take state y0 and progress 	*/
/*			forward from time t0 to t1, and return in yend. 			*/
/************************************************************************/
// http://en.wikipedia.org/wiki/Runge–Kutta_methods
void physics::ode(float y0[], float yend[], int len, float t0, float t1, dydt_func dydt)
{
	float dt = t1 - t0; // time step

	float * k1_array = k1->GetArray(); // Avoid unnecessary inline calls
	float * k2_array = k2->GetArray();
	float * k3_array = k3->GetArray();
	float * k4_array = k4->GetArray();
	float * ytemp_array = ytemp->GetArray();

	// CALCULATE k1
	CALL_MEMBER_FN(*this,dydt)(t0, y0, k1_array);		// (derivative of y0 at t0 stored into k1)
	// Take step with 0.5*dt*k1 and CALCULATE k2
	for(int i = 0; i < len; i ++) ytemp_array[i] = y0[i] + 0.5f*dt*k1_array[i];
	CALL_MEMBER_FN(*this,dydt)(t0 + 0.5f*dt, ytemp_array, k2_array);
	// Take step with 0.5*dt*k2 and CALCULATE k2
	for(int i = 0; i < len; i ++) ytemp_array[i] = y0[i] + 0.5f*dt*k2_array[i];
	CALL_MEMBER_FN(*this,dydt)(t0 + 0.5f*dt, ytemp_array, k3_array);
	// Take step with dt*k3 and CALCULATE k4
	for(int i = 0; i < len; i ++) ytemp_array[i] = y0[i] + dt*k3_array[i];
	CALL_MEMBER_FN(*this,dydt)(t0 + dt, ytemp_array, k4_array);

	// Calculate final value using weighted averaging of 4 slopes
	for(int i = 0; i < len; i ++)
		yend[i] = y0[i] + (dt*(k1_array[i] + 2.0f*k2_array[i] + 2.0f*k3_array[i] + k4_array[i]))/6.0f;
}
/************************************************************************/
/* Name:		dydt													*/
/* Description:	Given a state y[t], dydt simply calculates the			*/
/*			derivative at the time t, and returns the result in ydot	*/
/************************************************************************/
void physics::dydt(float t, float y[], float ydot[])
{
	rbobject::StateToObjects(y); // Store state in y[] into rbobjects
	for(int i = 0; i < (int)g_objectManager->GetNumRBObjects(); i++)
	{
		ComputeForceAndTorque(t, g_objectManager->GetRBObject(i));
		ddtStateToArray(g_objectManager->GetRBObject(i), &ydot[i * STATESIZE]);
	}
}
/************************************************************************/
/* Name:		ComputeForceAndTorque									*/
/* Description:	Compute time dependant force and torque					*/
/************************************************************************/
void physics::ComputeForceAndTorque(float t, rbobject * rb)
{
	// Zero force and add gravity
	if(!rb->immovable)
	{
		rb->force = GRAVITY;
		rb->torque.x = 0.0f; rb->torque.y = 0.0f; rb->torque.z = 0.0f;
	}
	else
	{
		rb->force.x = 0.0f; rb->force.y = 0.0f; rb->force.z = 0.0f;
		rb->torque.x = 0.0f; rb->torque.y = 0.0f; rb->torque.z = 0.0f;
	}
}
/************************************************************************/
/* Name:		ddtStateToArray											*/
/* Description:	Given a rbobject, copy derivative into array ydot		*/
/************************************************************************/
void physics::ddtStateToArray(rbobject * rb, float * ydot)
{
	// dx(t)/dt = vel(t)
	*ydot++ = rb->v[0]; *ydot++ = rb->v[1]; *ydot++ = rb->v[2];
	// qdot(t) = 0.5 * w(t) q(t)
	D3DXQUATERNION qomega(rb->omega.x, rb->omega.y, rb->omega.z, 0.0f);
	D3DXQUATERNION qdot = 0.5 * (qomega * rb->q); // returns quaternion product [0,rb->omega]q
	*ydot++ = qdot.w; *ydot++ = qdot.x; *ydot++ = qdot.y; *ydot++ = qdot.z; 
	// dP(t)/dt = F(t)
	*ydot++ = rb->force.x; *ydot++ = rb->force.y; *ydot++ = rb->force.z;
	// dL(t)/dt = torque(t)
	*ydot++ = rb->torque.x; *ydot++ = rb->torque.y; *ydot++ = rb->torque.z;
}