#include <windows.h>
#include "utils_and_misc_classes\SIMD_helpers\cpuid.h"
#include <stdio.h>
#include "utils_and_misc_classes\stringUtil.h"
#include <string>


// These are the bit flags that get set on calling cpuid
// with register eax set to 1
#define _MMX_FEATURE_BIT        0x00800000
#define _SSE_FEATURE_BIT        0x02000000
#define _SSE2_FEATURE_BIT       0x04000000

// This bit is set when cpuid is called with
// register set to 80000001h (only applicable to AMD)
#define _3DNOW_FEATURE_BIT      0x80000000

// These are the names of the various processors
#define PROC_AMD_AM486          L"AMD Am486"
#define PROC_AMD_K5             L"AMD K5"
#define PROC_AMD_K6             L"AMD K6"
#define PROC_AMD_K6_2           L"AMD K6-2"
#define PROC_AMD_K6_3           L"AMD K6-3"
#define PROC_AMD_ATHLON         L"AMD Athlon"
#define PROC_INTEL_486DX        L"INTEL 486DX"
#define PROC_INTEL_486SX        L"INTEL 486SX"
#define PROC_INTEL_486DX2       L"INTEL 486DX2"
#define PROC_INTEL_486SL        L"INTEL 486SL"
#define PROC_INTEL_486SX2       L"INTEL 486SX2"
#define PROC_INTEL_486DX2E      L"INTEL 486DX2E"
#define PROC_INTEL_486DX4       L"INTEL 486DX4"
#define PROC_INTEL_PENTIUM      L"INTEL Pentium"
#define PROC_INTEL_PENTIUM_MMX  L"INTEL Pentium-MMX"
#define PROC_INTEL_PENTIUM_PRO  L"INTEL Pentium-Pro"
#define PROC_INTEL_PENTIUM_II   L"INTEL Pentium-II"
#define PROC_INTEL_CELERON      L"INTEL Celeron"
#define PROC_INTEL_PENTIUM_III  L"INTEL Pentium-III"
#define PROC_INTEL_PENTIUM_4    L"INTEL Pentium-4"
#define PROC_CYRIX              L"Cyrix"
#define PROC_CENTAUR            L"Centaur"
#define PROC_UNKNOWN            L"Unknown"

// This is the maximum length of the vendor name
#define MAX_VNAME_LENGTH        12

int IsCPUID()
{
	__try {
		_asm {
			xor eax, eax
				cpuid
		}
	}
#pragma warning (suppress: 6320)
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return 0;
	}
	return 1;
}


/***
* int _os_support(int feature)
*   - Checks if OS Supports the capablity or not
*
* Entry:
*   feature: the feature we want to check if OS supports it.
*
* Exit:
*   Returns 1 if OS support exist and 0 when OS doesn't support it.
*
****************************************************************/

int _os_support(int feature)
{
	__try {
		switch (feature) {
		case _CPU_FEATURE_SSE:
			__asm {
				xorps xmm0, xmm0        // executing SSE instruction
			}
			break;
		case _CPU_FEATURE_SSE2:
			__asm {
				xorpd xmm0, xmm0        // executing SSE2 instruction
			}
			break;
		case _CPU_FEATURE_3DNOW:
			__asm {
				pfrcp mm0, mm0          // executing 3DNow! instruction
					emms
			}
			break;
		case _CPU_FEATURE_MMX:
			__asm {
				pxor mm0, mm0           // executing MMX instruction
					emms
			}
			break;
		}
	}
#pragma warning (suppress: 6320)
	__except (EXCEPTION_EXECUTE_HANDLER) {
		if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION) {
			return 0;
		}
		return 0;
	}
	return 1;
}


/***
*
* void map_mname(int, int, const char *, char *)
*   - Maps family and model to processor name
*
****************************************************/


void map_mname(int family, int ext_family, int model, int ext_model, const wchar_t *v_name, wchar_t *m_name)
{
	// Default to name not known
	m_name[0] = '\0';

	if (!wcsncmp(L"AuthenticAMD", v_name, 12)) {
		switch (family) { // extract family code
		case 4: // Am486/AM5x86
			wcscpy (m_name, PROC_AMD_AM486);
			break;

		case 5: // K6
			switch (model) { // extract model code
			case 0:
			case 1:
			case 2:
			case 3:
				wcscpy (m_name, PROC_AMD_K5);
				break;
			case 6:
			case 7:
				wcscpy (m_name, PROC_AMD_K6);
				break;
			case 8:
				wcscpy (m_name, PROC_AMD_K6_2);
				break;
			case 9:
			case 10:
			case 11:
			case 12:
			case 13:
			case 14:
			case 15:
				wcscpy (m_name, PROC_AMD_K6_3);
				break;
			}
			break;

		case 6: // Athlon
			// No model numbers are currently defined
			wcscpy (m_name, PROC_AMD_ATHLON);
			break;
		}
	}
	else if (!wcsncmp(L"GenuineIntel", v_name, 12)) {
		switch (family) { // extract family code
		case 4: // family 4
			switch (model) { // extract model code
			case 0: // model 0
			case 1: // model 1
				wcscpy (m_name, PROC_INTEL_486DX);
				break;
			case 2: // model 2
				wcscpy (m_name, PROC_INTEL_486SX);
				break;
			case 3: // model 3
				wcscpy (m_name, PROC_INTEL_486DX2);
				break;
			case 4: // model 4
				wcscpy (m_name, PROC_INTEL_486SL);
				break;
			case 5: // model 5
				wcscpy (m_name, PROC_INTEL_486SX2);
				break;
			case 7: // model 6
				wcscpy (m_name, PROC_INTEL_486DX2E);
				break;
			case 8: // model 7
				wcscpy (m_name, PROC_INTEL_486DX4);
				break;
			}
			break;

		case 5: // family 5
			switch (model) { // extract model code
			case 1: // model 1
			case 2: // model 2
			case 3: // model 3
				wcscpy (m_name, PROC_INTEL_PENTIUM);
				break;
			case 4: // model 4
				wcscpy (m_name, PROC_INTEL_PENTIUM_MMX);
				break;
			}
			break;

		case 6: // family 6
			switch (ext_model) {
			case 0: // ext model 0
				switch (model) { // extract model code
				case 1: // model 1
					wcscpy (m_name, PROC_INTEL_PENTIUM_PRO);
					break;
				case 2: // model 2
					wcscpy (m_name, PROC_INTEL_PENTIUM_PRO);
					break;
				case 3: // model 3
				case 5: // model 5
					wcscpy (m_name, PROC_INTEL_PENTIUM_II);
					break;  
				case 6: // model 6
					wcscpy (m_name, PROC_INTEL_CELERON);
					break;
				case 7: // model 7
				case 8: // model 8
				case 10: // model 10
					wcscpy (m_name, PROC_INTEL_PENTIUM_III);
					break;  
				case 13: // model 13
					wcscpy (m_name, L"Pentium M or Celeron M");
					break;  
				case 14: // model 14
					wcscpy (m_name, L"Core2 Duo or Solo");
					break;  
				case 15: // model 15
					wcscpy (m_name, L"Core2 Duo, mobile, Quad");
					break;  
				}
				break;
			case 1: // ext model 1
				switch (model) { // extract model code
				case 10: // model 10
					wcscpy (m_name, L"Intel Core i7");
					break;
				case 12: // model 12
					wcscpy (m_name, L"Intel Atom");
					break;
				case 13: // model 13
					wcscpy (m_name, L"Intel Xeon MP");
					break;
				case 14: // model 14
					wcscpy (m_name, L"Intel Core i5/i7");
					break;
				}
				break;
			case 2: // ext model 2
				switch (model) { // extract model code
				case 5: // model 5
					wcscpy (m_name, L"Intel Core i3, i5/i7 mobile");
					break;
				case 10: // model 10
					wcscpy (m_name, L"Intel Core i7");
					break;
				case 12: // model 12
					wcscpy (m_name, L"Intel Core i7 or Xeon");
					break;
				case 14: // model 14
					wcscpy (m_name, L"Intel Xeon MP");
					break;
				case 15: // model 15
					wcscpy (m_name, L"Intel Xeon MP");
					break;
				}
				break;
			}
			break;

		case 15 | (0x00 << 4): // family 15
			switch (model) {
			case 0:
				wcscpy (m_name, PROC_INTEL_PENTIUM_4);
				break;
			case 1:
				wcscpy (m_name, PROC_INTEL_PENTIUM_4);
				break;
			case 2:
				wcscpy (m_name, PROC_INTEL_PENTIUM_4);
				break;
			case 3:
				wcscpy (m_name, L"Pentium 4, Xeon, Celeron D");
				break;
			case 4:
				wcscpy (m_name, PROC_INTEL_PENTIUM_4);
				break;
			case 6:
				wcscpy (m_name, PROC_INTEL_PENTIUM_4);
				break;
			}
			break;
		}
	}
	else if (!wcsncmp(L"CyrixInstead", v_name, 12)) {
		wcscpy (m_name, PROC_CYRIX);
	}
	else if (!wcsncmp(L"CentaurHauls", v_name, 12)) {
		wcscpy (m_name, PROC_CENTAUR);
	}

	if (!m_name[0]) {
		wcscpy (m_name, PROC_UNKNOWN);
	}
}


/***
*
* int _cpuid (_p_info *pinfo)
*
* Entry:
*
*   pinfo: pointer to _p_info.
*
* Exit:
*
*   Returns int with capablity bit set even if pinfo = NULL
*
****************************************************/


int cpuid::get_cpuid (_p_info *pinfo)
{
	DWORD dwStandard = 0;
	DWORD dwFeature = 0;
	DWORD dwMax = 0;
	DWORD dwExt = 0;
	int feature = 0;
	int os_support = 0;
	union {
		char cBuf[MAX_VNAME_LENGTH + 1]; // add one for the null terminator
		struct {
			DWORD dw0;
			DWORD dw1;
			DWORD dw2;
		} s;
	} Ident;

	if (!IsCPUID()) {
		return 0;
	}

	_asm {
		push ebx
			push ecx
			push edx

			// get the vendor string
			xor eax, eax
			cpuid
			mov dwMax, eax
			mov Ident.s.dw0, ebx
			mov Ident.s.dw1, edx
			mov Ident.s.dw2, ecx

			// get the Standard bits
			mov eax, 1
			cpuid
			mov dwStandard, eax
			mov dwFeature, edx

			// get AMD-specials
			mov eax, 80000000h
			cpuid
			cmp eax, 80000000h
			jc notamd
			mov eax, 80000001h
			cpuid
			mov dwExt, edx

notamd:
		pop ecx
			pop ebx
			pop edx
	}

	if (dwFeature & _MMX_FEATURE_BIT) {
		feature |= _CPU_FEATURE_MMX;
		if (_os_support(_CPU_FEATURE_MMX))
			os_support |= _CPU_FEATURE_MMX;
	}
	if (dwExt & _3DNOW_FEATURE_BIT) {
		feature |= _CPU_FEATURE_3DNOW;
		if (_os_support(_CPU_FEATURE_3DNOW))
			os_support |= _CPU_FEATURE_3DNOW;
	}
	if (dwFeature & _SSE_FEATURE_BIT) {
		feature |= _CPU_FEATURE_SSE;
		if (_os_support(_CPU_FEATURE_SSE))
			os_support |= _CPU_FEATURE_SSE;
	}
	if (dwFeature & _SSE2_FEATURE_BIT) {
		feature |= _CPU_FEATURE_SSE2;
		if (_os_support(_CPU_FEATURE_SSE2))
			os_support |= _CPU_FEATURE_SSE2;
	}

	if (pinfo) {
		memset(pinfo, 0, sizeof(_p_info));

		pinfo->os_support = os_support;
		pinfo->feature = feature;
		pinfo->family = (dwStandard >> 8) & 0xF; // retrieve family
		pinfo->ext_family = (dwStandard >> 20) & 0xFF;
		pinfo->model = (dwStandard >> 4) & 0xF;  // retrieve model
		pinfo->ext_model = (dwStandard >> 16) & 0xF;
		pinfo->stepping = (dwStandard) & 0xF;    // retrieve stepping

		Ident.cBuf[MAX_VNAME_LENGTH] = 0;

		//char v_name[MAX_VNAME_LENGTH+1];

		//      strcpy(v_name, Ident.cBuf);

		wcscpy(pinfo->v_name,stringUtil::toWideString(Ident.cBuf, MAX_VNAME_LENGTH).c_str());

		map_mname(pinfo->family, 
			pinfo->ext_family,
			pinfo->model,
			pinfo->ext_model,
			pinfo->v_name,
			pinfo->model_name);

		pinfo->checks = _CPU_FEATURE_MMX |
			_CPU_FEATURE_SSE |
			_CPU_FEATURE_SSE2 |
			_CPU_FEATURE_3DNOW;
	}

	return feature;
}

void expand(int avail, int mask)
{
	wchar_t OutputText[128];

	if (mask & _CPU_FEATURE_MMX) {
		swprintf(OutputText,127,L"\t   - %s\t_CPU_FEATURE_MMX\n",
			avail & _CPU_FEATURE_MMX ? L"yes" : L"no ");
		OutputDebugString(OutputText);
	}
	if (mask & _CPU_FEATURE_SSE) {
		swprintf(OutputText,127,L"\t   - %s\t_CPU_FEATURE_SSE\n",
			avail & _CPU_FEATURE_SSE ? L"yes" : L"no ");
		OutputDebugString(OutputText);
	}
	if (mask & _CPU_FEATURE_SSE2) {
		swprintf(OutputText,127,L"\t   - %s\t_CPU_FEATURE_SSE2\n",
			avail & _CPU_FEATURE_SSE2 ? L"yes" : L"no ");
		OutputDebugString(OutputText);
	}
	if (mask & _CPU_FEATURE_3DNOW) {
		swprintf(OutputText,127,L"\t   - %s\t_CPU_FEATURE_3DNOW\n",
			avail & _CPU_FEATURE_3DNOW ? L"yes" : L"no ");
		OutputDebugString(OutputText);
	}
}

void cpuid::print_cpuid_info(_p_info *pinfo)
{
	wchar_t OutputText[128];
	wchar_t NewLineText[3];
	swprintf(NewLineText,3,L" \n");

	OutputDebugString(NewLineText); OutputDebugString(NewLineText);
	swprintf(OutputText,127,L"SOME CPU INFO FOR THIS SYSTEM:\n");						OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - v_name:\t\t%s\n", pinfo->v_name);					OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - model:\t\t\t%s\n", pinfo->model_name);				OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - family:\t\t%d\n", pinfo->family);					OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - ext. family:\t%d\n", pinfo->ext_family);				OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - model:\t\t\t%d\n", pinfo->model);					OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - ext. model:\t%d\n", pinfo->ext_model);				OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - stepping:\t\t%d\n", pinfo->stepping);				OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - feature:\t\t%08x\n", pinfo->feature);				OutputDebugString(OutputText);
	expand(pinfo->feature, pinfo->checks);
	swprintf(OutputText,127,L"   - os_support:\t\t%08x\n", pinfo->os_support);			OutputDebugString(OutputText);
	expand(pinfo->os_support, pinfo->checks);
	swprintf(OutputText,127,L"   - checks:\t\t\t%08x\n", pinfo->checks);				OutputDebugString(OutputText);
	OutputDebugString(NewLineText); OutputDebugString(NewLineText);
	swprintf(OutputText,127,L"SOME DATA TYPE INFO FOR THIS COMPILER:\n");				OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(int) \t\t = %d\n", sizeof(int));				OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(bool) \t\t = %d\n", sizeof(bool));			OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(short) \t\t = %d\n", sizeof(short));			OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(UINT) \t\t = %d\n", sizeof(UINT));			OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(long) \t\t = %d\n", sizeof(long));			OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(char) \t\t = %d\n", sizeof(char));			OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(wchar_t) \t = %d\n", sizeof(wchar_t));			OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(float) \t\t = %d\n", sizeof(float));			OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(double) \t = %d\n", sizeof(double));			OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(void *) \t = %d\n", sizeof(void *));			OutputDebugString(OutputText);
	swprintf(OutputText,127,L"   - sizeof(std::vector<double>) = %d\n", sizeof(std::vector<double>));	OutputDebugString(OutputText);
	OutputDebugString(NewLineText); OutputDebugString(NewLineText);
}


bool cpuid::check_sse(_p_info *pinfo)
{
	// Return true if sse is supported by CPU and OS
	return (pinfo->feature & _CPU_FEATURE_SSE) && (pinfo->os_support & _CPU_FEATURE_SSE);
}

bool cpuid::check_sse2(_p_info *pinfo)
{
	// Return true if sse is supported by CPU and OS
	return (pinfo->feature & _CPU_FEATURE_SSE2) && (pinfo->os_support & _CPU_FEATURE_SSE2);
}