// include/Button.h
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

// اضافه شدن آیکون WireIcon به لیست
enum class IconType { None, Save, Open, Grid, Menu, NewFile, Folder, Ruler, Clock, Settings, ExitIcon, WireIcon };

class Button {
public:
    Button(const SDL_FRect& rect,
           std::string label,
           SDL_Color normalColor,
           SDL_Color hoverColor,
           SDL_Color textColor,
           IconType icon = IconType::None);

    void setHovered(bool hovered);
    bool contains(float mouseX, float mouseY) const;
    void render(SDL_Renderer* renderer, TTF_Font* font) const;

    const std::string& getLabel() const;

private:
    SDL_FRect rect_;
    std::string label_;
    SDL_Color normalColor_;
    SDL_Color hoverColor_;
    SDL_Color textColor_;
    bool hovered_;
    IconType icon_;

    mutable uint64_t lastTick_{0};
    mutable float hoverFactor_{0.0f};

    void drawVectorIcon(SDL_Renderer* renderer, float x, float y, float size) const;
    void fillRoundedRect(SDL_Renderer* renderer, const SDL_FRect& rect, float radius, SDL_Color color) const;
};