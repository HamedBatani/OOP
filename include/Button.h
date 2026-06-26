//BUTTON.H
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>

class Button {
public:
    Button(const SDL_FRect& rect,
           std::string label,
           SDL_Color normalColor,
           SDL_Color hoverColor,
           SDL_Color textColor);

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
};