// include/CanvasRenderer.h
#pragma once

#include <iosfwd>
#include <vector>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Canvas.h"
#include "Point.h"
#include "Wire.h"

class ComponentInstance;

class CanvasRenderer {
public:
    explicit CanvasRenderer(const Canvas& canvas);

    void render(std::ostream& output) const;
    void renderState(std::ostream& output) const;
    void renderGridPreview(std::ostream& output, int columns = 21, int rows = 11) const;
    void renderSnapTest(std::ostream& output, const Point& rawMouseScreenPoint) const;

    void renderSDL(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight) const;

    void renderComponentsSDL(SDL_Renderer* renderer, TTF_Font* font, const std::vector<ComponentInstance>& components) const;

    // پارامتر isSimulating برای تشخیص حالت اجرا اضافه شد
    void renderWiresSDL(SDL_Renderer* renderer, const std::vector<Wire>& wires, bool isSimulating = false) const;

private:
    const Canvas& canvas_;

    void fillScreenCircle(SDL_Renderer* renderer, float cx, float cy, float r, SDL_Color c) const;
};