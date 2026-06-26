//MAINCPP
#include "AppState.h"
#include "StartMenu.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>



#include <iostream>
#include <string>

namespace {
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;

    void renderText(SDL_Renderer* renderer,
                    TTF_Font* font,
                    const std::string& text,
                    float x,
                    float y,
                    SDL_Color color,
                    bool centered = true) {
        if (!renderer || !font || text.empty()) {
            return;
        }

        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
        if (!surface) {
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_DestroySurface(surface);
            return;
        }

        SDL_FRect destination{
                centered ? x - static_cast<float>(surface->w) / 2.0f : x,
                y,
                static_cast<float>(surface->w),
                static_cast<float>(surface->h)
        };

        SDL_RenderTexture(renderer, texture, nullptr, &destination);

        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);
    }

    void renderPlaceholderScreen(SDL_Renderer* renderer,
                                 TTF_Font* font,
                                 const std::string& title,
                                 const std::string& subtitle) {
        const SDL_Color backgroundColor{24, 28, 36, 255};
        const SDL_Color titleColor{235, 240, 255, 255};
        const SDL_Color subtitleColor{170, 178, 194, 255};

        SDL_SetRenderDrawColor(renderer,
                               backgroundColor.r,
                               backgroundColor.g,
                               backgroundColor.b,
                               backgroundColor.a);
        SDL_RenderClear(renderer);

        renderText(renderer, font, title, WindowWidth / 2.0f, 235.0f, titleColor);
        renderText(renderer, font, subtitle, WindowWidth / 2.0f, 295.0f, subtitleColor);
    }

    TTF_Font* loadFont() {
        const char* path = "C:\\Users\\bilgi\\OneDrive\\Desktop\\DejaVuSans.ttf";
        TTF_Font* font = TTF_OpenFont(path, 24);
        if (!font) {
            std::cerr << "Font loading failed: " << SDL_GetError() << '\n';
            std::cin.get(); // ADD THIS LINE - keeps window open

            TTF_Quit();
            SDL_Quit();
        }
        return font;
    }
}

int main(int, char**) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }

    if (!TTF_Init()) {
        std::cerr << "SDL_ttf initialization failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Circuit Design Application",
                                          WindowWidth,
                                          WindowHeight,
                                          SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    TTF_Font* font = loadFont();
    if (!font) {
        std::cerr << "Font loading failed: " << SDL_GetError() << '\n';
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    StartMenu startMenu;
    AppState currentState = AppState::MainMenu;
    bool running = true;

    while (running && currentState != AppState::Exit) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
                currentState = AppState::Exit;
                break;
            }

            if (currentState == AppState::MainMenu) {
                startMenu.handleEvent(event);
            }
        }

        if (currentState == AppState::MainMenu) {
            const AppState requestedState = startMenu.getRequestedState();
            if (requestedState != AppState::MainMenu) {
                currentState = requestedState;
                startMenu.resetRequestedState();
            }
        }

        switch (currentState) {
            case AppState::MainMenu:
                startMenu.render(renderer, font);
                break;

            case AppState::NewProject:
                renderPlaceholderScreen(renderer,
                                        font,
                                        "New Project",
                                        "Blank circuit canvas placeholder");
                break;

            case AppState::OpenProject:
                renderPlaceholderScreen(renderer,
                                        font,
                                        "Open Project",
                                        "Project loading placeholder");
                break;

            case AppState::SelectPageSize:
                renderPlaceholderScreen(renderer,
                                        font,
                                        "Select Page Size",
                                        "Page size selection is handled in the menu");
                break;

            case AppState::RecentProjects:
                renderPlaceholderScreen(renderer,
                                        font,
                                        "Recent Projects",
                                        "Recent projects are shown in the menu");
                break;

            case AppState::Exit:
                running = false;
                break;
        }

        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}