#include "hooks.h"
#include <stdio.h>
#include <iostream>
#include <dbghelp.h>
#include "../datatypes/YYGML.h"
#include "../utils/logging.h"
#include "../Drawing.h"
#include "../utils/api.h"
#include "../utils/GMLParser.h"
#pragma comment(lib, "dbghelp.lib")

bool H::Setup()
{
	L_PRINT(LOG_INFO) << "Initializing stuff";
	MH_STATUS status = MH_Initialize();
	if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
	{
		MessageBoxA(NULL, "Minhook init fail.", "YYC Toolbox", MB_ICONERROR | MB_OK);
		L_PRINT(LOG_ERROR) << "Failed to init minhook";
		return false;
	}
	L_PRINT(LOG_INFO) << "Minhook init done";
	return true;
}

void H::AddExceptionHandler() {
	AddVectoredExceptionHandler(1, ExceptionHandler);
}

bool H::HookErrorFuncs() {
	bool bSuccess = true;

	if (!hkYYError.Create(MEM::PatternScan(nullptr, "48 89 4C 24 ? 48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? 53 48 81 EC"), &YYError))
		bSuccess &= false;

	if (!hkVMError.Create(MEM::PatternScan(nullptr, "48 89 54 24 ? 4C 89 44 24 ? 4C 89 4C 24 ? 53 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 84 24 ? ? ? ? 80 3D ? ? ? ? ? 48 8B DA"), &VMError))
		bSuccess &= false;

	L_PRINT(LOG_INFO) << "Hooks created";

	return bSuccess;
}

void H::Destroy()
{
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);

	MH_Uninitialize();
}

std::string GetExceptionString(DWORD dwExceptionCode, DWORD nNumberOfArguments, const ULONG_PTR* lpArguments) {
	std::ostringstream oss;

	switch (dwExceptionCode) {
	case EXCEPTION_ACCESS_VIOLATION:
		oss << "Access Violation";
		if (nNumberOfArguments >= 2) {
			oss << std::format(" ({} at address 0x{:016X})",
				lpArguments[0] ? "WRITE" : "READ", lpArguments[1]);
		}
		break;

	case EXCEPTION_IN_PAGE_ERROR:
		oss << "Page Error";
		if (nNumberOfArguments >= 3) {
			oss << std::format(" ({} at address 0x{:016X}, NTSTATUS: 0x{:08X})",
				lpArguments[0] ? "WRITE" : "READ", lpArguments[1], lpArguments[2]);
		}
		break;

	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		oss << "Array Bounds Exceeded";
		break;

	case EXCEPTION_BREAKPOINT:
		oss << "Breakpoint";
		break;

	case EXCEPTION_DATATYPE_MISALIGNMENT:
		oss << "Datatype Misalignment";
		break;

	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		oss << "Float Division by Zero";
		break;

	case EXCEPTION_ILLEGAL_INSTRUCTION:
		oss << "Illegal Instruction";
		break;

	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		oss << "Integer Division by Zero";
		break;

	case EXCEPTION_PRIV_INSTRUCTION:
		oss << "Privileged Instruction";
		break;

	case EXCEPTION_STACK_OVERFLOW:
		oss << "Stack Overflow";
		break;

	default:
		oss << "Unexpected unknown exception";
		break;
	}

	return oss.str();
}

std::string GetStackTrace() {
	void* stack[64];
	USHORT frames = CaptureStackBackTrace(0, 64, stack, nullptr);

	HANDLE process = GetCurrentProcess();

	static bool initialized = false;
	if (!initialized) {
		SymInitialize(process, nullptr, TRUE);
		initialized = true;
	}

	std::ostringstream oss;
	for (USHORT i = 0; i < frames; i++) {
		DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);

		char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = { 0 };
		SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(buffer);
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;

		DWORD64 displacement = 0;
		if (SymFromAddr(process, address, &displacement, symbol)) {
			oss << std::format("0x{:016X} - {} + 0x{:X}", address, symbol->Name, static_cast<DWORD>(displacement));
		}
		else {
			oss << std::format("0x{:016X} - [?]", address);
		}

		IMAGEHLP_LINE64 lineInfo = { 0 };
		lineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		DWORD displacementLine = 0;

		if (SymGetLineFromAddr64(process, address, &displacementLine, &lineInfo)) {
			oss << std::format(" ({}:{} + 0x{:X})", lineInfo.FileName, lineInfo.LineNumber, displacementLine);
		}

		oss << "\n";
	}

	return oss.str();
}

LONG WINAPI H::ExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo) {
	DWORD dwExceptionCode = ExceptionInfo->ExceptionRecord->ExceptionCode;
	DWORD dwExceptionFlags = ExceptionInfo->ExceptionRecord->ExceptionFlags;
	DWORD nNumberOfArguments = ExceptionInfo->ExceptionRecord->NumberParameters;
	uintptr_t dwExceptionAddress = reinterpret_cast<uintptr_t>(ExceptionInfo->ExceptionRecord->ExceptionAddress);
	ULONG_PTR* lpArguments = ExceptionInfo->ExceptionRecord->ExceptionInformation;

	if (dwExceptionCode == 0xE06D7363) // C++ Exception
		return EXCEPTION_CONTINUE_SEARCH;

	std::string exceptionMessage = GetExceptionString(dwExceptionCode, nNumberOfArguments, lpArguments);
	std::string stackTrace = GetStackTrace();

	std::string msg = std::format(R""""(YYC Toolbox has crashed!
Address: 0x{:016X}
Exception: {} (Code: 0x{:08X})
Stack Trace:
{}

Please open a ticket and copy this error (CTRL + C), then provide context on what you were doing.
Press Cancel to terminate the program. Press Retry if you want Toolbox to try and recover from this exception.
Warning: Proceeding from a hard-crash might result in undefined behavior and/or data corruption.)"""",
dwExceptionAddress, exceptionMessage, dwExceptionCode, stackTrace);

	int result = MessageBoxA(NULL, msg.c_str(), "YYC Toolbox - Goodbye!", MB_RETRYCANCEL | MB_ICONERROR | MB_DEFBUTTON2);
	if (result == IDRETRY)
	{
		ExceptionInfo->ContextRecord->Rip += 1;
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}


int YYErrors = 0;
bool showYYErrors = true;
void H::YYError(const char* format, ...)
{
	if (!showYYErrors)
		return;
	va_list args;

	va_start(args, format);
	char formatted[1024];
	vsprintf(formatted, format, args);
	MessageBoxA(NULL, formatted, "YYC Toolbox - Caught YYC error", MB_OK);
	if (YYErrors == 5) {
		const int result = MessageBoxA(NULL, "There were 5 errors in a row. Would you like to suppress all YYC errors from now on?", "YYC Toolbox - Question", MB_YESNO | MB_ICONQUESTION);
		showYYErrors = result != IDYES;
	}
	YYErrors++;
}

int VMErrors = 0;
bool showVMErrors = true;
void H::VMError(VMExec* ctx, const char* format, ...)
{
	if (!showVMErrors)
		return;
	va_list args;

	va_start(args, format);
	char formatted[1024];
	vsprintf(formatted, format, args);
	MessageBoxA(NULL, formatted, "YYC Toolbox - Caught VM error", MB_OK);
	if (VMErrors == 5) {
		const int result = MessageBoxA(NULL, "There were 5 errors in a row. Would you like to suppress all VM errors from now on?", "YYC Toolbox - Question", MB_YESNO | MB_ICONQUESTION);
		showVMErrors = result != IDYES;
	}
	VMErrors++;
}

// Event/Code hooking!!
typedef	void (*PFUNC_CODE)(CInstance* _pSelf, CInstance* _pOther, YYRValue& _result, int _count, YYRValue** _args);
struct HookedCodeFunction {
	CBaseHookObject<PFUNC_CODE> hook;
	std::string executionCode;
	bool isLua;
	int order;
};

// Credits to archie for coming up with how to recognize which script was called
std::vector<HookedCodeFunction> codeHooks{};
std::unordered_map<std::string, HookedCodeFunction> eventHooks{};
void H::UniversalCodeHook(CInstance* pSelf, CInstance* pOther, YYRValue& _result, int _count, YYRValue** _args)
{
	std::uint64_t ret_addr = reinterpret_cast<std::uint64_t>(_ReturnAddress());
	std::uint8_t* call_instruction = reinterpret_cast<std::uint8_t*>(ret_addr - 5);
	std::uint64_t call_address = reinterpret_cast<std::uint64_t>(MEM::GetAbsoluteAddress(call_instruction, 0x1));
	for (auto& hook : codeHooks) {
		auto& _hook = hook.hook;
		if (!_hook.IsHooked())
			continue;

		std::uint64_t base_addr = reinterpret_cast<std::uint64_t>(_hook.GetBaseAddress());
		if (base_addr != call_address)
			continue;

		int ctx = -1;
		if (pSelf && pSelf->m_Object)
			ctx = pSelf->m_Object->m_ID;

		const auto oOriginal = hook.hook.GetOriginal();
		if (hook.order == EHookExecutionOrder::ORDER_ORIGINAL_BEFORE)
			oOriginal(pSelf, pOther, _result, _count, _args);

		if (hook.isLua)
			API::ExecuteCode(hook.executionCode, ctx);
		else
			PARSER::ExecuteCode(hook.executionCode, ctx);

		if (hook.order == EHookExecutionOrder::ORDER_ORIGINAL_AFTER)
			oOriginal(pSelf, pOther, _result, _count, _args);
		break;
	}
}

bool H::ExecuteIt(CInstance* pSelf, CInstance* pOther, CCode* code, RValue* args)
{
	const auto oOriginal = hkExecuteIt.GetOriginal();
	if (!code)
		return oOriginal(pSelf, pOther, code, args);
	decltype(eventHooks)::iterator it = eventHooks.find(std::string(code->m_Name));
	if (it != eventHooks.end())
	{
		HookedCodeFunction hook = it->second;
		int ctx = -1;
		if (pSelf && pSelf->m_Object)
			ctx = pSelf->m_Object->m_ID;

		if (hook.order == EHookExecutionOrder::ORDER_ORIGINAL_BEFORE)
			oOriginal(pSelf, pOther, code, args);

		if (hook.isLua)
			API::ExecuteCode(hook.executionCode, ctx);
		else
			PARSER::ExecuteCode(hook.executionCode, ctx);

		if (hook.order == EHookExecutionOrder::ORDER_ORIGINAL_AFTER)
			oOriginal(pSelf, pOther, code, args);
		return true;
	}
	return oOriginal(pSelf, pOther, code, args);
}

bool H::AddUniversalHook(void* funcPtr, std::string code, bool isLua, int order, std::string name)
{
	bool bSuccess = true;
	if (!hkExecuteIt.IsHooked())
		if (!hkExecuteIt.Create(MEM::PatternScan(nullptr, "48 89 5C 24 ? 48 89 6C 24 ? 48 89 7C 24 ? 41 56 48 83 EC ? 80 3D"), &ExecuteIt)) {
			bSuccess &= false;
			L_PRINT(LOG_ERROR) << "Unable to create ExecuteIt hook. Object events will not work.";
		}

	decltype(eventHooks)::iterator it = eventHooks.find(name);
	if (it != eventHooks.end())
	{
		eventHooks.erase(it);
		L_PRINT(LOG_INFO) << "Erased already existing hook (unordered_map)";
	}
	size_t hooks = codeHooks.size();
	for (size_t i = 0; i < hooks; i++) {
		auto& hook = codeHooks[i];
		if (hook.hook.GetBaseAddress() == funcPtr) {
			if (!hook.hook.Remove()) {
				bSuccess &= false;
				L_PRINT(LOG_ERROR) << "Unable to restore / remove existing hook";
			}
			codeHooks.erase(codeHooks.begin() + i);
			L_PRINT(LOG_INFO) << "Erased existing hook (vector)";
			break;
		}
	}
	CBaseHookObject<decltype(&UniversalCodeHook)> new_hook{};
	if (!new_hook.Create(funcPtr, &UniversalCodeHook))
		return false;
	HookedCodeFunction hook_info{};
	hook_info.hook = new_hook;
	hook_info.executionCode = code;
	hook_info.isLua = isLua;
	hook_info.order = order;
	codeHooks.push_back(hook_info);
	if (name.rfind("gml_Object_", 0) == 0)
		eventHooks.insert({ name, hook_info });
	L_PRINT(LOG_INFO) << "Created new hook";
	return bSuccess;
}

bool H::RemoveUniversalHook(void* funcPtr, std::string name)
{
	bool bSuccess = true;

	decltype(eventHooks)::iterator it = eventHooks.find(name);
	if (it != eventHooks.end())
	{
		eventHooks.erase(it);
		L_PRINT(LOG_INFO) << "Erased hook (unordered_map)";
	}
	size_t hooks = codeHooks.size();
	for (size_t i = 0; i < hooks; i++) {
		auto& hook = codeHooks[i];
		if (hook.hook.GetBaseAddress() == funcPtr) {
			if (!hook.hook.Remove()) {
				bSuccess &= false;
				L_PRINT(LOG_ERROR) << "Unable to restore / remove hook";
			}
			codeHooks.erase(codeHooks.begin() + i);
			L_PRINT(LOG_INFO) << "Erased hook (vector)";
			break;
		}
	}

	return bSuccess;
}
