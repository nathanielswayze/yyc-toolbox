#pragma once
#define GM_FASTCALL __fastcall

// used: chookobject
#include "../dependencies/detourhook.h"
#include <string>

struct CInstance;
struct RValue;
struct YYRValue;
struct VMExec;
struct CCode;

namespace H
{
	bool Setup();
	void AddExceptionHandler();
	bool HookErrorFuncs();
	void Destroy();

	// Toolbox Hooks
	LONG __stdcall ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo);

	// YYC Hooks
	void YYError(const char* format, ...);
	void VMError(VMExec* ctx, const char* format, ...);
	void UniversalCodeHook(CInstance* pSelf, CInstance* pOther, YYRValue& _result, int _count, YYRValue** _args);
	bool ExecuteIt(CInstance* pSelf, CInstance* pOther, CCode* code, RValue* args);
	bool AddUniversalHook(void* funcPtr, std::string code, bool isLua, int order, std::string name);
	bool RemoveUniversalHook(void* funcPtr, std::string name);

	inline CBaseHookObject<decltype(&YYError)> hkYYError = {};
	inline CBaseHookObject<decltype(&VMError)> hkVMError = {};
	inline CBaseHookObject<decltype(&ExecuteIt)> hkExecuteIt = {};
}