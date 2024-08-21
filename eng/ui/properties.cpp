#include "properties.h"

#include "imgui.h"

namespace ImGui {

// taken from misc/cpp/imgui_stdlib.cpp
struct InputTextCallback_UserData
{
	std::string *Str;
	ImGuiInputTextCallback  ChainCallback;
	void *ChainCallbackUserData;
};

static int InputTextCallback(ImGuiInputTextCallbackData *data)
{
	InputTextCallback_UserData *user_data = (InputTextCallback_UserData *)data->UserData;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		// Resize string callback
		// If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
		std::string *str = user_data->Str;
		IM_ASSERT(data->Buf == str->c_str());
		str->resize(data->BufTextLen);
		data->Buf = (char *)str->c_str();
	} else if (user_data->ChainCallback)
	{
		// Forward to user callback, if any
		data->UserData = user_data->ChainCallbackUserData;
		return user_data->ChainCallback(data);
	}
	return 0;
}

bool InputText(const char *label, std::string *str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr)
{
	IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallback_UserData cb_user_data;
	cb_user_data.Str = str;
	cb_user_data.ChainCallback = callback;
	cb_user_data.ChainCallbackUserData = user_data;
	return InputText(label, (char *)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
}

}

namespace eng {

void DrawProperty(std::string_view name, utl::AnyRef obj, int depth)
{
	using FnDraw = void(*)(std::string_view &name, utl::AnyRef &obj);
	static std::unordered_map<utl::TypeInfo const *, FnDraw> s_type2draw {
		{ utl::TypeInfo::Get<float>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputFloat(name.data(), obj.Get<float>());
		} },
		{ utl::TypeInfo::Get<double>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputDouble(name.data(), obj.Get<double>());
		} },
		{ utl::TypeInfo::Get<int8_t>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputScalar(name.data(), ImGuiDataType_S8, obj.Get<int8_t>());
		} },
		{ utl::TypeInfo::Get<uint8_t>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputScalar(name.data(), ImGuiDataType_U8, obj.Get<uint8_t>());
		} },
		{ utl::TypeInfo::Get<int16_t>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputScalar(name.data(), ImGuiDataType_S16, obj.Get<int16_t>());
		} },
		{ utl::TypeInfo::Get<uint16_t>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputScalar(name.data(), ImGuiDataType_U16, obj.Get<uint16_t>());
		} },
		{ utl::TypeInfo::Get<int32_t>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputScalar(name.data(), ImGuiDataType_S32, obj.Get<int32_t>());
		} },
		{ utl::TypeInfo::Get<uint32_t>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputScalar(name.data(), ImGuiDataType_U32, obj.Get<uint32_t>());
		} },
		{ utl::TypeInfo::Get<int64_t>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputScalar(name.data(), ImGuiDataType_S64, obj.Get<int64_t>());
		} },
		{ utl::TypeInfo::Get<uint64_t>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::InputScalar(name.data(), ImGuiDataType_U64, obj.Get<uint64_t>());
		} },
		{ utl::TypeInfo::Get<bool>(), [](std::string_view &name, utl::AnyRef &obj) {
			ImGui::Checkbox(name.data(), obj.Get<bool>());
		} },
		{ utl::TypeInfo::Get<glm::vec3>(), [](std::string_view &name, utl::AnyRef &obj) {
			glm::vec3 *v = obj.Get<glm::vec3>();
			ImGui::InputFloat3(name.data(), &(*v)[0]);
		} },
		{ utl::TypeInfo::Get<std::string>(), [](std::string_view &name, utl::AnyRef &obj) {
			std::string *str = obj.Get<std::string>();
			ImGui::InputText(name.data(), str);
		} },
	};

	if (auto it = s_type2draw.find(obj._type); it != s_type2draw.end()) {
		it->second(name, obj);
	} else if (obj._type->_isPointer) {
		utl::AnyRef pointsTo = obj.GetPointsTo();
		if (pointsTo) {
			DrawProperty(name, obj.GetPointsTo(), depth);
		} else {
			ImGui::Text("%s: %s = <<Empty>>", name.data(), obj._type->_name.c_str());
		}
	} else if (obj._type->_isClass) {
		if (ImGui::TreeNodeEx(obj._instance, depth < 8 ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None, "%s: %s", name.data(), obj._type->_name.c_str())) {
			obj.ForMembers([depth](std::string_view memberName, utl::AnyRef refMember) {
				DrawProperty(memberName, refMember, depth + 1);
				return utl::Enum::Continue;
			});
			ImGui::TreePop();
		}
	} else if (obj._type->_isArray) {
		if (ImGui::TreeNodeEx(obj._instance, (depth < 8 && obj.GetArraySize() <= 16) ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None, "%s: %s", name.data(), obj._type->_name.c_str(), ImGuiWindowFlags_None)) {
			for (size_t i = 0; i < obj.GetArraySize(); ++i) {
				std::string indName = utl::strPrintf("%s[%zu]", name.data(), i);
				DrawProperty(indName, obj.GetArrayElement(i), depth + 1);
			}
			ImGui::TreePop();
		}
	} else {
		ImGui::Text("%s: %s", name.data(), obj._type->_name.c_str());
	}
}

bool DrawPropertiesWindow(std::string_view name, utl::AnyRef obj)
{
	ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_None);
	//ImGui::SetWindowPos(ImVec2(50, 50), ImGuiCond_Once);
	ImGui::SetWindowSize(ImVec2(400, 600), ImGuiCond_Once);

	if (obj) {
		DrawProperty(name, obj, 0);
	}

	bool focused = ImGui::IsWindowFocused();

	ImGui::End();

	return focused;
}

}