// src/Toolbar.cpp
#include "Toolbar.h"

Toolbar::Toolbar(float x, float y, float width, float height) : bounds_{x, y, width, height} {
    SDL_Color btnColor{45, 55, 72, 255};
    SDL_Color hoverColor{66, 153, 225, 255};
    SDL_Color txtColor{255, 255, 255, 255};

    // عرض دکمه Grid Toggle به 140 پیکسل افزایش یافت و دکمه Main Menu اضافه شد
    tools_.emplace_back(SDL_FRect{x + 10, y + 5, 70, height - 10}, "Save", btnColor, hoverColor, txtColor);
    tools_.emplace_back(SDL_FRect{x + 90, y + 5, 70, height - 10}, "Load", btnColor, hoverColor, txtColor);
    tools_.emplace_back(SDL_FRect{x + 170, y + 5, 140, height - 10}, "Grid Toggle", btnColor, hoverColor, txtColor);
    tools_.emplace_back(SDL_FRect{x + 320, y + 5, 120, height - 10}, "Main Menu", btnColor, hoverColor, txtColor);
}

void Toolbar::handleEvent(const SDL_Event& event, std::string& actionTriggered) {
    float mx, my;
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        mx = event.motion.x; my = event.motion.y;
        for (auto& btn : tools_) btn.setHovered(btn.contains(mx, my));
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        mx = event.button.x; my = event.button.y;
        for (auto& btn : tools_) {
            if (btn.contains(mx, my)) {
                actionTriggered = btn.getLabel();
                return;
            }
        }
    }
}

void Toolbar::render(SDL_Renderer* renderer, TTF_Font* font) const {
    // رنگ پس‌زمینه نوار ابزار (مدرن و تیره)
    SDL_SetRenderDrawColor(renderer, 32, 38, 50, 255);
    SDL_RenderFillRect(renderer, &bounds_);

    // رسم خط حاشیه پایین (Border Bottom) برای ایجاد عمق
    SDL_SetRenderDrawColor(renderer, 20, 25, 35, 255); // رنگ تیره برای سایه
    SDL_RenderLine(renderer, bounds_.x, bounds_.y + bounds_.h - 1.0f, bounds_.x + bounds_.w, bounds_.y + bounds_.h - 1.0f);

    for (const auto& btn : tools_) {
        btn.render(renderer, font);
    }
}