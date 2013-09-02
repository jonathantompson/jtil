/*************************************************************
**				obboxRenderitems class						**
*************************************************************/
// File:		obboxRenderitems.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include	"physics\obbtree_related\obboxRenderitems.h"
#include	"utils_and_misc_classes\stringUtil.h"

#include	"utils_and_misc_classes\new_redefine.h" // MUST COME LAST

obboxRenderitems::obboxRenderitems()
{
	cHullVert = NULL;
	cHullVertCount = 0;
	cHullInd = NULL;
	cHullIndCount = 0;
	convexHullVertexBuffer = NULL;
	convexHullIndexBuffer = NULL;
	convexHullVertexBufferSize = 0;
	convexHullIndexBufferSize = 0;
	OBBVertexBuffer = NULL;
	OBBIndexBuffer = NULL;
	OBBVertexBufferSize = 0;
	OBBIndexBufferSize = 0;	
}
obboxRenderitems::~obboxRenderitems()
{
	ReleaseCOM(convexHullVertexBuffer);
	ReleaseCOM(convexHullIndexBuffer);
	ReleaseCOM(OBBVertexBuffer);
	ReleaseCOM(OBBIndexBuffer);	
	if(cHullVert) { delete [] cHullVert; cHullVert = NULL; }
	if(cHullInd) { delete [] cHullInd; cHullInd = NULL; }
}