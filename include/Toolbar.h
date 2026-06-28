// include/Toolbar.h
#pragma once
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <string> // <--- این خط کلیدی اضافه شد
#include "Button.h"

class Toolbar {
public:
    Toolbar(float x, float y, float width, float height);
    void handleEvent(const SDL_Event& event, std::string& actionTriggered);
    void render(SDL_Renderer* renderer, TTF_Font* font) const;

private:
    SDL_FRect bounds_;
    std::vector<Button> tools_;
};