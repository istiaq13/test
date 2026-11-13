#pragma once
#include "includes.h"
#include "../headers/config.h"
#include <mutex>

#define IMGUI_DEFINE_MATH_OPERATORS

struct widget_t
{
    ImGuiID id;
    bool value_changed;
    ImRect rect;
    ImVec2 size;
    bool visible;
    bool hovered;

    widget_t* operator->() {
        return this;
    }

    const widget_t* operator->() const {
        return this;
    }
};

class c_widgets
{
public:

    bool begin_child(std::string_view name, std::string_view button_text = "");
    bool begin_child(std::string_view name, std::string_view button1_text, std::string_view button2_text, bool* btn2_clicked);
    bool begin_child_ex(std::string_view name, float custom_width = 0.f, bool glowing_border = false, std::string_view button1_text = "", std::string_view button2_text = "", bool* btn2_clicked = nullptr);
    bool begin_child_toggle(std::string_view name, bool* header_switch, const char* switch_label, float custom_width = 0.f, bool glowing_border = false);
    bool begin_child_scrollable(std::string_view name, float custom_width = 0.f, float custom_height = 0.f, bool glowing_border = false);

    widget_t radial_fov(std::string_view name, int* callback, int min, int max, const char* unit = "°", float custom_radius = 0.f, const char* sub_label = "FOV", bool title_at_bottom = false, float custom_width = 0.f);
    widget_t radial_fov(std::string_view name, float* callback, float min, float max, const char* unit = "%", float custom_radius = 0.f, const char* sub_label = "MISS", bool title_at_bottom = false, const char* fmt_str = nullptr, float custom_width = 0.f);
    widget_t stepper_int(std::string_view name, int* callback, int min, int max, int step = 5, const char* format = "%dms", float custom_width = 0.f);
    widget_t skeleton_selector(std::string_view name, multi_dropdown_t* bones, const ImVec2& custom_size = ImVec2(0, 0), bool show_labels = true);
    widget_t status_badge(std::string_view name, bool active);
    widget_t switch_toggle(std::string_view name, bool* callback, std::string_view label_right = "", std::string_view label_left = "", float custom_width = 0.f);
    widget_t pill_toggle(std::string_view name, bool* callback, float custom_width = 0.f, float custom_height = 0.f);
    widget_t button_toggle(std::string_view name, bool* callback, float custom_width = 0.f, float custom_height = 0.f);
    widget_t color_button_toggle(std::string_view name, bool* callback, float* color, bool alpha = true, float custom_width = 0.f, float custom_height = 0.f);
    widget_t button_toggle_3(std::string_view name, int* callback, float custom_width = 0.f, float custom_height = 0.f);
    widget_t button_dummy(std::string_view name, float custom_width = 0.f, float custom_height = 0.f);
    widget_t checkbox_slider(std::string_view key_cb, std::string_view key_slider, std::string_view custom_label = "", float force_min = -1.f);
    widget_t inline_slider(std::string_view label, std::string_view key_slider, bool enabled = true);

    void end_child();

    widget_t section(std::string_view icon, int index, int& count);

    widget_t checkbox(std::string_view key, std::string_view custom_label = "");

    widget_t color_checkbox(std::string_view key, std::string_view custom_label = "");

    widget_t color_checkbox_dropdown(std::string_view checkbox_key, std::string_view dropdown_key, std::string_view custom_label = "");

    widget_t slider_int(std::string_view key, bool enabled = true);

    widget_t slider_float(std::string_view key, std::string_view custom_label = "");

    widget_t range_int(std::string_view key);

    widget_t range_float(std::string_view key);

    widget_t button(std::string_view name, float custom_width = 0.f);

    widget_t selectable(std::string_view name, bool active);

    widget_t dropdown(std::string_view key);

    widget_t multi_dropdown(std::string_view key);

    widget_t text_field(std::string_view key);

    widget_t key_select(std::string_view name, int* key, float custom_width = 0.f);

    widget_t mode_select(std::string_view name, int* mode);

    void color_edit_ex(std::string_view name, std::string_view widget_name, float* color, bool alpha, const ImRect& rect, const ImRect& clickable_rect, bool* value_changed);

    widget_t hex_field(std::string_view name, char* buf, int size);

    widget_t config(std::string_view name, std::string_view date, int index, int& count, int ind);

    widget_t config_field(std::string_view name, char* buf, int size);

    widget_t create_button(std::string_view name);

    widget_t sub_dropdown(std::string_view name, int* callback, std::vector<std::string> items, float custom_width = 0.f);
    widget_t placement_dropdown(std::string_view dropdown_key, std::string_view label = "Placement");

    widget_t playerlist(std::string_view name, std::vector<std::string>& items, int& selected_index, float height = 200.0f);
};

inline std::unique_ptr<c_widgets> widgets = std::make_unique<c_widgets>();

enum notify_type
{
    warning = 0,
    error = 1
};

struct notify_state
{
    int notify_id;
    std::string text;
    notify_type type{ error };

    ImVec2 window_size{ 0, 0 };
    float notify_alpha{ 0 };
    bool active_notify{ true };
    float notify_timer{ 0 };
    float notify_pos{ 0 };
};

class c_notify
{
public:
    void setup_notify();

    void add_notify(std::string_view text, notify_type type);

private:
    ImVec2 render_notify(int notify_id, float notify_alpha, float notify_timer, float notify_pos, const std::string& text, notify_type type);

    float notify_time{ 3.5f };
    int notify_count{ 0 };

    float notify_spacing{ 10 };
    ImVec2 notify_padding{ 20, 20 };

    std::vector<notify_state> notifications;
    std::mutex notify_mutex;

};

inline std::unique_ptr<c_notify> notify = std::make_unique<c_notify>();
