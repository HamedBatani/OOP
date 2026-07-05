// src/main.cpp
// Project Signature: mosayeb agheli

#include "AppState.h"
#include "StartMenu.h"
#include "Canvas.h"
#include "CanvasRenderer.h"
#include "Toolbar.h"
#include "ComponentLibrary.h"
#include "ProjectManager.h"
#include "ComponentInstance.h"
#include "Wire.h"
#include "Junction.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <string>
#include <memory>
#include <map>
#include <algorithm>
#include <cctype>
#include <set>

namespace {
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;

    void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, float x, float y, SDL_Color color, bool centered = true) {
        if (!renderer || !font || text.empty()) return;
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
        if (!surface) return;
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) { SDL_DestroySurface(surface); return; }

        SDL_FRect destination{ centered ? x - static_cast<float>(surface->w) / 2.0f : x, y, static_cast<float>(surface->w), static_cast<float>(surface->h) };
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

    // -----------------------------------------------------------------
    // تابع کمکی جدید: پیدا کردن یک وایر در vector<Wire> بر اساس uid
    // (لازم است چون WireAnchor از uid استفاده می‌کند، نه ایندکس، تا با
    //  حذف/جابجایی وایرهای دیگر خراب نشود)
    // -----------------------------------------------------------------
    Wire* findWireByUid(std::vector<Wire>& wires, const std::string& uid) {
        for (auto& w : wires) {
            if (w.uid == uid) return &w;
        }
        return nullptr;
    }

    // -----------------------------------------------------------------
    // تابع کمکی جدید: محاسبه‌ی موقعیت واقعی فعلی یک انکر
    // اگر PinLock باشد، از لیست کامپوننت‌ها موقعیت پین را می‌گیرد
    // اگر WireLock باشد، از وایر میزبان (با uid) موقعیت روی سگمنت را می‌گیرد
    // اگر Free باشد، همان cachedWorldPos را برمی‌گرداند
    // -----------------------------------------------------------------
    Point resolveAnchorPosition(const WireAnchor& anchor,
                                const std::vector<ComponentInstance>& components,
                                std::vector<Wire>& wires) {
        if (anchor.isPinLock()) {
            for (const auto& comp : components) {
                if (comp.labelId == anchor.lockedCompId) {
                    for (const auto& pin : comp.pins) {
                        if (pin.designation == anchor.lockedPinName) {
                            return pin.calculatedWorldPos;
                        }
                    }
                }
            }
            return anchor.cachedWorldPos;
        }
        if (anchor.isWireLock()) {
            Wire* hostWire = findWireByUid(wires, anchor.lockedWireUid);
            if (hostWire) {
                return hostWire->resolvePointOnSegment(anchor.lockedSegmentIndex, anchor.lockedSegmentT);
            }
            return anchor.cachedWorldPos;
        }
        return anchor.cachedWorldPos;
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

    Toolbar toolbar{ 0, 0, 800, 45 };
    ComponentLibrary compLib{ 0, 45, 200, 555 };

    std::unique_ptr<Canvas> canvas = nullptr;
    std::unique_ptr<CanvasRenderer> canvasRenderer = nullptr;

    bool isPanning = false;
    std::string activeAction = "";
    std::string selectedComponent = "None";
    bool isSaveDialogOpen = false;
    std::string saveFileName = "";

    std::vector<ComponentInstance> placedComponents;
    std::map<std::string, int> componentCounters;
    std::vector<Wire> circuitWires;

    bool isSelectDragging = false;
    Point selectDragStartScreen{0.0f, 0.0f};
    SDL_FRect visualSelectBox{0.0f, 0.0f, 0.0f, 0.0f};

    bool isDraggingComponents = false;
    Point dragStartWorldMouse{0.0f, 0.0f};

    bool isWireModeActive = false;

    bool isWiring = false;
    Point wiringStartPoint{0.0f, 0.0f};
    std::string activeWiringCompId = "";
    std::string activeWiringPinName = "";

    bool isPropertiesDialogOpen = false;
    int editingComponentIndex = -1;
    std::string editLabelBuf = "";
    std::string editValueBuf = "";
    int activeEditField = 0;

    bool isContextMenuOpen = false;
    Point contextMenuPos{0.0f, 0.0f};
    int contextTargetIndex = -1;
    const std::vector<std::string> contextOptions = {"Properties", "Rotate 90", "Mirror Horiz", "Mirror Vert", "Delete"};
    int hoveredContextOption = -1;

    // ------------------------------------------------------------------
    // متغیرهای جدید برای درگ کردن کل وایر (فقط وقتی هر دو سر آزاد باشند -
    // طبق تصمیم گرفته‌شده در گفتگو) و برای نگه‌داشتن هایلایت وایر هاور شده
    // ------------------------------------------------------------------
    bool isDraggingWire = false;
    int draggingWireIndex = -1;
    Point wireDragStartWorldMouse{0.0f, 0.0f};
    std::vector<Point> wireDragStartRoutingPoints;

    int hoveredWireIndexForDrag = -1;

    // ------------------------------------------------------------------
    // متغیرهای جدید برای درگ تک-نقطه‌ای انتهای آزاد یک وایر (حل درخواست
    // اول کاربر: «وقتی وایری رو تا یه جا می‌کشم و متوقف می‌کنم، بعدش
    // بتونم همون نقطه‌ی تنها/آزادو دوباره بگیرم و بکشم و به یه جای
    // دیگه وصل کنم»). این با درگ کل وایر فرق دارد: فقط یک سر آزاد
    // حرکت می‌کند، نه کل بدنه.
    // ------------------------------------------------------------------
    bool isDraggingWireEndpoint = false;
    int draggingEndpointWireIndex = -1;
    bool draggingEndpointIsStart = false; // true اگر سر شروع، false اگر سر پایان
    const float endpointGrabRadius = 10.0f;

    // ------------------------------------------------------------------
    // تابع جدید: propagateWireUpdates
    // این تابع جایگزین updateConnectedWires نمی‌شود، بلکه علاوه بر آن
    // اضافه می‌شود و منطق قدیمی updateConnectedWires را هم فراخوانی می‌کند.
    // کارش این است که بعد از هرگونه تغییر (جابجایی کامپوننت یا وایر)،
    // تمام وایرهایی که به صورت WireLock به وایرهای دیگر گره خورده‌اند را
    // هم به‌روز کند (حل مشکل ۱ و ۲ گفتگو: زنجیره‌ی junctionها).
    // از یک حلقه‌ی محدود (چند بار تکرار) استفاده می‌شود تا زنجیره‌های
    // چندسطحی از junctionها هم درست resolve شوند، بدون ریسک حلقه‌ی بی‌نهایت.
    // ------------------------------------------------------------------
    auto updateConnectedWires = [&](const ComponentInstance& comp) {
        for (auto& wire : circuitWires) {
            if (wire.startCompId == comp.labelId) {
                for (const auto& p : comp.pins) {
                    if (p.designation == wire.startPinName) {
                        // فیکس مشکل «به‌هم‌ریختن روتینگ»: به‌جای بازسازی کامل مسیر با
                        // یک خم ساده (updateOrthogonalRoute)، از rerouteSmartPreservingShape
                        // استفاده می‌کنیم که شکل نسبی مسیر قبلی (تعداد و جهت خم‌ها) را
                        // حفظ می‌کند و فقط بخش نزدیک به سر جابجاشده را تغییر می‌دهد --
                        // دقیقاً طبق مثالی که در گفتگو توضیح داده شد.
                        Point endPos = wire.routingPoints.empty() ? p.calculatedWorldPos : wire.routingPoints.back();
                        wire.rerouteSmartPreservingShape(p.calculatedWorldPos, endPos);
                        wire.startAnchor.cachedWorldPos = p.calculatedWorldPos;
                        break;
                    }
                }
            }
            if (wire.endCompId == comp.labelId) {
                for (const auto& p : comp.pins) {
                    if (p.designation == wire.endPinName) {
                        Point startPos = wire.routingPoints.empty() ? p.calculatedWorldPos : wire.routingPoints.front();
                        wire.rerouteSmartPreservingShape(startPos, p.calculatedWorldPos);
                        wire.endAnchor.cachedWorldPos = p.calculatedWorldPos;
                        break;
                    }
                }
            }
        }
    };

    auto propagateWireUpdates = [&](int maxIterations = 4) {
        // چند بار تکرار می‌کنیم چون ممکن است وایر A به وایر B و وایر B هم
        // به وایر C وصل باشد (زنجیره‌ی چندسطحی). با چند پاس، همه resolve می‌شوند.
        for (int iteration = 0; iteration < maxIterations; ++iteration) {
            bool anyChanged = false;

            for (auto& wire : circuitWires) {
                Point currentStart = wire.routingPoints.empty() ? Point{0.0f, 0.0f} : wire.routingPoints.front();
                Point currentEnd = wire.routingPoints.empty() ? Point{0.0f, 0.0f} : wire.routingPoints.back();

                Point newStart = currentStart;
                Point newEnd = currentEnd;

                if (wire.startAnchor.isWireLock()) {
                    newStart = resolveAnchorPosition(wire.startAnchor, placedComponents, circuitWires);
                } else if (wire.startAnchor.isPinLock()) {
                    newStart = resolveAnchorPosition(wire.startAnchor, placedComponents, circuitWires);
                }

                if (wire.endAnchor.isWireLock()) {
                    newEnd = resolveAnchorPosition(wire.endAnchor, placedComponents, circuitWires);
                } else if (wire.endAnchor.isPinLock()) {
                    newEnd = resolveAnchorPosition(wire.endAnchor, placedComponents, circuitWires);
                }

                const float epsilon = 0.01f;
                bool changed = (std::hypot(newStart.x - currentStart.x, newStart.y - currentStart.y) > epsilon) ||
                               (std::hypot(newEnd.x - currentEnd.x, newEnd.y - currentEnd.y) > epsilon);

                if (changed) {
                    // فیکس مشکل «به‌هم‌ریختن روتینگ»: همان دلیلی که در updateConnectedWires
                    // توضیح داده شد -- به‌جای بازسازی کامل، شکل نسبی مسیر حفظ می‌شود
                    wire.rerouteSmartPreservingShape(newStart, newEnd);
                    anyChanged = true;
                }
            }

            if (!anyChanged) break;
        }
    };

    // ------------------------------------------------------------------
    // تابع جدید: rehomeWireEndpointIfOnWire
    // وقتی کاربر یک وایر جدید را تا وسط یک وایر دیگر می‌کشد و رها می‌کند،
    // این تابع بررسی می‌کند که آیا نقطه‌ی رهاسازی روی بدنه‌ی یک وایر دیگر
    // افتاده است یا نه. اگر بله، به‌جای یک Point آزاد صرف، یک WireLock
    // واقعی می‌سازد تا این دو وایر واقعاً به هم گره بخورند (حل اصلی‌ترین
    // بخش مشکل ۱ و ۲ گفتگو).
    // ------------------------------------------------------------------
    auto rehomeWireEndpointIfOnWire = [&](Wire& wire, bool isStartEndpoint, const Point& releasePoint, float tolerance) -> Point {
        Wire::ClosestPointResult best;
        best.distance = 1e9f;
        int bestWireIdx = -1;

        for (size_t i = 0; i < circuitWires.size(); ++i) {
            // یک وایر نباید به خودش قفل شود
            if (circuitWires[i].uid == wire.uid) continue;
            if (!circuitWires[i].isCompleted) continue;

            auto candidate = circuitWires[i].findClosestPointOnWire(releasePoint);
            if (candidate.found && candidate.distance < tolerance && candidate.distance < best.distance) {
                best = candidate;
                bestWireIdx = static_cast<int>(i);
            }
        }

        if (bestWireIdx >= 0) {
            WireAnchor lockedAnchor = WireAnchor::makeWireLock(
                    circuitWires[bestWireIdx].uid, best.segmentIndex, best.t, best.point);
            if (isStartEndpoint) {
                wire.startAnchor = lockedAnchor;
            } else {
                wire.endAnchor = lockedAnchor;
            }
            return best.point;
        }

        // اگر روی هیچ وایر دیگری نبود، انکر آزاد باقی می‌ماند (رفتار قدیمی حفظ می‌شود)
        if (isStartEndpoint) {
            if (wire.startAnchor.kind != AnchorKind::PinLock) {
                wire.startAnchor = WireAnchor::makeFree(releasePoint);
            }
        } else {
            if (wire.endAnchor.kind != AnchorKind::PinLock) {
                wire.endAnchor = WireAnchor::makeFree(releasePoint);
            }
        }
        return releasePoint;
    };

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

                if (isPropertiesDialogOpen) {
                    if (event.type == SDL_EVENT_TEXT_INPUT) {
                        if (activeEditField == 0 && editLabelBuf.size() < 10) editLabelBuf += event.text.text;
                        else if (activeEditField == 1 && editValueBuf.size() < 12) editValueBuf += event.text.text;
                    } else if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.key == SDLK_BACKSPACE) {
                            if (activeEditField == 0 && !editLabelBuf.empty()) editLabelBuf.pop_back();
                            else if (activeEditField == 1 && !editValueBuf.empty()) editValueBuf.pop_back();
                        } else if (event.key.key == SDLK_TAB) {
                            activeEditField = (activeEditField + 1) % 2;
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
                    continue;
                }

                if (isSaveDialogOpen) {
                    if (event.type == SDL_EVENT_TEXT_INPUT) {
                        if (saveFileName.size() < 25) saveFileName += event.text.text;
                    } else if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.key == SDLK_BACKSPACE && !saveFileName.empty()) {
                            saveFileName.pop_back();
                        } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                            if (!saveFileName.empty()) {
                                std::string fullPath = saveFileName + ".txt";
                                if (ProjectManager::saveProject(fullPath, compLib.getActiveList(), placedComponents, circuitWires)) {
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

                if (isContextMenuOpen) {
                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        float mx = event.motion.x, my = event.motion.y;
                        if (mx >= contextMenuPos.x && mx <= contextMenuPos.x + 160.0f && my >= contextMenuPos.y && my <= contextMenuPos.y + (contextOptions.size() * 35.0f)) {
                            hoveredContextOption = static_cast<int>((my - contextMenuPos.y) / 35.0f);
                        } else hoveredContextOption = -1;
                    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (event.button.button == SDL_BUTTON_LEFT) {
                            float mx = event.button.x, my = event.button.y;
                            if (mx >= contextMenuPos.x && mx <= contextMenuPos.x + 160.0f && my >= contextMenuPos.y && my <= contextMenuPos.y + (contextOptions.size() * 35.0f)) {
                                int selectedOption = static_cast<int>((my - contextMenuPos.y) / 35.0f);
                                if (contextTargetIndex >= 0 && contextTargetIndex < static_cast<int>(placedComponents.size())) {
                                    auto& comp = placedComponents[contextTargetIndex];
                                    if (selectedOption == 0) {
                                        isPropertiesDialogOpen = true; editingComponentIndex = contextTargetIndex;
                                        editLabelBuf = comp.labelId; editValueBuf = comp.valueStr; activeEditField = 0;
                                    } else if (selectedOption == 1) {
                                        comp.rotationDegrees = (comp.rotationDegrees + 90) % 360;
                                        comp.updatePinPositions();
                                        updateConnectedWires(comp);
                                        propagateWireUpdates();
                                    } else if (selectedOption == 2) {
                                        comp.isMirroredH = !comp.isMirroredH;
                                        comp.updatePinPositions();
                                        updateConnectedWires(comp);
                                        propagateWireUpdates();
                                    } else if (selectedOption == 3) {
                                        comp.isMirroredV = !comp.isMirroredV;
                                        comp.updatePinPositions();
                                        updateConnectedWires(comp);
                                        propagateWireUpdates();
                                    } else if (selectedOption == 4) {
                                        std::string delId = comp.labelId;
                                        circuitWires.erase(std::remove_if(circuitWires.begin(), circuitWires.end(), [&](const Wire& w) {
                                            return w.startCompId == delId || w.endCompId == delId;
                                        }), circuitWires.end());
                                        placedComponents.erase(placedComponents.begin() + contextTargetIndex);
                                    }
                                }
                            }
                            isContextMenuOpen = false; continue;
                        } else if (event.button.button == SDL_BUTTON_RIGHT) isContextMenuOpen = false;
                    }
                    if (event.type != SDL_EVENT_MOUSE_MOTION) continue;
                }

                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_ESCAPE) {
                        selectedComponent = "None";
                        isWireModeActive = false;
                        for (auto& comp : placedComponents) comp.isSelected = false;
                        for (auto& wire : circuitWires) wire.isSelected = false;
                        if (isWiring) { isWiring = false; circuitWires.pop_back(); }
                        if (isDraggingWire) { isDraggingWire = false; draggingWireIndex = -1; }
                        if (isDraggingWireEndpoint) { isDraggingWireEndpoint = false; draggingEndpointWireIndex = -1; }
                    }
                    else if ((event.key.mod & SDL_KMOD_CTRL) && event.key.key == SDLK_S) {
                        isSaveDialogOpen = true; saveFileName = "";
                    }
                    else if (selectedComponent == "None" || selectedComponent.empty()) {
                        if (event.key.key == SDLK_R) {
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) {
                                    comp.rotationDegrees = (comp.rotationDegrees + 90) % 360;
                                    comp.updatePinPositions();
                                    updateConnectedWires(comp);
                                    propagateWireUpdates();
                                }
                            }
                        }
                        else if (event.key.key == SDLK_M) {
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) {
                                    comp.isMirroredH = !comp.isMirroredH;
                                    comp.updatePinPositions();
                                    updateConnectedWires(comp);
                                    propagateWireUpdates();
                                }
                            }
                        }
                        else if (event.key.key == SDLK_V) {
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) {
                                    comp.isMirroredV = !comp.isMirroredV;
                                    comp.updatePinPositions();
                                    updateConnectedWires(comp);
                                    propagateWireUpdates();
                                }
                            }
                        }
                        else if (event.key.key == SDLK_DELETE || event.key.key == SDLK_BACKSPACE) {
                            std::vector<std::string> idsToDelete;
                            for(auto& c : placedComponents) if(c.isSelected) idsToDelete.push_back(c.labelId);

                            std::set<std::string> uidsToDelete;
                            for (auto& w : circuitWires) {
                                if (w.isSelected) uidsToDelete.insert(w.uid);
                            }

                            circuitWires.erase(std::remove_if(circuitWires.begin(), circuitWires.end(), [&](const Wire& w) {
                                return w.isSelected ||
                                       std::find(idsToDelete.begin(), idsToDelete.end(), w.startCompId) != idsToDelete.end() ||
                                       std::find(idsToDelete.begin(), idsToDelete.end(), w.endCompId) != idsToDelete.end();
                            }), circuitWires.end());

                            // اگر وایری که حذف شد میزبان یک WireLock بود، آن انکرها را آزاد کن
                            // (جلوگیری از dangling reference به یک uid دیگر موجود نیست)
                            for (auto& w : circuitWires) {
                                if (w.startAnchor.isWireLock() && uidsToDelete.count(w.startAnchor.lockedWireUid) > 0) {
                                    w.startAnchor = WireAnchor::makeFree(w.routingPoints.empty() ? Point{0.0f,0.0f} : w.routingPoints.front());
                                }
                                if (w.endAnchor.isWireLock() && uidsToDelete.count(w.endAnchor.lockedWireUid) > 0) {
                                    w.endAnchor = WireAnchor::makeFree(w.routingPoints.empty() ? Point{0.0f,0.0f} : w.routingPoints.back());
                                }
                            }

                            placedComponents.erase(std::remove_if(placedComponents.begin(), placedComponents.end(),
                                                                  [](const ComponentInstance& comp) { return comp.isSelected; }), placedComponents.end());
                        }
                    }
                }

                toolbar.handleEvent(event, activeAction);
                compLib.handleEvent(event, selectedComponent);

                if (activeAction == "Wire Toggle") {
                    isWireModeActive = !isWireModeActive;
                    selectedComponent = "None";
                    for (auto& comp : placedComponents) comp.isSelected = false;
                    activeAction = "";
                } else if (activeAction == "Save") {
                    isSaveDialogOpen = true; saveFileName = ""; activeAction = "";
                } else if (activeAction == "Load") {
                    canvas = nullptr; canvasRenderer = nullptr; placedComponents.clear(); circuitWires.clear(); currentState = AppState::MainMenu; activeAction = "";
                } else if (activeAction == "Grid Toggle" && canvas) {
                    canvas->grid().setVisible(!canvas->grid().isVisible()); activeAction = "";
                } else if (activeAction == "Main Menu") {
                    canvas = nullptr; canvasRenderer = nullptr; placedComponents.clear(); circuitWires.clear(); currentState = AppState::MainMenu; activeAction = "";
                }

                if (canvas) {
                    float canvasMouseX = event.motion.x - 200.0f;
                    float canvasMouseY = event.motion.y - 45.0f;

                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        canvas->setMouseScreenPosition({canvasMouseX, canvasMouseY});
                        Point currentWorldMouse = canvas->mouseWorldPosition();

                        if (isPanning) {
                            canvas->pan({event.motion.xrel, event.motion.yrel});
                        }
                        else if (isDraggingComponents) {
                            Point mouseDelta = currentWorldMouse - dragStartWorldMouse;

                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) {
                                    comp.worldPos = canvas->snapToGrid(comp.dragStartPos + mouseDelta);
                                    comp.updatePinPositions();
                                    updateConnectedWires(comp);
                                }
                            }
                            propagateWireUpdates();
                        }
                        else if (isDraggingWire && draggingWireIndex >= 0 && draggingWireIndex < static_cast<int>(circuitWires.size())) {
                            // -------------------------------------------------------------
                            // منطق جدید: درگ کردن کل وایر (فقط وقتی isFullyFree() باشد -
                            // این شرط پیش از شروع درگ در MOUSE_BUTTON_DOWN چک شده است)
                            // -------------------------------------------------------------
                            Point mouseDelta = currentWorldMouse - wireDragStartWorldMouse;
                            Point snappedDelta = canvas->snapToGrid(mouseDelta);

                            Wire& draggedWire = circuitWires[draggingWireIndex];
                            draggedWire.routingPoints.clear();
                            for (const auto& originalPoint : wireDragStartRoutingPoints) {
                                draggedWire.routingPoints.push_back(originalPoint + snappedDelta);
                            }
                            if (!draggedWire.routingPoints.empty()) {
                                draggedWire.startAnchor.cachedWorldPos = draggedWire.routingPoints.front();
                                draggedWire.endAnchor.cachedWorldPos = draggedWire.routingPoints.back();
                            }
                            propagateWireUpdates();
                        }
                        else if (isDraggingWireEndpoint && draggingEndpointWireIndex >= 0 && draggingEndpointWireIndex < static_cast<int>(circuitWires.size())) {
                            // -------------------------------------------------------------
                            // منطق جدید: درگ تک-نقطه‌ای نوک آزاد یک وایر (حل درخواست اول
                            // کاربر در گفتگو). هنگام درگ، پین‌های نزدیک هایلایت می‌شوند تا
                            // کاربر بتواند نوک را به یک پین واقعی اسنپ/وصل کند.
                            // -------------------------------------------------------------
                            float sensitivity = 10.0f / canvas->zoom();
                            for (auto& comp : placedComponents) comp.checkPinHover(currentWorldMouse, sensitivity);

                            Point targetPoint = canvas->snapToGrid(currentWorldMouse);
                            for (auto& comp : placedComponents) {
                                for (auto& pin : comp.pins) {
                                    if (pin.isHighlighted) { targetPoint = pin.calculatedWorldPos; }
                                }
                            }

                            Wire& w = circuitWires[draggingEndpointWireIndex];
                            if (draggingEndpointIsStart) {
                                Point fixedEnd = w.routingPoints.empty() ? targetPoint : w.routingPoints.back();
                                w.rerouteSmartPreservingShape(targetPoint, fixedEnd);
                            } else {
                                Point fixedStart = w.routingPoints.empty() ? targetPoint : w.routingPoints.front();
                                w.rerouteSmartPreservingShape(fixedStart, targetPoint);
                            }
                        }
                        else if (isSelectDragging) {
                            float x1 = selectDragStartScreen.x, y1 = selectDragStartScreen.y;
                            float x2 = static_cast<float>(event.motion.x), y2 = static_cast<float>(event.motion.y);
                            visualSelectBox.x = std::min(x1, x2); visualSelectBox.y = std::min(y1, y2);
                            visualSelectBox.w = std::abs(x2 - x1); visualSelectBox.h = std::abs(y2 - y1);
                        }
                        else {
                            // --- باگ اینجا بود! ---
                            // تشخیص پین‌ها رو آوردم بيرون تا همیشه، حتی وقتي isWiring روشنه، چک کنه ببینه موس رو کدوم پینه
                            float sensitivity = 10.0f / canvas->zoom();
                            for (auto& comp : placedComponents) comp.checkPinHover(currentWorldMouse, sensitivity);

                            if (isWiring) {
                                Point targetPoint = canvas->snapToGrid(currentWorldMouse);
                                for(auto& comp : placedComponents) {
                                    for(auto& pin : comp.pins) {
                                        if(pin.isHighlighted) { targetPoint = pin.calculatedWorldPos; }
                                    }
                                }
                                circuitWires.back().updateOrthogonalRoute(wiringStartPoint, targetPoint);
                            } else {
                                // -------------------------------------------------------------
                                // منطق جدید: هاور روی بدنه‌ی یک وایر آزاد، برای دادن فیدبک بصری
                                // که این وایر قابل درگ کامل است (فقط برای وایرهای isFullyFree)
                                // -------------------------------------------------------------
                                hoveredWireIndexForDrag = -1;
                                float wireSensitivity = 6.0f / canvas->zoom();
                                for (int i = static_cast<int>(circuitWires.size()) - 1; i >= 0; --i) {
                                    if (circuitWires[i].isFullyFree() && circuitWires[i].containsPoint(currentWorldMouse, wireSensitivity)) {
                                        hoveredWireIndexForDrag = i;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                        if (event.wheel.y > 0) canvas->zoomBy(1.1f);
                        else if (event.wheel.y < 0) canvas->zoomBy(0.9f);

                        Point currentWorldMouse = canvas->mouseWorldPosition();
                        float sensitivity = 10.0f / canvas->zoom();
                        for (auto& comp : placedComponents) comp.checkPinHover(currentWorldMouse, sensitivity);
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (event.button.button == SDL_BUTTON_MIDDLE) isPanning = true;
                        else if (event.button.button == SDL_BUTTON_RIGHT) {
                            selectedComponent = "None";
                            isWireModeActive = false;
                            if (isWiring) { isWiring = false; circuitWires.pop_back(); continue; }

                            if (event.button.x >= 200 && event.button.x <= WindowWidth && event.button.y >= 45 && event.button.y <= WindowHeight) {
                                Point worldMouse = canvas->mouseWorldPosition();
                                bool hitDetected = false; int hitIndex = -1;

                                for (int i = static_cast<int>(placedComponents.size()) - 1; i >= 0; --i) {
                                    SDL_FRect box = placedComponents[i].getWorldBoundingBox();
                                    if (worldMouse.x >= box.x && worldMouse.x <= box.x + box.w && worldMouse.y >= box.y && worldMouse.y <= box.y + box.h) {
                                        hitDetected = true; hitIndex = i; break;
                                    }
                                }
                                if (hitDetected) {
                                    isContextMenuOpen = true; contextMenuPos = { static_cast<float>(event.button.x), static_cast<float>(event.button.y) }; contextTargetIndex = hitIndex;
                                    if (contextMenuPos.x > WindowWidth - 160.0f) contextMenuPos.x = WindowWidth - 160.0f;
                                    if (contextMenuPos.y > WindowHeight - (contextOptions.size() * 35.0f)) contextMenuPos.y = WindowHeight - (contextOptions.size() * 35.0f);
                                } else {
                                    for (auto& comp : placedComponents) comp.isSelected = false;
                                    for (auto& w : circuitWires) w.isSelected = false;
                                }
                            }
                        }
                        else if (event.button.button == SDL_BUTTON_LEFT) {
                            if (event.button.x >= 200 && event.button.x <= WindowWidth && event.button.y >= 45 && event.button.y <= WindowHeight) {

                                if (isWiring) {
                                    Point snapPt = canvas->snapToGrid(canvas->mouseWorldPosition());
                                    bool validConnection = false;
                                    std::string eComp = "", ePin = "";
                                    for(auto& comp : placedComponents) {
                                        for(auto& pin : comp.pins) {
                                            if(pin.isHighlighted) {
                                                eComp = comp.labelId; ePin = pin.designation; snapPt = pin.calculatedWorldPos;
                                                validConnection = true; break;
                                            }
                                        }
                                        if(validConnection) break;
                                    }

                                    if (validConnection && activeWiringCompId == eComp && activeWiringPinName == ePin) {
                                        // کلیک روی همون پایه مبدا نادیده گرفته بشه
                                    } else {
                                        Wire& newWire = circuitWires.back();
                                        if (validConnection) {
                                            newWire.endCompId = eComp;
                                            newWire.endPinName = ePin;
                                            newWire.endAnchor = WireAnchor::makePinLock(eComp, ePin, snapPt);
                                        } else {
                                            // -------------------------------------------------------------
                                            // منطق جدید: اگر روی هیچ پینی رها نشد، بررسی کن که آیا روی
                                            // بدنه‌ی یک وایر دیگر افتاده است یا نه (ساخت WireLock واقعی).
                                            // این دقیقاً حل‌کننده‌ی مشکل ۱ و ۲ از گفتگو است.
                                            // -------------------------------------------------------------
                                            float lockTolerance = 8.0f / canvas->zoom();
                                            snapPt = rehomeWireEndpointIfOnWire(newWire, false, snapPt, lockTolerance);
                                        }
                                        newWire.isCompleted = true;
                                        newWire.updateOrthogonalRoute(wiringStartPoint, snapPt);
                                        isWiring = false;
                                        // -------------------------------------------------------------
                                        // فیکس باگ ریشه‌ای: بعد از تکمیل یک وایر در حالت Wire Mode، این
                                        // حالت باید خاموش شود، وگرنه کلیک بعدی روی بوم دوباره وارد حالت
                                        // وایرکشی جدید می‌شود به‌جای اینکه اجازه بدهد کاربر روی وایر
                                        // موجود کلیک کند و درگش کند. این ریشه‌ی اصلی "درگ اصلا کار نمی‌کرد" بود.
                                        // -------------------------------------------------------------
                                        isWireModeActive = false;
                                        propagateWireUpdates();
                                    }
                                    continue;
                                }

                                bool clickedOnPin = false;
                                Point pinWorldPos; std::string pCompId, pPinName;

                                for(const auto& comp : placedComponents) {
                                    for(const auto& pin : comp.pins) {
                                        if(pin.isHighlighted) {
                                            clickedOnPin = true; pinWorldPos = pin.calculatedWorldPos;
                                            pCompId = comp.labelId; pPinName = pin.designation; break;
                                        }
                                    }
                                    if(clickedOnPin) break;
                                }

                                if ((clickedOnPin || isWireModeActive) && selectedComponent == "None") {
                                    isWiring = true;
                                    wiringStartPoint = clickedOnPin ? pinWorldPos : canvas->snapToGrid(canvas->mouseWorldPosition());
                                    activeWiringCompId = pCompId;
                                    activeWiringPinName = pPinName;
                                    circuitWires.push_back(Wire(pCompId, pPinName, wiringStartPoint));

                                    // -------------------------------------------------------------
                                    // منطق جدید: اگر وایرکشی نه از یک پین بلکه از وسط یک وایر دیگر
                                    // شروع شده باشد (isWireModeActive بدون clickedOnPin)، همان‌جا هم
                                    // یک WireLock برای نقطه‌ی شروع بساز.
                                    // -------------------------------------------------------------
                                    if (!clickedOnPin) {
                                        float lockTolerance = 8.0f / canvas->zoom();
                                        Wire& justCreated = circuitWires.back();
                                        rehomeWireEndpointIfOnWire(justCreated, true, wiringStartPoint, lockTolerance);
                                    }
                                    continue;
                                }

                                if (selectedComponent != "None" && !selectedComponent.empty()) {
                                    Point worldTarget = canvas->snapToGrid(canvas->mouseWorldPosition());
                                    char prefix = std::toupper(selectedComponent[0]);
                                    std::string prefixStr(1, prefix); componentCounters[prefixStr]++;
                                    std::string finalId = prefixStr + std::to_string(componentCounters[prefixStr]);
                                    placedComponents.emplace_back(selectedComponent, finalId, "", worldTarget);
                                }
                                else {
                                    Point worldMouse = canvas->mouseWorldPosition();
                                    bool hitCompDetected = false; int hitCompIndex = -1;

                                    for (int i = static_cast<int>(placedComponents.size()) - 1; i >= 0; --i) {
                                        SDL_FRect box = placedComponents[i].getWorldBoundingBox();
                                        if (worldMouse.x >= box.x && worldMouse.x <= box.x + box.w && worldMouse.y >= box.y && worldMouse.y <= box.y + box.h) {
                                            hitCompDetected = true; hitCompIndex = i; break;
                                        }
                                    }

                                    if (hitCompDetected) {
                                        if (event.button.clicks == 2) {
                                            isPropertiesDialogOpen = true; editingComponentIndex = hitCompIndex;
                                            editLabelBuf = placedComponents[hitCompIndex].labelId; editValueBuf = placedComponents[hitCompIndex].valueStr;
                                            activeEditField = 0; isDraggingComponents = false;
                                        } else {
                                            if (!placedComponents[hitCompIndex].isSelected) {
                                                if (!(event.key.mod & SDL_KMOD_SHIFT)) {
                                                    for (auto& c : placedComponents) c.isSelected = false;
                                                    for (auto& w : circuitWires) w.isSelected = false;
                                                }
                                                placedComponents[hitCompIndex].isSelected = true;
                                            }
                                            isDraggingComponents = true; dragStartWorldMouse = worldMouse;
                                            for (auto& comp : placedComponents) if (comp.isSelected) comp.dragStartPos = comp.worldPos;
                                        }
                                    } else {
                                        // -------------------------------------------------------------
                                        // منطق جدید: اول بررسی کن آیا کلیک دقیقاً روی یک نوک آزاد
                                        // (endpoint که Free است، نه PinLock و نه WireLock) افتاده است.
                                        // این اولویت بالاتری از hit-test کل بدنه‌ی وایر دارد چون کاربر
                                        // در این حالت قصدش گرفتن دقیق همان نوک برای کشیدن است، نه
                                        // جابجایی کل وایر. حل درخواست اول کاربر در گفتگو.
                                        // -------------------------------------------------------------
                                        bool hitFreeEndpoint = false;
                                        int hitEndpointWireIndex = -1;
                                        bool hitEndpointIsStart = false;
                                        float endpointSensitivity = endpointGrabRadius / canvas->zoom();

                                        for (int i = static_cast<int>(circuitWires.size()) - 1; i >= 0; --i) {
                                            Wire& w = circuitWires[i];
                                            if (w.routingPoints.empty()) continue;

                                            if (w.startAnchor.isFree()) {
                                                Point sp = w.routingPoints.front();
                                                if (sp.distanceTo(worldMouse) <= endpointSensitivity) {
                                                    hitFreeEndpoint = true; hitEndpointWireIndex = i; hitEndpointIsStart = true; break;
                                                }
                                            }
                                            if (w.endAnchor.isFree()) {
                                                Point ep = w.routingPoints.back();
                                                if (ep.distanceTo(worldMouse) <= endpointSensitivity) {
                                                    hitFreeEndpoint = true; hitEndpointWireIndex = i; hitEndpointIsStart = false; break;
                                                }
                                            }
                                        }

                                        if (hitFreeEndpoint) {
                                            if (!(event.key.mod & SDL_KMOD_SHIFT)) {
                                                for (auto& c : placedComponents) c.isSelected = false;
                                                for (auto& w : circuitWires) w.isSelected = false;
                                            }
                                            circuitWires[hitEndpointWireIndex].isSelected = true;
                                            isDraggingWireEndpoint = true;
                                            draggingEndpointWireIndex = hitEndpointWireIndex;
                                            draggingEndpointIsStart = hitEndpointIsStart;
                                            continue;
                                        }

                                        bool hitWireDetected = false; int hitWireIndex = -1;
                                        float wireSensitivity = 6.0f / canvas->zoom();

                                        for (int i = static_cast<int>(circuitWires.size()) - 1; i >= 0; --i) {
                                            if (circuitWires[i].containsPoint(worldMouse, wireSensitivity)) {
                                                hitWireDetected = true; hitWireIndex = i; break;
                                            }
                                        }

                                        if (hitWireDetected) {
                                            if (!(event.key.mod & SDL_KMOD_SHIFT)) {
                                                for (auto& c : placedComponents) c.isSelected = false;
                                                for (auto& w : circuitWires) w.isSelected = false;
                                            }
                                            circuitWires[hitWireIndex].isSelected = true;

                                            // -------------------------------------------------------------
                                            // منطق جدید: شروع درگ کردن کل وایر -- فقط اگر هر دو سرش آزاد باشد
                                            // (طبق تصمیم گرفته‌شده در گفتگو: وایری که به یک پین قفل است
                                            //  نباید به‌صورت کامل کشیده شود، چون آن سر باید ثابت بماند)
                                            // -------------------------------------------------------------
                                            if (circuitWires[hitWireIndex].isFullyFree()) {
                                                isDraggingWire = true;
                                                draggingWireIndex = hitWireIndex;
                                                wireDragStartWorldMouse = worldMouse;
                                                wireDragStartRoutingPoints = circuitWires[hitWireIndex].routingPoints;
                                            }
                                        }
                                        else {
                                            if (!(event.key.mod & SDL_KMOD_SHIFT)) {
                                                for (auto& c : placedComponents) c.isSelected = false;
                                                for (auto& w : circuitWires) w.isSelected = false;
                                            }
                                            isSelectDragging = true;
                                            selectDragStartScreen = { static_cast<float>(event.button.x), static_cast<float>(event.button.y) };
                                            visualSelectBox = { static_cast<float>(event.button.x), static_cast<float>(event.button.y), 0.0f, 0.0f };
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                        if (event.button.button == SDL_BUTTON_MIDDLE) isPanning = false;
                        else if (event.button.button == SDL_BUTTON_LEFT) {
                            if (isDraggingComponents) isDraggingComponents = false;
                            else if (isDraggingWire) {
                                isDraggingWire = false;
                                draggingWireIndex = -1;
                                wireDragStartRoutingPoints.clear();
                            }
                            else if (isDraggingWireEndpoint && draggingEndpointWireIndex >= 0 && draggingEndpointWireIndex < static_cast<int>(circuitWires.size())) {
                                // -------------------------------------------------------------
                                // منطق جدید: پایان درگ نوک آزاد. بررسی می‌کنیم آیا موس روی یک
                                // پین واقعی است (اتصال PinLock) یا روی بدنه‌ی یک وایر دیگر
                                // (اتصال WireLock) یا هیچ‌کدام (باقی‌ماندن Free در نقطه‌ی جدید).
                                // -------------------------------------------------------------
                                Wire& w = circuitWires[draggingEndpointWireIndex];
                                Point releasePoint = draggingEndpointIsStart
                                                     ? (w.routingPoints.empty() ? Point{0.0f,0.0f} : w.routingPoints.front())
                                                     : (w.routingPoints.empty() ? Point{0.0f,0.0f} : w.routingPoints.back());

                                bool connectedToPin = false;
                                std::string foundCompId, foundPinName;
                                Point foundPinPos = releasePoint;
                                for (const auto& comp : placedComponents) {
                                    for (const auto& pin : comp.pins) {
                                        if (pin.isHighlighted) {
                                            connectedToPin = true;
                                            foundCompId = comp.labelId;
                                            foundPinName = pin.designation;
                                            foundPinPos = pin.calculatedWorldPos;
                                            break;
                                        }
                                    }
                                    if (connectedToPin) break;
                                }

                                if (connectedToPin) {
                                    if (draggingEndpointIsStart) {
                                        w.startCompId = foundCompId;
                                        w.startPinName = foundPinName;
                                        w.startAnchor = WireAnchor::makePinLock(foundCompId, foundPinName, foundPinPos);
                                        Point fixedEnd = w.routingPoints.empty() ? foundPinPos : w.routingPoints.back();
                                        w.rerouteSmartPreservingShape(foundPinPos, fixedEnd);
                                    } else {
                                        w.endCompId = foundCompId;
                                        w.endPinName = foundPinName;
                                        w.endAnchor = WireAnchor::makePinLock(foundCompId, foundPinName, foundPinPos);
                                        Point fixedStart = w.routingPoints.empty() ? foundPinPos : w.routingPoints.front();
                                        w.rerouteSmartPreservingShape(fixedStart, foundPinPos);
                                    }
                                } else {
                                    float lockTolerance = 8.0f / canvas->zoom();
                                    rehomeWireEndpointIfOnWire(w, draggingEndpointIsStart, releasePoint, lockTolerance);
                                }

                                propagateWireUpdates();
                                isDraggingWireEndpoint = false;
                                draggingEndpointWireIndex = -1;
                            }
                            else if (isSelectDragging) {
                                isSelectDragging = false;
                                Point worldStart = canvas->screenToWorld({ visualSelectBox.x - 200.0f, visualSelectBox.y - 45.0f });
                                Point worldEnd = canvas->screenToWorld({ (visualSelectBox.x + visualSelectBox.w) - 200.0f, (visualSelectBox.y + visualSelectBox.h) - 45.0f });

                                float minX = std::min(worldStart.x, worldEnd.x), maxX = std::max(worldStart.x, worldEnd.x);
                                float minY = std::min(worldStart.y, worldEnd.y), maxY = std::max(worldStart.y, worldEnd.y);

                                for (auto& comp : placedComponents) {
                                    SDL_FRect box = comp.getWorldBoundingBox();
                                    bool intersectX = minX < (box.x + box.w) && maxX > box.x;
                                    bool intersectY = minY < (box.y + box.h) && maxY > box.y;
                                    if (intersectX && intersectY) comp.isSelected = true;
                                }

                                for (auto& wire : circuitWires) {
                                    if (wire.routingPoints.empty()) continue;
                                    float wMinX = wire.routingPoints[0].x, wMaxX = wire.routingPoints[0].x;
                                    float wMinY = wire.routingPoints[0].y, wMaxY = wire.routingPoints[0].y;
                                    for (const auto& pt : wire.routingPoints) {
                                        wMinX = std::min(wMinX, pt.x); wMaxX = std::max(wMaxX, pt.x);
                                        wMinY = std::min(wMinY, pt.y); wMaxY = std::max(wMaxY, pt.y);
                                    }
                                    bool intersectX = minX <= wMaxX && maxX >= wMinX;
                                    bool intersectY = minY <= wMaxY && maxY >= wMinY;
                                    if (intersectX && intersectY) wire.isSelected = true;
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
                    placedComponents.clear(); circuitWires.clear(); componentCounters.clear();

                    if (startMenu.shouldLoadProject()) {
                        std::vector<std::string> loadedList;
                        if (ProjectManager::loadProject(startMenu.getSelectedProjectFile(), loadedList, placedComponents, circuitWires)) {
                            compLib.setActiveList(loadedList);
                            for (const auto& comp : placedComponents) {
                                if (!comp.type.empty()) {
                                    char prefix = std::toupper(comp.type[0]);
                                    std::string prefixStr(1, prefix); componentCounters[prefixStr]++;
                                }
                            }
                        }
                        startMenu.resetLoadProject();
                    } else compLib.setActiveList({});
                }
                startMenu.resetRequestedState();
            }
        }

        int currentW, currentH;
        SDL_GetWindowSize(window, &currentW, &currentH);

        switch (currentState) {
            case AppState::MainMenu: startMenu.render(renderer, font); break;

            case AppState::NewProject:
                if (canvasRenderer) {
                    SDL_Rect canvasViewport{ 200, 45, currentW - 200, currentH - 45 };
                    SDL_SetRenderViewport(renderer, &canvasViewport);

                    canvasRenderer->renderSDL(renderer, font, currentW - 200, currentH - 45);
                    canvasRenderer->renderWiresSDL(renderer, circuitWires);
                    canvasRenderer->renderComponentsSDL(renderer, font, placedComponents);

                    SDL_SetRenderViewport(renderer, nullptr);
                    toolbar.render(renderer, font); compLib.render(renderer, font, selectedComponent);

                    if (isWireModeActive) {
                        renderText(renderer, font, "WIRE MODE [Press ESC to cancel]", currentW / 2.0f + 100.0f, 60.0f, {0, 180, 80, 255});
                    }

                    if (isSelectDragging) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 120, 215, 45); SDL_RenderFillRect(renderer, &visualSelectBox);
                        SDL_SetRenderDrawColor(renderer, 0, 120, 215, 255); SDL_RenderRect(renderer, &visualSelectBox);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                    }

                    if (isPropertiesDialogOpen && editingComponentIndex >= 0 && editingComponentIndex < static_cast<int>(placedComponents.size())) {
                        const auto& comp = placedComponents[editingComponentIndex];
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                        SDL_FRect screenRect{0, 0, static_cast<float>(currentW), static_cast<float>(currentH)};
                        SDL_RenderFillRect(renderer, &screenRect);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        SDL_FRect dialogRect{ currentW / 2.0f - 180.0f, currentH / 2.0f - 120.0f, 360.0f, 240.0f };
                        SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255); SDL_RenderFillRect(renderer, &dialogRect);
                        SDL_SetRenderDrawColor(renderer, 100, 110, 130, 255); SDL_RenderRect(renderer, &dialogRect);

                        std::string titleStr = comp.type + " Properties";
                        renderText(renderer, font, titleStr, currentW / 2.0f, dialogRect.y + 15.0f, {255, 255, 255, 255});

                        renderText(renderer, font, "Designator Part Label:", dialogRect.x + 20.0f, dialogRect.y + 55.0f, {200, 210, 230, 255}, false);
                        SDL_FRect labelBoxRect{ dialogRect.x + 20.0f, dialogRect.y + 82.0f, dialogRect.w - 40.0f, 32.0f };
                        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255); SDL_RenderFillRect(renderer, &labelBoxRect);
                        SDL_SetRenderDrawColor(renderer, activeEditField == 0 ? 66 : 80, activeEditField == 0 ? 153 : 90, activeEditField == 0 ? 225 : 110, 255);
                        SDL_RenderRect(renderer, &labelBoxRect);
                        std::string dispLabel = editLabelBuf; if (activeEditField == 0 && (SDL_GetTicks() / 500) % 2 == 0) dispLabel += "_";
                        renderText(renderer, font, dispLabel, labelBoxRect.x + 8.0f, labelBoxRect.y + 4.0f, {255, 255, 255, 255}, false);

                        std::string valPrompt = "Technical Value Specification:";
                        if (comp.type == "Resistor") valPrompt = "Resistance Value (Ohm):";
                        else if (comp.type == "Capacitor") valPrompt = "Capacitance (Farad):";
                        else if (comp.type == "DC Source" || comp.type == "AC Source") valPrompt = "Source Voltage (Volt):";
                        else if (comp.type == "Inductor") valPrompt = "Inductance Value (Henry):";

                        renderText(renderer, font, valPrompt, dialogRect.x + 20.0f, dialogRect.y + 125.0f, {200, 210, 230, 255}, false);
                        SDL_FRect valBoxRect{ dialogRect.x + 20.0f, dialogRect.y + 152.0f, dialogRect.w - 40.0f, 32.0f };
                        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255); SDL_RenderFillRect(renderer, &valBoxRect);
                        SDL_SetRenderDrawColor(renderer, activeEditField == 1 ? 66 : 80, activeEditField == 1 ? 153 : 90, activeEditField == 1 ? 225 : 110, 255);
                        SDL_RenderRect(renderer, &valBoxRect);
                        std::string dispVal = editValueBuf; if (activeEditField == 1 && (SDL_GetTicks() / 500) % 2 == 0) dispVal += "_";
                        renderText(renderer, font, dispVal, valBoxRect.x + 8.0f, valBoxRect.y + 4.0f, {255, 255, 255, 255}, false);

                        renderText(renderer, font, "TAB: Switch | ENTER: Confirm | ESC: Cancel", currentW / 2.0f, dialogRect.y + 205.0f, {150, 160, 180, 255});
                    }

                    if (isSaveDialogOpen) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                        SDL_FRect screenRect{0, 0, static_cast<float>(currentW), static_cast<float>(currentH)};
                        SDL_RenderFillRect(renderer, &screenRect);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        SDL_FRect dialogRect{ currentW / 2.0f - 160.0f, currentH / 2.0f - 80.0f, 320.0f, 160.0f };
                        SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255); SDL_RenderFillRect(renderer, &dialogRect);
                        SDL_SetRenderDrawColor(renderer, 100, 110, 130, 255); SDL_RenderRect(renderer, &dialogRect);

                        renderText(renderer, font, "Enter Project Name:", currentW / 2.0f, dialogRect.y + 20.0f, {255, 255, 255, 255});

                        SDL_FRect inputRect{ dialogRect.x + 20.0f, dialogRect.y + 60.0f, dialogRect.w - 40.0f, 40.0f };
                        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255); SDL_RenderFillRect(renderer, &inputRect);
                        SDL_SetRenderDrawColor(renderer, 80, 90, 110, 255); SDL_RenderRect(renderer, &inputRect);

                        std::string displayText = saveFileName; if ((SDL_GetTicks() / 500) % 2 == 0) displayText += "_";
                        renderText(renderer, font, displayText, inputRect.x + 10.0f, inputRect.y + 8.0f, {200, 210, 230, 255}, false);

                        renderText(renderer, font, "Press ENTER to save, ESC to cancel", currentW / 2.0f, dialogRect.y + 120.0f, {150, 160, 180, 255});
                    }

                    if (isContextMenuOpen) {
                        float menuWidth = 160.0f; float menuHeight = contextOptions.size() * 35.0f;
                        SDL_FRect shadowRect{contextMenuPos.x + 4.0f, contextMenuPos.y + 4.0f, menuWidth, menuHeight};
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100); SDL_RenderFillRect(renderer, &shadowRect);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        SDL_FRect menuBg{contextMenuPos.x, contextMenuPos.y, menuWidth, menuHeight};
                        SDL_SetRenderDrawColor(renderer, 45, 50, 60, 255); SDL_RenderFillRect(renderer, &menuBg);
                        SDL_SetRenderDrawColor(renderer, 100, 110, 130, 255); SDL_RenderRect(renderer, &menuBg);

                        for (size_t i = 0; i < contextOptions.size(); ++i) {
                            SDL_FRect itemRect{contextMenuPos.x, contextMenuPos.y + i * 35.0f, menuWidth, 35.0f};
                            if (static_cast<int>(i) == hoveredContextOption) {
                                if (i == 4) SDL_SetRenderDrawColor(renderer, 180, 50, 50, 255);
                                else SDL_SetRenderDrawColor(renderer, 60, 120, 180, 255);
                                SDL_RenderFillRect(renderer, &itemRect);
                            }
                            SDL_Color textColor = {240, 240, 250, 255};
                            renderText(renderer, font, contextOptions[i], itemRect.x + 15.0f, itemRect.y + 6.0f, textColor, false);
                            if (i < contextOptions.size() - 1) {
                                SDL_SetRenderDrawColor(renderer, 65, 70, 80, 255);
                                SDL_RenderLine(renderer, itemRect.x + 10.0f, itemRect.y + 34.0f, itemRect.x + menuWidth - 10.0f, itemRect.y + 34.0f);
                            }
                        }
                    }
                }
                break;
            default: break;
        }
        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput(window); TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
    TTF_Quit(); SDL_Quit(); return 0;
}