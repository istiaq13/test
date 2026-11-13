#include "../headers/includes.h"
#include "../headers/functions.h"
#include "../../imgui_impl_dx11.h"

void c_font::initialize()
{
    try
    {
        if (data.empty())
        {
            if (!inter_medium_data.empty() && inter_medium_data.size() < 500000)
            {
                add(inter_medium_data, 14);
                add(inter_medium_data, 13);
                add(inter_medium_data, 15);
                add(inter_medium_data, 12);
                add(inter_medium_data, 11);
                add(inter_medium_data, 10);
                add(inter_medium_data, 16);
            }
            if (!inter_semibold_data.empty() && inter_semibold_data.size() < 500000)
            {
                add(inter_semibold_data, 14);
                add(inter_semibold_data, 13);
                add(inter_semibold_data, 15);
                add(inter_semibold_data, 16);
                add(inter_semibold_data, 10);
                add(inter_semibold_data, 11);
                add(inter_semibold_data, 12);
                add(inter_semibold_data, 18);
                add(inter_semibold_data, 20);
                add(inter_semibold_data, 22);
            }
            if (!icons_data.empty() && icons_data.size() < 500000)
            {
                add(icons_data, 15);
                add(icons_data, 14);
                add(icons_data, 10);
                add(icons_data, 8);
                add(icons_data, 12);
                add(icons_data, 16);
            }
            if (!watermark_data.empty() && watermark_data.size() < 500000)
            {
                add(watermark_data, 48);
            }
        }
    }
    catch (...)
    {
        data.clear();
    }
}

void c_font::update()
{
    try
    {
        static bool fonts_loaded = false;

        if (!fonts_loaded && (var->gui.dpi_changed || data.empty()))
        {
            initialize();

            if (data.empty())
                return;

            var->gui.dpi = var->gui.stored_dpi / 100.f;

            ImFontConfig cfg;
            cfg.FontDataOwnedByAtlas = false;

            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->Clear();

            for (auto& font_t : data)
            {
                if (font_t.data_ptr && font_t.data_len > 0 && font_t.data_len < 1000000)
                {
                    try
                    {
                        font_t.font = io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(font_t.data_ptr), (int)font_t.data_len, SCALE(font_t.size), &cfg, io.Fonts->GetGlyphRangesCyrillic());
                    }
                    catch (...)
                    {
                        font_t.font = nullptr;
                    }
                }
            }

            try
            {
                io.Fonts->Build();
                ImGui_ImplDX11_CreateDeviceObjects();

                if (!io.Fonts->Fonts.empty() && io.Fonts->Fonts[0])
                {
                    io.FontDefault = io.Fonts->Fonts[0];
                }
            }
            catch (...)
            {
                var->gui.dpi_changed = false;
                return;
            }

            var->gui.dpi_changed = false;
            fonts_loaded = true;
        }
    }
    catch (...)
    {
        var->gui.dpi_changed = false;
    }
}

ImFont* c_font::get(const std::vector<unsigned char>& font_data, float size)
{
    try
    {
        if (font_data.empty())
            return ImGui::GetIO().Fonts->Fonts[0];

        const unsigned char* ptr = font_data.data();
        int isize = (int)std::round(size);

        for (const auto& item : data)
        {
            if (item.data_ptr == ptr && (int)std::round(item.size) == isize)
            {
                if (item.font && item.font->IsLoaded())
                    return item.font;
                else
                    return ImGui::GetIO().Fonts->Fonts[0];
            }
        }

        add(font_data, size);
        var->gui.dpi_changed = true;

        return ImGui::GetIO().Fonts->Fonts[0];
    }
    catch (...)
    {
        return ImGui::GetIO().Fonts->Fonts[0];
    }
}

void c_font::add(const std::vector<unsigned char>& font_data, float size)
{
    try
    {
        data.push_back({ font_data.data(), font_data.size(), size, nullptr });
    }
    catch (...)
    {
    }
}