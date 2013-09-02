/*************************************************************
**						stringHasher						**
**************************************************************/
// File:		stringHasher.h
// Author:		Jonathan Tompson
// e-mail:		jjt2119@columbia.edu or jonathantompson@gmail.com

#ifndef stringHasher_h
#define stringHasher_h

#include <string>
#include <hash_map>

// The following class defines a hash function for strings 
class stringHasher : public stdext::hash_compare <std::wstring>
{
public:

  size_t operator() (const std::wstring& s) const
  {
    size_t h = 0;
    std::wstring::const_iterator p, p_end;
    for(p = s.begin(), p_end = s.end(); p != p_end; ++p)
    {
      h = 31 * h + (*p);
    }
    return h;
  }

  bool operator() (const std::wstring& s1, const std::wstring& s2) const
  {
    return s1 < s2;
  }
};

#endif