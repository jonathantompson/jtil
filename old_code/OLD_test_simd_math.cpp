#include <iostream>		// iostream header: allows input/output functions
#include <string>		// string functions (strcpy, etc)
#include <stdio.h>
#include <math.h>
#include <limits>
#include "utils_and_misc_classes\SIMD_helpers\SIMDFuncs.h"
#include "utils_and_misc_classes\SIMD_helpers\dataAlign.h"
#include "utils_and_misc_classes\SIMD_helpers\D3DXVECTOR3A16.h"
#include "utils_and_misc_classes\math\double3x3.h"
#include "physics\obbtree_related\obbox.h"
#include "physics\objects\rbobjectMesh.h"
#include "clk\clk.h"
#include "utils_and_misc_classes\SIMD_helpers\cpuid.h"
#include "utils_and_misc_classes\stringUtil.h"

#include	<d3d11.h>				// main Direct3D header
#include	<d3dx11.h>				// Direct3D Extensions header

#define DATA_ALIGNMENT 16
#define NUM_CALCS 100000

#define M_PI 3.14159265f

void PRINTMAT(D3DXMATRIXA16 * m, const wchar_t * y, wchar_t * buf )				
{			
	swprintf(buf,255,L"\n\t\tMatrix %s:",y); OutputDebugString(buf);
	swprintf(buf,255,L"\n\t\t\t\t[ %12.4f %12.4f %12.4f %12.4f ]",m->_11,m->_12,m->_13,m->_14); OutputDebugString(buf);
	swprintf(buf,255,L"\n\t\t\t\t[ %12.4f %12.4f %12.4f %12.4f ]",m->_21,m->_22,m->_23,m->_24); OutputDebugString(buf);
	swprintf(buf,255,L"\n\t\t\t\t[ %12.4f %12.4f %12.4f %12.4f ]",m->_31,m->_32,m->_33,m->_34); OutputDebugString(buf);
	swprintf(buf,255,L"\n\t\t\t\t[ %12.4f %12.4f %12.4f %12.4f ]\n",m->_41,m->_42,m->_43,m->_44); OutputDebugString(buf);
}
void PRINTVEC(D3DXVECTOR3A16 * v, const wchar_t * y, wchar_t * buf)							
{		
	swprintf(buf,255,L"\n\t\tVector %s:",y); OutputDebugString(buf);
	swprintf(buf,255,L"\n\t\t\t\t[ %12.4f %12.4f %12.4f ]\n",v->x,v->y,v->z); OutputDebugString(buf);	
}

void PRINTVEC(D3DXVECTOR3 * v, const wchar_t * y, wchar_t * buf)							
{		
	swprintf(buf,255,L"\n\t\tVector %s:",y); OutputDebugString(buf);
	swprintf(buf,255,L"\n\t\t\t\t[ %12.4f %12.4f %12.4f ]\n",v->x,v->y,v->z); OutputDebugString(buf);	
}

void CLEARVEC(D3DXVECTOR3 * v)
{ v->x = 0; v->y = 0; v->z = 0; }
void CLEARVEC(float v[4])
{ v[0] = 0; v[1] = 0; v[2] = 0; }
//{ v->x = 0; v->y = 0; v->z = 0; }
void CLEARVEC(D3DXVECTOR3A16 * v)
{ v->x = 0; v->y = 0; v->z = 0; v->w = 0; }
void CLEARMAT(D3DXMATRIXA16 * m)
{ m->_11 = 0; m->_12 = 0; m->_13 = 0; m->_14 = 0;
  m->_21 = 0; m->_22 = 0; m->_23 = 0; m->_24 = 0;
  m->_31 = 0; m->_32 = 0; m->_33 = 0; m->_34 = 0;
  m->_41 = 0; m->_42 = 0; m->_43 = 0; m->_44 = 0; }

#pragma optimize( "", off ) // DON'T OPTIMIZE OUR MAIN FUNCTION --> IT WILL REMOVE REDUNDANT FOR LOOP 
							 // COMPUTATION SO WE CAN'T TEST RELATIVE SPEED
int main () 
{ 
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	wchar_t buf[256];
	D3DXVECTOR3A16 * vec0 = NULL;
	D3DXVECTOR3A16 * vec1 = NULL;			
	D3DXVECTOR3A16 * vecT = NULL;			// Temporary space for a vector
	D3DXVECTOR3A16 * vecR = NULL;			// Result Vector
	D3DXMATRIXA16 * mat0 = NULL;
	D3DXMATRIXA16 * mat1 = NULL;
	D3DXMATRIXA16 * matT = NULL;		// Temporary space for a matrix
	D3DXMATRIXA16 * matR = NULL;		// Result Matrix
	__m128 pm1, pm2, pm3, pm4, pm5;
	obbox * obbA; obbox * obbB;
	rbobjectMesh * rboA; rbobjectMesh * rboB;

	try
	{

		std::cout << "NOTE: ALL OUTPUT IS IN THE VISUAL STUDIO DEBUG OUTPUT WINDOW\n";
		swprintf(buf,255,L"\nTEST SIMD MATH\n"); OutputDebugString(buf);

		cpuid::_p_info info;
		cpuid::get_cpuid(&info);
		cpuid::print_cpuid_info(&info);
		if(!(cpuid::check_sse(&info) && check_sse2(&info)))
		{
			std::wstring err = L"CPU does not support SSE & SSE2-SIMD instructions! \nPlease run on Pentium III CPU or newer. \nSIMD will be disabled at performance cost.";
			MessageBox(NULL, err.c_str(), L"Warning", MB_OK | MB_ICONERROR);
		}

		swprintf(buf,255,L"\n\tAllocating space and checking for allignment:\n\n"); OutputDebugString(buf);
		clk g_clk;
		float start, finish;

		D3DXVECTOR3 _vec0dx, _vec1dx, _vecTdx, _vecRdx;
		D3DXVECTOR3 *_vec0 = &_vec0dx; D3DXVECTOR3 * _vec1 = &_vec1dx; D3DXVECTOR3 *_vecT = &_vecTdx; D3DXVECTOR3 * _vecR = &_vecRdx;

		// Call _aligned_malloc
		aligned_new_constructor(vec0,DATA_ALIGNMENT,D3DXVECTOR3A16,D3DXVECTOR3A16());
		aligned_new_constructor(vec1,DATA_ALIGNMENT,D3DXVECTOR3A16,D3DXVECTOR3A16());
		aligned_new_constructor(vecT,DATA_ALIGNMENT,D3DXVECTOR3A16,D3DXVECTOR3A16());
		aligned_new_constructor(vecR,DATA_ALIGNMENT,D3DXVECTOR3A16,D3DXVECTOR3A16());
		aligned_new_constructor(obbA,DATA_ALIGNMENT,obbox,obbox()); 
		aligned_new_constructor(obbB,DATA_ALIGNMENT,obbox,obbox());
		aligned_new_constructor(rboA,DATA_ALIGNMENT,rbobjectMesh,rbobjectMesh()); 
		aligned_new_constructor(rboB,DATA_ALIGNMENT,rbobjectMesh,rbobjectMesh());
		aligned_new(mat0,DATA_ALIGNMENT,D3DXMATRIXA16);
		aligned_new(mat1,DATA_ALIGNMENT,D3DXMATRIXA16);
		aligned_new(matT,DATA_ALIGNMENT,D3DXMATRIXA16);
		aligned_new(matR,DATA_ALIGNMENT,D3DXMATRIXA16);

		// Check for memory alignment
		std::cout << "Checking for memory alignment:\n";
		dataAlign::CheckAligned(vec0, DATA_ALIGNMENT, L"vec0");
		dataAlign::CheckAligned(vec1, DATA_ALIGNMENT, L"vec1");
		dataAlign::CheckAligned(vecT, DATA_ALIGNMENT, L"vecT");
		dataAlign::CheckAligned(vecR, DATA_ALIGNMENT, L"vecR");
		dataAlign::CheckAligned(mat0->m, DATA_ALIGNMENT, L"mat0->m");
		dataAlign::CheckAligned(mat1->m, DATA_ALIGNMENT, L"mat1->m");
		dataAlign::CheckAligned(matT->m, DATA_ALIGNMENT, L"matT->m");
		dataAlign::CheckAligned(matR->m, DATA_ALIGNMENT, L"matR->m");
		dataAlign::CheckAligned(&rboA->x, DATA_ALIGNMENT, L"&rboA->x");
		dataAlign::CheckAligned(&rboA->R, DATA_ALIGNMENT, L"&rboA->R");
		dataAlign::CheckAligned(&rboA->scale, DATA_ALIGNMENT, L"&rboA->scale");
		dataAlign::CheckAligned(&rboB->x, DATA_ALIGNMENT, L"&rboB->x");
		dataAlign::CheckAligned(&rboB->R, DATA_ALIGNMENT, L"&rboB->R");
		dataAlign::CheckAligned(&rboB->scale, DATA_ALIGNMENT, L"&rboB->scale");
		dataAlign::CheckAligned(&obbA->boxDimension, DATA_ALIGNMENT, L"&obbA->boxDimension");
		dataAlign::CheckAligned(&obbA->orientMatrix, DATA_ALIGNMENT, L"&obbA->orientMatrix");
		dataAlign::CheckAligned(&obbB->boxDimension, DATA_ALIGNMENT, L"&obbB->boxDimension");
		dataAlign::CheckAligned(&obbB->orientMatrix, DATA_ALIGNMENT, L"&obbB->orientMatrix");

		// Fill up variables with data
		vec0->x = 1;	vec0->y = 2;	vec0->z = 3;						_vec0->x = 1; _vec0->y = 2; _vec0->z = 3; 
		vec1->x = 4;	vec1->y = 5;	vec1->z = 6;						_vec1->x = 4; _vec1->y = 5; _vec1->z = 6; 
		mat0->_11 = 7;	mat0->_12 = 8;	mat0->_13 = 9;	mat0->_14 = 0;
		mat0->_21 = 10;	mat0->_22 = 11;	mat0->_23 = 12;	mat0->_24 = 0;
		mat0->_31 = 13;	mat0->_32 = 14;	mat0->_33 = 15;	mat0->_34 = 0;
		mat0->_41 = 0;	mat0->_42 = 0;	mat0->_43 = 0;	mat0->_44 = 1;
		mat1->_11 = 16;	mat1->_12 = 17;	mat1->_13 = 18;	mat1->_14 = 0;
		mat1->_21 = 19;	mat1->_22 = 20;	mat1->_23 = 21;	mat1->_24 = 0;
		mat1->_31 = 22;	mat1->_32 = 23;	mat1->_33 = 24;	mat1->_34 = 0;
		mat1->_41 = 0;	mat1->_42 = 0;	mat1->_43 = 0;	mat1->_44 = 1;
		swprintf(buf,255,L"\n-\tStarting Values:\n"); OutputDebugString(buf);
		PRINTVEC(vec0,L"vec0",buf);
		PRINTVEC(vec1,L"vec1",buf);
		PRINTMAT(mat0,L"mat0",buf);
		PRINTMAT(mat1,L"mat1",buf);

		// Perform some Operations

		// VECTOR-VECTOR
		swprintf(buf,255,L"\n-\tPerforming %d series of DirectX vector operations\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(_vecT); CLEARVEC(_vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			*_vecR = *_vec0 + *_vec1;		// <5,			7,			9>
			*_vecT =* _vec1 + *_vecR;		// <9,			12,			15>
			*_vecR = *_vecR - *_vecT;		// <-4,			-5,			-6>
			*_vecT = *_vecT - *_vecR;		// <13,			17,			21>
			_vecR->x = _vecR->x * _vecT->x; _vecR->y = _vecR->y * _vecT->y; _vecR->z = _vecR->z * _vecT->z;		// <-52,		-85,		-126>
			_vecT->x = _vecT->x * _vecR->x; _vecT->y = _vecT->y * _vecR->y; _vecT->z = _vecT->z * _vecR->z;		// <-676,		-1445,		-2646>
			_vecR->x = _vecR->x / _vecT->x; _vecR->y = _vecR->y / _vecT->y; _vecR->z = _vecR->z / _vecT->z;		// <0.07692,	0.05882,	0.04761>
			_vecT->x = _vecT->x / _vecR->x; _vecT->y = _vecT->y / _vecR->y; _vecT->z = _vecT->z / _vecR->z;		// <-8788.35,	-24566.47,	-55576.55>
		}
		
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Calculation Results:"); OutputDebugString(buf);
		PRINTVEC(_vecT,L"_vecT",buf);
		PRINTVEC(_vecR,L"_vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-VECTOR SIMD
		swprintf(buf,255,L"\n-\tPerforming %d series of SSE vector operations\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(vecT); CLEARVEC(vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_VECTORA16(vec0,xmm0);	// xmm0 = vec0			= <1,			2,			3>
			LOAD_VECTORA16(vec1,xmm1);	// xmm1 = vec1			= <4,			5,			6>
			ADD_VECTORS(xmm0,xmm1);     // xmm0 = xmm0 + xmm1	= <5,			7,			9>
			ADD_VECTORS(xmm1,xmm0);     // xmm1 = xmm1 + xmm0	= <9,			12,			15>
			SUB_VECTORS(xmm0,xmm1);     // xmm0 = xmm0 - xmm1	= <-4,			-5,			-6>
			SUB_VECTORS(xmm1,xmm0);     // xmm1 = xmm1 - xmm0	= <13,			17,			21>
			MUL_VECTORS(xmm0,xmm1);     // xmm0 = xmm0 * xmm1	= <-52,			-85,		-126>
			MUL_VECTORS(xmm1,xmm0);     // xmm1 = xmm1 * xmm0	= <-676,		-1445,		-2646>
			DIV_VECTORS(xmm0,xmm1);     // xmm0 = xmm0 / xmm1	= <0.07692,		0.05882,	0.04761>
			DIV_VECTORS(xmm1,xmm0);     // xmm1 = xmm1 / xmm0	= <-8788.35,	-24566.47,	-55576.55>
		}
		STORE_VECTORA16(vecR,xmm0);	// vecR = xmm0			= <0.07692,		0.07423,	0.04761>
		STORE_VECTORA16(vecT,xmm1); // vecT = xmm1			= <-8788.35,	-24566.47,	-55576.55>	
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Calculation Results:"); OutputDebugString(buf);
		PRINTVEC(vecT,L"vecT",buf);
		PRINTVEC(vecR,L"vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-VECTOR - DOT PRODUCT
		swprintf(buf,255,L"\n-\tPerforming %d DirectX DOT PRODUCTS\n",NUM_CALCS); OutputDebugString(buf);
		start = g_clk.GetTime();
		float d;
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			D3DXVec3Dot( _vec0, _vec1 );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value: dot(vec0,vec1) = %11.4f",D3DXVec3Dot( _vec0, _vec1 )); OutputDebugString(buf);
		swprintf(buf,255,L"\n\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-VECTOR SIMD - DOT PRODUCT
		swprintf(buf,255,L"\n-\tPerforming %d SSE DOT PRODUCTS\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(vecT); CLEARVEC(vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_VECTORA16(vec0,xmm0);		// xmm0 = vec0			= <1,			2,			3>
			LOAD_VECTORA16(vec1,xmm1);		// xmm1 = vec1			= <4,			5,			6>
			DOT_VECTOR3(xmm0,xmm1);			// xmm0 = DOT(xmm0,xmm1)= <32,			32,			32>
		}
		STORE_VECTORA16(vecR,xmm0);		// vecR = xmm0			= <32,			32,			32>
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value: dot(vec0,vec1) = %11.4f",vecR->x); OutputDebugString(buf);
		swprintf(buf,255,L"\n\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-VECTOR SIMD SSE3 - DOT PRODUCT
		swprintf(buf,255,L"\n-\tPerforming %d SSE3 DOT PRODUCTS\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(vecT); CLEARVEC(vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_VECTORA16(vec0,xmm0);		// xmm0 = vec0			= <1,			2,			3>
			LOAD_VECTORA16(vec1,xmm1);		// xmm1 = vec1			= <4,			5,			6>
			DOT_VECTOR3_SSE3(xmm0,xmm1);	// xmm0 = DOT(xmm0,xmm1)= <32,			32,			32>
			//DOT_VECTOR3_SSE4P1(xmm0,xmm1);	// xmm0 = DOT(xmm0,xmm1)= <32,			32,			32>
		}
		STORE_VECTORA16(vecR,xmm0);		// vecR = xmm0			= <32,			32,			32>
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value: dot(vec0,vec1) = %11.4f",vecR->x); OutputDebugString(buf);
		swprintf(buf,255,L"\n\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-VECTOR - CROSS PRODUCT
		swprintf(buf,255,L"\n-\tPerforming %d DirectX CROSS PRODUCTS\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(_vecT); CLEARVEC(_vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			D3DXVec3Cross( _vecR, _vec0, _vec1 );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTVEC(_vecR,L"_vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-VECTOR SIMD - CROSS PRODUCT
		swprintf(buf,255,L"\n-\tPerforming %d SSE CROSS PRODUCTS\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(vecT); CLEARVEC(vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_VECTORA16(vec0,xmm0);			// xmm0 = vec0			= <1,			2,			3>
			LOAD_VECTORA16(vec1,xmm1);			// xmm1 = vec1			= <4,			5,			6>
			CROSS_VECTOR3(xmm0,xmm1,xmm2,xmm3);	// xmm0 = CROSS(xmm0,xmm1)= <32,			32,			32>
		}
		STORE_VECTORA16(vecR,xmm0);				// vecR = xmm0			= <32,			32,			32>
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTVEC(vecR,L"vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MARIX-MATRIX
		swprintf(buf,255,L"\n-\tPerforming %d series of DirectX matrix operations\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); CLEARMAT(matT);
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			*matR = *mat0 + *mat1;
			*matT = *mat1 + *matR;
			*matR = *matR - *matT;
			*matT = *matT - *matR;
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Calculation Results:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		PRINTMAT(matT,L"matT",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MARIX-MATRIX SIMD
		swprintf(buf,255,L"\n-\tPerforming %d series of SSE matrix operations\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); CLEARMAT(matT);
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_MATRIXA16(mat0,xmm0,xmm1,xmm2,xmm3);
			LOAD_MATRIXA16(mat1,xmm4,xmm5,xmm6,xmm7);
			ADD_MATRICES(xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7);
			ADD_MATRICES(xmm4,xmm5,xmm6,xmm7,xmm0,xmm1,xmm2,xmm3);
			SUB_MATRICES(xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7);
			SUB_MATRICES(xmm4,xmm5,xmm6,xmm7,xmm0,xmm1,xmm2,xmm3);
		}
		STORE_MATRIXA16(matR,xmm0,xmm1,xmm2,xmm3);
		STORE_MATRIXA16(matT,xmm4,xmm5,xmm6,xmm7);
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Calculation Results:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		PRINTMAT(matT,L"matT",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-MATRIX - VECTOR-MATRIX MULTIPLY
		swprintf(buf,255,L"\n-\tPerforming %d DirectX (D3DXVec3TransformCoord) VECTOR-MATRIX MULTIPLIES\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(_vecT); CLEARVEC(_vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			D3DXVec3TransformCoord(_vecR, _vec0, mat0);
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTVEC(_vecR,L"_vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-MATRIX - VECTOR-MATRIX MULTIPLY Jonno
		swprintf(buf,255,L"\n-\tPerforming %d Jonno (MatrixMult_ATran_B) VECTOR-MATRIX MULTIPLIES\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(_vecT); CLEARVEC(_vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			MatrixMult_ATran_B(_vecR, mat0, _vec0 );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTVEC(_vecR,L"_vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-MATRIX - VECTOR-MATRIX MULTIPLY SIMD
		swprintf(buf,255,L"\n-\tPerforming %d SSE VECTOR-MATRIX MULTIPLIES\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(vecT); CLEARVEC(vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_VECTORA16(vec0,xmm0);			
			LOAD_MATRIXA16(mat0,xmm4,xmm5,xmm6,xmm7);
			MULT_VECTOR4A16MATRIXA16(xmm0,xmm1,xmm2,xmm4,xmm5,xmm6,xmm7); // Wont destroy matrix
			// MULT_VECTOR4A16MATRIXA16_SSE4(xmm0,xmm4,xmm5,xmm6,xmm7); // Will destroy matrix
		}
		STORE_VECTORA16(vecR,xmm0);	
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTVEC(vecR,L"vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-MATRIX SIMD - VECTOR3-MATRIX MULTIPLY
		swprintf(buf,255,L"\n-\tPerforming %d SSE VECTOR3-MATRIX3x3 MULTIPLIES\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(vecT); CLEARVEC(vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_VECTORA16(vec0,xmm0);			
			LOAD_MATRIXA16(mat0,xmm4,xmm5,xmm6,xmm7);
			MULT_VECTOR3A16MATRIXA16(xmm0,xmm1,xmm2,xmm4,xmm5,xmm6,xmm7); // Wont destroy matrix
		}
		STORE_VECTORA16(vecR,xmm0);	
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTVEC(vecR,L"vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-MATRIX - VECTOR-MATRIX TRANSPOSE MULTIPLY
		swprintf(buf,255,L"\n-\tPerforming %d DirectX (D3DXMatrixTranspose + D3DXVec3TransformCoord) VECTOR-MATRIX TRANSPOSE MULTIPLIES\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(_vecT); CLEARVEC(_vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			D3DXMatrixTranspose(matR, mat0);
			D3DXVec3TransformCoord(_vecR, _vec0, matR);
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTVEC(_vecR,L"_vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-MATRIX - VECTOR-MATRIX TRANSPOSE MULTIPLY Jonno
		swprintf(buf,255,L"\n-\tPerforming %d Jonno (MatrixMult) VECTOR-MATRIX TRANSPOSE MULTIPLIES\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(_vecT); CLEARVEC(_vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			MatrixMult(_vecR, mat0, _vec0 );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTVEC(_vecR,L"_vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// VECTOR-MATRIX SIMD - VECTOR3-MATRIX TRANSPOSE MULTIPLY
		swprintf(buf,255,L"\n-\tPerforming %d SSE VECTOR3-MATRIX3x3 TRANSPOSE MULTIPLIES\n",NUM_CALCS); OutputDebugString(buf);
		CLEARVEC(vecT); CLEARVEC(vecR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_VECTORA16(vec0,xmm0);			
			LOAD_MATRIXA16(mat0,xmm4,xmm5,xmm6,xmm7);
			MULT_VECTOR3A16MATRIXA16_TRAN(xmm0,xmm1,xmm2,xmm3,xmm4,xmm5,xmm6,xmm7); 
			//LOAD_VECTORA16(vec0,xmm0);			
			//LOAD_MATRIXA16(mat0,xmm4,xmm5,xmm6,xmm7);
			//MULT_VECTOR3A16MATRIXA16_TRAN_OLD(xmm0,xmm1,xmm2,xmm4,xmm5,xmm6,xmm7); // Will destroy matrix
		}
		STORE_VECTORA16(vecR,xmm0);	
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTVEC(vecR,L"vecR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - TRANSPOSE
		swprintf(buf,255,L"\n-\tPerforming %d DirectX MATRIX TRANSPOSES\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR);
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			D3DXMatrixTranspose(matR, mat0);
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - SSE TRANSPOSE
		swprintf(buf,255,L"\n-\tPerforming %d SSE MATRIX TRANSPOSES\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_MATRIXA16(mat0,xmm4,xmm5,xmm6,xmm7);
			TRAN_MATRIXA16(xmm4,xmm5,xmm6,xmm7,xmm0);
		}
		STORE_MATRIXA16(matR,xmm4,xmm5,xmm6,xmm7);	
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - ABS
		swprintf(buf,255,L"\n-\tPerforming %d Jonno's MATRIX ABS + EPSILON (MatrixAbsFastPlusEpsilon)'s\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			MatrixAbsFastPlusEpsilon(matR, mat0);
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - SSE ABS
		swprintf(buf,255,L"\n-\tPerforming %d SSE MATRIX ABS + EPSILON\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR);
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_MATRIXA16(mat0,xmm4,xmm5,xmm6,xmm7);
			ABS_PLUS_EPSILONBIG_MATRIXA16(xmm4,xmm5,xmm6,xmm7,xmm0);
		}
		STORE_MATRIXA16(matR,xmm4,xmm5,xmm6,xmm7);	
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - MATRIX MATRIX Multiply
		swprintf(buf,255,L"\n-\tPerforming %d DirectX Matrix-Matrix Multiplies (D3DXMatrixMultiply)'s\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			D3DXMatrixMultiply(matR, mat0, mat1);
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - MATRIX MATRIX Multiply Jonno
		swprintf(buf,255,L"\n-\tPerforming %d Jonno Matrix-Matrix Multiplies (MatrixMult)'s\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			MatrixMult( matR, mat0, mat1 );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - MATRIX MATRIX Multiply SSE
		swprintf(buf,255,L"\n-\tPerforming %d SSE Matrix-Matrix Multiplies\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_MATRIXA16(mat1,xmm4,xmm5,xmm6,xmm7);
			MULT_MATRIXA16MATRIXA16(matR,mat0,xmm0,xmm1,xmm2,xmm4,xmm5,xmm6,xmm7);
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - MATRIX MATRIX Multiply SSE 3x3
		swprintf(buf,255,L"\n-\tPerforming %d SSE Matrix3x3-Matrix3x3 Multiplies\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_MATRIXA16(mat1,xmm4,xmm5,xmm6,xmm7);
			MULT_MATRIXA16MATRIXA16_3x3(matR,mat0,xmm0,xmm1,xmm2,xmm4,xmm5,xmm6,xmm7);
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - MATRIX TRANSPOSE MATRIX Multiply DirectX
		swprintf(buf,255,L"\n-\tPerforming %d DirectX Matrix-Transpose-Matrix Multiplies (D3DXMatrixTranspose + D3DXVec3TransformCoord)'s\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			D3DXMatrixTranspose(matT, mat0);
			D3DXMatrixMultiply(matR, matT, mat1);
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - MATRIX TRANSPOSE MATRIX Multiply Jonno
		swprintf(buf,255,L"\n-\tPerforming %d Jonno Matrix-Transpose-Matrix Multiplies (MatrixMult_ATran_B)'s\n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			MatrixMult_ATran_B( matR, mat0, mat1 );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// MATRIX - MATRIX TRANSPOSE MATRIX Multiply SSE
		swprintf(buf,255,L"\n-\tPerforming %d SSE Matrix-Transpose-Matrix Multiplies \n",NUM_CALCS); OutputDebugString(buf);
		CLEARMAT(matR); 
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			LOAD_MATRIXA16(mat0,xmm4,xmm5,xmm6,xmm7);
			MULT_MATRIXA16MATRIXA16_3x3(matR,mat1,xmm0,xmm1,xmm2,xmm4,xmm5,xmm6,xmm7);
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value:"); OutputDebugString(buf);
		PRINTMAT(matR,L"matR",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);


		//*****************************************************//
		//********************* OBB TESTS *********************//
		//*****************************************************//
		// CORRECT ANSWER IS AT 'root\prenderer\source\OBBTests_correct_answer.jpg'

		obbA->isLeaf = false; obbB->isLeaf = false;
		// Fill the obbox elements up with data
		obbA->boxDimension.x = 0.3f; obbA->boxDimension.y = 0.5f; obbA->boxDimension.z = 0.7f;
		obbB->boxDimension.x = 0.11f;	obbB->boxDimension.y = 0.13f;	obbB->boxDimension.z = 0.17f;
		obbA->boxCenterObjectCoord.x = 0.19f; obbA->boxCenterObjectCoord.y = 0.23f; obbA->boxCenterObjectCoord.z = 0.29f; 
		obbB->boxCenterObjectCoord.x = 0.31f; obbB->boxCenterObjectCoord.y = 0.37f; obbB->boxCenterObjectCoord.z = 0.41f; 
		D3DXVECTOR3 rotAxisobbA(0.43f,0.47f,0.53f); D3DXMatrixRotationAxis(&obbA->orientMatrix,&rotAxisobbA,M_PI*0.71f);
		D3DXVECTOR3 rotAxisobbB(0.59f,0.61f,0.67f); D3DXMatrixRotationAxis(&obbB->orientMatrix,&rotAxisobbB,M_PI*2.73f);
		// Fill the rbobjects with data
		rboA->x.x = 10*vec0->x; rboA->x.y = 10*vec0->y; rboA->x.z = 10*vec0->z;
		rboB->x.x = 10*vec1->x; rboB->x.y = 10*vec1->y; rboB->x.z = 10*vec1->z;
		rboA->scale = 1.1f; rboB->scale = 0.9f; 
		D3DXVECTOR3 rotAxisA(0.5f,0.1f,0.9f); D3DXMatrixRotationAxis(&rboA->R,&rotAxisA,M_PI/0.5613f);
		D3DXVECTOR3 rotAxisB(0.7f,0.4f,0.8f); D3DXMatrixRotationAxis(&rboB->R,&rotAxisB,M_PI*1.6423f);
		int result;
		
		// OBBOX - OBBOX-OBBOX COLLISION TEST OLD VERSION
		CLEARMAT(&obbox::AbsR);CLEARMAT(&obbox::R);CLEARVEC(&obbox::t_);CLEARVEC(obbox::a_OBBDim);CLEARVEC(obbox::b_OBBDim);
		swprintf(buf,255,L"\n-\tPerforming %d obbox::TestOBBCollision_original() tests\n",NUM_CALCS); OutputDebugString(buf);
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			//result = obbox::TestOBBCollision_original( obbA, rboA, obbA, rboA );    // worst case is that they overlap
			result = obbox::TestOBBCollision_original( obbA, rboA, obbB, rboB );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value: result = %d",result); OutputDebugString(buf);
		PRINTMAT(&obbox::AbsR,L"obbox::AbsR",buf);
		PRINTMAT(&obbox::R,L"obbox::R",buf);
		PRINTVEC(&obbox::t_,L"obbox::t_",buf);
		PRINTVEC(&D3DXVECTOR3(obbox::a_OBBDim[0],obbox::a_OBBDim[1],obbox::a_OBBDim[2]),L"obbox::a_OBBDim",buf);
		PRINTVEC(&D3DXVECTOR3(obbox::b_OBBDim[0],obbox::b_OBBDim[1],obbox::b_OBBDim[2]),L"obbox::b_OBBDim",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// Now storing Orient Matrix in transpose form:
		D3DXMATRIXA16 temp;
		D3DXMatrixTranspose(&temp,&obbA->orientMatrix); CopyMat(&obbA->orientMatrix,&temp);
		D3DXMatrixTranspose(&temp,&obbB->orientMatrix); CopyMat(&obbB->orientMatrix,&temp);
		
		// OBBOX - OBBOX-OBBOX COLLISION TEST NEW VERSION
		CLEARMAT(&obbox::AbsR);CLEARMAT(&obbox::R);CLEARVEC(&obbox::t_);CLEARVEC(obbox::a_OBBDim);CLEARVEC(obbox::b_OBBDim);
		swprintf(buf,255,L"\n-\tPerforming %d obbox::TestOBBCollision() tests\n",NUM_CALCS); OutputDebugString(buf);
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			//result = obbox::TestOBBCollision( obbA, rboA, obbA, rboA );    // worst case is that they overlap
			result = obbox::TestOBBCollision( obbA, rboA, obbB, rboB );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value: result = %d",result); OutputDebugString(buf);
		PRINTMAT(&obbox::AbsR,L"obbox::AbsR",buf);
		PRINTMAT(&obbox::R,L"obbox::R",buf);
		PRINTVEC(&obbox::t_,L"obbox::t_",buf);
		PRINTVEC(&D3DXVECTOR3(obbox::a_OBBDim[0],obbox::a_OBBDim[1],obbox::a_OBBDim[2]),L"obbox::a_OBBDim",buf);
		PRINTVEC(&D3DXVECTOR3(obbox::b_OBBDim[0],obbox::b_OBBDim[1],obbox::b_OBBDim[2]),L"obbox::b_OBBDim",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// OBBOX - OBBOX-OBBOX COLLISION TEST DirectX
		CLEARMAT(&obbox::AbsR);CLEARMAT(&obbox::R);CLEARVEC(&obbox::t_);CLEARVEC(obbox::a_OBBDim);CLEARVEC(obbox::b_OBBDim);
		swprintf(buf,255,L"\n-\tPerforming %d obbox::TestOBBCollision_DirectX() tests\n",NUM_CALCS); OutputDebugString(buf);
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			//result = obbox::TestOBBCollision_DirectX( obbA, rboA, obbA, rboA );    // worst case is that they overlap
			result = obbox::TestOBBCollision_DirectX( obbA, rboA, obbB, rboB );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value: result = %d",result); OutputDebugString(buf);
		PRINTMAT(&obbox::AbsR,L"obbox::AbsR",buf);
		PRINTMAT(&obbox::R,L"obbox::R",buf);
		PRINTVEC(&obbox::t_,L"obbox::t_",buf);
		PRINTVEC(&D3DXVECTOR3(obbox::a_OBBDim[0],obbox::a_OBBDim[1],obbox::a_OBBDim[2]),L"obbox::a_OBBDim",buf);
		PRINTVEC(&D3DXVECTOR3(obbox::b_OBBDim[0],obbox::b_OBBDim[1],obbox::b_OBBDim[2]),L"obbox::b_OBBDim",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);

		// OBBOX - OBBOX-OBBOX COLLISION TEST SIMD
		CLEARMAT(&obbox::AbsR);CLEARMAT(&obbox::R);CLEARVEC(&obbox::t_);CLEARVEC(obbox::a_OBBDim);CLEARVEC(obbox::b_OBBDim);
		swprintf(buf,255,L"\n-\tPerforming %d obbox::TestOBBCollision_SIMD() tests\n",NUM_CALCS); OutputDebugString(buf);
		start = g_clk.GetTime();
		for(int i = 0; i < NUM_CALCS; i ++)
		{
			//result = obbox::TestOBBCollision_SIMD( obbA, rboA, obbA, rboA );   // worst case is that they overlap
			result = obbox::TestOBBCollision_SIMD( obbA, rboA, obbB, rboB );
		}
		finish = g_clk.GetTime();
		swprintf(buf,255,L"\t\tFinal Value: result = %d",result); OutputDebugString(buf);
		PRINTMAT(&obbox::AbsR,L"obbox::AbsR",buf);
		PRINTMAT(&obbox::R,L"obbox::R",buf);
		PRINTVEC(&obbox::t_,L"obbox::t_",buf);
		PRINTVEC(&D3DXVECTOR3(obbox::a_OBBDim[0],obbox::a_OBBDim[1],obbox::a_OBBDim[2]),L"obbox::a_OBBDim",buf);
		PRINTVEC(&D3DXVECTOR3(obbox::b_OBBDim[0],obbox::b_OBBDim[1],obbox::b_OBBDim[2]),L"obbox::b_OBBDim",buf);
		swprintf(buf,255,L"\t\tCalculation time was: %.3fus\n",(double(finish)-double(start))*1e6); OutputDebugString(buf);
	}
	catch (const std::exception &e)
	{
		MessageBox(NULL, stringUtil::toWideString(e.what(),-1).c_str(), L"Error", MB_OK | MB_ICONERROR);
	}

	// Cleanup memory with _aligned_free
	swprintf(buf,255,L"\n-\tFreeing memory and exiting... \n\n"); OutputDebugString(buf);
	aligned_delete(vec0,D3DXVECTOR3A16);
	aligned_delete(vec1,D3DXVECTOR3A16);
	aligned_delete(vecT,D3DXVECTOR3A16);
	aligned_delete(vecR,D3DXVECTOR3A16);
	aligned_delete(mat0,D3DXMATRIXA16);
	aligned_delete(mat1,D3DXMATRIXA16);
	aligned_delete(matT,D3DXMATRIXA16);
	aligned_delete(matR,D3DXMATRIXA16);
	aligned_delete_destructor(obbA,obbox);
	aligned_delete_destructor(obbB,obbox);
	aligned_delete(rboA,obboxTempVar);
	aligned_delete(rboB,obboxTempVarSSE);

	return 0;

}

#pragma optimize( "", on ) // TURN THEM BACK ON FOR EVERYTHING ELSE

