#include "LuaEngine.h"
#include <iostream>
#include "../logging.h"
#include "../../Drawing.h"
#include "../api.h"

LuaEngine* g_pLuaEngine = new LuaEngine();

lua_State* LuaEngine::L()
{
	return m_L;
}

void LuaEngine::report_errors(int state)
{
	if (state)
	{
		const char* err = lua_tostring(m_L, -1);
		L_PRINT(LOG_ERROR) << "LUA ERR: " << err;
		ImGuiToast toast(ImGuiToastType::Error);
		toast.setTitle("Lua Error");
		toast.setContent(err);
		ImGui::InsertNotification(toast);
		lua_pop(m_L, 1);
		Drawing::bErrorOccurred = true;
	}
}

void LuaEngine::ExecuteFile(const char* file)
{
	if (YY_ISINVALIDPTR(file) || YY_ISINVALIDPTR(m_L))
	{
		L_PRINT(LOG_ERROR) << "LUA ERR: invalid file passed to script engine!";
		return;
	}

	int state = luaL_dofile(m_L, file);
	report_errors(state);
}


void LuaEngine::ExecuteString(const char* expression)
{
	if (YY_ISINVALIDPTR(expression) || YY_ISINVALIDPTR(m_L))
	{
		L_PRINT(LOG_ERROR) << "LUA ERR: invalid expression passed to script engine!";
		return;
	}

	int state = luaL_dostring(m_L, expression);
	report_errors(state);
}

void PrintOverride(const char* s, lua_State* L) {
	if (YY_ISINVALIDPTR(s))
	{
		L_PRINT(LOG_INFO) << "";
		return;
	}
	L_PRINT(LOG_INFO) << s;
}

IEngineInterface GetEngine()
{
	static IEngineInterface engine{};
	return engine;
}

IRValue CallFunction(lua_State* L) {
	const char* FunctionName = lua_tostring(L, 1);
	luaL_argcheck(L, !YY_ISINVALIDPTR(FunctionName), 1, "function name expected");
	std::vector<YYRValue> args;
	for (int i = 2; i <= lua_gettop(L); i++) {
		luaL_argcheck(L, lua_isuserdata(L, i), i, "RValue expected");
		IRValue* value = luabridge::Stack<IRValue*>::get(L, i);
		args.push_back(value->m_Val);
	}
	YYRValue ret = API::CallFunction(FunctionName, args);
	return IRValue(ret);
}

IRValue ConvertRValue(lua_State* L) {
	int stack_size = lua_gettop(L);
	if (stack_size < 1)
		return IRValue{};
	IRValue result{};
	if (stack_size > 1) {
		result.m_Val.pRefArray = new RefDynamicArrayOfRValue();
		result.m_Val.pRefArray->pArray = new RValue[stack_size];
		result.m_Val.pRefArray->length = stack_size;
		result.m_Val.kind = VALUE_ARRAY;
		for (int i = 1; i <= stack_size; i++) {
			switch (lua_type(L, i)) {
			case LUA_TNUMBER: {
				RValue tmp{};
				tmp.kind = VALUE_REAL;
				tmp.val = lua_tonumber(L, i);
				result.m_Val.pRefArray->pArray[i - 1] = tmp;
				break;
			}
			case LUA_TBOOLEAN: {
				RValue tmp{};
				tmp.val = lua_toboolean(L, i);
				tmp.kind = VALUE_BOOL;
				result.m_Val.pRefArray->pArray[i - 1] = tmp;
				break;
			}
			case LUA_TSTRING: {
				RValue tmp{};
				YYCreateString(&tmp, lua_tostring(L, i));
				result.m_Val.pRefArray->pArray[i - 1] = tmp;
				break;
			}
			case LUA_TNIL: {
				RValue tmp{};
				tmp.v64 = 0;
				tmp.flags = 0;
				tmp.kind = VALUE_UNDEFINED;
				result.m_Val.pRefArray->pArray[i - 1] = tmp;
				break;
			}
			default:
				luaL_error(L, std::format("unacceptable argument #{} passed to RValue", i).c_str());
				break;
			}
		}
		return result;
	}
	switch (lua_type(L, 1)) {
	case LUA_TNUMBER:
		result.m_Val.kind = VALUE_REAL;
		result.m_Val.val = lua_tonumber(L, 1);
		break;
	case LUA_TBOOLEAN:
		result.m_Val.val = lua_toboolean(L, 1);
		result.m_Val.kind = VALUE_BOOL;
		break;
	case LUA_TSTRING:
		YYCreateString(&result.m_Val, lua_tostring(L, 1));
		break;
	case LUA_TNIL:
		// result is already undefined anyway
		break;
	default:
		luaL_error(L, "unacceptable argument passed to RValue");
		break;
	}
	return result;
}

void LuaEngine::Init()
{
	LOCKLUA();
	using namespace luabridge;
	getGlobalNamespace(L())
		.addFunction("print", &PrintOverride)
		.addFunction("RValue", &ConvertRValue)
		.beginNamespace("API")
		.beginClass<IEngineInterface>("EngineInterface")
		.addFunction("ResourceIdByName", &IEngineInterface::ResourceIdByName)
		.addFunction("GetObjectById", &IEngineInterface::ObjectById)
		.addFunction("GetObjectByName", &IEngineInterface::ObjectByName)
		.addFunction("Eval", &IEngineInterface::EvaluateGML)
		.endClass()
		.beginClass<IObjectInterface>("ObjectInterface")
		.addProperty("Name", &IObjectInterface::GetName, &IObjectInterface::SetName)
		.addProperty("Parent", &IObjectInterface::m_ParentObject, false)
		.addProperty("Flags", &IObjectInterface::m_Flags, false)
		.addProperty("Sprite", &IObjectInterface::GetSprite, &IObjectInterface::SetSprite)
		.addProperty("Depth", &IObjectInterface::GetDepth, &IObjectInterface::SetDepth)
		.addProperty("Mask", &IObjectInterface::m_Mask, false)
		.addProperty("ID", &IObjectInterface::m_ID, false)
		.addFunction("Spawn", &IObjectInterface::Spawn)
		.addFunction("Destroy", &IObjectInterface::Destroy)
		.endClass()
		.beginClass<IRValue>("RValue")
		.addProperty("type", &IRValue::m_Kind, false)
		.addFunction("toBool", &IRValue::AsBool)
		.addFunction("toString", &IRValue::GetString)
		.addFunction("toReal", &IRValue::GetValue)
		.addFunction("stringify", &IRValue::ToString)
		.addFunction("__add", &IRValue::operator+)
		.addFunction("__mul", &IRValue::operator*)
		.endClass()
		.addFunction("GetEngine", &GetEngine)
		.addFunction("CallFunction", &CallFunction)
		.endNamespace();
}