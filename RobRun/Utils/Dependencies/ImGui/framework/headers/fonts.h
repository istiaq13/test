#pragma once
#include "includes.h"

class c_font
{
public:
    void initialize();
    void update();

    ImFont* get(const std::vector<unsigned char>& font_data, float size);

private:
    struct font_data
    {
        const unsigned char* data_ptr = nullptr;
        size_t data_len = 0;
        float size = 0.f;
        ImFont* font = nullptr;
    };

    void add(const std::vector<unsigned char>& font_data, float size);

    std::vector<font_data> data;
};

inline std::unique_ptr<c_font> font = std::make_unique<c_font>();