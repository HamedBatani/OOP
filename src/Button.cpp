//BUTTON.CPP
#include "Button.h"


#include <utility>

Button::Button(const SDL_FRect& rect,
               std::string label,
               SDL_Color normalColor,
               SDL_Color hoverColor,
               SDL_Color textColor)
        : rect_(rect),
          label_(std::move(label)),
          normalColor_(normalColor),
          hoverColor_(hoverColor),
          textColor_(textColor),
          hovered_(false) {}

void Button::setHovered(bool hovered) {
    hovered_ = hovered;
}

bool Button::contains(float mouseX, float mouseY) const {
    return mouseX >= rect_.x &&
           mouseX <= rect_.x + rect_.w &&
           mouseY >= rect_.y &&
           mouseY <= rect_.y + rect_.h;
}

void Button::render(SDL_Renderer* renderer, TTF_Font* font) const {
    if (!renderer) {
        return;
    }

    const SDL_Color color = hovered_ ? hoverColor_ : normalColor_;

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect_);

    if (!font || label_.empty()) {
        return;
    }

    SDL_Surface* textSurface = TTF_RenderText_Blended(font, label_.c_str(), 0, textColor_);
    if (!textSurface) {
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!textTexture) {
        SDL_DestroySurface(textSurface);
        return;
    }

    const SDL_FRect textRect{
            rect_.x + (rect_.w - static_cast<float>(textSurface->w)) / 2.0f,
            rect_.y + (rect_.h - static_cast<float>(textSurface->h)) / 2.0f,
            static_cast<float>(textSurface->w),
            static_cast<float>(textSurface->h)
    };

    SDL_RenderTexture(renderer, textTexture, nullptr, &textRect);

    SDL_DestroyTexture(textTexture);
    SDL_DestroySurface(textSurface);
}

const std::string& Button::getLabel() const {
    return label_;
}