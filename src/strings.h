#ifndef _STRINGS_H_
#define _STRINGS_H_
#include "nsis_tchar.h"

#if defined(_UNICODE) || defined(UNICODE)
#define String std::wstring
#define Cout std::wcout
#else
#define String std::string
#define Cout std::cout
#endif

#endif /* _STRINGS_H_ */