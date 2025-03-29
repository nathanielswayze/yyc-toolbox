#include "YYGML.h"
#include "../defines.h"
#include "../utils/api.h"

bool Variable_GetBuiltIn_Direct(YYObjectBase* inst, int var_ind, int array_ind, RValue* val) {
	// Basically rebuilt??
	val->kind = VALUE_UNSET;
	if (var_ind > 9999)
		return false;

	auto builtin_routine = reinterpret_cast<RVariableRoutine_t*>(MEM::GetAbsoluteAddress(MEM::PatternScan(nullptr, "4C 8D 35 ? ? ? ? 49 03 DE"), 0x3));
	RVariableRoutine_t rout = builtin_routine[var_ind];
	if (YY_ISINVALIDPTR(rout.GetVar_Func))
		return false;

	return rout.GetVar_Func(inst, array_ind, val);
	//using fOriginal = bool __fastcall(YYObjectBase*, int, int, RValue*);
	//auto oOriginal = reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "41 C7 41 ? ? ? ? ? 45 8B D0"));
	//return oOriginal(inst, var_ind, array_ind, val);
};
bool Variable_GetValue_Direct(YYObjectBase* inst, int var_ind, int array_ind, RValue* val, bool fPrepareArray, bool fPartOfPop) {
	using fOriginal = bool __fastcall(YYObjectBase*, int, int, RValue*, bool, bool);
	auto oOriginal = reinterpret_cast<fOriginal*>(MEM::GetAbsoluteAddress(MEM::PatternScan(nullptr, "E9 ? ? ? ? 85 C9 78 ? 41 81 FA"), 0x1));
	return oOriginal(inst, var_ind, array_ind, val, fPrepareArray, fPartOfPop);
};
int Variable_BuiltIn_Find(const char* Src) {
	using fOriginal = int __fastcall(const char*);
	return reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "48 89 5C 24 ? 57 48 83 EC ? 48 8B 3D ? ? ? ? E8 ? ? ? ? 44 8B 4F"))(Src);
}

int64 INT64_RValue(const RValue* _pV) {
	using fOriginal = int64 __fastcall(const RValue*);
	return reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "48 89 5C 24 ? 57 48 83 EC ? 48 8B F9 33 DB 8B 49"))(_pV);
}

int32 INT32_RValue(const RValue* _pV) {
	using fOriginal = int32 __fastcall(const RValue*);
	return reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "40 53 48 83 EC ? 44 8B 49 ? 45 33 C0"))(_pV);
}

bool BOOL_RValue(const RValue* _pV) {
	using fOriginal = bool __fastcall(const RValue*);
	return reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "40 53 48 83 EC ? 44 8B 49 ? 45 32 C0"))(_pV);
}

double REAL_RValue_Ex(const RValue* _pV) {
	using fOriginal = bool __fastcall(const RValue*);
	return reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "40 53 48 83 EC ? 8B 41 ? 0F 57 C0"))(_pV);
}

CInstance* YYGML_FindInstance(int _ind) {
	using fOriginal = CInstance* __fastcall(int);
	return reinterpret_cast<fOriginal*>(MEM::PatternScan(nullptr, "85 C9 78 ? 48 63 15"))(_ind);
}

bool g_fCopyOnWriteEnabled = false;
SLLVMVars* g_pLLVMVars = nullptr;
int64 g_CurrentArrayOwner = 0;

CInstanceInternal& CInstance::GetMembers()
{
	RValue inst_id{};
	if (!Variable_GetBuiltIn_Direct(this, API::GetVariableId("id"), INT_MIN, &inst_id))
		return this->SequenceInstanceOnly.Members;

	int32_t self_id = INT32_RValue(&inst_id);
	if (this->MembersOnly.Members.m_ID == self_id)
		return this->MembersOnly.Members;

	if (this->SequenceInstanceOnly.Members.m_ID == self_id)
		return this->SequenceInstanceOnly.Members;

	if (this->WithSkeletonMask.Members.m_ID == self_id)
		return this->WithSkeletonMask.Members;

	return this->SequenceInstanceOnly.Members;
}
