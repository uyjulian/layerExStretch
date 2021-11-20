

#if defined(_M_IX86) || defined(_M_X64) || defined(_M_AMD64)
#pragma warning(push)
#pragma warning(disable:4305)
#define USE_SSE2
#include "sse_mathfun.h"
#undef USE_SSE2
#pragma warning(pop)
#endif
