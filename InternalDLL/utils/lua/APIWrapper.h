#pragma once
#include "../../defines.h"
#include "../logging.h"
#include "../api.h"
#include "../crt.h"
#include <vector>
#include <format>

class IRValue {
public:
	RValue m_Val;
	unsigned int m_Kind;
	IRValue() {
		m_Val = RValue{};
		m_Val.v64 = 0;
		m_Val.flags = 0;
		m_Val.kind = VALUE_UNDEFINED;
		m_Kind = m_Val.kind;
	}
	IRValue(RValue orig) {
		m_Val = orig;
		m_Kind = m_Val.kind;
	}

	const char* GetString() const { return m_Val.GetString(); }
	std::string ToString() const { return API::RValueToString(m_Val); }
	bool AsBool() const { return m_Val.val != 0; }
	double GetValue() const { return m_Val.val; }

	IRValue operator+(const IRValue& other) const {
		IRValue result{};
		if ((m_Val.kind == VALUE_REAL || m_Val.kind == VALUE_BOOL) && (other.m_Val.kind == VALUE_REAL || other.m_Val.kind == VALUE_BOOL))
		{
			result.m_Val.kind = VALUE_REAL;
			result.m_Val.val = m_Val.val + other.m_Val.val;
			return result;
		}
		if (m_Val.kind == VALUE_STRING && other.m_Val.kind == VALUE_STRING) {
			const char* str1 = m_Val.GetString();
			const char* str2 = other.m_Val.GetString();
			size_t end_size = CRT::StringLength(str1) + CRT::StringLength(str2) + 1;
			char* end_str = new char[end_size];
			CRT::StringCopy(end_str, str1);
			CRT::StringCat(end_str, str2);
			YYCreateString(&result.m_Val, end_str);
			return result;
		}
		return result;
	}

	IRValue operator-(const IRValue& other) const {
		IRValue result{};
		if ((m_Val.kind == VALUE_REAL || m_Val.kind == VALUE_BOOL) && (other.m_Val.kind == VALUE_REAL || other.m_Val.kind == VALUE_BOOL))
		{
			result.m_Val.kind = VALUE_REAL;
			result.m_Val.val = m_Val.val - other.m_Val.val;
		}
		return result;
	}

	IRValue operator*(const IRValue& other) const {
		IRValue result{};
		if ((m_Val.kind == VALUE_REAL || m_Val.kind == VALUE_BOOL) && (other.m_Val.kind == VALUE_REAL || other.m_Val.kind == VALUE_BOOL))
		{
			result.m_Val.kind = VALUE_REAL;
			result.m_Val.val = m_Val.val * other.m_Val.val;
			return result;
		}
		if (m_Val.kind == VALUE_STRING && other.m_Val.kind == VALUE_REAL) {
			const char* str1 = m_Val.GetString();
			size_t end_size = CRT::StringLength(str1) * other.m_Val.val + 1;
			if (other.m_Val.val < 0 || end_size <= CRT::StringLength(str1))
			{
				YYCreateString(&result.m_Val, "");
				return result;
			}
			char* end_str = new char[end_size];
			CRT::StringCopy(end_str, str1);
			for (int i = 0; i < other.m_Val.val - 1; i++) {
				CRT::StringCat(end_str, str1);
			}
			YYCreateString(&result.m_Val, end_str);
			return result;
		}
		return result;
	}

	IRValue operator/(const IRValue& other) const {
		IRValue result{};
		if ((m_Val.kind == VALUE_REAL || m_Val.kind == VALUE_BOOL) && (other.m_Val.kind == VALUE_REAL || other.m_Val.kind == VALUE_BOOL))
		{
			result.m_Val.kind = VALUE_REAL;
			result.m_Val.val = m_Val.val / other.m_Val.val;
		}
		return result;
	}
};

class IObjectInstance {
private:
	CInstance* m_Original;
	CInstanceInternal m_Internal;
public:
	int m_ID;
	IObjectInstance() {
		m_Original = nullptr;
	}

	IObjectInstance(CInstance* inst) {
		m_Original = inst;
		m_Internal = inst->GetMembers();
		m_ID = m_Internal.m_ID;
	}

	IRValue Index(const char* name) {
		RValue instanceId{};
		instanceId.kind = VALUE_REAL;
		instanceId.val = m_ID;
		RValue varName{};
		YYCreateString(&varName, name);
		YYRValue var = API::CallFunction("variable_instance_get", std::vector<YYRValue> { instanceId, varName });
		return IRValue(var);
	}

	void NewIndex(const char* name, IRValue* val) {
		RValue instanceId{};
		instanceId.kind = VALUE_REAL;
		instanceId.val = m_ID;
		RValue varName{};
		YYCreateString(&varName, name);
		API::CallFunction("variable_instance_set", std::vector<YYRValue> { instanceId, varName, val->m_Val });
	}
};

class IObjectInterface {
private:
	CObjectGM* m_Original;
public:
	const char* m_Name;
	IObjectInterface* m_ParentObject;
	CPhysicsDataGM m_PhysicsData;
	uint32_t m_Flags;
	int32_t m_SpriteIndex;
	int32_t m_Depth;
	int32_t m_Parent;
	int32_t m_Mask;
	int32_t m_ID;

	IObjectInterface() {
		m_Name = "mr uninitialized";
		m_ParentObject = nullptr;
		m_PhysicsData = CPhysicsDataGM{};
		m_Flags = 0;
		m_SpriteIndex = 0;
		m_Depth = 0;
		m_Parent = 0;
		m_Mask = 0;
		m_ID = 0;
		m_Original = nullptr;
	}
#pragma warning( push )
#pragma warning( disable : 26495 )
	IObjectInterface(CObjectGM* obj) {
		if (YY_ISINVALIDPTR(obj))
			return;
		m_Name = obj->m_Name;
		m_ParentObject = new IObjectInterface(obj->m_ParentObject);
		m_PhysicsData = obj->m_PhysicsData;
		m_Flags = obj->m_Flags;
		m_SpriteIndex = obj->m_SpriteIndex;
		m_Depth = obj->m_Depth;
		m_Parent = obj->m_Parent;
		m_Mask = obj->m_Mask;
		m_ID = obj->m_ID;
		m_Original = obj;
	}
#pragma warning( pop )

	const char* GetName() const { return m_Name; }
	void SetName(const char* name) { m_Original->m_Name = CRT::PreserveString(name); }
	int GetSprite() const { return m_SpriteIndex; }
	void SetSprite(int sprite) { m_Original->m_SpriteIndex = sprite; }
	int GetDepth() const { return m_Depth; }
	void SetDepth(int depth) { m_Original->m_Depth = depth; }

	void Spawn(int x, int y, std::optional<std::string> layer, std::optional<std::string> room) {
		if (room.has_value() && !room.value().empty())
		{
			API::CallFunction("room_instance_add", { room.value(), std::to_string(x), std::to_string(y), std::to_string(m_Original->m_ID) });
			return;
		}
		std::string layer_converted = "0";
		if (layer.has_value() && !layer.value().empty())
			layer_converted = std::format("\"{}\"", layer.value());
		API::CallFunction("instance_create_layer", {std::to_string(x), std::to_string(y), layer_converted, std::to_string(m_Original->m_ID)});
	}

	void Destroy() {
		API::CallFunction("instance_destroy", { std::to_string(m_Original->m_ID) });
	}

	std::vector<IObjectInstance> GetInstances() {
		std::vector<IObjectInstance> result{};
		double instances = API::CallFunction("instance_number", { std::to_string(m_Original->m_ID) }).asReal();

		for (double i = 0; i < instances; i++) {
			YYRValue instance = API::CallFunction("instance_find", { std::to_string(m_Original->m_ID), std::to_string(i) });
			CInstance* pInstance = YYGML_FindInstance(instance);
			if (!YY_ISINVALIDPTR(pInstance))
				result.push_back(pInstance);
		}

		return result;
	}

	void NewIndex(const char* name, IRValue* val) {
		auto instances = this->GetInstances();
		for (auto& instance : instances) {
			instance.NewIndex(name, val);
		}
	}
};

class IEngineInterface
{
public:
	IEngineInterface() {}
	int ResourceIdByName(const char* name) {
		return API::GetResourceIdByName(name);
	}
	IObjectInterface ObjectById(int id) {
		CObjectGM* obj = API::GetObjectById(id);
		if (YY_ISINVALIDPTR(obj))
			return IObjectInterface{};
		return IObjectInterface(obj);
	}
	IObjectInterface ObjectByName(const char* name) {
		int id = ResourceIdByName(name);
		return ObjectById(id);
	}
	void EvaluateGML(const char* code);
};