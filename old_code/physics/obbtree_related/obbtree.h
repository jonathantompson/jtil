// File:		obbtree.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is header obbject to store OBB tree data:
// S. Gottschalk et. al. "OBBTree: A Hierarchical Structure for Rapid Iterference Detection"

#ifndef obbtree_h
#define obbtree_h

#include "dxInclude.h"

class obbox;
class double3;
class double3x3;
class rbobjectMesh;
class intPair;
class obbtreeTempVar;
template <typename T> class vecA;

class obbtree 
{
public:

					obbtree(DWORD _numFaces, int _numVerticies);
					~obbtree();

	friend class	rbobject; // This is lazy I know!
	friend class	rbobjectMesh;
	friend class	obbox;
	friend class	debugObject;
	friend class	app;

	void			InitOBBTree(rbobjectMesh * rbo, LPCTSTR obbDataFilename );	// Recursive wrapper, to initialize root and start recursion
	obbox *			GetObbToRender(); // Get a pointer to the obbox we're going to render

	// Access modifier functions for classes / functions that aren't friends
	inline int *	GetIndices(){ return indices; }
	inline int		GetNumIndicies(){ return numIndices; }

	// Collision Tests and support functions
	static void		TestOBBTreeCollision( rbobjectMesh * a, rbobjectMesh * b, bool useSSESIMD);
	static void		TestOBBTreeCollisionDebug(rbobjectMesh * a, rbobjectMesh * b, bool useSSESIMD); // Puts root node on stack and enters debug mode
	static void		TestOBBTreeCollisionDebug(bool useSSESIMD); // Tests remaining pairs in obbtree::obbox_pairs in debug mode

	void			MoveUp();
	void			MoveLeft();
	void			MoveRight();
 
	// File IO of OBB Tree
	void			writeObbToDisk( LPCTSTR Filename );
	void			writeObbToDiskCompressed( LPCTSTR Filename );
	bool			readObbFromDisk( rbobjectMesh * rbo, LPCTSTR Filename );
	bool			readObbFromDiskCompressed( rbobjectMesh * rbo, LPCTSTR Filename );

private:
	// obbox array is stored in a vector aray
	vecA<obbox> *	tree; // 2n - 1 cells in the tree --> This is a vector that can increase in size.
	int *			indices; // 3 indicies per face. Now use one global pool rather than dynamic memory for every node.
	int				numIndices;
	int				curOBBRender; // Current obbox and CH to render
	bool			scaleSet; // A flag to make sure the obbox scale is only set once.
	DWORD			numFaces;		// At the root node
	int				numVerticies;	// At the root node

	// Big array of temporary variables used when building OBBOX tree.  Only allocated when needed and deleted to save space when no longer needed.
	obbtreeTempVar * temp;
};

#endif