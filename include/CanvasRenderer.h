// CanvasRenderer.h
#pragma once

#include <iosfwd>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "Canvas.h"
#include "Point.h"

class CanvasRenderer {
public:
    explicit CanvasRenderer(const Canvas& canvas);

    void render(std::ostream& output) const;
    void renderState(std::ostream& output) const;
    void renderGridPreview(std::ostream& output, int columns = 21, int rows = 11) const;
    void renderSnapTest(std::ostream& output, const Point& rawMouseScreenPoint) const;
    void renderSDL(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight) const;

private:
    const Canvas& canvas_;
};