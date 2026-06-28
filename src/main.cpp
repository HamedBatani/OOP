// src/main.cpp
#include "AppState.h"
#include "StartMenu.h"
#include "Canvas.h"
#include "CanvasRenderer.h"
#include "Toolbar.h"
#include "ComponentLibrary.h"
#include "ProjectManager.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <string>
#include <memory>

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
        const char* path = "C:\\Users\\Asus\\OneDrive\\Desktop\\DejaVuSans.ttf";
        TTF_Font* font = TTF_OpenFont(path, 24);
        if (!font) {
            std::cerr << "Font loading failed: " << SDL_GetError() << '\n';
            std::cin.get();
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

    SDL_StartTextInput(window);

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

    Toolbar toolbar{ 0, 0, 800, 50 };
    // پنل کتابخانه به اندازه کل فضای باقی‌مانده (۵۵۰ پیکسل) کشیده شد
    ComponentLibrary compLib{ 0, 50, 180, 550 };

    std::unique_ptr<Canvas> canvas = nullptr;
    std::unique_ptr<CanvasRenderer> canvasRenderer = nullptr;

    bool isPanning = false;
    std::string activeAction = "";
    std::string selectedComponent = "None";

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
            else if (currentState == AppState::NewProject) {
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if ((event.key.mod & SDL_KMOD_CTRL) && event.key.key == SDLK_S) {
                        ProjectManager::saveProject("circuit.txt");
                        std::cout << "Project Saved via Shortcut (Ctrl+S)!\n";
                    }
                }

                toolbar.handleEvent(event, activeAction);
                compLib.handleEvent(event, selectedComponent);

                if (activeAction == "Save") {
                    ProjectManager::saveProject("circuit.txt");
                    activeAction = "";
                } else if (activeAction == "Load") {
                    ProjectManager::loadProject("circuit.txt");
                    activeAction = "";
                } else if (activeAction == "Grid Toggle" && canvas) {
                    canvas->grid().setVisible(!canvas->grid().isVisible());
                    activeAction = "";
                }

                if (canvas) {
                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        float canvasMouseX = event.motion.x - 180.0f;
                        float canvasMouseY = event.motion.y - 50.0f;
                        canvas->setMouseScreenPosition({canvasMouseX, canvasMouseY});

                        if (isPanning) {
                            canvas->pan({event.motion.xrel, event.motion.yrel});
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                        if (event.wheel.y > 0) {
                            canvas->zoomBy(1.1f);
                        } else if (event.wheel.y < 0) {
                            canvas->zoomBy(0.9f);
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (event.button.button == SDL_BUTTON_MIDDLE) {
                            isPanning = true;
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                        if (event.button.button == SDL_BUTTON_MIDDLE) {
                            isPanning = false;
                        }
                    }
                }
            }
        }

        if (currentState == AppState::MainMenu) {
            const AppState requestedState = startMenu.getRequestedState();
            if (requestedState != AppState::MainMenu) {
                currentState = requestedState;

                if (currentState == AppState::NewProject) {
                    const PageSize& size = startMenu.getSelectedPageSize();
                    canvas = std::make_unique<Canvas>(static_cast<float>(size.width), static_cast<float>(size.height));
                    canvasRenderer = std::make_unique<CanvasRenderer>(*canvas);
                }
                startMenu.resetRequestedState();
            }
        }

        int currentW, currentH;
        SDL_GetWindowSize(window, &currentW, &currentH);

        switch (currentState) {
            case AppState::MainMenu:
                startMenu.render(renderer, font);
                break;

            case AppState::NewProject:
                if (canvasRenderer) {
                    SDL_Rect canvasViewport{ 180, 50, 620, 550 };
                    SDL_SetRenderViewport(renderer, &canvasViewport);

                    canvasRenderer->renderSDL(renderer, font, 620, 550);

                    SDL_SetRenderViewport(renderer, nullptr);

                    toolbar.render(renderer, font);
                    compLib.render(renderer, font, selectedComponent);
                }
                break;

            case AppState::OpenProject:
                renderPlaceholderScreen(renderer, font, "Open Project", "Project loading placeholder");
                break;

            case AppState::SelectPageSize:
                renderPlaceholderScreen(renderer, font, "Select Page Size", "Page size selection is handled in the menu");
                break;

            case AppState::RecentProjects:
                renderPlaceholderScreen(renderer, font, "Recent Projects", "Recent projects are shown in the menu");
                break;

            case AppState::Exit:
                running = false;
                break;
        }

        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput(window);

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
