// File:		obbtree.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

// This is header obbject to store OBB tree data:
// S. Gottschalk et. al. "OBBTree: A Hierarchical Structure for Rapid Iterference Detection"

#include "physics\obbtree_related\obbtree.h"
#include "physics\obbtree_related\obbox.h" 
#include "physics\obbtree_related\obbtreeTempVar.h" 
#include "physics\objects\rbobjectMesh.h"
#include "physics\objects\rbobjectMeshData.h"
#include "objectManager\objectManager.h"
#include "hud\hud.h"
#include "physics\obbtree_related\obboxRenderitems.h"
#include "utils_and_misc_classes\stringUtil.h"
#include <sys/stat.h>
#include <zlib.h>
#include <UI\UI.h>
#include <UI\varNames.h>
#include "utils_and_misc_classes\data_structures\vecA.h"
#include "utils_and_misc_classes\SIMD_helpers\dataAlign.h"
#include "app.h"

#include <new>        // Must #include this to use "placement new"

//#include "utils_and_misc_classes\new_redefine.h" // CAN'T USE THIS WITH PLACEMENT NEW

#pragma warning( push )			// Edit Jonathan Tompson - 31st Jan 2011
#pragma warning( disable:4238 )	// Edit Jonathan Tompson - 31st Jan 2011

//#define CHECK_TESTOBBCOLLISION_SIMD // if defined obbtree will test both collision routines and compare results.

/************************************************************************/
/* Name:		InitOBBTree												*/
/* Description: Create OBB Tree from vertex and mesh data in rbobject	*/
/************************************************************************/
void obbtree::InitOBBTree(rbobjectMesh * rbo, LPCTSTR Filename )
{	
	if(temp == NULL) // Make space for temporary variables
		aligned_new_constructor(temp,DATA_ALIGNMENT,obbtreeTempVar,obbtreeTempVar(numFaces,numVerticies));

	MSG Message;
	Message.message = WM_NULL;

	if(g_UI->GetComboBoxVal(&var_OBBRebuild) != 1)
	{
		bool readOK = false;
		switch(g_UI->GetComboBoxVal(&var_OBBDataType))
		{
		case OBB_COMPRESSED:	
			readOK = readObbFromDiskCompressed(rbo, Filename);
			break;
		case OBB_UNCOMPRESSED:
			readOK = readObbFromDisk(rbo, Filename);
			break;
		default:
			throw std::runtime_error("obbtree::InitOBBTree() - Incorrect obb datafile Type");
		}
		if(readOK) // Try reading the OBB from Disk if we're not forcing a rebuild
		{
			aligned_delete_destructor(temp,obbtreeTempVar);
			return; // If we sucessfully read from disk then just return after freeing temporary space
		}
	}

	// Otherwise build OBBTree from scratch...
	// Initalize root data structure & copy the indices to the root --> Could use memcpy, but this is safer
	obbox * curObbox;
	curObbox = tree->Add_retPtr(); // Get the next allocated block and increase size by 1

	curObbox->numFaces = numFaces;
	curObbox->indices = 0; // First index starts at zero 
	// this->indicies is a scratch pad we will use to shift indicies around amongst sub obbox nodes temporarily
	int * indexBuffer = rbo->meshData->GetIndexBuffer();
	for(DWORD i = 0; i < ( numFaces * 3); i ++)
		this->indices[i] = indexBuffer[i];
	curObbox->parent = -1;								// root has no parent
	curObbox->depth = 0;								// depth starts at zero
	curObbox->isLeaf = false;							// Assume not true for now (could be single triangle though)
	curObbox->t = this;
	
	// Start a flattened recursive procedure using a hand-built stack (LIFO)
	// --> overhead of recursive function calls were very high --> Stack overflow problems for very large models (>1M triangles).
	std::vector<int> obbStack;
	obbStack.reserve( 2*numFaces ); // # leaves is n, n=2^(h-1), h is height, h=log2(n)+1
	obbStack.push_back(0); // Add the root (index is always 0)

	int curObboxNum = 0; 
	g_objectManager->GetHud()->RenderOBBProgressText(curObboxNum+1,2*numFaces-1);
	while(!obbStack.empty() && !g_app->cancel ) // Recursive procedure
	{
		if(PeekMessage(&Message,NULL,0,0,PM_REMOVE)) // PM_REMOVE --> Remove off message queue when read
		{
			// Translate the message and dispatch it to WindowProc()
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		} 
		else 
		{
			if((curObboxNum & 31) == 0 || curObboxNum < 10 ) // Only output to debug window every 32 boxes (or the first 10)
			{
				// Include a progress check
				g_objectManager->GetHud()->RenderOBBProgressText(curObboxNum+1,2*numFaces-1);
			}
			int curIndex = obbStack.back(); // Get the next index and remove it from the stack
			obbStack.pop_back();
			curObbox = tree->GetElem(curIndex); // Get the obbox at the current index

			curObbox->BuildOBBTree(curIndex, &obbStack, rbo->meshData); // Build the current node --> Will add child nodes to the stack if necesary
			
			curObboxNum += 1;
		}
	}

	// We're done with some scrap variables.  Delete them to save space for runtime.
	if(temp != NULL)
	{ temp->~obbtreeTempVar(); _aligned_free(temp); temp = NULL; }

	if(!g_app->cancel)
	{
		// Now save OBB tree to Disk at the specified filename
		switch(g_UI->GetComboBoxVal(&var_OBBDataType))
		{
		case OBB_COMPRESSED:	
			writeObbToDiskCompressed(Filename);
			break;
		case OBB_UNCOMPRESSED:
			writeObbToDisk(Filename);
			break;
		default:
			throw std::runtime_error("obbtree::InitOBBTree() - Incorrect obb datafile Type");
		}
	}
}

/************************************************************************/
/* Name:		writeObbToDisk											*/
/* Description: Write OBB stream to disk								*/
/************************************************************************/
void obbtree::writeObbToDisk( LPCTSTR Filename )
{
	// Get the filename strings we need
	std::wstring obbFilename = Filename; obbFilename.append(L"_OBB.bin");
	std::wstring obbFilename_ind = Filename; obbFilename_ind.append(L"_OBB_index.bin");
	std::wstring obbFilename_CH_ind = Filename; obbFilename_CH_ind.append(L"_OBB_CH_ind.bin");
	std::wstring obbFilename_CH_vert = Filename; obbFilename_CH_vert.append(L"_OBB_CH_vert.bin");
	std::wstring obbFilename_RI = Filename; obbFilename_RI.append(L"_OBB_RI.bin");

	// See if the file exists
	std::ifstream fin(obbFilename.c_str(), std::ios::binary);					// 1. OBB Tree data (with obbox objects)
	if(fin.is_open() && (g_UI->GetComboBoxVal(&var_OBBRebuild) != 1)) // If we can open the file and we're not trying to rebuild then just return.
		return;
	fin.close();

	// THERE ARE 5 STREAMS PER OBJECT:
	std::ofstream fout(obbFilename.c_str(), std::ios::binary);					// 1. OBB Tree data (with obbox objects)
	std::ofstream fout_ind(obbFilename_ind.c_str(), std::ios::binary);			// 2. Index data (int *) that goes along with obbtree
	std::ofstream fout_CH_ind(obbFilename_CH_ind.c_str(), std::ios::binary);	// 3. Convex hull indices data (UINT *) that goes along with obbtree
	std::ofstream fout_CH_vert(obbFilename_CH_vert.c_str(), std::ios::binary);	// 4. Convex hull vertex data (double *) that goes along with obbtree
	std::ofstream fout_RI(obbFilename_RI.c_str(),std:: ios::binary);			// 4. Convex hull render items data

	fout.seekp(0); fout_ind.seekp(0); // Go to the start of the files (default)
	fout_CH_ind.seekp(0); fout_CH_vert.seekp(0); fout_RI.seekp(0);

	UINT size = tree->Size();
	UINT capacity = tree->Capacity();

	fout.write((char *)(&capacity), 1*sizeof(UINT)); // Save the capacity of the array
	fout.write((char *)(tree->GetArray()), capacity*sizeof(obbox)); // Save the whole array
	fout.write((char *)(&size), 1*sizeof(UINT)); // Also save the size of the array (not necessarily same as capacity)
	fout_ind.write((char *)(indices), numIndices*sizeof(int)); // 3 indicies per triangle (or face)

	// Now iterate through the tree array, writing dynamically allocated memory to disk
	// This is slow, but is only done for convex hull rendering (so not many blocks)
	for(UINT i = 0; i < size; i ++)
	{
		obbox * curObbox = tree->GetElem(i);
		if(curObbox->renderitems)
		{
			fout_RI.write((char *)(curObbox->renderitems), sizeof(obboxRenderitems));
			if(curObbox->renderitems->cHullVert)
				fout_CH_vert.write((char *)(curObbox->renderitems->cHullVert), (curObbox->renderitems->cHullVertCount * 3)*sizeof(float)); // 3 points per vertex
			if(curObbox->renderitems->cHullInd)
				fout_CH_ind.write((char *)(curObbox->renderitems->cHullInd), (curObbox->renderitems->cHullIndCount)*sizeof(UINT));
		}
	}

	fout.close();
	fout_ind.close();	
	fout_CH_ind.close();
	fout_CH_vert.close();
	fout_RI.close();

}

/************************************************************************/
/* Name:		writeObbToDiskCompressed								*/
/* Description: Write OBB stream to disk using gzstream					*/
/************************************************************************/
void obbtree::writeObbToDiskCompressed( LPCTSTR Filename )
{
	// Get the filename strings we need
	std::wstring obbFilename = Filename; obbFilename.append(L"_OBB.bin.gz");
	std::wstring obbFilename_ind = Filename; obbFilename_ind.append(L"_OBB_index.bin.gz");
	std::wstring obbFilename_CH_ind = Filename; obbFilename_CH_ind.append(L"_OBB_CH_ind.bin.gz");
	std::wstring obbFilename_CH_vert = Filename; obbFilename_CH_vert.append(L"_OBB_CH_vert.bin.gz");
	std::wstring obbFilename_RI = Filename; obbFilename_RI.append(L"_OBB_RI.bin.gz");

	// See if the file exists
	gzFile fin = gzopen(stringUtil::toNarrowString( obbFilename.c_str(), -1).c_str(), "rb");					// 1. OBB Tree data (with obbox objects)
	if(fin && (g_UI->GetComboBoxVal(&var_OBBRebuild) != 1)) // If we can open the file and we're not trying to rebuild then just return.
	{
		gzclose(fin); return;
	}
	gzclose(fin);

	// THERE ARE 5 STREAMS PER OBJECT:
	gzFile fout = gzopen(stringUtil::toNarrowString( obbFilename.c_str(), -1).c_str(), "wb9"); // 9 is the compression level (9 is best compression)
	if(fout == NULL) 
		throw std::runtime_error("obbtree::writeObbToDiskCompressed() - Could not open gzFile");
	gzFile fout_ind = gzopen(stringUtil::toNarrowString( obbFilename_ind.c_str(), -1).c_str(), "wb9");
	if(fout_ind == NULL) 
		throw std::runtime_error("obbtree::writeObbToDiskCompressed() - Could not open gzFile");
	gzFile fout_CH_ind = gzopen(stringUtil::toNarrowString( obbFilename_CH_ind.c_str(), -1).c_str(), "wb9");
	if(fout_CH_ind == NULL) 
		throw std::runtime_error("obbtree::writeObbToDiskCompressed() - Could not open gzFile");
	gzFile fout_CH_vert = gzopen(stringUtil::toNarrowString( obbFilename_CH_vert.c_str(), -1).c_str(), "wb9");
	if(fout_CH_vert == NULL) 
		throw std::runtime_error("obbtree::writeObbToDiskCompressed() - Could not open gzFile");
	gzFile fout_RI = gzopen(stringUtil::toNarrowString( obbFilename_RI.c_str(), -1).c_str(), "wb9");
	if(fout_RI == NULL) 
		throw std::runtime_error("obbtree::writeObbToDiskCompressed() - Could not open gzFile");

	UINT size = tree->Size();
	UINT capacity = tree->Capacity();

	gzwrite(fout,(char *)(&capacity),  1*sizeof(UINT)); // Save the capacity of the array
	gzwrite(fout,(char *)(tree->GetArray()), capacity*sizeof(obbox)); // Save the whole array
	gzwrite(fout,(char *)(&size),  1*sizeof(UINT)); // Also save the size of the array (not necessarily same as capacity)
	gzwrite(fout_ind,(char *)(indices), numIndices*sizeof(int)); // 3 indicies per triangle (or face)

	// Now iterate through the tree array, writing dynamically allocated memory to disk
	// This is slow, but is only done for convex hull rendering (so not many blocks)
	for(UINT i = 0; i < size; i ++)
	{
		obbox * curObbox = tree->GetElem(i);
		if(curObbox->renderitems)
		{
			gzwrite(fout_RI,(char *)(curObbox->renderitems), sizeof(obboxRenderitems));
			if(curObbox->renderitems->cHullVert)
			{
				gzwrite(fout_CH_vert,(char *)(curObbox->renderitems->cHullVert), (curObbox->renderitems->cHullVertCount * 3)*sizeof(float)); // 3 points per vertex
			}
			if(curObbox->renderitems->cHullInd)
			{
				gzwrite(fout_CH_ind,(char *)(curObbox->renderitems->cHullInd), (curObbox->renderitems->cHullIndCount)*sizeof(UINT));
			}
		}
	}

	gzclose(fout);
	gzclose(fout_ind);
	gzclose(fout_CH_ind);
	gzclose(fout_CH_vert);
	gzclose(fout_RI);
}

/************************************************************************/
/* Name:		readObbFromDisk											*/
/* Description: Read OBB stream from disk								*/
/************************************************************************/
bool obbtree::readObbFromDisk( rbobjectMesh * rbo, LPCTSTR Filename )
{
	// Get the filename strings we need
	std::wstring obbFilename = Filename; obbFilename.append(L"_OBB.bin");
	std::wstring obbFilename_ind = Filename; obbFilename_ind.append(L"_OBB_index.bin");
	std::wstring obbFilename_CH_ind = Filename; obbFilename_CH_ind.append(L"_OBB_CH_ind.bin");
	std::wstring obbFilename_CH_vert = Filename; obbFilename_CH_vert.append(L"_OBB_CH_vert.bin");
	std::wstring obbFilename_RI = Filename; obbFilename_RI.append(L"_OBB_RI.bin");

	// THERE ARE 5 STREAMS: 
	std::ifstream fin(obbFilename.c_str(), std::ios::binary);				// 1. OBB Tree data (with obbox objects)
	if(!fin.is_open()) // If we can't open the file then just return
		return false;
	std::ifstream fin_ind(obbFilename_ind.c_str(), std::ios::binary);		// 2. Index data (int *) that goes along with obbtree
	if(!fin_ind.is_open())
		return false;
	std::ifstream fin_CH_ind(obbFilename_CH_ind.c_str(), std::ios::binary);	// 3. Convex hull indices data (UINT *) that goes along with obbtree
	if(!fin_CH_ind.is_open())
		return false;
	std::ifstream fin_CH_vert(obbFilename_CH_vert.c_str(), std::ios::binary);// 4. Convex hull vertex data (double *) that goes along with obbtree
	if(!fin_CH_vert.is_open())
		return false;
	std::ifstream fin_RI(obbFilename_RI.c_str(),std:: ios::binary);			// 5. Convex hull vertex data (double *) that goes along with obbtree
	if(!fin_RI.is_open())
		return false;
	fin.seekg(0); fin_ind.seekg(0); // Go to the start of the files (default)
	fin_CH_ind.seekg(0); fin_CH_vert.seekg(0); fin_RI.seekg(0);

	// GET THE TREE ARRAY:
	//		1. get filesize and check it makes sense
	struct stat results;
	stat(stringUtil::toNarrowString(obbFilename.c_str(),-1).c_str(), &results); // results.st_size is in bytes
	float num_obbox_elements = ((float)results.st_size-2.0f*(float)sizeof(UINT))/(float)sizeof(obbox); // There is a single integer value at the end of the file

	//      2. Check that the number of elements is a whole number of obbox elements --> Basic error checking for data corruption
	if(fabs(num_obbox_elements - (int)num_obbox_elements) >= 0.000001f)
		throw std::runtime_error("obbtree::readObbFromDisk() - Loaded _OBB file size is not a whole number of obbox elements.  Consider rebuilding obb tree");

	UINT capacity;
	UINT size;

	//		3. Read in the capacity of the array and make sure there is enough room allocated already
	fin.read((char *)(&capacity),sizeof(UINT));
	if(tree->Capacity() < capacity)
		throw std::runtime_error("obbtree::readObbFromDisk() - The stored capacity is now the correct size.");

	//		4. Read in the whole array at once (space has already been allocated)
	if((UINT)num_obbox_elements != capacity)
		throw std::runtime_error("obbtree::readObbFromDisk() - Loaded _OBB file is not the correct size.  Consider rebuilding obb tree");
	fin.read((char *)(tree->GetArray()),results.st_size-2*sizeof(UINT));

	//		5. Read in the size of the array (not necessaryily the capacity)
	fin.read((char *)(&size),sizeof(UINT));
	tree->ForceSize(size);

	// GET THE INDEX ARRAY:
	//		1. get filesize and check it makes sense
	stat(stringUtil::toNarrowString(obbFilename_ind.c_str(),-1).c_str(), &results); // results.st_size is in bytes
	float num_index_elements = (float)results.st_size/(float)sizeof(int);

	//      2. Check that the number of elements is a whole number of int elements
	if(fabs(num_index_elements - (int)num_index_elements) >= 0.000001f)
		throw std::runtime_error("obbtree::readObbFromDisk() - Loaded _OBB_index file size is not a whole number of integer elements.  Consider rebuilding obb tree");

	//		3. Read in the whole array at once (space has already been allocated)
	if((int)num_index_elements != numIndices)
		throw std::runtime_error("obbtree::readObbFromDisk() - Loaded _OBB_index file is not the correct size.  Consider rebuilding obb tree");
	fin_ind.read((char *)(indices),results.st_size);

	// Now iterate through the tree array, reading dynamically allocated buffers from disk and fixing pointers
	for(UINT i = 0; i < size; i ++)
	{
		obbox * curObbox = tree->GetElem(i);
		curObbox->t = this;
		if(curObbox->depth+1 <= (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
		{
			if(curObbox->renderitems)
			{
				curObbox->renderitems = new obboxRenderitems;
				fin_RI.read((char *)(curObbox->renderitems), sizeof(obboxRenderitems));
				if(curObbox->renderitems->cHullVert)
				{
					curObbox->renderitems->cHullVert = new float[curObbox->renderitems->cHullVertCount *3];
					fin_CH_vert.read((char *)(curObbox->renderitems->cHullVert), (curObbox->renderitems->cHullVertCount*3)*sizeof(float)); // 3 indicies per triangle (or face)
				}

				if(curObbox->renderitems->cHullInd) // pointer is not null, but currently it is incorrect
				{
					curObbox->renderitems->cHullInd = new UINT[curObbox->renderitems->cHullIndCount];
					fin_CH_ind.read((char *)(curObbox->renderitems->cHullInd), (curObbox->renderitems->cHullIndCount)*sizeof(UINT)); // 3 indicies per triangle (or face)
				}
			}
			// Try initializing Convex Hull and OBB for rendering if obbox is within depth
			if(curObbox->depth+1 <= (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
			{
				if(curObbox->renderitems) // If we have convex hull data and we want to render it
					curObbox->initConvexHullForRendering(); 
				else
					throw std::runtime_error("obbtree::readObbFromDisk() - Convex hull data on disk doesn't cover desired render depth. Consider Re-building.");
				if(g_UI->GetSetting<bool>(&var_drawOBBAsLines))
					curObbox->initOBBForRenderingLines();
				else
					curObbox->initOBBForRendering();	
			}
		}
		else
		{
			curObbox->renderitems = NULL;
		}
	}

	fin.close();
	fin_ind.close();	
	fin_CH_ind.close();
	fin_CH_vert.close();	
	fin_RI.close();	

	return true;
}

/************************************************************************/
/* Name:		readObbFromDiskCompressed								*/
/* Description: Read OBB stream from disk								*/
/************************************************************************/
bool obbtree::readObbFromDiskCompressed( rbobjectMesh * rbo, LPCTSTR Filename )
{

	// Get the filename strings we need
	std::wstring obbFilename = Filename; obbFilename.append(L"_OBB.bin.gz");
	std::wstring obbFilename_ind = Filename; obbFilename_ind.append(L"_OBB_index.bin.gz");
	std::wstring obbFilename_CH_ind = Filename; obbFilename_CH_ind.append(L"_OBB_CH_ind.bin.gz");
	std::wstring obbFilename_CH_vert = Filename; obbFilename_CH_vert.append(L"_OBB_CH_vert.bin.gz");
	std::wstring obbFilename_RI = Filename; obbFilename_RI.append(L"_OBB_RI.bin.gz");

	// THERE ARE 5 STREAMS: 
	gzFile fin = gzopen(stringUtil::toNarrowString( obbFilename.c_str(), -1).c_str(), "rb");				// 1. OBB Tree data (with obbox objects)			
	if(!fin) // If we can't open the file then just return
		return false;
	gzFile fin_ind = gzopen(stringUtil::toNarrowString( obbFilename_ind.c_str(), -1).c_str(), "rb");		// 2. Index data (int *) that goes along with obbtree	
	if(!fin_ind) // If we can't open the file then just return
		return false;
	gzFile fin_CH_ind = gzopen(stringUtil::toNarrowString( obbFilename_CH_ind.c_str(), -1).c_str(), "rb");	// 3. Convex hull indices data (UINT *) that goes along with obbtree	
	if(!fin_CH_ind) // If we can't open the file then just return
		return false;
	gzFile fin_CH_vert = gzopen(stringUtil::toNarrowString( obbFilename_CH_vert.c_str(), -1).c_str(), "rb");// 4. Convex hull vertex data (double *) that goes along with obbtree
	if(!fin_CH_vert) // If we can't open the file then just return
		return false;
	gzFile fin_RI = gzopen(stringUtil::toNarrowString( obbFilename_RI.c_str(), -1).c_str(), "rb");			// 5. Convex hull vertex data (double *) that goes along with obbtree
	if(!fin_RI) // If we can't open the file then just return
		return false;

	// GET THE TREE ARRAY:
	//		1. get filesize and check it makes sense
			// More difficult on compressed data --> Skip this

	//      2. Check that the number of elements is a whole number of obbox elements --> Basic error checking for data corruption
			// More difficult on compressed data --> Skip this

	UINT capacity;
	UINT size;

	//		3. Read in the capacity of the array and make sure there is enough room allocated already
	int numReadElements = gzread(fin, (char *)(&capacity), sizeof(UINT));
	if(tree->Capacity() < capacity)
		tree->SetCapacity(capacity);

	//		3. Read in the whole array at once (space has already been allocated)
			// More difficult on compressed data --> Skip this
	numReadElements = gzread(fin, (char *)(tree->GetArray()), capacity*sizeof(obbox));

	//		4. Read in the size of the array (not necessaryily the capacity)
	numReadElements = gzread(fin, (char *)(&size), sizeof(UINT));
	tree->ForceSize(size);

	// GET THE INDEX ARRAY:
	//		1. get filesize and check it makes sense
			// More difficult on compressed data --> Skip this

	//      2. Check that the number of elements is a whole number of int elements
			// More difficult on compressed data --> Skip this

	//		3. Read in the whole array at once (space has already been allocated)
	numReadElements = gzread(fin_ind, (char *)(indices), numIndices*sizeof(int));

	// Now iterate through the tree array, reading dynamically allocated buffers from disk and fixing pointers
	for(UINT i = 0; i < size; i ++)
	{
		obbox * curObbox = tree->GetElem(i);
		curObbox->t = this;
		if(curObbox->depth+1 <= (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
		{
			if(curObbox->renderitems)
			{
				curObbox->renderitems = new obboxRenderitems;
				numReadElements = gzread(fin_RI, (char *)(curObbox->renderitems), sizeof(obboxRenderitems));
				if(curObbox->renderitems->cHullVert)
				{
					curObbox->renderitems->cHullVert = new float[curObbox->renderitems->cHullVertCount *3];
					numReadElements = gzread(fin_CH_vert,(char *)(curObbox->renderitems->cHullVert), (curObbox->renderitems->cHullVertCount*3)*sizeof(float)); // 3 indicies per triangle (or face)
				}

				if(curObbox->renderitems->cHullInd) // pointer is not null, but currently it is incorrect
				{
					curObbox->renderitems->cHullInd = new UINT[curObbox->renderitems->cHullIndCount];
					numReadElements = gzread(fin_CH_ind,(char *)(curObbox->renderitems->cHullInd), (curObbox->renderitems->cHullIndCount)*sizeof(UINT)); // 3 indicies per triangle (or face)
				}
			}
			// Try initializing Convex Hull and OBB for rendering if obbox is within depth
			if(curObbox->depth+1 <= (int)g_UI->GetComboBoxVal(&var_OBBRenderDepth))
			{
				if(curObbox->renderitems) // If we have convex hull data and we want to render it
					curObbox->initConvexHullForRendering(); 
				else
					throw std::runtime_error("obbtree::readObbFromDiskCompressed() - Convex hull data on disk doesn't cover desired render depth. Consider Re-building.");
				if(g_UI->GetSetting<bool>(&var_drawOBBAsLines))
					curObbox->initOBBForRenderingLines();
				else
					curObbox->initOBBForRendering();	
			}
		}
		else
		{
			curObbox->renderitems = NULL;
		}
	}

	gzclose(fin);
	gzclose(fin_ind);
	gzclose(fin_CH_ind);
	gzclose(fin_CH_vert);
	gzclose(fin_RI);

	return true;
}

#pragma warning( pop )			// Edit Jonathan Tompson - 31st Jan 2011