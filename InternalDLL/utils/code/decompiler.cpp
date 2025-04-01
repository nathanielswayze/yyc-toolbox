#include "decompiler.h"
#include "../RemoteAPI.h"
#include "../api.h"
#include <cstdint>

DecompilerResult* DECOMPILER::DecompileFn(DecompilerInput* input)
{
	DecompilerResult* result = new DecompilerResult();

	SLLVMVars* vars = API::GetVariables();
	if (vars == nullptr || vars->pYYStackTrace == nullptr)
		return result;

	auto pInputFunc = reinterpret_cast<std::uint8_t*>(input->pFunc);
	if (pInputFunc == nullptr)
		return result;

	std::vector<ZydisDisassembledInstruction> instructions = REMOTE::DisassembleFn(pInputFunc);
	// Insert decompiler here

	return result;
}
