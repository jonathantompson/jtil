/*************************************************************
**						SIMDFuncs							**
**************************************************************/
// File:		SIMDFuncs.cpp
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#include "utils_and_misc_classes\SIMD_helpers\SIMDFuncs.h"
#include "dxInclude.h"
#include "utils_and_misc_classes\new_redefine.h" // MUST COME LAST

// < 0111...111, 0111...111, 0111...111, 0111...111 >
__declspec(align(DATA_ALIGNMENT)) unsigned int IEEE754_NAN[4] = {0x7fffffff,0x7fffffff,0x7fffffff,0x7fffffff};
// < 1000...000, 1000...000, 1000...000, 1000...000 >
__declspec(align(DATA_ALIGNMENT)) unsigned int IEEE754_SIGNHIGH[4] = {0x80000000,0x80000000,0x80000000,0x80000000};
// 0.00000000000000000000001 --> 1e-23  (http://babbage.cs.qc.edu/IEEE-754/Decimal.html)
__declspec(align(DATA_ALIGNMENT)) unsigned int IEEE754_EPSILON[4] = {0x19416D9A,0x19416D9A,0x19416D9A,0x19416D9A};
// 0.0000001 --> 1e-7
__declspec(align(DATA_ALIGNMENT)) unsigned int IEEE754_EPSILONBIG[4] = {0x33D6BF95,0x33D6BF95,0x33D6BF95,0x33D6BF95};

