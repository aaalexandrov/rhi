#include "properties.h"

#include "imgui.h"

namespace eng {

void DrawProperty(std::string_view name, utl::AnyRef obj)
{
	if (obj._type == utl::TypeInfo::Get<float>()) {
		ImGui::InputFloat(name.data(), obj.Get<float>());
	} else if (obj._type == utl::TypeInfo::Get<double>()) {
		ImGui::InputDouble(name.data(), obj.Get<double>());
	} else if (obj._type == utl::TypeInfo::Get<int8_t>()) {
		ImGui::InputScalar(name.data(), ImGuiDataType_S8, obj.Get<int8_t>());
	} else if (obj._type == utl::TypeInfo::Get<uint8_t>()) {
		ImGui::InputScalar(name.data(), ImGuiDataType_U8, obj.Get<uint8_t>());
	} else if (obj._type == utl::TypeInfo::Get<int16_t>()) {
		ImGui::InputScalar(name.data(), ImGuiDataType_S16, obj.Get<int16_t>());
	} else if (obj._type == utl::TypeInfo::Get<uint16_t>()) {
		ImGui::InputScalar(name.data(), ImGuiDataType_U16, obj.Get<uint16_t>());
	} else if (obj._type == utl::TypeInfo::Get<int32_t>()) {
		ImGui::InputScalar(name.data(), ImGuiDataType_S32, obj.Get<int32_t>());
	} else if (obj._type == utl::TypeInfo::Get<uint32_t>()) {
		ImGui::InputScalar(name.data(), ImGuiDataType_U32, obj.Get<uint32_t>());
	} else if (obj._type == utl::TypeInfo::Get<int64_t>()) {
		ImGui::InputScalar(name.data(), ImGuiDataType_S64, obj.Get<int64_t>());
	} else if (obj._type == utl::TypeInfo::Get<uint64_t>()) {
		ImGui::InputScalar(name.data(), ImGuiDataType_U64, obj.Get<uint64_t>());
	} else if (obj._type == utl::TypeInfo::Get<bool>()) {
		ImGui::Checkbox(name.data(), obj.Get<bool>());
	} else if (obj._type == utl::TypeInfo::Get<std::string>()) {
		std::string *str = obj.Get<std::string>();
		char const *ss = str->c_str();
		std::string buf = *str;
		buf.reserve(256);
		if (ImGui::InputText(name.data(), (char *)buf.c_str(), buf.capacity()))
			*str = buf;
	} else if (obj._type->_isClass) {
		if (ImGui::CollapsingHeader(utl::strPrintf("%s: %s", name.data(), obj._type->_name.c_str()).c_str(), ImGuiTreeNodeFlags_None)) {
			obj.ForMembers([](std::string_view memberName, utl::AnyRef refMember) {
				DrawProperty(memberName, refMember);
				return utl::Enum::Continue;
			});
		}
	} else if (obj._type->_isArray) {
		if (ImGui::CollapsingHeader(utl::strPrintf("%s: %s", name.data(), obj._type->_name.c_str()).c_str(), ImGuiTreeNodeFlags_None)) {
			for (size_t i = 0; i < obj.GetArraySize(); ++i) {
				std::string indName = utl::strPrintf("%s[%u64]", name.data(), i);
				DrawProperty(indName, obj.GetArrayElement(i));
			}
		}
	} else {
		ImGui::Text(name.data());
	}
}

void DrawPropertiesWindow(std::string_view name, utl::AnyRef obj)
{
	ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_None);
	ImGui::SetWindowPos(ImVec2(50, 50), ImGuiCond_Once);
	ImGui::SetWindowSize(ImVec2(300, 300), ImGuiCond_Once);

	if (obj) {
		DrawProperty(name, obj);
	}

	ImGui::End();
}

}