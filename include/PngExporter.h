#pragma once
#include <string>
#include <SDL3/SDL.h>

class PngExporter {
public:
    static bool saveRendererRegion(SDL_Renderer* renderer, const SDL_Rect& region,
                                   const std::string& filename, std::string* error = nullptr);
};
