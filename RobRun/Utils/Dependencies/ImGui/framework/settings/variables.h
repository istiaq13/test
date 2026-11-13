#pragma once
#include <string>
#include <vector>
#include "../../imgui.h"
#include "../headers/flags.h"

enum class section_id
{
	visuals,
	settings,
};

struct config_t
{
	std::string name;
	std::string date;
	int index;
};

struct ID3D11ShaderResourceView;

class c_variables
{
public:
	struct
	{
		window_flags flags{ window_flags_no_saved_settings | window_flags_no_nav | window_flags_no_decoration | window_flags_no_scroll_with_mouse | window_flags_no_scrollbar | window_flags_no_background };
		ImVec2 size{ 865, 560 };

		ImVec2 padding{ 0, 0 };
		ImVec2 spacing{ 0, 0 };
		float shadow{ 0 };
		float border{ 0 };
		float rounding{ 12 };
		float scrollbar_rounding{ 5 };
		float scrollbar{ 10 };
	} window;

	struct
	{
		std::pair<section_id, std::vector<std::string>> sections_data{ section_id::visuals, { "D", "G" } };
		section_id active_section{ section_id::visuals };
		float section_alpha{ 1.f };

		std::vector<int> sub_section_data{ 0, 0 };
		std::vector<int> active_sub_section{ 0, 0 };
		float sub_section_alpha{ 0 };

		std::vector<std::string> section_name{ "Visuals", "Settings" };
		std::vector<std::string> section_sub
		{
			"esp, visuals",
			"configs, menu"
		};
		std::vector<std::vector<std::string>> sub_section_name{ {"Player"}, {"Settings"} };
		std::vector<std::string> section_desc
		{
			"Configure player ESP and visual indicators",
			"Customize configs and account settings"
		};

		float sidebar_width{ 175 };
		float topbar_height{ 50 };
		float padding{ 10 };
		float content_rounding{ 8 };
		float logo_height{ 72 };

		std::vector<config_t> configs;

		float dpi{ 1 };
		int stored_dpi{ 100 };
		bool dpi_changed{ true };

		bool use_animations{ true };
		float anim_speed{ 1 };

		float dimming_alpha{ 0 };

		ID3D11ShaderResourceView* logo_texture = nullptr;
		ID3D11ShaderResourceView* image_texture = nullptr;
	} gui;

	gui_style style;

};

inline std::unique_ptr<c_variables> var = std::make_unique<c_variables>();
