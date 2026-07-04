// src/Toolbar.cpp
#include "Toolbar.h"

Toolbar::Toolbar(float x, float y, float width, float height) : bounds_{x, y, width, 45.0f} {
    SDL_Color bg{24, 24, 24, 255};
    SDL_Color hover{55, 60, 70, 255};
    SDL_Color text{220, 220, 225, 255};

    // دکمه Wire اضافه و فاصله‌ها مجدد تنظیم شد
    tools_.emplace_back(SDL_FRect{x + 10, y + 5, 80, 35}, "Save", bg, hover, text, IconType::Save);
    tools_.emplace_back(SDL_FRect{x + 100, y + 5, 80, 35}, "Open", bg, hover, text, IconType::Open);
    tools_.emplace_back(SDL_FRect{x + 190, y + 5, 80, 35}, "Wire", bg, hover, text, IconType::WireIcon); // دکمه جدید!
    tools_.emplace_back(SDL_FRect{x + 280, y + 5, 80, 35}, "Grid", bg, hover, text, IconType::Grid);
    tools_.emplace_back(SDL_FRect{x + 370, y + 5, 90, 35}, "Menu", bg, hover, text, IconType::Menu);
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
                std::string rawLabel = btn.getLabel();
                if (rawLabel == "Save") actionTriggered = "Save";
                else if (rawLabel == "Open") actionTriggered = "Load";
                else if (rawLabel == "Wire") actionTriggered = "Wire Toggle"; // اکشن جدید
                else if (rawLabel == "Grid") actionTriggered = "Grid Toggle";
                else if (rawLabel == "Menu") actionTriggered = "Main Menu";
                return;
            }
        }
    }
}

void Toolbar::render(SDL_Renderer* renderer, TTF_Font* font) const {
    SDL_SetRenderDrawColor(renderer, 24, 24, 24, 255);
    SDL_RenderFillRect(renderer, &bounds_);

    SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
    SDL_RenderLine(renderer, bounds_.x, bounds_.y + bounds_.h - 1.0f, bounds_.x + bounds_.w, bounds_.y + bounds_.h - 1.0f);

    for (const auto& btn : tools_) {
        btn.render(renderer, font);
    }
}