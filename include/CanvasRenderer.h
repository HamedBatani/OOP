// include/CanvasRenderer.h
#pragma once

#include <iosfwd>
#include <vector> // <-- FIX 1: Added to resolve the 'std::vector' error
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Canvas.h"
#include "Point.h"

// FIX 2: Forward declaration tells the header that ComponentInstance exists
class ComponentInstance;

class CanvasRenderer {
public:
    explicit CanvasRenderer(const Canvas& canvas);

    void render(std::ostream& output) const;
    void renderState(std::ostream& output) const;
    void renderGridPreview(std::ostream& output, int columns = 21, int rows = 11) const;
    void renderSnapTest(std::ostream& output, const Point& rawMouseScreenPoint) const;

    // متد گرافیکی بوم طراحی
    void renderSDL(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight) const;

    // FIX 3: Placed INSIDE the class definition so the compiler knows it belongs to CanvasRenderer
    void renderComponentsSDL(SDL_Renderer* renderer, TTF_Font* font, const std::vector<ComponentInstance>& components) const;

private:
    const Canvas& canvas_;
};