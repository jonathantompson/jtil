//
//  simd_macros.h
//
//  Created by Jonathan Tompson on 4/16/13.
//
//  Some macros to make SIMD functions more readable
//

#ifndef UNNAMED_SIMED_MACROS_HEADER
#define UNNAMED_SIMED_MACROS_HEADER

#include <xmmintrin.h> 
#include "jtil/alignment/data_align.h"

__declspec(align(ALIGNMENT)) extern unsigned int IEEE754_NAN[4];
__declspec(align(ALIGNMENT)) extern unsigned int IEEE754_SIGNHIGH[4];
__declspec(align(ALIGNMENT)) extern unsigned int IEEE754_EPSILON[4];
__declspec(align(ALIGNMENT)) extern unsigned int IEEE754_EPSILONBIG[4];

// Name:		LOAD_VECTORA16
// Description:	Load the 16byte aligned vector into the SSE register (xmm0,xmm1,...,xmm7)

#define	LOAD_VECTORA16(A, V0) \
	__asm {mov edx, A}\
	__asm {movaps V0, [edx]}


// Name:		LOAD_VECTORA16_INSTANCE	
// Description:	Load the 16byte aligned vector into the SSE register (xmm0,xmm1,...,xmm7)
// USE THIS VERSION WHEN A IS NOT A POINTER BUT AN ACTUAL INSTANCE			

#define	LOAD_VECTORA16_INSTANCE(A, V0) \
	__asm {lea edx, A}\
	__asm {movaps V0, [edx]}


// Name:		LOAD_FLOATA16
// Description:	Load the 16byte aligned float into the SSE register (xmm0,xmm1,...,xmm7)

#define	LOAD_FLOATA16(A, V0) \
	__asm {lea edx, A} \
	__asm {movss V0, [edx]} \
	REPLICATE_X(V0);


// Name:		STORE_VECTORA16
// Description:	Store the 16byte aligned vector from the SSE register xmm0,xmm1,...,xmm7)

#define	STORE_VECTORA16(A, V0) \
	__asm {mov edx, A}\
	__asm {movaps [edx], V0}


// Name:		STORE_VECTORA16_INSTANCE
// Description:	Store the 16byte aligned vector from the SSE register xmm0,xmm1,...,xmm7)
// USE THIS VERSION WHEN A IS NOT A POINTER BUT AN ACTUAL INSTANCE			

#define	STORE_VECTORA16_INSTANCE(A, V0) \
	__asm {lea edx, A}\
	__asm {movaps [edx], V0}


// Name:		COPY_XMM	
// Description:	Copy the value from V1 to V0											

#define	COPY_XMM(V0, V1) \
	__asm {movaps V0, V1}


// Name:		ADD_VECTORS	
// Description: Add the xmm registers: V0 = V0 + V1										

#define	ADD_VECTORS(V0, V1) \
	__asm {addps V0, V1}


// Name:		SUB_VECTORS	
// Description: Subtract the xmm registers: V0 = V0 - V1								

#define	SUB_VECTORS(V0, V1) \
	__asm {subps V0, V1}


// Name:		MUL_VECTORS	
// Description: Multiply the xmm registers: V0 = V0 * V1								

#define	MUL_VECTORS(V0, V1) \
	__asm {mulps V0, V1}


// Name:		DIV_VECTORS	
// Description: Multiply the xmm registers: V0 = V0 / V1								

#define	DIV_VECTORS(V0, V1) \
	__asm {divps V0, V1}


// Name:		ADDACROSS_VECTOR		
// Description:	xmm0 = xmm0.V0 + xmm0.V1 + xmm0.V2 + xmm0.V3 							
// This adds accross the registers -> Should be avoided if possible.		

#define	ADDACROSS_VECTOR(V0, Vtemp_0 ) \
	__asm{movaps Vtemp_0, V0 } \
	__asm{shufps V0, V0, _MM_SHUFFLE(2, 3, 0, 1) } \
	__asm{addps V0, Vtemp_0 } \
	__asm{movaps Vtemp_0, V0 } \
	__asm{shufps V0, V0, _MM_SHUFFLE(0, 1, 2, 3) } \
	__asm{addps V0, Vtemp_0 }


// Name:		DOT_VECTOR3	
// Description: Perform V3 dot product of V0 = DOT( V0, V1 )		 w components are ignored	

// Helper to work out 8-bit mask for shufps instruction
#define SHUFFLE_PARAM(x, y, z, w) \
	((x) | ((y) << 2) | ((z) << 4) | ((w) << 6))
#define R_SHUFFLE_PS(x, y, z, w) \
	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))

// Replicate X element into all values
#define REPLICATE_X(v) \
	__asm {shufps v, v, SHUFFLE_PARAM(0,0,0,0)}
// Replicate Y element into all values
#define REPLICATE_Y(v) \
	__asm {shufps v, v, SHUFFLE_PARAM(1,1,1,1)}
// Replicate Z element into all values
#define REPLICATE_Z(v) \
	__asm {shufps v, v, SHUFFLE_PARAM(2,2,2,2)}
// Replicate W element into all values
#define REPLICATE_W(v) \
	__asm {shufps v, v, SHUFFLE_PARAM(3,3,3,3)}

// THIS IS BEST SSE VERSION BUT STILL SLOWER THAN DIRECTX
#define DOT_VECTOR(V0, V1) \
	MUL_VECTORS(V0,V1) \
	ADDACROSS_VECTOR(V0,V1)

//// This wont destroy the V1
#define	DOT_VECTOR_NODESTROY(V0, V1, Vtemp) \
	MUL_VECTORS(V0,V1) \
	ADDACROSS_VECTOR(V0,Vtemp)

#define	DOT_VECTOR3(V0, V1) \
	MUL_VECTORS(V0,V1)\
	COPY_XMM(V1,V0)\
	__asm {shufps V0, V0, SHUFFLE_PARAM(2,2,0,0)}\
	ADD_VECTORS(V0,V1)\
	REPLICATE_Y(V1)\
	ADD_VECTORS(V0,V1)\
	REPLICATE_X(V0)

// haddps is slow on some CPUs...
#define	DOT_VECTOR3_SSE3(V0, V1) \
	__asm {mulps V0, V1 }\
	__asm {haddps V0, V0 }\
	__asm {haddps V0, V0 }

// Very little support yet
#define	DOT_VECTOR3_SSE4P1(V0, V1) \
	__asm {dpps V0, V1, 0x55 }


// Name:		CROSS_VECTOR3
// Description: Perform V3 cross product of V0 = CROSS( V0, V1 )	w components are ignored	
// Requires 2 temporary registers!!										

#define	CROSS_VECTOR3(V0, V1, V_T0, V_T1) \
	__asm{ MOVAPS V_T0, V0 } \
	__asm{ MOVAPS V_T1, V1 } \
	__asm{ SHUFPS V0, V0, 0xD8 } \
	__asm{ SHUFPS V1, V1, 0xE1 } \
	__asm{ MULPS  V0, V1 } \
	__asm{ SHUFPS V_T0, V_T0, 0xE1 } \
	__asm{ SHUFPS V_T1, V_T1, 0xD8 } \
	__asm{ MULPS V_T0, V_T1 } \
	__asm{ SUBPS  V0, V_T0 } 


// Name:		AND_VECTORS, OR_VECTORS, XOR_VECTORS, ANDNOT_VECTORS					
// Description:	Bitwise operations of packed floating point numbers						

#define	AND_VECTORS(V0, V1 ) \
	__asm{ andps V0, V1 }

#define	OR_VECTORS(V0, V1 ) \
	__asm{ orps V0, V1 }

#define	XOR_VECTORS(V0, V1 ) \
	__asm{ xorps V0, V1 }

#define	ANDNOT_VECTORS(V0, V1 ) \
	__asm{ andnps V0, V1 }


// Name:		ABS_VECTOR	
// Description:	Find the absolute value of the vector elements.							

#define	ABS_VECTOR(V0, Vtemp_0 ) \
	__asm {movaps Vtemp_0, IEEE754_NAN} \
	AND_VECTORS(V0, Vtemp_0);


// Name:		NEGATE_VECTOR
// Description:	Negate the sign of the of the vector elements							

#define	NEGATE_VECTOR(V0, Vtemp_0 ) \
	__asm {movaps Vtemp_0, IEEE54_SIGNHIGH} \
	XOR_VECTORS(V0, Vtemp_0);


// Name:		LOAD_MATRIXA16
// Description:	Load the 16byte aligned vector into the SSE registers (xmm0,xmm1,...,xmm7)

#define	LOAD_MATRIXA16(A, M0_0, M0_1, M0_2, M0_3) \
	__asm {mov edx, A}\
	__asm {movaps M0_0, [edx]}\
	__asm {movaps M0_1, [edx+0x10]}\
	__asm {movaps M0_2, [edx+0x20]}\
	__asm {movaps M0_3, [edx+0x30]}


// Name:		LOAD_MATRIXA16_INSTANCE	
// Description:	Load the 16byte aligned vector into the SSE registers (xmm0,xmm1,...,xmm7)
// USE THIS VERSION WHEN A IS NOT A POINTER BUT AN ACTUAL INSTANCE			

#define	LOAD_MATRIXA16_INSTANCE(A, M0_0, M0_1, M0_2, M0_3) \
	__asm {lea edx, A}\
	__asm {movaps M0_0, [edx]}\
	__asm {movaps M0_1, [edx+0x10]}\
	__asm {movaps M0_2, [edx+0x20]}\
	__asm {movaps M0_3, [edx+0x30]}


// Name:		STORE_MATRIXA16
// Description:	Store the 16byte aligned vector from the SSE registerS xmm0,xmm1,...,xmm7)

#define	STORE_MATRIXA16(A, M0_0, M0_1, M0_2, M0_3) \
	__asm {mov edx, A}\
	__asm {movaps [edx], M0_0}\
	__asm {movaps [edx+0x10], M0_1}\
	__asm {movaps [edx+0x20], M0_2}\
	__asm {movaps [edx+0x30], M0_3}


// Name:		STORE_MATRIXA16_INSTANCE
// Description:	Store the 16byte aligned vector from the SSE registerS xmm0,xmm1,...,xmm7)
// USE THIS VERSION WHEN A IS NOT A POINTER BUT AN ACTUAL INSTANCE			

#define	STORE_MATRIXA16_INSTANCE(A, M0_0, M0_1, M0_2, M0_3) \
	__asm {lea edx, A}\
	__asm {movaps [edx], M0_0}\
	__asm {movaps [edx+0x10], M0_1}\
	__asm {movaps [edx+0x20], M0_2}\
	__asm {movaps [edx+0x30], M0_3}


// Name:		ADD_MATRICES
// Description: Add the matricies; each group of 4 registers stores a matrix			

#define	ADD_MATRICES(M0_0, M0_1, M0_2, M0_3, M1_0, M1_1, M1_2, M1_3) \
	__asm {addps M0_0, M1_0}\
	__asm {addps M0_1, M1_1}\
	__asm {addps M0_2, M1_2}\
	__asm {addps M0_3, M1_3}


// Name:		SUB_MATRICES
// Description: Subtract the matricies; each group of 4 registers stores a matrix		

#define	SUB_MATRICES(M0_0, M0_1, M0_2, M0_3, M1_0, M1_1, M1_2, M1_3) \
	__asm {subps M0_0, M1_0}\
	__asm {subps M0_1, M1_1}\
	__asm {subps M0_2, M1_2}\
	__asm {subps M0_3, M1_3}


// Name:		MULT_VECTOR3A16MATRIXA16
// Description:	Perform vector Matrix mutliplication: V0 = M0.V0						
// Requires 2 temporary registries -> Wont destroy matrix!!				

// --> Column Major Form (ie OpenGL)
#define	MULT_VECTOR4A16MATRIXA16_COLMAJ(V0, V_T0, V_T1, M0_0, M0_1, M0_2, M0_3) \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0xFF } \
	__asm {mulps V_T0, M0_0 } \
	__asm {movaps V_T1, V_T0 } \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0xAA } \
	__asm {mulps V_T0, M0_1 } \
	__asm {addps V_T1, V_T0 } \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0x55 } \
	__asm {mulps V_T0, M0_2 } \
	__asm {addps V_T1, V_T0 } \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0x00 } \
	__asm {mulps V_T0, M0_3 } \
	__asm {addps V_T1, V_T0 } \
	__asm {movaps V0, V_T1 } 

// --> Row Major Form (ie DirectX) --> This function is the same as MatrixMult_ATran_B()
#define	MULT_VECTOR4A16MATRIXA16(V0, V_T0, V_T1, M0_0, M0_1, M0_2, M0_3) \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0x00} \
	__asm {mulps V_T0, M0_0 } \
	__asm {movaps V_T1, V_T0 } \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0x55 } \
	__asm {mulps V_T0, M0_1 } \
	__asm {addps V_T1, V_T0 } \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0xAA } \
	__asm {mulps V_T0, M0_2 } \
	__asm {addps V_T1, V_T0 } \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0xFF } \
	__asm {mulps V_T0, M0_3 } \
	__asm {addps V_T1, V_T0 } \
	__asm {movaps V0, V_T1 } 

// Will destroy matrix but doesn't need temp registers --> HASN'T BEEN TESTED!!
#define	MULT_VECTOR4A16MATRIXA16_SSE4(V0, M0_0, M0_1, M0_2, M0_3) \
	__asm {mulps M0_0,V0 } \
	__asm {mulps M0_1,V0 } \
	__asm {mulps M0_2,V0 } \
	__asm {mulps M0_3,V0 } \
	__asm {haddps M0_0,M0_0 } \
	__asm {haddps M0_0,M0_0 } \
	__asm {haddps M0_1,M0_1 } \
	__asm {haddps M0_1,M0_1 } \
	__asm {haddps M0_2,M0_2 } \
	__asm {haddps M0_2,M0_2 } \
	__asm {haddps M0_3,M0_3 } \
	__asm {haddps M0_3,M0_3 } \
	__asm {shufps M0_0,M0_1,00000000b  } \
	__asm {shufps M0_0,M0_2,00001000b } \
	__asm {blendps M0_0,M0_3,00001000b } \
	__asm {movaps V0, M0_0 }


// Name:		MULT_VECTOR3A16MATRIXA16
// Description:	Perform vector Matrix mutliplication: V0 = M0.V0						
// Requires 2 temporary registries -> Wont destroy matrix!!				

// --> Row Major Form (ie DirectX) --> This function is the same as MatrixMult_ATran_B()
#define	MULT_VECTOR3A16MATRIXA16(V0, V_T0, V_T1, M0_0, M0_1, M0_2, M0_3) \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0x00} \
	__asm {mulps V_T0, M0_0 } \
	__asm {movaps V_T1, V_T0 } \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0x55 } \
	__asm {mulps V_T0, M0_1 } \
	__asm {addps V_T1, V_T0 } \
	__asm {movaps V_T0, V0 } \
	__asm {shufps V_T0, V_T0, 0xAA } \
	__asm {mulps V_T0, M0_2 } \
	__asm {addps V_T1, V_T0 } \
	__asm {movaps V0, V_T1 } 
	

// Name:		TRAN_MATRIXA16
// Description:	Perform Matrix mutliplication: M0 = M0^T								

#define	TRAN_MATRIXA16(M0_0, M0_1, M0_2, M0_3, Vtemp_0 ) \
	__asm {movaps Vtemp_0, M0_2 } \
	__asm {unpcklps M0_2, M0_3 } \
	__asm {unpckhps	Vtemp_0, M0_3 } \
	__asm {movaps M0_3, M0_0 } \
	__asm {unpcklps M0_0, M0_1 } \
	__asm {unpckhps M0_3, M0_1 } \
	__asm {movaps M0_1, M0_0 } \
	__asm {shufps M0_0, M0_2, R_SHUFFLE_PS( 0, 1, 0, 1 ) } \
	__asm {shufps M0_1, M0_2, R_SHUFFLE_PS( 2, 3, 2, 3 ) } \
	__asm {movaps M0_2, M0_3 } \
	__asm {shufps M0_2, Vtemp_0, R_SHUFFLE_PS( 0, 1, 0, 1 ) } \
	__asm {shufps M0_3, Vtemp_0, R_SHUFFLE_PS( 2, 3, 2, 3 ) }


// Name:		MULT_VECTOR3A16MATRIXA16
// Description:	Perform vector Matrix mutliplication: V0 = M0.V0						

// --> Row Major Form (ie DirectX) --> This function is the same as MatrixMult_ATran_B() -> WONT destroy matrix!!
#define	MULT_VECTOR3A16MATRIXA16_TRAN(V0, V_T0, V_T1, V_T2, M0_0, M0_1, M0_2, M0_3) \
	__asm {movaps V_T0, V0 } \
	__asm {movaps V_T1, V0 } \
	DOT_VECTOR_NODESTROY(V0, M0_0, V_T2) \
	DOT_VECTOR_NODESTROY(V_T0, M0_1, V_T2) \
	DOT_VECTOR_NODESTROY(V_T1, M0_2, V_T2) \
	__asm {unpcklps V0, V_T0 } \
	__asm {movlhps V0, V_T1 }

// --> Row Major Form (ie DirectX) --> This function is the same as MatrixMult_ATran_B() -> Will destroy matrix!!
#define	MULT_VECTOR3A16MATRIXA16_TRAN_OLD(V0, V_T0, V_T1, M0_0, M0_1, M0_2, M0_3) \
	TRAN_MATRIXA16(M0_0, M0_1, M0_2, M0_3, V_T0); \
	MULT_VECTOR3A16MATRIXA16(V0, V_T0, V_T1, M0_0, M0_1, M0_2, M0_3);


// Name:		ABS_MATRIXA16
// Description:	Find the absolute value of the matrix elements.							

#define	ABS_MATRIXA16(M0_0, M0_1, M0_2, M0_3, Vtemp_0 ) \
	__asm {movaps Vtemp_0, IEEE754_NAN} \
	AND_VECTORS(M0_0, Vtemp_0); \
	AND_VECTORS(M0_1, Vtemp_0); \
	AND_VECTORS(M0_2, Vtemp_0); \
	AND_VECTORS(M0_3, Vtemp_0); 


// Name:		ABS_PLUS_EPSILON_MATRIXA16
// Description:	Find the absolute value of the matrix elements then add 1e-27			

#define	ABS_PLUS_EPSILON_MATRIXA16(M0_0, M0_1, M0_2, M0_3, Vtemp_0 ) \
	__asm {movaps Vtemp_0, IEEE754_NAN} \
	AND_VECTORS(M0_0, Vtemp_0); \
	AND_VECTORS(M0_1, Vtemp_0); \
	AND_VECTORS(M0_2, Vtemp_0); \
	AND_VECTORS(M0_3, Vtemp_0); \
	__asm {movaps Vtemp_0, IEEE754_EPSILON} \
	ADD_VECTORS(M0_0, Vtemp_0); \
	ADD_VECTORS(M0_1, Vtemp_0); \
	ADD_VECTORS(M0_2, Vtemp_0); \
	ADD_VECTORS(M0_3, Vtemp_0);


// Name:		ABS_PLUS_EPSILONBIG_MATRIXA16											
// Description:	Find the absolute value of the matrix elements then add 1e-7			

#define	ABS_PLUS_EPSILONBIG_MATRIXA16(M0_0, M0_1, M0_2, M0_3, Vtemp_0 ) \
	__asm {movaps Vtemp_0, IEEE754_NAN} \
	AND_VECTORS(M0_0, Vtemp_0); \
	AND_VECTORS(M0_1, Vtemp_0); \
	AND_VECTORS(M0_2, Vtemp_0); \
	AND_VECTORS(M0_3, Vtemp_0); \
	__asm {movaps Vtemp_0, IEEE754_EPSILONBIG} \
	ADD_VECTORS(M0_0, Vtemp_0); \
	ADD_VECTORS(M0_1, Vtemp_0); \
	ADD_VECTORS(M0_2, Vtemp_0); \
	ADD_VECTORS(M0_3, Vtemp_0);


// Name:		MULT_MATRIXA16MATRIXA16	
// Description:	Calculate the matrix multiplication MOUT = M0 * M1						
// Usage: M1 is already preloaded into SSE registers.						
// Mout = M0 * M1.		M1 remains undestroyed on completion.				

#define	MULT_MATRIXA16MATRIXA16(MOUT, M0, Vtemp0, Vtemp1, Vtemp2, M1_0, M1_1, M1_2, M1_3) \
	__asm {mov ecx, MOUT } \
	__asm {mov edx, M0 } \
	__asm {movaps Vtemp0, [edx] } \
	MULT_VECTOR4A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x10]} \
	MULT_VECTOR4A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x10], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x20]} \
	MULT_VECTOR4A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x20], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x30]} \
	MULT_VECTOR4A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x30], Vtemp0}


// Name:		MULT_MATRIXA16MATRIXA16_INSTANCE										
// Description:	Calculate the matrix multiplication MOUT = M0 * M1						
// Usage: M1 is already preloaded into SSE registers.						
// Mout = M0 * M1.		M1 remains undestroyed on completion.				
// USE THIS VERSION WHEN A IS NOT A POINTER BUT AN ACTUAL INSTANCE			

#define	MULT_MATRIXA16MATRIXA16_INSTANCE(MOUT, M0, Vtemp0, Vtemp1, Vtemp2, M1_0, M1_1, M1_2, M1_3) \
	__asm {lea ecx, MOUT } \
	__asm {lea edx, M0 } \
	__asm {movaps Vtemp0, [edx] } \
	MULT_VECTOR4A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x10]} \
	MULT_VECTOR4A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x10], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x20]} \
	MULT_VECTOR4A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x20], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x30]} \
	MULT_VECTOR4A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x30], Vtemp0}


// Name:		MULT_MATRIXA16MATRIXA16_3x3
// Description:	Calculate the matrix multiplication MOUT = M0 * M1	(3x3)				
// Usage: M1 is already preloaded into SSE registers.						
// Mout = M0 * M1.		M1 remains undestroyed on completion.				
// ONLY 25% FASTER THAN SCALAR IMPLIMENTATION!!							

#define	MULT_MATRIXA16MATRIXA16_3x3(MOUT, M0, Vtemp0, Vtemp1, Vtemp2, M1_0, M1_1, M1_2, M1_3) \
	__asm {mov ecx, MOUT } \
	__asm {mov edx, M0 } \
	__asm {movaps Vtemp0, [edx] } \
	MULT_VECTOR3A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x10]} \
	MULT_VECTOR3A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x10], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x20]} \
	MULT_VECTOR3A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x20], Vtemp0}

#define	MULT_MATRIXA16MATRIXA16_3x3_INSTANCE(MOUT, M0, Vtemp0, Vtemp1, Vtemp2, M1_0, M1_1, M1_2, M1_3) \
	__asm {lea ecx, MOUT } \
	__asm {mov edx, M0 } \
	__asm {movaps Vtemp0, [edx] } \
	MULT_VECTOR3A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x10]} \
	MULT_VECTOR3A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x10], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x20]} \
	MULT_VECTOR3A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x20], Vtemp0}

#define	MULT_MATRIXA16MATRIXA16_3x3_INSTANCEINSTANCE(MOUT, M0, Vtemp0, Vtemp1, Vtemp2, M1_0, M1_1, M1_2, M1_3) \
	__asm {lea ecx, MOUT } \
	__asm {lea edx, M0 } \
	__asm {movaps Vtemp0, [edx] } \
	MULT_VECTOR3A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x10]} \
	MULT_VECTOR3A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x10], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x20]} \
	MULT_VECTOR3A16MATRIXA16(Vtemp0,Vtemp1,Vtemp2,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x20], Vtemp0}


// Name:		GETBOOL_VECTORA16		
// Description:	four element boolian functions (>,<,==,!=) leave the four vectors NAN or 0

#define	GETBOOL_VECTOR3A16(ret_UINT, V0, VTemp) \
	__asm {movaps VTemp, V0} \
	REPLICATE_Y(VTemp); \
	OR_VECTORS(V0,VTemp); \
	__asm {movaps VTemp, V0} \
	REPLICATE_Z(VTemp); \
	OR_VECTORS(V0,VTemp); \
	__asm {lea edx, ret_UINT } \
	__asm {movss [edx], V0 } 


// Name:		MatrixMult_A_BTran		
// Description:	Calculate the matrix multiplication MOUT = M0 * M1^T					
// Usage: M1 is already preloaded into SSE registers.						
// Mout = M0 * M1^T.		M1 remains undestroyed on completion.			

// This is same as MatrixMult_A_BTran
#define	MULT_MATRIXA16MATRIXA16_3x3_BTRAN(MOUT, M0, Vtemp0, Vtemp1, Vtemp2, Vtemp3, M1_0, M1_1, M1_2, M1_3) \
	__asm {mov ecx, MOUT } \
	__asm {mov edx, M0 } \
	__asm {movaps Vtemp0, [edx] } \
	MULT_VECTOR3A16MATRIXA16_TRAN(Vtemp0,Vtemp1,Vtemp2,Vtemp3,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x10]} \
	MULT_VECTOR3A16MATRIXA16_TRAN(Vtemp0,Vtemp1,Vtemp2,Vtemp3,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x10], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x20]} \
	MULT_VECTOR3A16MATRIXA16_TRAN(Vtemp0,Vtemp1,Vtemp2,Vtemp3,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x20], Vtemp0}

// This is same as MatrixMult_A_BTran
#define	MULT_MATRIXA16MATRIXA16_3x3_BTRAN_INSTANCE(MOUT, M0, Vtemp0, Vtemp1, Vtemp2, M1_0, M1_1, M1_2, M1_3) \
	__asm {lea ecx, MOUT } \
	__asm {lea edx, M0 } \
	__asm {movaps Vtemp0, [edx] } \
	MULT_VECTOR3A16MATRIXA16_TRAN(Vtemp0,Vtemp1,Vtemp2,Vtemp3,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x10]} \
	MULT_VECTOR3A16MATRIXA16_TRAN(Vtemp0,Vtemp1,Vtemp2,Vtemp3,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x10], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x20]} \
	MULT_VECTOR3A16MATRIXA16_TRAN(Vtemp0,Vtemp1,Vtemp2,Vtemp3,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x20], Vtemp0}


// Name:		MatrixMult_A_BTran		
// Description:	Calculate the matrix multiplication MOUT = M0^T * M1					
// Usage: M1 is already preloaded into SSE registers.						
// Mout = M0 * M1^T.		M1 remains undestroyed on completion.			

// This is same as MatrixMult_ATran_B
#define	MULT_MATRIXA16MATRIXA16_3x3_ATRAN(MOUT, M0, Vtemp0, Vtemp1, Vtemp2, Vtemp3, M1_0, M1_1, M1_2, M1_3) \
	__asm {mov ecx, MOUT } \
	__asm {mov edx, M0 } \
	__asm {movaps Vtemp0, [edx] } \
	MULT_VECTOR3A16MATRIXA16_TRAN(Vtemp0,Vtemp1,Vtemp2,Vtemp3,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x10]} \
	MULT_VECTOR3A16MATRIXA16_TRAN(Vtemp0,Vtemp1,Vtemp2,Vtemp3,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x10], Vtemp0} \
	__asm {movaps Vtemp0, [edx+0x20]} \
	MULT_VECTOR3A16MATRIXA16_TRAN(Vtemp0,Vtemp1,Vtemp2,Vtemp3,M1_0,M1_1,M1_2,M1_3); \
	__asm {movaps [ecx+0x20], Vtemp0}

#endif
