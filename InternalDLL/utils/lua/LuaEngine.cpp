#include "LuaEngine.h"
#include <iostream>
#include "../api.h"
#include "../logging.h"
#include "../GMLParser.h"
#include "../../Drawing.h"

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

void PrintTable(lua_State* L, int idx, std::stringstream& ss, int depth = 0) {
	if (depth > 3) {
		ss << "{...}";
		return;
	}

	ss << "{";
	bool first = true;

	lua_pushvalue(L, idx);
	lua_pushnil(L);

	while (lua_next(L, -2) != 0) {
		if (!first) ss << ", ";
		first = false;

		if (lua_type(L, -2) == LUA_TSTRING) {
			ss << lua_tostring(L, -2) << " = ";
		}
		else {
			ss << "[" << lua_tonumber(L, -2) << "] = ";
		}

		int type = lua_type(L, -1);
		switch (type) {
		case LUA_TTABLE:
			PrintTable(L, lua_gettop(L), ss, depth + 1);
			break;
		case LUA_TSTRING:
			ss << "\"" << lua_tostring(L, -1) << "\"";
			break;
		case LUA_TNUMBER:
			ss << lua_tonumber(L, -1);
			break;
		case LUA_TBOOLEAN:
			ss << (lua_toboolean(L, -1) ? "true" : "false");
			break;
		default:
		{
			const void* p = lua_topointer(L, -1);
			ss << "<" << lua_typename(L, type) << " at 0x" << std::hex << p << ">";
		}
		break;
		}

		lua_pop(L, 1);
	}

	lua_pop(L, 1);
	ss << "}";
}

void PrintOverride(lua_State* L) {
	int n = lua_gettop(L);
	std::stringstream ss;

	for (int i = 1; i <= n; i++) {
		if (i > 1) ss << " ";

		int type = lua_type(L, i);
		switch (type) {
		case LUA_TNIL:
			ss << "nil";
			break;
		case LUA_TBOOLEAN:
			ss << (lua_toboolean(L, i) ? "true" : "false");
			break;
		case LUA_TNUMBER:
			ss << lua_tonumber(L, i);
			break;
		case LUA_TSTRING:
			ss << lua_tostring(L, i);
			break;
		case LUA_TTABLE:
			PrintTable(L, i, ss);
			break;
		default:
		{
			const void* p = lua_topointer(L, -1);
			ss << "<" << lua_typename(L, type) << " at 0x" << std::hex << p << ">";
		}
		break;
		}
	}

	L_PRINT(LOG_INFO) << ss.str();
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

void IEngineInterface::EvaluateGML(const char* code) {
	PARSER::ExecuteCode(code);
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
		.beginClass<IObjectInstance>("ObjectInstance")
		.addFunction("__index", &IObjectInstance::Index)
		.addFunction("__newindex", &IObjectInstance::NewIndex)
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
		.addFunction("GetInstances", &IObjectInterface::GetInstances)
		.addFunction("__newindex", &IObjectInterface::NewIndex)
		.endClass()
		.beginClass<IRValue>("RValue")
		.addProperty("type", &IRValue::m_Kind, false)
		.addFunction("toBool", &IRValue::AsBool)
		.addFunction("toString", &IRValue::GetString)
		.addFunction("toReal", &IRValue::GetValue)
		.addFunction("stringify", &IRValue::ToString)
		.addFunction("__add", &IRValue::operator+)
		.addFunction("__sub", &IRValue::operator-)
		.addFunction("__mul", &IRValue::operator*)
		.addFunction("__div", &IRValue::operator/)
		.endClass()
		.addFunction("GetEngine", &GetEngine)
		.addFunction("CallFunction", &CallFunction)
		.endNamespace();
}