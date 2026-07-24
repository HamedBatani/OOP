// src/Button.cpp
#include "Button.h"
#include <utility>
#include <vector>
#include <cmath>

namespace {
    constexpr float PI = 3.14159265358979323846f;
}

Button::Button(const SDL_FRect& rect, std::string label, SDL_Color normalColor, SDL_Color hoverColor, SDL_Color textColor, IconType icon)
        : rect_(rect), label_(std::move(label)), normalColor_(normalColor),
          hoverColor_(hoverColor), textColor_(textColor), hovered_(false), icon_(icon) {}

void Button::setHovered(bool hovered) { hovered_ = hovered; }

bool Button::contains(float mouseX, float mouseY) const {
    return mouseX >= rect_.x && mouseX <= rect_.x + rect_.w && mouseY >= rect_.y && mouseY <= rect_.y + rect_.h;
}

void Button::fillRoundedRect(SDL_Renderer* renderer, const SDL_FRect& rect, float radius, SDL_Color color) const {
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_FRect r1{rect.x + radius, rect.y, rect.w - 2 * radius, rect.h};
    SDL_FRect r2{rect.x, rect.y + radius, radius, rect.h - 2 * radius};
    SDL_FRect r3{rect.x + rect.w - radius, rect.y + radius, radius, rect.h - 2 * radius};
    SDL_RenderFillRect(renderer, &r1); SDL_RenderFillRect(renderer, &r2); SDL_RenderFillRect(renderer, &r3);

    auto fillQuadrant = [&](float cx, float cy, float startAngle) {
        const int segments = 8; std::vector<SDL_Vertex> v;
        SDL_FColor fc{color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
        float step = (PI / 2.0f) / segments;
        for (int i = 0; i < segments; ++i) {
            v.push_back({{cx, cy}, fc, {0, 0}});
            v.push_back({{cx + radius * std::cos(startAngle + i * step), cy + radius * std::sin(startAngle + i * step)}, fc, {0, 0}});
            v.push_back({{cx + radius * std::cos(startAngle + (i + 1) * step), cy + radius * std::sin(startAngle + (i + 1) * step)}, fc, {0, 0}});
        }
        SDL_RenderGeometry(renderer, nullptr, v.data(), v.size(), nullptr, 0);
    };

    fillQuadrant(rect.x + rect.w - radius, rect.y + rect.h - radius, 0.0f);
    fillQuadrant(rect.x + radius, rect.y + rect.h - radius, PI / 2.0f);
    fillQuadrant(rect.x + radius, rect.y + radius, PI);
    fillQuadrant(rect.x + rect.w - radius, rect.y + radius, -PI / 2.0f);
}

void Button::drawVectorIcon(SDL_Renderer* renderer, float x, float y, float size) const {
    SDL_SetRenderDrawColor(renderer, textColor_.r, textColor_.g, textColor_.b, textColor_.a);
    float cx = x + size / 2.0f, cy = y + size / 2.0f, s = size * 0.8f;

    auto drawCircle = [&](float cx, float cy, float r) {
        for(int i=0; i<24; ++i) SDL_RenderLine(renderer, cx+r*std::cos(i*PI/12.f), cy+r*std::sin(i*PI/12.f), cx+r*std::cos((i+1)*PI/12.f), cy+r*std::sin((i+1)*PI/12.f));
    };

    if (icon_ == IconType::Save) {
        SDL_FRect body{cx - s/2, cy - s/2, s, s}; SDL_RenderRect(renderer, &body);
        SDL_FRect label{cx - s/3, cy - s/2, s*0.66f, s*0.3f}; SDL_RenderRect(renderer, &label);
    }
    else if (icon_ == IconType::Open || icon_ == IconType::Folder) {
        SDL_RenderLine(renderer, cx-s/2, cy-s/4, cx-s/4, cy-s/4); SDL_RenderLine(renderer, cx-s/4, cy-s/4, cx-s/8, cy-s/8);
        SDL_RenderLine(renderer, cx-s/8, cy-s/8, cx+s/2, cy-s/8); SDL_RenderLine(renderer, cx+s/2, cy-s/8, cx+s/2, cy+s/2);
        SDL_RenderLine(renderer, cx+s/2, cy+s/2, cx-s/2, cy+s/2); SDL_RenderLine(renderer, cx-s/2, cy+s/2, cx-s/2, cy-s/4);
    }
    else if (icon_ == IconType::Grid) {
        SDL_FRect outer{cx - s/2, cy - s/2, s, s}; SDL_RenderRect(renderer, &outer);
        SDL_RenderLine(renderer, cx, cy - s/2, cx, cy + s/2); SDL_RenderLine(renderer, cx - s/2, cy, cx + s/2, cy);
    }
    else if (icon_ == IconType::Menu) {
        SDL_RenderLine(renderer, cx+s/4, cy-s/4, cx-s/4, cy); SDL_RenderLine(renderer, cx-s/4, cy, cx+s/4, cy+s/4);
    }
    else if (icon_ == IconType::PlayIcon) {
        SDL_SetRenderDrawColor(renderer, 40, 200, 80, 255);
        SDL_FColor fc = {40.f/255.f, 200.f/255.f, 80.f/255.f, 1.0f};
        SDL_Vertex v[3] = {{{cx - s/3, cy - s/2}, fc, {0,0}}, {{cx + s/2, cy}, fc, {0,0}}, {{cx - s/3, cy + s/2}, fc, {0,0}}};
        SDL_RenderGeometry(renderer, nullptr, v, 3, nullptr, 0);
    }
    else if (icon_ == IconType::PauseIcon) {
        SDL_SetRenderDrawColor(renderer, 240, 180, 40, 255);
        SDL_FRect b1{cx - s/3, cy - s/2, s/4, s}; SDL_RenderFillRect(renderer, &b1);
        SDL_FRect b2{cx + s/12, cy - s/2, s/4, s}; SDL_RenderFillRect(renderer, &b2);
    }
    else if (icon_ == IconType::StopIcon) {
        SDL_SetRenderDrawColor(renderer, 220, 60, 60, 255);
        SDL_FRect b{cx - s/2.5f, cy - s/2.5f, s*0.8f, s*0.8f}; SDL_RenderFillRect(renderer, &b);
    }
    else if (icon_ == IconType::StepIcon) {
        SDL_SetRenderDrawColor(renderer, 40, 180, 240, 255);
        SDL_FColor fc = {40.f/255.f, 180.f/255.f, 240.f/255.f, 1.0f};
        SDL_FRect bar{cx + s/6, cy - s/2, s/5, s}; SDL_RenderFillRect(renderer, &bar);
        SDL_Vertex v[3] = {{{cx - s/2.5f, cy - s/2}, fc, {0,0}}, {{cx + s/8, cy}, fc, {0,0}}, {{cx - s/2.5f, cy + s/2}, fc, {0,0}}};
        SDL_RenderGeometry(renderer, nullptr, v, 3, nullptr, 0);
    }
    else if (icon_ == IconType::ExitIcon) {
        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
        SDL_RenderLine(renderer, cx-s/3, cy-s/3, cx+s/3, cy+s/3); SDL_RenderLine(renderer, cx-s/3, cy+s/3, cx+s/3, cy-s/3);
    }
    else if (icon_ == IconType::WireIcon) {
        SDL_SetRenderDrawColor(renderer, 0, 180, 80, 255);
        drawCircle(cx - s/2.5f, cy + s/4, 2); drawCircle(cx + s/2.5f, cy - s/4, 2);
        SDL_RenderLine(renderer, cx - s/2.5f, cy + s/4, cx, cy + s/4);
        SDL_RenderLine(renderer, cx, cy + s/4, cx, cy - s/4);
        SDL_RenderLine(renderer, cx, cy - s/4, cx + s/2.5f, cy - s/4);
    }
}

void Button::render(SDL_Renderer* renderer, TTF_Font* font) const {
    if (!renderer) return;

    uint64_t currentTick = SDL_GetTicks();
    if (lastTick_ == 0) lastTick_ = currentTick;
    float dt = (currentTick - lastTick_) / 1000.0f;
    lastTick_ = currentTick;

    if (hovered_) {
        hoverFactor_ += dt / 0.15f;
        if (hoverFactor_ > 1.0f) hoverFactor_ = 1.0f;
    } else {
        hoverFactor_ -= dt / 0.15f;
        if (hoverFactor_ < 0.0f) hoverFactor_ = 0.0f;
    }

    float scale = 1.0f + (0.02f * hoverFactor_);
    float w = rect_.w * scale, h = rect_.h * scale;
    float x = rect_.x - (w - rect_.w) / 2.0f;
    float y = rect_.y - (h - rect_.h) / 2.0f;

    SDL_Color color;
    color.r = static_cast<Uint8>(normalColor_.r + (hoverColor_.r - normalColor_.r) * hoverFactor_);
    color.g = static_cast<Uint8>(normalColor_.g + (hoverColor_.g - normalColor_.g) * hoverFactor_);
    color.b = static_cast<Uint8>(normalColor_.b + (hoverColor_.b - normalColor_.b) * hoverFactor_);
    color.a = 255;

    if (hoverFactor_ > 0.01f) {
        fillRoundedRect(renderer, {x + 2.0f, y + 4.0f, w, h}, 8.0f, {0, 0, 0, static_cast<Uint8>(40 * hoverFactor_)});
    }

    fillRoundedRect(renderer, {x, y, w, h}, 8.0f, color);

    if (!font || label_.empty()) return;
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, label_.c_str(), 0, textColor_);
    if (!textSurface) return;

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) { SDL_DestroySurface(textSurface); return; }

    float iconSize = 18.0f * scale;
    float totalWidth = static_cast<float>(textSurface->w) * scale;
    if (icon_ != IconType::None) totalWidth += iconSize + 8.0f * scale;

    float startX = x + (w - totalWidth) / 2.0f;
    float textY = y + (h - static_cast<float>(textSurface->h) * scale) / 2.0f;

    if (icon_ != IconType::None) {
        drawVectorIcon(renderer, startX, y + (h - iconSize) / 2.0f, iconSize);
        startX += iconSize + 8.0f * scale;
    }

    const SDL_FRect textRect{startX, textY, static_cast<float>(textSurface->w) * scale, static_cast<float>(textSurface->h) * scale};
    SDL_RenderTexture(renderer, textTexture, nullptr, &textRect);
    SDL_DestroyTexture(textTexture);
    SDL_DestroySurface(textSurface);
}

const std::string& Button::getLabel() const { return label_; }