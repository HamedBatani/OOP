// src/main.cpp
#include "AppState.h"
#include "StartMenu.h"
#include "Canvas.h"
#include "CanvasRenderer.h"
#include "Toolbar.h"
#include "ComponentLibrary.h"
#include "ProjectManager.h"
#include "ComponentInstance.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <string>
#include <memory>
#include <map>
#include <algorithm>

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

    bool isSaveDialogOpen = false;
    std::string saveFileName = "";

    std::vector<ComponentInstance> placedComponents;
    std::map<std::string, int> componentCounters;

    bool isSelectDragging = false;
    Point selectDragStartScreen{0.0f, 0.0f};
    SDL_FRect visualSelectBox{0.0f, 0.0f, 0.0f, 0.0f};

    bool isDraggingComponents = false;
    Point dragStartWorldMouse{0.0f, 0.0f};

    // ==========================================
    // متغیرهای بخش ۴.۷: مدیریت پنجره ویژگی‌ها (Properties)
    // ==========================================
    bool isPropertiesDialogOpen = false;
    int editingComponentIndex = -1;
    std::string editLabelBuf = "";
    std::string editValueBuf = "";
    int activeEditField = 0; // 0 = فیلد شناسه (Label), 1 = فیلد مقدار فنی (Value)

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

                // --- مدیریت انحصاری رویدادهای پنجره پاپ‌آپ ویژگی‌ها (۴.۷) ---
                if (isPropertiesDialogOpen) {
                    if (event.type == SDL_EVENT_TEXT_INPUT) {
                        if (activeEditField == 0 && editLabelBuf.size() < 10) {
                            editLabelBuf += event.text.text;
                        } else if (activeEditField == 1 && editValueBuf.size() < 12) {
                            editValueBuf += event.text.text;
                        }
                    } else if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.key == SDLK_BACKSPACE) {
                            if (activeEditField == 0 && !editLabelBuf.empty()) editLabelBuf.pop_back();
                            else if (activeEditField == 1 && !editValueBuf.empty()) editValueBuf.pop_back();
                        } else if (event.key.key == SDLK_TAB) {
                            activeEditField = (activeEditField + 1) % 2; // جابجایی بین دو باکس ورودی
                        } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                            if (!editLabelBuf.empty() && editingComponentIndex >= 0) {
                                placedComponents[editingComponentIndex].labelId = editLabelBuf;
                                placedComponents[editingComponentIndex].valueStr = editValueBuf;
                            }
                            isPropertiesDialogOpen = false;
                        } else if (event.key.key == SDLK_ESCAPE) {
                            isPropertiesDialogOpen = false;
                        }
                    }
                    continue; // مسدود کردن ارسال ورودی به پنل‌های پشت مدال
                }

                // --- مدیریت رویدادهای پنجره پاپ‌آپ Save ---
                if (isSaveDialogOpen) {
                    if (event.type == SDL_EVENT_TEXT_INPUT) {
                        if (saveFileName.size() < 25) {
                            saveFileName += event.text.text;
                        }
                    } else if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.key == SDLK_BACKSPACE && !saveFileName.empty()) {
                            saveFileName.pop_back();
                        } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                            if (!saveFileName.empty()) {
                                std::string fullPath = saveFileName + ".txt";
                                if (ProjectManager::saveProject(fullPath, compLib.getActiveList())) {
                                    startMenu.addSavedProject(fullPath);
                                }
                            }
                            isSaveDialogOpen = false;
                        } else if (event.key.key == SDLK_ESCAPE) {
                            isSaveDialogOpen = false;
                        }
                    }
                    continue;
                }

                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_ESCAPE) {
                        selectedComponent = "None";
                        for (auto& comp : placedComponents) comp.isSelected = false;
                    }
                    else if ((event.key.mod & SDL_KMOD_CTRL) && event.key.key == SDLK_S) {
                        isSaveDialogOpen = true;
                        saveFileName = "";
                    }
                    else if (selectedComponent == "None" || selectedComponent.empty()) {
                        if (event.key.key == SDLK_R) {
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) {
                                    comp.rotationDegrees = (comp.rotationDegrees + 90) % 360;
                                    comp.updatePinPositions();
                                }
                            }
                        }
                        else if (event.key.key == SDLK_M) {
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) {
                                    comp.isMirroredH = !comp.isMirroredH;
                                    comp.updatePinPositions();
                                }
                            }
                        }
                        else if (event.key.key == SDLK_V) {
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) {
                                    comp.isMirroredV = !comp.isMirroredV;
                                    comp.updatePinPositions();
                                }
                            }
                        }
                        else if (event.key.key == SDLK_DELETE || event.key.key == SDLK_BACKSPACE) {
                            placedComponents.erase(
                                    std::remove_if(placedComponents.begin(), placedComponents.end(),
                                                   [](const ComponentInstance& comp) { return comp.isSelected; }),
                                    placedComponents.end()
                            );
                        }
                    }
                }

                toolbar.handleEvent(event, activeAction);
                compLib.handleEvent(event, selectedComponent);

                if (activeAction == "Save") {
                    isSaveDialogOpen = true;
                    saveFileName = "";
                    activeAction = "";
                } else if (activeAction == "Load") {
                    canvas = nullptr;
                    canvasRenderer = nullptr;
                    placedComponents.clear();
                    currentState = AppState::MainMenu;
                    activeAction = "";
                } else if (activeAction == "Grid Toggle" && canvas) {
                    canvas->grid().setVisible(!canvas->grid().isVisible());
                    activeAction = "";
                } else if (activeAction == "Main Menu") {
                    canvas = nullptr;
                    canvasRenderer = nullptr;
                    placedComponents.clear();
                    currentState = AppState::MainMenu;
                    activeAction = "";
                }

                if (canvas) {
                    float canvasMouseX = event.motion.x - 180.0f;
                    float canvasMouseY = event.motion.y - 50.0f;

                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        canvas->setMouseScreenPosition({canvasMouseX, canvasMouseY});

                        if (isPanning) {
                            canvas->pan({event.motion.xrel, event.motion.yrel});
                        }
                        else if (isDraggingComponents) {
                            Point currentWorldMouse = canvas->mouseWorldPosition();
                            Point mouseDelta = currentWorldMouse - dragStartWorldMouse;

                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) {
                                    Point targetPos = comp.dragStartPos + mouseDelta;
                                    comp.worldPos = canvas->snapToGrid(targetPos);
                                    comp.updatePinPositions();
                                }
                            }
                        }
                        else if (isSelectDragging) {
                            float x1 = selectDragStartScreen.x;
                            float y1 = selectDragStartScreen.y;
                            float x2 = static_cast<float>(event.motion.x);
                            float y2 = static_cast<float>(event.motion.y);

                            visualSelectBox.x = std::min(x1, x2);
                            visualSelectBox.y = std::min(y1, y2);
                            visualSelectBox.w = std::abs(x2 - x1);
                            visualSelectBox.h = std::abs(y2 - y1);
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                        if (event.wheel.y > 0) canvas->zoomBy(1.1f);
                        else if (event.wheel.y < 0) canvas->zoomBy(0.9f);
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (event.button.button == SDL_BUTTON_MIDDLE) {
                            isPanning = true;
                        }
                        else if (event.button.button == SDL_BUTTON_RIGHT) {
                            selectedComponent = "None";
                        }
                        else if (event.button.button == SDL_BUTTON_LEFT) {
                            if (event.button.x >= 180 && event.button.x <= 800 && event.button.y >= 50 && event.button.y <= 600) {

                                if (selectedComponent != "None" && !selectedComponent.empty()) {
                                    Point worldTarget = canvas->snapToGrid(canvas->mouseWorldPosition());

                                    char prefix = std::toupper(selectedComponent[0]);
                                    std::string prefixStr(1, prefix);
                                    componentCounters[prefixStr]++;
                                    std::string finalId = prefixStr + std::to_string(componentCounters[prefixStr]);

                                    placedComponents.emplace_back(selectedComponent, finalId, "", worldTarget);
                                }
                                else {
                                    Point worldMouse = canvas->mouseWorldPosition();
                                    bool hitDetected = false;
                                    int hitIndex = -1;

                                    int totalComponents = static_cast<int>(placedComponents.size());
                                    for (int i = totalComponents - 1; i >= 0; --i) {
                                        SDL_FRect box = placedComponents[i].getWorldBoundingBox();
                                        if (worldMouse.x >= box.x && worldMouse.x <= box.x + box.w &&
                                            worldMouse.y >= box.y && worldMouse.y <= box.y + box.h) {
                                            hitDetected = true;
                                            hitIndex = i;
                                            break;
                                        }
                                    }

                                    if (hitDetected) {
                                        // ==========================================
                                        // بخش ۴.۷: تشخیص دابل‌کلیک جهت باز کردن ویژگی‌ها
                                        // ==========================================
                                        if (event.button.clicks == 2) {
                                            isPropertiesDialogOpen = true;
                                            editingComponentIndex = hitIndex;
                                            editLabelBuf = placedComponents[hitIndex].labelId;
                                            editValueBuf = placedComponents[hitIndex].valueStr;
                                            activeEditField = 0;
                                            isDraggingComponents = false; // لغو پروسه جابجایی
                                        }
                                        else {
                                            // کلیک تک معمولی جهت انتخاب یا جابجایی
                                            if (!placedComponents[hitIndex].isSelected) {
                                                if (!(event.key.mod & SDL_KMOD_SHIFT)) {
                                                    for (auto& comp : placedComponents) comp.isSelected = false;
                                                }
                                                placedComponents[hitIndex].isSelected = true;
                                            }

                                            isDraggingComponents = true;
                                            dragStartWorldMouse = worldMouse;
                                            for (auto& comp : placedComponents) {
                                                if (comp.isSelected) {
                                                    comp.dragStartPos = comp.worldPos;
                                                }
                                            }
                                        }
                                    }
                                    else {
                                        if (!(event.key.mod & SDL_KMOD_SHIFT)) {
                                            for (auto& comp : placedComponents) comp.isSelected = false;
                                        }
                                        isSelectDragging = true;
                                        selectDragStartScreen = { static_cast<float>(event.button.x), static_cast<float>(event.button.y) };
                                        visualSelectBox = { static_cast<float>(event.button.x), static_cast<float>(event.button.y), 0.0f, 0.0f };
                                    }
                                }
                            }
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                        if (event.button.button == SDL_BUTTON_MIDDLE) {
                            isPanning = false;
                        }
                        else if (event.button.button == SDL_BUTTON_LEFT) {
                            if (isDraggingComponents) {
                                isDraggingComponents = false;
                            }
                            else if (isSelectDragging) {
                                isSelectDragging = false;

                                Point worldStart = canvas->screenToWorld({ visualSelectBox.x - 180.0f, visualSelectBox.y - 50.0f });
                                Point worldEnd = canvas->screenToWorld({ (visualSelectBox.x + visualSelectBox.w) - 180.0f, (visualSelectBox.y + visualSelectBox.h) - 50.0f });

                                float minX = std::min(worldStart.x, worldEnd.x);
                                float maxX = std::max(worldStart.x, worldEnd.x);
                                float minY = std::min(worldStart.y, worldEnd.y);
                                float maxY = std::max(worldStart.y, worldEnd.y);

                                for (auto& comp : placedComponents) {
                                    SDL_FRect box = comp.getWorldBoundingBox();
                                    if (box.x >= minX && (box.x + box.w) <= maxX && box.y >= minY && (box.y + box.h) <= maxY) {
                                        comp.isSelected = true;
                                    }
                                }
                            }
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
                    placedComponents.clear();
                    componentCounters.clear();

                    if (startMenu.shouldLoadProject()) {
                        std::vector<std::string> loadedList;
                        if (ProjectManager::loadProject(startMenu.getSelectedProjectFile(), loadedList)) {
                            compLib.setActiveList(loadedList);
                        }
                        startMenu.resetLoadProject();
                    } else {
                        compLib.setActiveList({});
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
                    canvasRenderer->renderComponentsSDL(renderer, font, placedComponents);

                    SDL_SetRenderViewport(renderer, nullptr);

                    toolbar.render(renderer, font);
                    compLib.render(renderer, font, selectedComponent);

                    if (isSelectDragging) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

                        SDL_SetRenderDrawColor(renderer, 0, 120, 215, 45);
                        SDL_RenderFillRect(renderer, &visualSelectBox);

                        SDL_SetRenderDrawColor(renderer, 0, 120, 215, 255);
                        SDL_RenderRect(renderer, &visualSelectBox);

                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                    }

                    // ==========================================
                    // رندر گرافیکی پنجره ویژگی‌ها مدال (۴.۷)
                    // ==========================================
                    if (isPropertiesDialogOpen && editingComponentIndex >= 0 && editingComponentIndex < static_cast<int>(placedComponents.size())) {
                        const auto& comp = placedComponents[editingComponentIndex];

                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                        SDL_FRect screenRect{0, 0, static_cast<float>(currentW), static_cast<float>(currentH)};
                        SDL_RenderFillRect(renderer, &screenRect);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        // کادر شکیل مدال تنظیمات ویژگی‌ها
                        SDL_FRect dialogRect{ currentW / 2.0f - 180.0f, currentH / 2.0f - 120.0f, 360.0f, 240.0f };
                        SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255);
                        SDL_RenderFillRect(renderer, &dialogRect);
                        SDL_SetRenderDrawColor(renderer, 100, 110, 130, 255);
                        SDL_RenderRect(renderer, &dialogRect);

                        // سربرگ عنوان پنجره
                        std::string titleStr = comp.type + " Properties";
                        renderText(renderer, font, titleStr, currentW / 2.0f, dialogRect.y + 15.0f, {255, 255, 255, 255});

                        // باکس ورودی فیلد اول: شناسه مدار (Label Designator)
                        renderText(renderer, font, "Designator Part Label:", dialogRect.x + 20.0f, dialogRect.y + 55.0f, {200, 210, 230, 255}, false);
                        SDL_FRect labelBoxRect{ dialogRect.x + 20.0f, dialogRect.y + 82.0f, dialogRect.w - 40.0f, 32.0f };
                        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255);
                        SDL_RenderFillRect(renderer, &labelBoxRect);
                        SDL_SetRenderDrawColor(renderer, activeEditField == 0 ? 66 : 80, activeEditField == 0 ? 153 : 90, activeEditField == 0 ? 225 : 110, 255);
                        SDL_RenderRect(renderer, &labelBoxRect);
                        std::string dispLabel = editLabelBuf;
                        if (activeEditField == 0 && (SDL_GetTicks() / 500) % 2 == 0) dispLabel += "_";
                        renderText(renderer, font, dispLabel, labelBoxRect.x + 8.0f, labelBoxRect.y + 4.0f, {255, 255, 255, 255}, false);

                        // تغییر پویای برچسب فیلد دوم متناسب با ماهیت الکترونیکی المان جاری (بند ۷.۴ سند طراحی)
                        std::string valPrompt = "Technical Value Specification:";
                        if (comp.type == "Resistor") valPrompt = "Resistance Value (Ohm):";
                        else if (comp.type == "Capacitor") valPrompt = "Capacitance (Farad):";
                        else if (comp.type == "DC Source" || comp.type == "AC Source") valPrompt = "Source Voltage (Volt):";
                        else if (comp.type == "Inductor") valPrompt = "Inductance Value (Henry):";

                        // باکس ورودی فیلد دوم: مقدار مهندسی (Technical Value)
                        renderText(renderer, font, valPrompt, dialogRect.x + 20.0f, dialogRect.y + 125.0f, {200, 210, 230, 255}, false);
                        SDL_FRect valBoxRect{ dialogRect.x + 20.0f, dialogRect.y + 152.0f, dialogRect.w - 40.0f, 32.0f };
                        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255);
                        SDL_RenderFillRect(renderer, &valBoxRect);
                        SDL_SetRenderDrawColor(renderer, activeEditField == 1 ? 66 : 80, activeEditField == 1 ? 153 : 90, activeEditField == 1 ? 225 : 110, 255);
                        SDL_RenderRect(renderer, &valBoxRect);
                        std::string dispVal = editValueBuf;
                        if (activeEditField == 1 && (SDL_GetTicks() / 500) % 2 == 0) dispVal += "_";
                        renderText(renderer, font, dispVal, valBoxRect.x + 8.0f, valBoxRect.y + 4.0f, {255, 255, 255, 255}, false);

                        // راهنمای ناوبری پنجره
                        renderText(renderer, font, "TAB: Switch | ENTER: Confirm | ESC: Cancel", currentW / 2.0f, dialogRect.y + 205.0f, {150, 160, 180, 255});
                    }

                    // --- رسم کردن پنجره پاپ‌آپ Save رو صفحه ---
                    if (isSaveDialogOpen) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                        SDL_FRect screenRect{0, 0, static_cast<float>(currentW), static_cast<float>(currentH)};
                        SDL_RenderFillRect(renderer, &screenRect);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        SDL_FRect dialogRect{ currentW / 2.0f - 160.0f, currentH / 2.0f - 80.0f, 320.0f, 160.0f };
                        SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255);
                        SDL_RenderFillRect(renderer, &dialogRect);
                        SDL_SetRenderDrawColor(renderer, 100, 110, 130, 255);
                        SDL_RenderRect(renderer, &dialogRect);

                        renderText(renderer, font, "Enter Project Name:", currentW / 2.0f, dialogRect.y + 20.0f, {255, 255, 255, 255});

                        SDL_FRect inputRect{ dialogRect.x + 20.0f, dialogRect.y + 60.0f, dialogRect.w - 40.0f, 40.0f };
                        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255);
                        SDL_RenderFillRect(renderer, &inputRect);
                        SDL_SetRenderDrawColor(renderer, 80, 90, 110, 255);
                        SDL_RenderRect(renderer, &inputRect);

                        std::string displayText = saveFileName;
                        if ((SDL_GetTicks() / 500) % 2 == 0) displayText += "_";
                        renderText(renderer, font, displayText, inputRect.x + 10.0f, inputRect.y + 8.0f, {200, 210, 230, 255}, false);

                        renderText(renderer, font, "Press ENTER to save, ESC to cancel", currentW / 2.0f, dialogRect.y + 120.0f, {150, 160, 180, 255});
                    }
                }
                break;

            default:
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