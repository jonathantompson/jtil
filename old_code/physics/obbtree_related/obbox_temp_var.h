/*************************************************************
**						obboxTempVar						**
**************************************************************/
// File:		obboxTempVar.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef obboxTempVar_h
#define obboxTempVar_h

#include	"main.h"
#include	<d3d9.h>				// main Direct3D header
#pragma warning( disable : 4996 )	// disable deprecated warning 
#include	<d3dx9.h>				// Direct3D Extensions header

__declspec(align(DATA_ALIGNMENT)) class obboxTempVar
{
public:
	friend class obbox;
	friend class app;

	// Constructor / Destructor
    obboxTempVar();
	~obboxTempVar();

private:

	// VARIABLES USED IN OBB COLLISION ROUTINE --> AVOID MULTIPLE ALLOCATIONS ON STACK
	//		start 1st 16 byte block
	D3DXVECTOR3		t_aOBB;
	UINT			static_padding_0;
	//		start 2nd 16 byte block
	D3DXVECTOR3		t_bOBB;
	UINT			static_padding_1;
	//		start 3rd 16 byte block
	D3DXVECTOR3		t_a_world;
	UINT			static_padding_2;
	//		start 4th 16 byte block
	D3DXVECTOR3		t_b_world;
	UINT			static_padding_3;
	//		start 4th 16 byte block
	D3DXMATRIXA16	R;
	//		start 8th 16 byte block
	D3DXMATRIXA16	R_aOBB_world;
	//		start 12th 16 byte block
	D3DXMATRIXA16	R_bOBB_world;
	//		start 16th 16 byte block
	D3DXMATRIXA16	R_aOBB_Tran;
	//		start 20th 16 byte block
	D3DXMATRIXA16	R_bOBB_Tran;
	//		start 21st 16 byte block
	D3DXVECTOR3		t_world;	
	UINT			static_padding_4;
	//		start 22nd 16 byte block
	D3DXVECTOR3		t_;
	UINT			static_padding_5;
	//		start 23rd 16 byte block
	float			a_OBBDim[3];
	UINT			static_padding_6;
	//		start 24th 16 byte block
	float			b_OBBDim[3];
	UINT			static_padding_7;
	//		start 25th 16 byte block
	float			ra, rb;
	UINT			static_padding_8[2];
	//		start 26th 16 byte block
	D3DXMATRIXA16	AbsR;

#ifdef SPHERE_REJECT
	//		start 30th 16 byte block
	D3DXVECTOR3		a_boxDimScaled;
	UINT			static_padding_9;
	//		start 31st 16 byte block
	D3DXVECTOR3		b_boxDimScaled;
	UINT			static_padding_10;
	//		start 32nd 16 byte block
	float			a_radius, b_radius;
	UINT			static_padding_11[2];
#endif
};

#endif