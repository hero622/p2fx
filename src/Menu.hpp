#pragma once
#include "Variable.hpp"

#include "Modules/Engine.hpp"

#include "../lib/imgui/imgui.h"

namespace Menu {
	inline bool g_shouldDraw = false;

	void Draw();
	void Init();
};

namespace CImGui {
	inline void Checkbox(const char *label, const char *var, int onvalue = 1) {
		bool val = Variable(var).GetInt() == onvalue ? true : false;
		ImGui::Checkbox(label, &val);
		Variable(var).SetValue(val ? onvalue : 0);
	}

	inline void CheckboxInv(const char *label, const char *var, int onvalue = 1) {
		bool val = Variable(var).GetInt() == onvalue ? false : true;
		ImGui::Checkbox(label, &val);
		Variable(var).SetValue(val ? 0 : onvalue);
	}

	inline void Slider(const char *label, const char *var, int min, int max, const char *format = "%d") {
		int val = std::clamp(Variable(var).GetInt(), min, max);
		ImGui::SliderInt(label, &val, min, max, format);
		Variable(var).SetValue(val);
	}

	inline void Sliderf(const char *label, const char *var, float min, float max, const char *format = "%.2f") {
		float val = std::clamp(Variable(var).GetFloat(), min, max);
		ImGui::SliderFloat(label, &val, min, max, format);
		Variable(var).SetValue(val);
	}

	inline void Colorpicker3(const char *label, const char *var) {
		auto val = Utils::GetColor(Variable(var).GetString());
		float col[3] = {val->r / 255.0f, val->g / 255.0f, val->b / 255.0f};
		ImGui::ColorEdit3(label, col, ImGuiColorEditFlags_NoInputs);
		Variable(var).SetValue(Utils::ssprintf("%.0f %.0f %.0f", col[0] * 255.0f, col[1] * 255.0f, col[2] * 255.0f).c_str());
	}

	inline void Combo(const char *label, const char *var, const char *items) {
		int val = Variable(var).GetInt();
		ImGui::Combo(label, &val, items);
		Variable(var).SetValue(val);
	}

	inline void Button(const char *label, const char *cmd, const ImVec2 &size = ImVec2(0, 0)) {
		if (ImGui::Button(label, size)) {
			engine->ExecuteCommand(cmd);
		}
	}
}