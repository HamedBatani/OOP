// src/Toolbar.cpp
#include "Toolbar.h"

Toolbar::Toolbar(float x, float y, float width, float height) : bounds_{x, y, width, height} {
    // ایجاد دکمه‌های پرکاربرد
    SDL_Color btnColor{45, 55, 72, 255};
    SDL_Color hoverColor{66, 153, 225, 255};
    SDL_Color txtColor{255, 255, 255, 255};

    tools_.emplace_back(SDL_FRect{x + 10, y + 5, 80, height - 10}, "Save", btnColor, hoverColor, txtColor);
    tools_.emplace_back(SDL_FRect{x + 100, y + 5, 80, height - 10}, "Load", btnColor, hoverColor, txtColor);
    tools_.emplace_back(SDL_FRect{x + 190, y + 5, 80, height - 10}, "Grid Toggle", btnColor, hoverColor, txtColor);
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
    SDL_SetRenderDrawColor(renderer, 26, 32, 44, 255); // رنگ پس‌زمینه نوار ابزار
    SDL_RenderFillRect(renderer, &bounds_);
    for (const auto& btn : tools_) btn.render(renderer, font);
}