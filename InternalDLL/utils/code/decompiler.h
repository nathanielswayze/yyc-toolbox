#pragma once

struct DecompilerInput {
	void* pFunc;
	int nType;
};

struct DecompilerResult {
	bool bSuccess;
	float fConfidence;
	char* pCode;
};

namespace DECOMPILER {
	DecompilerResult* DecompileFn(DecompilerInput* input);
}