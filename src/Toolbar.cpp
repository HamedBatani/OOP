// src/Toolbar.cpp
#include "Toolbar.h"

Toolbar::Toolbar(float x, float y, float width, float height) : bounds_{x, y, width, height} {
    SDL_Color bg{24, 24, 24, 255};
    SDL_Color hover{55, 60, 70, 255};
    SDL_Color text{220, 220, 225, 255};

    tools_.emplace_back(SDL_FRect{x + 10, y + 5, 75, 35}, "Save", bg, hover, text, IconType::Save);
    tools_.emplace_back(SDL_FRect{x + 90, y + 5, 75, 35}, "Save As", bg, hover, text, IconType::None);
    tools_.emplace_back(SDL_FRect{x + 90, y + 5, 75, 35}, "Open", bg, hover, text, IconType::Open);
    tools_.emplace_back(SDL_FRect{x + 90, y + 5, 75, 35}, "Undo", bg, hover, text, IconType::None);
    tools_.emplace_back(SDL_FRect{x + 90, y + 5, 75, 35}, "Redo", bg, hover, text, IconType::None);
    tools_.emplace_back(SDL_FRect{x + 90, y + 5, 75, 35}, "Export", bg, hover, text, IconType::None);
    tools_.emplace_back(SDL_FRect{x + 170, y + 5, 75, 35}, "Wire", bg, hover, text, IconType::WireIcon);
    tools_.emplace_back(SDL_FRect{x + 250, y + 5, 75, 35}, "Grid", bg, hover, text, IconType::Grid);

    // دکمه‌های کنترل شبیه‌سازی (گنجاندن Step)
    tools_.emplace_back(SDL_FRect{x + 335, y + 5, 75, 35}, "Run", bg, hover, text, IconType::PlayIcon);
    tools_.emplace_back(SDL_FRect{x + 415, y + 5, 80, 35}, "Pause", bg, hover, text, IconType::PauseIcon);
    tools_.emplace_back(SDL_FRect{x + 500, y + 5, 75, 35}, "Step", bg, hover, text, IconType::StepIcon);
    tools_.emplace_back(SDL_FRect{x + 580, y + 5, 75, 35}, "Stop", bg, hover, text, IconType::StopIcon);

    tools_.emplace_back(SDL_FRect{x + 665, y + 5, 80, 35}, "Menu", bg, hover, text, IconType::Menu);
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
                else if (rawLabel == "Save As") actionTriggered = "Save As";
                else if (rawLabel == "Open") actionTriggered = "Load";
                else if (rawLabel == "Undo") actionTriggered = "Undo";
                else if (rawLabel == "Redo") actionTriggered = "Redo";
                else if (rawLabel == "Export") actionTriggered = "Export";
                else if (rawLabel == "Wire") actionTriggered = "Wire Toggle";
                else if (rawLabel == "Grid") actionTriggered = "Grid Toggle";
                else if (rawLabel == "Run") actionTriggered = "Run";
                else if (rawLabel == "Pause") actionTriggered = "Pause";
                else if (rawLabel == "Step") actionTriggered = "Step";
                else if (rawLabel == "Stop") actionTriggered = "Stop";
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

void Toolbar::setBounds(float x, float y, float width, float height) {
    bounds_ = {x, y, width, height};
    if (tools_.empty()) return;

    constexpr float margin = 8.0f;
    constexpr float gap = 6.0f;
    const float available = std::max(1.0f, width - margin * 2.0f - gap * static_cast<float>(tools_.size() - 1));
    const float buttonWidth = available / static_cast<float>(tools_.size());
    const float buttonHeight = std::max(28.0f, height - 10.0f);
    float buttonX = x + margin;
    for (auto& tool : tools_) {
        tool.setRect({buttonX, y + 5.0f, buttonWidth, buttonHeight});
        buttonX += buttonWidth + gap;
    }
}
