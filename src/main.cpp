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
        if (!renderer || !font || text.empty()) return;

        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
        if (!surface) return;

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

        SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
        SDL_RenderClear(renderer);

        renderText(renderer, font, title, WindowWidth / 2.0f, 235.0f, titleColor);
        renderText(renderer, font, subtitle, WindowWidth / 2.0f, 295.0f, subtitleColor);
    }

    TTF_Font* loadFont() {
        const char* path = "C:\\Users\\bilgi\\OneDrive\\Desktop\\DejaVuSans.ttf";
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
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    if (!TTF_Init()) { SDL_Quit(); return 1; }

    SDL_Window* window = SDL_CreateWindow("Circuit Design Application", WindowWidth, WindowHeight, SDL_WINDOW_RESIZABLE);
    if (!window) { TTF_Quit(); SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) { SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }

    SDL_StartTextInput(window);

    TTF_Font* font = loadFont();
    if (!font) { SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); TTF_Quit(); SDL_Quit(); return 1; }

    StartMenu startMenu;
    AppState currentState = AppState::MainMenu;
    bool running = true;

    Toolbar toolbar{ 0, 0, 800, 50 };
    ComponentLibrary compLib{ 0, 50, 180, 550 };

    std::unique_ptr<Canvas> canvas = nullptr;
    std::unique_ptr<CanvasRenderer> canvasRenderer = nullptr;

    bool isPanning = false;
    std::string activeAction = "";
    std::string selectedComponent = "None";

    // متغیرهای مدیریت پنجره ذخیره‌سازی
    bool isSaveDialogOpen = false;
    std::string saveFileName = "";

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

                // --- مدیریت رویدادهای پنجره پاپ‌آپ Save ---
                if (isSaveDialogOpen) {
                    if (event.type == SDL_EVENT_TEXT_INPUT) {
                        if (saveFileName.size() < 25) { // محدودیت کاراکتر اسم
                            saveFileName += event.text.text;
                        }
                    } else if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.key == SDLK_BACKSPACE && !saveFileName.empty()) {
                            saveFileName.pop_back();
                        } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                            if (!saveFileName.empty()) {
                                std::string fullPath = saveFileName + ".txt";
                                // ذخیره پروژه در فایل
                                if (ProjectManager::saveProject(fullPath, compLib.getActiveList())) {
                                    startMenu.addSavedProject(fullPath); // اضافه کردن مستقیم به منوی اصلی
                                }
                            }
                            isSaveDialogOpen = false;
                        } else if (event.key.key == SDLK_ESCAPE) {
                            isSaveDialogOpen = false; // بستن پنجره با دکمه Esc
                        }
                    }
                    continue; // وقتی دیالوگ بازه، رویدادها به پنل‌های دیگه ارسال نشن
                }

                // هندل کردن میانبر Save (Ctrl+S)
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if ((event.key.mod & SDL_KMOD_CTRL) && event.key.key == SDLK_S) {
                        isSaveDialogOpen = true;
                        saveFileName = "";
                    }
                }

                toolbar.handleEvent(event, activeAction);
                compLib.handleEvent(event, selectedComponent);

                if (activeAction == "Save") {
                    isSaveDialogOpen = true; // باز کردن پنجره ذخیره
                    saveFileName = "";
                    activeAction = "";
                } else if (activeAction == "Load") {
                    canvas = nullptr;
                    canvasRenderer = nullptr;
                    currentState = AppState::MainMenu; // برگشت به منو برای انتخاب فایل
                    activeAction = "";
                } else if (activeAction == "Grid Toggle" && canvas) {
                    canvas->grid().setVisible(!canvas->grid().isVisible());
                    activeAction = "";
                } else if (activeAction == "Main Menu") {
                    canvas = nullptr;
                    canvasRenderer = nullptr;
                    currentState = AppState::MainMenu;
                    activeAction = "";
                }

                if (canvas) {
                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        float canvasMouseX = event.motion.x - 180.0f;
                        float canvasMouseY = event.motion.y - 50.0f;
                        canvas->setMouseScreenPosition({canvasMouseX, canvasMouseY});

                        if (isPanning) canvas->pan({event.motion.xrel, event.motion.yrel});
                    }
                    else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                        if (event.wheel.y > 0) canvas->zoomBy(1.1f);
                        else if (event.wheel.y < 0) canvas->zoomBy(0.9f);
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (event.button.button == SDL_BUTTON_MIDDLE) isPanning = true;
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                        if (event.button.button == SDL_BUTTON_MIDDLE) isPanning = false;
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

                    // لود کردن فایل در صورت درخواست از منو
                    if (startMenu.shouldLoadProject()) {
                        std::vector<std::string> loadedList;
                        if (ProjectManager::loadProject(startMenu.getSelectedProjectFile(), loadedList)) {
                            compLib.setActiveList(loadedList);
                        }
                        startMenu.resetLoadProject();
                    } else {
                        compLib.setActiveList({}); // پاک کردن لیست برای یه پروژه واقعا جدید
                    }
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

                    // --- رسم کردن پنجره پاپ‌آپ Save رو صفحه ---
                    if (isSaveDialogOpen) {
                        // یک پس‌زمینه نیمه‌شفاف برای تاریک کردن پشت دیالوگ
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                        SDL_FRect screenRect{0, 0, static_cast<float>(currentW), static_cast<float>(currentH)};
                        SDL_RenderFillRect(renderer, &screenRect);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        // کادر دیالوگ
                        SDL_FRect dialogRect{ currentW / 2.0f - 160.0f, currentH / 2.0f - 80.0f, 320.0f, 160.0f };
                        SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255);
                        SDL_RenderFillRect(renderer, &dialogRect);
                        SDL_SetRenderDrawColor(renderer, 100, 110, 130, 255);
                        SDL_RenderRect(renderer, &dialogRect);

                        renderText(renderer, font, "Enter Project Name:", currentW / 2.0f, dialogRect.y + 20.0f, {255, 255, 255, 255});

                        // باکس ورودی متن
                        SDL_FRect inputRect{ dialogRect.x + 20.0f, dialogRect.y + 60.0f, dialogRect.w - 40.0f, 40.0f };
                        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255);
                        SDL_RenderFillRect(renderer, &inputRect);
                        SDL_SetRenderDrawColor(renderer, 80, 90, 110, 255);
                        SDL_RenderRect(renderer, &inputRect);

                        // متن در حال تایپ به همراه نشانگر چشمک‌زن (Cursor)
                        std::string displayText = saveFileName;
                        if ((SDL_GetTicks() / 500) % 2 == 0) displayText += "_";
                        renderText(renderer, font, displayText, inputRect.x + 10.0f, inputRect.y + 8.0f, {200, 210, 230, 255}, false);

                        renderText(renderer, font, "Press ENTER to save, ESC to cancel", currentW / 2.0f, dialogRect.y + 120.0f, {150, 160, 180, 255});
                    }
                }
                break;

            case AppState::OpenProject:
            case AppState::SelectPageSize:
            case AppState::RecentProjects:
                // چون این منوها داخل خود StartMenu رندر میشن این حالت‌ها رو خالی گذاشتیم
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