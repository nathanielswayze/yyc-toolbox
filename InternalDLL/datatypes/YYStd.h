// Copyright © Opera Norway AS. All rights reserved.
// This file is an original work developed by Opera.
#include "../dependencies/memory.h"
#ifndef __YYSTD_H__
#define __YYSTD_H__

#if defined(YYLLVM) && defined(YYLLVM_SEP_DLL)
#define YYCEXTERN __declspec( dllimport )
#define YYCEXPORT extern "C" __declspec( dllexport )
#elif defined(__YYLLVM__) && defined(YYLLVM_SEP_DLL)
#define YYCEXTERN __declspec( dllexport )
#define YYCEXPORT 
#else
#define YYCEXTERN 
#define YYCEXPORT 
#endif

#if defined(YYPS4)|| defined (YYPS5)
#define FORCEINLINE					inline
#define FORCEINLINE_ATTR			__attribute__((always_inline))
#define NOINLINE_ATTR				__attribute__((noinline))
#define NOTHROW_ATTR				__attribute__((nothrow))
#define WEAK_ATTR					__attribute__((weak))
#define CONST_ATTR					__attribute__((const))
#define PURE_ATTR					__attribute__((pure))
#define YYASSUME(...)				/* if (!(__VA_ARGS__)) __builtin_unreachable() */
#define FORCEOPTIMIZE_OFF_START		
#define FORCEOPTIMIZE_OFF_END		
#define OPTIMIZE_OFF_ATTR			__attribute__((optnone))
#include <stdlib.h>
#elif defined(__GNUC__)
#define FORCEINLINE					inline
#define FORCEINLINE_ATTR			__attribute__((always_inline))
#define NOINLINE_ATTR				__attribute__((noinline))
#define NOTHROW_ATTR				__attribute__((nothrow))
#define WEAK_ATTR					__attribute__((weak))
#define CONST_ATTR					__attribute__((const))
#define PURE_ATTR					__attribute__((pure))
#define YYASSUME(...)				/* if (!(__VA_ARGS__)) __builtin_unreachable() */
#define FORCEOPTIMIZE_OFF_START		
#define FORCEOPTIMIZE_OFF_END		
#define OPTIMIZE_OFF_ATTR			__attribute__((optnone))
#include <stdlib.h>
#ifndef alloca
#include <alloca.h>
#endif
#elif defined(__clang__) && defined(_MSC_VER)
#define FORCEINLINE					__forceinline
#define FORCEINLINE_ATTR 
#define NOINLINE_ATTR				__declspec(noinline) __attribute__((noinline)) 
#define NOTHROW_ATTR				__declspec(nothrow)
#define WEAK_ATTR					__declspec(selectany) __attribute__((weak))
#define CONST_ATTR					__attribute__((const))
#define PURE_ATTR					__attribute__((pure))
#define YYASSUME(...)				/* if (!(__VA_ARGS__)) __builtin_unreachable() */
#define FORCEOPTIMIZE_OFF_START		
#define FORCEOPTIMIZE_OFF_END		
#define OPTIMIZE_OFF_ATTR 			__attribute__((optnone))
#include <malloc.h>
#elif	defined(_MSC_VER)
#define FORCEINLINE					__forceinline
#define FORCEINLINE_ATTR 
#define NOINLINE_ATTR				__declspec(noinline)			
#define NOTHROW_ATTR				__declspec(nothrow)
#define WEAK_ATTR					__declspec(selectany)
#define CONST_ATTR					
#define PURE_ATTR
#define YYASSUME(...)				/* if (!(__VA_ARGS__)) __assume(0) */
#define FORCEOPTIMIZE_OFF_START		__pragma(optimize("", off))
#define FORCEOPTIMIZE_OFF_END		__pragma(optimize("", on))
#define OPTIMIZE_OFF_ATTR 
#include <malloc.h>
#ifndef alloca
#define alloca						_alloca
#endif
#else
#define FORCEINLINE					inline
#define FORCEINLINE_ATTR 
#define NOINLINE_ATTR				
#define NOTHROW_ATTR
#define WEAK_ATTR	
#define CONST_ATTR
#define PURE_ATTR
#define YYASSUME(...)
#define FORCEOPTIMIZE_OFF_START
#define FORCEOPTIMIZE_OFF_END
#define OPTIMIZE_OFF_ATTR 
#endif




#ifndef __YOYO_TYPES_H__
typedef		int		int32;
typedef		long long int64;
struct YYRValue;
struct RValue;
struct CInstance;
class YYCEXTERN YYGMLException
{
public:
	char			m_object[16];

	YYGMLException(CInstance* _pSelf, CInstance* _pOther, const char* _pMessage, const char* _pLongMessage, const char* _filename, int _line, const char** ppStackTrace, int numLines);
	YYGMLException(const YYRValue& _val);
	const RValue&		GetExceptionObject() const;
};
#endif


#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

struct RValue;

static void* YYAlloc(int _size) {
	using MemoryManagerAlloc = void* __fastcall(int a1);
	auto oAlloc = *reinterpret_cast<MemoryManagerAlloc*>(MEM::GetAbsoluteAddress(MEM::PatternScan(nullptr, "E8 ? ? ? ? 48 89 03 89 6B"), 0x1));
	return oAlloc(_size);
};
static void* YYRealloc(void* _p, int _size) {
	using fOriginal = void* __fastcall(void*, int);
	auto oOriginal = reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "48 83 EC ? 48 63 D2"));
	return oOriginal(_p, _size);
};
static void YYFree(const void* _p) {
	using fOriginal = void __fastcall(const void*);
	auto oOriginal = *reinterpret_cast<fOriginal*>(MEM::GetAbsoluteAddress(MEM::PatternScan(nullptr, "E8 ? ? ? ? 90 48 8B C3 EB"), 0x1));
	return oOriginal(_p);
};
static const char* YYStrDup(const char* _pS) {
	using fOriginal = const char* __fastcall(const char*);
	auto oOriginal = *reinterpret_cast<fOriginal*>(MEM::GetAbsoluteAddress(MEM::PatternScan(nullptr, "E8 ? ? ? ? 49 89 06 B0"), 0x1));
	return oOriginal(_pS);
};

static int YYstrnlen(const char* _pS, int n) {
	using fOriginal = int __fastcall(const char*, int);
	auto oOriginal = reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "48 89 5C 24 ? 57 48 83 EC ? 48 63 DA 48 8B F9 4C 8B C3"));
	return oOriginal(_pS, n);
};
static void YYCreateString(RValue* _pVal, const char* _pS) {
	using fOriginal = void __fastcall(RValue*, const char*);
	auto oOriginal = *reinterpret_cast<fOriginal*>(MEM::GetAbsoluteAddress(MEM::PatternScan(nullptr, "E8 ? ? ? ? 2B FB"), 0x1));
	return oOriginal(_pVal, _pS);
};
static void YYSetString(RValue* _pVal, const char* _pS) {
	using fOriginal = void __fastcall(RValue*, const char*);
	auto oOriginal = *reinterpret_cast<fOriginal*>(MEM::GetAbsoluteAddress(MEM::PatternScan(nullptr, "E8 ? ? ? ? F2 41 0F 10 46"), 0x1));
	return oOriginal(_pVal, _pS);
};
static void YYConstString(RValue* _pVal, const char* _pS) {
	using fOriginal = void __fastcall(RValue*, const char*);
	auto oOriginal = reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 48 8B DA B9 ? ? ? ? E8 ? ? ? ? 48 89 44 24"));
	return oOriginal(_pVal, _pS);
};
static char* YYGetString(RValue* _pVal, int _pS) {
	using fOriginal = char* __fastcall(RValue*, int);
	auto oOriginal = *reinterpret_cast<fOriginal*>(MEM::GetAbsoluteAddress(MEM::PatternScan(nullptr, "E8 ? ? ? ? 48 63 8E"), 0x1));
	return oOriginal(_pVal, _pS);
};
static void YYStrFree(const char* _pS) {
	YYFree(_pS);
};
static bool FunctionFind(const char* name, int* idx) {
	using fOriginal = bool __fastcall(const char*, int*);
	auto oOriginal = reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? C7 02"));
	return oOriginal(name, idx);
}


#endif
