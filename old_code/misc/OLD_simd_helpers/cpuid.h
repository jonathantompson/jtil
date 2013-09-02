#ifndef _INC_CPUID
#define _INC_CPUID

#define _CPU_FEATURE_MMX    0x0001
#define _CPU_FEATURE_SSE    0x0002
#define _CPU_FEATURE_SSE2   0x0004
#define _CPU_FEATURE_3DNOW  0x0008

#define _MAX_VNAME_LEN  13
#define _MAX_MNAME_LEN  30

namespace cpuid 
{
	typedef struct _processor_info {
		wchar_t v_name[_MAX_VNAME_LEN];     // vendor name
		wchar_t model_name[_MAX_MNAME_LEN]; // name of model
											// e.g. Intel Pentium-Pro
		int family;                         // family of the processor
											// e.g. 6 = Pentium-Pro architecture
		int ext_family;
		int model;                          // model of processor
											// e.g. 1 = Pentium-Pro for family = 6
		int ext_model;
		int stepping;                       // processor revision number
		int feature;                        // processor feature
											// (same as return value from _cpuid)
		int os_support;                     // does OS Support the feature?
		int checks;                         // mask of checked bits in feature
											// and os_support fields
	} _p_info;

	int		get_cpuid (_p_info *);
	void	print_cpuid_info(_p_info *);
	bool	check_sse(_p_info *pinfo);
	bool	check_sse2(_p_info *pinfo);
}


#endif
