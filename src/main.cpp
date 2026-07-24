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

    enum class SimState { Stopped, Running, Paused };

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

    Wire* findWireByUid(std::vector<Wire>& wires, const std::string& uid) {
        for (auto& w : wires) {
            if (w.uid == uid) return &w;
        }
        return nullptr;
    }

    Point resolveAnchorPosition(const WireAnchor& anchor, const std::vector<ComponentInstance>& components, std::vector<Wire>& wires) {
        if (anchor.isPinLock()) {
            for (const auto& comp : components) {
                if (comp.labelId == anchor.lockedCompId) {
                    for (const auto& pin : comp.pins) {
                        if (pin.designation == anchor.lockedPinName) return pin.calculatedWorldPos;
                    }
                }
            }
            return anchor.cachedWorldPos;
        }
        if (anchor.isWireLock()) {
            Wire* hostWire = findWireByUid(wires, anchor.lockedWireUid);
            if (hostWire) return hostWire->resolvePointOnSegment(anchor.lockedSegmentIndex, anchor.lockedSegmentT);
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
    std::string editDelayBuf = "";
    int activeEditField = 0;
    int maxEditFields = 2;

    bool isContextMenuOpen = false;
    Point contextMenuPos{0.0f, 0.0f};
    int contextTargetIndex = -1;
    const std::vector<std::string> contextOptions = {"Properties", "Rotate 90", "Mirror Horiz", "Mirror Vert", "Delete"};
    int hoveredContextOption = -1;

    bool isDraggingWire = false;
    int draggingWireIndex = -1;
    Point wireDragStartWorldMouse{0.0f, 0.0f};
    std::vector<Point> wireDragStartRoutingPoints;
    int hoveredWireIndexForDrag = -1;

    bool isDraggingWireEndpoint = false;
    int draggingEndpointWireIndex = -1;
    bool draggingEndpointIsStart = false;
    const float endpointGrabRadius = 10.0f;

    SimState simState = SimState::Stopped;
    uint64_t simTimeMs = 0;
    uint64_t lastTicks = SDL_GetTicks();

    auto updateConnectedWires = [&](const ComponentInstance& comp) {
        for (auto& wire : circuitWires) {
            if (wire.startCompId == comp.labelId) {
                for (const auto& p : comp.pins) {
                    if (p.designation == wire.startPinName) {
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
        for (int iteration = 0; iteration < maxIterations; ++iteration) {
            bool anyChanged = false;
            for (auto& wire : circuitWires) {
                Point currentStart = wire.routingPoints.empty() ? Point{0.0f, 0.0f} : wire.routingPoints.front();
                Point currentEnd = wire.routingPoints.empty() ? Point{0.0f, 0.0f} : wire.routingPoints.back();
                Point newStart = currentStart; Point newEnd = currentEnd;

                if (wire.startAnchor.isWireLock() || wire.startAnchor.isPinLock()) newStart = resolveAnchorPosition(wire.startAnchor, placedComponents, circuitWires);
                if (wire.endAnchor.isWireLock() || wire.endAnchor.isPinLock()) newEnd = resolveAnchorPosition(wire.endAnchor, placedComponents, circuitWires);

                bool changed = (std::hypot(newStart.x - currentStart.x, newStart.y - currentStart.y) > 0.01f) ||
                               (std::hypot(newEnd.x - currentEnd.x, newEnd.y - currentEnd.y) > 0.01f);

                if (changed) { wire.rerouteSmartPreservingShape(newStart, newEnd); anyChanged = true; }
            }
            if (!anyChanged) break;
        }
    };

    auto rehomeWireEndpointIfOnWire = [&](Wire& wire, bool isStartEndpoint, const Point& releasePoint, float tolerance) -> Point {
        Wire::ClosestPointResult best; best.distance = 1e9f; int bestWireIdx = -1;
        for (size_t i = 0; i < circuitWires.size(); ++i) {
            if (circuitWires[i].uid == wire.uid || !circuitWires[i].isCompleted) continue;
            auto candidate = circuitWires[i].findClosestPointOnWire(releasePoint);
            if (candidate.found && candidate.distance < tolerance && candidate.distance < best.distance) {
                best = candidate; bestWireIdx = static_cast<int>(i);
            }
        }
        if (bestWireIdx >= 0) {
            WireAnchor lockedAnchor = WireAnchor::makeWireLock(circuitWires[bestWireIdx].uid, best.segmentIndex, best.t, best.point);
            if (isStartEndpoint) wire.startAnchor = lockedAnchor; else wire.endAnchor = lockedAnchor;
            return best.point;
        }
        if (isStartEndpoint) { if (wire.startAnchor.kind != AnchorKind::PinLock) wire.startAnchor = WireAnchor::makeFree(releasePoint); }
        else { if (wire.endAnchor.kind != AnchorKind::PinLock) wire.endAnchor = WireAnchor::makeFree(releasePoint); }
        return releasePoint;
    };

    while (running && currentState != AppState::Exit) {
        uint64_t currentTicks = SDL_GetTicks();
        uint64_t deltaMs = currentTicks - lastTicks;
        lastTicks = currentTicks;

        if (simState == SimState::Running) {
            simTimeMs += deltaMs;
            for (auto& comp : placedComponents) {
                if (comp.type == "Clock Generator") {
                    uint64_t period = 500;
                    comp.interactiveStateBool = ((simTimeMs / period) % 2 == 0);
                }
            }
        }

        if (simState == SimState::Running || simState == SimState::Paused) {
            for (auto& wire : circuitWires) {
                wire.currentLogicState = DigitalState::Undefined;
                wire.currentVoltage = 0.0f;

                for (const auto& comp : placedComponents) {
                    if (wire.startCompId == comp.labelId || wire.endCompId == comp.labelId) {
                        if (comp.type == "Potentiometer") {
                            wire.currentVoltage = comp.potWiperPosition * 5.0f;
                            wire.currentLogicState = LogicEngine::voltageToLogic(wire.currentVoltage);
                        }
                        else if (comp.type == "Clock Generator" || comp.type == "Push Button") {
                            wire.currentVoltage = comp.interactiveStateBool ? 5.0f : 0.0f;
                            wire.currentLogicState = comp.interactiveStateBool ? DigitalState::High : DigitalState::Low;
                        }
                        else if (comp.type == "Switch") {
                            wire.currentVoltage = comp.interactiveStateBool ? 5.0f : 0.0f;
                            wire.currentLogicState = comp.interactiveStateBool ? DigitalState::High : DigitalState::Undefined;
                        }
                        else if (comp.type == "Ground") {
                            wire.currentVoltage = 0.0f;
                            wire.currentLogicState = DigitalState::Low;
                        }
                        else if (comp.type == "DC Source" || comp.type == "Battery") {
                            wire.currentVoltage = 5.0f;
                            wire.currentLogicState = DigitalState::High;
                        }
                    }
                }
            }

            for (auto& comp : placedComponents) {
                if (comp.type == "ADC") {
                    float adcInputVoltage = 0.0f;
                    for (auto& wire : circuitWires) {
                        if ((wire.startCompId == comp.labelId && wire.startPinName == "Vin") ||
                            (wire.endCompId == comp.labelId && wire.endPinName == "Vin")) {
                            adcInputVoltage = wire.currentVoltage;
                            break;
                        }
                    }
                    std::vector<DigitalState> outBits = AdcComponent::performConversion(adcInputVoltage, 5.0f, 0.0f, comp.adcResolutionBits);
                    for (int i = 0; i < comp.adcResolutionBits; ++i) {
                        std::string targetPin = "D" + std::to_string(i);
                        for (auto& wire : circuitWires) {
                            if ((wire.startCompId == comp.labelId && wire.startPinName == targetPin) ||
                                (wire.endCompId == comp.labelId && wire.endPinName == targetPin)) {
                                wire.currentLogicState = outBits[i];
                            }
                        }
                    }
                }
            }
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { running = false; currentState = AppState::Exit; break; }
            if (currentState == AppState::MainMenu) startMenu.handleEvent(event);
            else if (currentState == AppState::NewProject) {
                if (isPropertiesDialogOpen) {
                    if (event.type == SDL_EVENT_TEXT_INPUT) {
                        if (activeEditField == 0 && editLabelBuf.size() < 10) editLabelBuf += event.text.text;
                        else if (activeEditField == 1 && editValueBuf.size() < 12) editValueBuf += event.text.text;
                        else if (activeEditField == 2 && editDelayBuf.size() < 12) editDelayBuf += event.text.text;
                    } else if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.key == SDLK_BACKSPACE) {
                            if (activeEditField == 0 && !editLabelBuf.empty()) editLabelBuf.pop_back();
                            else if (activeEditField == 1 && !editValueBuf.empty()) editValueBuf.pop_back();
                            else if (activeEditField == 2 && !editDelayBuf.empty()) editDelayBuf.pop_back();
                        } else if (event.key.key == SDLK_TAB) {
                            activeEditField = (activeEditField + 1) % maxEditFields;
                        } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                            if (!editLabelBuf.empty() && editingComponentIndex >= 0) {
                                placedComponents[editingComponentIndex].labelId = editLabelBuf;
                                placedComponents[editingComponentIndex].valueStr = editValueBuf;

                                if (placedComponents[editingComponentIndex].type == "Colored LED") {
                                    if (editValueBuf == "Green" || editValueBuf == "green") placedComponents[editingComponentIndex].ledColorMode = 1;
                                    else if (editValueBuf == "Blue" || editValueBuf == "blue") placedComponents[editingComponentIndex].ledColorMode = 2;
                                    else placedComponents[editingComponentIndex].ledColorMode = 0;
                                }
                                else if (placedComponents[editingComponentIndex].type == "7-Segment Display") {
                                    try { placedComponents[editingComponentIndex].activeSevenSegmentByte = static_cast<uint8_t>(std::stoul(editValueBuf, nullptr, 16)); }
                                    catch (...) { placedComponents[editingComponentIndex].activeSevenSegmentByte = 0x5B; }
                                }
                                else if (placedComponents[editingComponentIndex].type == "ADC") {
                                    try {
                                        placedComponents[editingComponentIndex].adcResolutionBits = std::max(1, std::stoi(editValueBuf));
                                        placedComponents[editingComponentIndex].adcConversionDelayNs = std::stof(editDelayBuf);
                                    } catch(...) {}
                                    placedComponents[editingComponentIndex].worldHeight = std::max(70.0f, placedComponents[editingComponentIndex].adcResolutionBits * 15.0f);
                                    placedComponents[editingComponentIndex].initializeBasePins();
                                    placedComponents[editingComponentIndex].updatePinPositions();
                                }
                                else if (placedComponents[editingComponentIndex].type == "DAC") {
                                    try {
                                        placedComponents[editingComponentIndex].dacResolutionBits = std::max(1, std::stoi(editValueBuf));
                                        placedComponents[editingComponentIndex].dacConversionDelayNs = std::stof(editDelayBuf);
                                    } catch(...) {}
                                    placedComponents[editingComponentIndex].worldHeight = std::max(70.0f, placedComponents[editingComponentIndex].dacResolutionBits * 15.0f);
                                    placedComponents[editingComponentIndex].initializeBasePins();
                                    placedComponents[editingComponentIndex].updatePinPositions();
                                }
                            }
                            isPropertiesDialogOpen = false;
                        } else if (event.key.key == SDLK_ESCAPE) { isPropertiesDialogOpen = false; }
                    }
                    continue;
                }

                if (isSaveDialogOpen) {
                    if (event.type == SDL_EVENT_TEXT_INPUT) {
                        if (saveFileName.size() < 25) saveFileName += event.text.text;
                    } else if (event.type == SDL_EVENT_KEY_DOWN) {
                        if (event.key.key == SDLK_BACKSPACE && !saveFileName.empty()) saveFileName.pop_back();
                        else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                            if (!saveFileName.empty()) {
                                std::string fullPath = saveFileName + ".txt";
                                if (ProjectManager::saveProject(fullPath, compLib.getActiveList(), placedComponents, circuitWires)) startMenu.addSavedProject(fullPath);
                            }
                            isSaveDialogOpen = false;
                        } else if (event.key.key == SDLK_ESCAPE) isSaveDialogOpen = false;
                    }
                    continue;
                }

                if (isContextMenuOpen) {
                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        float mx = event.motion.x, my = event.motion.y;
                        if (mx >= contextMenuPos.x && mx <= contextMenuPos.x + 160.0f && my >= contextMenuPos.y && my <= contextMenuPos.y + (contextOptions.size() * 35.0f)) hoveredContextOption = static_cast<int>((my - contextMenuPos.y) / 35.0f);
                        else hoveredContextOption = -1;
                    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (event.button.button == SDL_BUTTON_LEFT) {
                            float mx = event.button.x, my = event.button.y;
                            if (mx >= contextMenuPos.x && mx <= contextMenuPos.x + 160.0f && my >= contextMenuPos.y && my <= contextMenuPos.y + (contextOptions.size() * 35.0f)) {
                                int selectedOption = static_cast<int>((my - contextMenuPos.y) / 35.0f);
                                if (contextTargetIndex >= 0 && contextTargetIndex < static_cast<int>(placedComponents.size())) {
                                    auto& comp = placedComponents[contextTargetIndex];
                                    if (selectedOption == 0) {
                                        isPropertiesDialogOpen = true; editingComponentIndex = contextTargetIndex;
                                        editLabelBuf = comp.labelId; editValueBuf = comp.valueStr;
                                        editDelayBuf = std::to_string(static_cast<int>(comp.type == "ADC" ? comp.adcConversionDelayNs : (comp.type == "DAC" ? comp.dacConversionDelayNs : comp.propagationDelayNs)));
                                        maxEditFields = (comp.type == "ADC" || comp.type == "DAC") ? 3 : 2; activeEditField = 0;
                                    } else if (selectedOption == 1) {
                                        comp.rotationDegrees = (comp.rotationDegrees + 90) % 360; comp.updatePinPositions(); updateConnectedWires(comp); propagateWireUpdates();
                                    } else if (selectedOption == 2) {
                                        comp.isMirroredH = !comp.isMirroredH; comp.updatePinPositions(); updateConnectedWires(comp); propagateWireUpdates();
                                    } else if (selectedOption == 3) {
                                        comp.isMirroredV = !comp.isMirroredV; comp.updatePinPositions(); updateConnectedWires(comp); propagateWireUpdates();
                                    } else if (selectedOption == 4) {
                                        std::string delId = comp.labelId;
                                        circuitWires.erase(std::remove_if(circuitWires.begin(), circuitWires.end(), [&](const Wire& w) { return w.startCompId == delId || w.endCompId == delId; }), circuitWires.end());
                                        placedComponents.erase(placedComponents.begin() + contextTargetIndex);
                                    }
                                }
                            }
                            isContextMenuOpen = false; continue;
                        } else if (event.button.button == SDL_BUTTON_RIGHT) isContextMenuOpen = false;
                    }
                    if (event.type != SDL_EVENT_MOUSE_MOTION) continue;
                }

                if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
                    for (auto& comp : placedComponents) {
                        if (comp.type == "Push Button") comp.interactiveStateBool = false;
                        if (comp.type == "Keypad 4x4") { comp.activeKeypadRow = -1; comp.activeKeypadCol = -1; }
                    }
                }

                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.key == SDLK_ESCAPE) {
                        selectedComponent = "None"; isWireModeActive = false;
                        for (auto& comp : placedComponents) comp.isSelected = false;
                        for (auto& wire : circuitWires) wire.isSelected = false;
                        if (isWiring) { isWiring = false; circuitWires.pop_back(); }
                        if (isDraggingWire) { isDraggingWire = false; draggingWireIndex = -1; }
                        if (isDraggingWireEndpoint) { isDraggingWireEndpoint = false; draggingEndpointWireIndex = -1; }
                    }
                    else if ((event.key.mod & SDL_KMOD_CTRL) && event.key.key == SDLK_S) { isSaveDialogOpen = true; saveFileName = ""; }
                    else if (selectedComponent == "None" || selectedComponent.empty()) {
                        if (event.key.key == SDLK_R) {
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) { comp.rotationDegrees = (comp.rotationDegrees + 90) % 360; comp.updatePinPositions(); updateConnectedWires(comp); propagateWireUpdates(); }
                            }
                        }
                        else if (event.key.key == SDLK_M) {
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) { comp.isMirroredH = !comp.isMirroredH; comp.updatePinPositions(); updateConnectedWires(comp); propagateWireUpdates(); }
                            }
                        }
                        else if (event.key.key == SDLK_V) {
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) { comp.isMirroredV = !comp.isMirroredV; comp.updatePinPositions(); updateConnectedWires(comp); propagateWireUpdates(); }
                            }
                        }
                        else if (event.key.key == SDLK_DELETE || event.key.key == SDLK_BACKSPACE) {
                            std::vector<std::string> idsToDelete;
                            for(auto& c : placedComponents) if(c.isSelected) idsToDelete.push_back(c.labelId);
                            std::set<std::string> uidsToDelete;
                            for (auto& w : circuitWires) if (w.isSelected) uidsToDelete.insert(w.uid);

                            circuitWires.erase(std::remove_if(circuitWires.begin(), circuitWires.end(), [&](const Wire& w) {
                                return w.isSelected || std::find(idsToDelete.begin(), idsToDelete.end(), w.startCompId) != idsToDelete.end() || std::find(idsToDelete.begin(), idsToDelete.end(), w.endCompId) != idsToDelete.end();
                            }), circuitWires.end());

                            for (auto& w : circuitWires) {
                                if (w.startAnchor.isWireLock() && uidsToDelete.count(w.startAnchor.lockedWireUid) > 0) w.startAnchor = WireAnchor::makeFree(w.routingPoints.empty() ? Point{0.0f,0.0f} : w.routingPoints.front());
                                if (w.endAnchor.isWireLock() && uidsToDelete.count(w.endAnchor.lockedWireUid) > 0) w.endAnchor = WireAnchor::makeFree(w.routingPoints.empty() ? Point{0.0f,0.0f} : w.routingPoints.back());
                            }
                            placedComponents.erase(std::remove_if(placedComponents.begin(), placedComponents.end(), [](const ComponentInstance& comp) { return comp.isSelected; }), placedComponents.end());
                        }
                    }
                }

                toolbar.handleEvent(event, activeAction);
                compLib.handleEvent(event, selectedComponent);

                if (activeAction == "Wire Toggle") {
                    isWireModeActive = !isWireModeActive; selectedComponent = "None";
                    for (auto& comp : placedComponents) comp.isSelected = false; activeAction = "";
                } else if (activeAction == "Save") { isSaveDialogOpen = true; saveFileName = ""; activeAction = "";
                } else if (activeAction == "Load") { canvas = nullptr; canvasRenderer = nullptr; placedComponents.clear(); circuitWires.clear(); currentState = AppState::MainMenu; activeAction = "";
                } else if (activeAction == "Grid Toggle" && canvas) { canvas->grid().setVisible(!canvas->grid().isVisible()); activeAction = "";
                } else if (activeAction == "Main Menu") { canvas = nullptr; canvasRenderer = nullptr; placedComponents.clear(); circuitWires.clear(); currentState = AppState::MainMenu; activeAction = "";
                } else if (activeAction == "Run") {
                    simState = SimState::Running; activeAction = "";
                } else if (activeAction == "Pause") {
                    if (simState == SimState::Running) simState = SimState::Paused;
                    activeAction = "";
                } else if (activeAction == "Stop") {
                    simState = SimState::Stopped; simTimeMs = 0;
                    for (auto& comp : placedComponents) { if (comp.type == "Clock Generator") comp.interactiveStateBool = false; }
                    activeAction = "";
                    // --- مدیریت دکمه Step (بند 8.4) ---
                } else if (activeAction == "Step") {
                    if (simState == SimState::Running) simState = SimState::Paused;
                    else if (simState == SimState::Stopped) { simState = SimState::Paused; simTimeMs = 0; }

                    simTimeMs += 100; // پیشروی دقیق 100 میلی‌ثانیه برای واکشی گام بعدی

                    for (auto& comp : placedComponents) {
                        if (comp.type == "Clock Generator") {
                            uint64_t period = 500;
                            comp.interactiveStateBool = ((simTimeMs / period) % 2 == 0);
                        }
                        // محل قرارگیری متد stepInstruction() میکرو در آینده
                    }
                    activeAction = "";
                }

                if (canvas) {
                    float canvasMouseX = event.motion.x - 200.0f; float canvasMouseY = event.motion.y - 45.0f;

                    if (event.type == SDL_EVENT_MOUSE_MOTION) {
                        canvas->setMouseScreenPosition({canvasMouseX, canvasMouseY}); Point currentWorldMouse = canvas->mouseWorldPosition();

                        if (isPanning) canvas->pan({event.motion.xrel, event.motion.yrel});
                        else if (isDraggingComponents) {
                            Point mouseDelta = currentWorldMouse - dragStartWorldMouse;
                            for (auto& comp : placedComponents) {
                                if (comp.isSelected) { comp.worldPos = canvas->snapToGrid(comp.dragStartPos + mouseDelta); comp.updatePinPositions(); updateConnectedWires(comp); }
                            }
                            propagateWireUpdates();
                        }
                        else if (isDraggingWire && draggingWireIndex >= 0 && draggingWireIndex < static_cast<int>(circuitWires.size())) {
                            Point mouseDelta = currentWorldMouse - wireDragStartWorldMouse; Point snappedDelta = canvas->snapToGrid(mouseDelta);
                            Wire& draggedWire = circuitWires[draggingWireIndex]; draggedWire.routingPoints.clear();
                            for (const auto& originalPoint : wireDragStartRoutingPoints) draggedWire.routingPoints.push_back(originalPoint + snappedDelta);
                            if (!draggedWire.routingPoints.empty()) { draggedWire.startAnchor.cachedWorldPos = draggedWire.routingPoints.front(); draggedWire.endAnchor.cachedWorldPos = draggedWire.routingPoints.back(); }
                            propagateWireUpdates();
                        }
                        else if (isDraggingWireEndpoint && draggingEndpointWireIndex >= 0 && draggingEndpointWireIndex < static_cast<int>(circuitWires.size())) {
                            float sensitivity = 10.0f / canvas->zoom();
                            for (auto& comp : placedComponents) comp.checkPinHover(currentWorldMouse, sensitivity);
                            Point targetPoint = canvas->snapToGrid(currentWorldMouse);
                            for (auto& comp : placedComponents) { for (auto& pin : comp.pins) { if (pin.isHighlighted) targetPoint = pin.calculatedWorldPos; } }
                            Wire& w = circuitWires[draggingEndpointWireIndex];
                            if (draggingEndpointIsStart) { Point fixedEnd = w.routingPoints.empty() ? targetPoint : w.routingPoints.back(); w.rerouteSmartPreservingShape(targetPoint, fixedEnd); }
                            else { Point fixedStart = w.routingPoints.empty() ? targetPoint : w.routingPoints.front(); w.rerouteSmartPreservingShape(fixedStart, targetPoint); }
                        }
                        else if (isSelectDragging) {
                            float x1 = selectDragStartScreen.x, y1 = selectDragStartScreen.y, x2 = static_cast<float>(event.motion.x), y2 = static_cast<float>(event.motion.y);
                            visualSelectBox.x = std::min(x1, x2); visualSelectBox.y = std::min(y1, y2); visualSelectBox.w = std::abs(x2 - x1); visualSelectBox.h = std::abs(y2 - y1);
                        }
                        else {
                            float sensitivity = 10.0f / canvas->zoom();
                            for (auto& comp : placedComponents) comp.checkPinHover(currentWorldMouse, sensitivity);
                            if (isWiring) {
                                Point targetPoint = canvas->snapToGrid(currentWorldMouse);
                                for(auto& comp : placedComponents) { for(auto& pin : comp.pins) { if(pin.isHighlighted) targetPoint = pin.calculatedWorldPos; } }
                                circuitWires.back().updateOrthogonalRoute(wiringStartPoint, targetPoint);
                            } else {
                                hoveredWireIndexForDrag = -1; float wireSensitivity = 8.0f / canvas->zoom();
                                for (int i = static_cast<int>(circuitWires.size()) - 1; i >= 0; --i) {
                                    if (circuitWires[i].containsPoint(currentWorldMouse, wireSensitivity)) { hoveredWireIndexForDrag = i; break; }
                                }
                            }
                        }
                    }
                    else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                        Point currentWorldMouse = canvas->mouseWorldPosition();
                        bool handledByPotentiometer = false;

                        for (auto& comp : placedComponents) {
                            if (comp.type == "Potentiometer") {
                                SDL_FRect box = comp.getWorldBoundingBox();
                                if (currentWorldMouse.x >= box.x && currentWorldMouse.x <= box.x + box.w &&
                                    currentWorldMouse.y >= box.y && currentWorldMouse.y <= box.y + box.h) {

                                    comp.potWiperPosition += event.wheel.y * 0.05f;
                                    if (comp.potWiperPosition > 1.0f) comp.potWiperPosition = 1.0f;
                                    if (comp.potWiperPosition < 0.0f) comp.potWiperPosition = 0.0f;
                                    handledByPotentiometer = true;
                                }
                            }
                        }

                        if (!handledByPotentiometer) {
                            if (event.wheel.y > 0) canvas->zoomBy(1.1f); else if (event.wheel.y < 0) canvas->zoomBy(0.9f);
                        }

                        float sensitivity = 10.0f / canvas->zoom();
                        for (auto& comp : placedComponents) comp.checkPinHover(currentWorldMouse, sensitivity);
                    }
                    else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                        if (event.button.button == SDL_BUTTON_MIDDLE) isPanning = true;
                        else if (event.button.button == SDL_BUTTON_RIGHT) {
                            selectedComponent = "None"; isWireModeActive = false;
                            if (isWiring) { isWiring = false; circuitWires.pop_back(); continue; }
                            if (event.button.x >= 200 && event.button.x <= WindowWidth && event.button.y >= 45 && event.button.y <= WindowHeight) {
                                Point worldMouse = canvas->mouseWorldPosition(); bool hitDetected = false; int hitIndex = -1;
                                for (int i = static_cast<int>(placedComponents.size()) - 1; i >= 0; --i) {
                                    SDL_FRect box = placedComponents[i].getWorldBoundingBox();
                                    if (worldMouse.x >= box.x && worldMouse.x <= box.x + box.w && worldMouse.y >= box.y && worldMouse.y <= box.y + box.h) { hitDetected = true; hitIndex = i; break; }
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
                                    bool validConnection = false; std::string eComp = "", ePin = "";
                                    for(auto& comp : placedComponents) {
                                        for(auto& pin : comp.pins) {
                                            if(pin.isHighlighted) { eComp = comp.labelId; ePin = pin.designation; snapPt = pin.calculatedWorldPos; validConnection = true; break; }
                                        }
                                        if(validConnection) break;
                                    }
                                    if (validConnection && activeWiringCompId == eComp && activeWiringPinName == ePin) {}
                                    else {
                                        Wire& newWire = circuitWires.back();
                                        if (validConnection) { newWire.endCompId = eComp; newWire.endPinName = ePin; newWire.endAnchor = WireAnchor::makePinLock(eComp, ePin, snapPt); }
                                        else { float lockTolerance = 8.0f / canvas->zoom(); snapPt = rehomeWireEndpointIfOnWire(newWire, false, snapPt, lockTolerance); }
                                        newWire.isCompleted = true; newWire.updateOrthogonalRoute(wiringStartPoint, snapPt);
                                        isWiring = false; isWireModeActive = false; propagateWireUpdates();
                                    }
                                    continue;
                                }

                                bool clickedOnPin = false; Point pinWorldPos; std::string pCompId, pPinName;
                                for(const auto& comp : placedComponents) {
                                    for(const auto& pin : comp.pins) {
                                        if(pin.isHighlighted) { clickedOnPin = true; pinWorldPos = pin.calculatedWorldPos; pCompId = comp.labelId; pPinName = pin.designation; break; }
                                    }
                                    if(clickedOnPin) break;
                                }

                                if ((clickedOnPin || isWireModeActive) && selectedComponent == "None") {
                                    isWiring = true; wiringStartPoint = clickedOnPin ? pinWorldPos : canvas->snapToGrid(canvas->mouseWorldPosition());
                                    activeWiringCompId = pCompId; activeWiringPinName = pPinName; circuitWires.push_back(Wire(pCompId, pPinName, wiringStartPoint));
                                    if (!clickedOnPin) { float lockTolerance = 8.0f / canvas->zoom(); Wire& justCreated = circuitWires.back(); rehomeWireEndpointIfOnWire(justCreated, true, wiringStartPoint, lockTolerance); }
                                    continue;
                                }

                                if (selectedComponent != "None" && !selectedComponent.empty()) {
                                    Point worldTarget = canvas->snapToGrid(canvas->mouseWorldPosition()); std::string prefixStr = "U";
                                    if (selectedComponent == "Ground") prefixStr = "GND"; else if (selectedComponent == "Battery") prefixStr = "B"; else if (selectedComponent == "Clock Generator") prefixStr = "CLK"; else if (selectedComponent == "Switch") prefixStr = "SW"; else if (selectedComponent == "Push Button") prefixStr = "PB"; else if (selectedComponent == "Potentiometer") prefixStr = "RV"; else if (selectedComponent == "Colored LED") prefixStr = "LED"; else if (selectedComponent == "7-Segment Display") prefixStr = "SEG"; else { char prefix = std::toupper(selectedComponent[0]); prefixStr = std::string(1, prefix); }
                                    componentCounters[prefixStr]++; std::string finalId = prefixStr + std::to_string(componentCounters[prefixStr]);
                                    placedComponents.emplace_back(selectedComponent, finalId, "", worldTarget);
                                }
                                else {
                                    Point worldMouse = canvas->mouseWorldPosition(); bool hitCompDetected = false; int hitCompIndex = -1;
                                    for (int i = static_cast<int>(placedComponents.size()) - 1; i >= 0; --i) {
                                        SDL_FRect box = placedComponents[i].getWorldBoundingBox();
                                        if (worldMouse.x >= box.x && worldMouse.x <= box.x + box.w && worldMouse.y >= box.y && worldMouse.y <= box.y + box.h) { hitCompDetected = true; hitCompIndex = i; break; }
                                    }

                                    if (hitCompDetected) {
                                        if (placedComponents[hitCompIndex].type == "Keypad 4x4") {
                                            Point m = worldMouse - placedComponents[hitCompIndex].worldPos;
                                            int rot = placedComponents[hitCompIndex].rotationDegrees;
                                            float lx = m.x, ly = m.y;
                                            if (rot == 90) { lx = m.y; ly = -m.x; } else if (rot == 180) { lx = -m.x; ly = -m.y; } else if (rot == 270) { lx = -m.y; ly = m.x; }
                                            if (placedComponents[hitCompIndex].isMirroredH) lx = -lx;
                                            if (placedComponents[hitCompIndex].isMirroredV) ly = -ly;

                                            float startX = -35.0f; float startY = -45.0f;
                                            float cellW = 70.0f / 4.0f; float cellH = 90.0f / 4.0f;
                                            int col = std::floor((lx - startX) / cellW);
                                            int row = std::floor((ly - startY) / cellH);

                                            if (col >= 0 && col < 4 && row >= 0 && row < 4) {
                                                placedComponents[hitCompIndex].activeKeypadRow = row;
                                                placedComponents[hitCompIndex].activeKeypadCol = col;
                                            } else {
                                                if (!placedComponents[hitCompIndex].isSelected) {
                                                    if (!(event.key.mod & SDL_KMOD_SHIFT)) { for (auto& c : placedComponents) c.isSelected = false; for (auto& w : circuitWires) w.isSelected = false; }
                                                    placedComponents[hitCompIndex].isSelected = true;
                                                }
                                                isDraggingComponents = true; dragStartWorldMouse = worldMouse;
                                                for (auto& comp : placedComponents) if (comp.isSelected) comp.dragStartPos = comp.worldPos;
                                            }
                                        }
                                        else if (placedComponents[hitCompIndex].type == "Switch") { placedComponents[hitCompIndex].interactiveStateBool = !placedComponents[hitCompIndex].interactiveStateBool; }
                                        else if (placedComponents[hitCompIndex].type == "Push Button") { placedComponents[hitCompIndex].interactiveStateBool = true; }
                                        else if (event.button.clicks == 2) {
                                            isPropertiesDialogOpen = true; editingComponentIndex = hitCompIndex;
                                            editLabelBuf = placedComponents[hitCompIndex].labelId; editValueBuf = placedComponents[hitCompIndex].valueStr;
                                            editDelayBuf = std::to_string(static_cast<int>(placedComponents[hitCompIndex].type == "ADC" ? placedComponents[hitCompIndex].adcConversionDelayNs : (placedComponents[hitCompIndex].type == "DAC" ? placedComponents[hitCompIndex].dacConversionDelayNs : placedComponents[hitCompIndex].propagationDelayNs)));
                                            maxEditFields = (placedComponents[hitCompIndex].type == "ADC" || placedComponents[hitCompIndex].type == "DAC") ? 3 : 2; activeEditField = 0; isDraggingComponents = false;
                                        } else {
                                            if (!placedComponents[hitCompIndex].isSelected) {
                                                if (!(event.key.mod & SDL_KMOD_SHIFT)) { for (auto& c : placedComponents) c.isSelected = false; for (auto& w : circuitWires) w.isSelected = false; }
                                                placedComponents[hitCompIndex].isSelected = true;
                                            }
                                            isDraggingComponents = true; dragStartWorldMouse = worldMouse;
                                            for (auto& comp : placedComponents) if (comp.isSelected) comp.dragStartPos = comp.worldPos;
                                        }
                                    } else {
                                        bool hitFreeEndpoint = false; int hitEndpointWireIndex = -1; bool hitEndpointIsStart = false;
                                        float endpointSensitivity = endpointGrabRadius / canvas->zoom();
                                        for (int i = static_cast<int>(circuitWires.size()) - 1; i >= 0; --i) {
                                            Wire& w = circuitWires[i]; if (w.routingPoints.empty()) continue;
                                            if (w.startAnchor.isFree()) { Point pFreeStart = w.routingPoints.front(); if (pFreeStart.distanceTo(worldMouse) <= endpointSensitivity) { hitFreeEndpoint = true; hitEndpointWireIndex = i; hitEndpointIsStart = true; break; } }
                                            if (w.endAnchor.isFree()) { Point pFreeEnd = w.routingPoints.back(); if (pFreeEnd.distanceTo(worldMouse) <= endpointSensitivity) { hitFreeEndpoint = true; hitEndpointWireIndex = i; hitEndpointIsStart = false; break; } }
                                        }
                                        if (hitFreeEndpoint) {
                                            if (!(event.key.mod & SDL_KMOD_SHIFT)) { for (auto& c : placedComponents) c.isSelected = false; for (auto& w : circuitWires) w.isSelected = false; }
                                            circuitWires[hitEndpointWireIndex].isSelected = true; isDraggingWireEndpoint = true; draggingEndpointWireIndex = hitEndpointWireIndex; draggingEndpointIsStart = hitEndpointIsStart; continue;
                                        }

                                        bool hitWireDetected = false; int hitWireIndex = -1;
                                        float wireSensitivity = 8.0f / canvas->zoom();
                                        for (int i = static_cast<int>(circuitWires.size()) - 1; i >= 0; --i) {
                                            if (circuitWires[i].containsPoint(worldMouse, wireSensitivity)) { hitWireDetected = true; hitWireIndex = i; break; }
                                        }

                                        if (hitWireDetected) {
                                            if (!(event.key.mod & SDL_KMOD_SHIFT)) { for (auto& c : placedComponents) c.isSelected = false; for (auto& w : circuitWires) w.isSelected = false; }
                                            circuitWires[hitWireIndex].isSelected = true;
                                            isDraggingWire = true; draggingWireIndex = hitWireIndex; wireDragStartWorldMouse = worldMouse; wireDragStartRoutingPoints = circuitWires[hitWireIndex].routingPoints;
                                        }
                                        else {
                                            if (!(event.key.mod & SDL_KMOD_SHIFT)) { for (auto& c : placedComponents) c.isSelected = false; for (auto& w : circuitWires) w.isSelected = false; }
                                            isSelectDragging = true; selectDragStartScreen = { static_cast<float>(event.button.x), static_cast<float>(event.button.y) }; visualSelectBox = { static_cast<float>(event.button.x), static_cast<float>(event.button.y), 0.0f, 0.0f };
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
                            else if (isDraggingWire) { isDraggingWire = false; draggingWireIndex = -1; wireDragStartRoutingPoints.clear(); }
                            else if (isDraggingWireEndpoint && draggingEndpointWireIndex >= 0 && draggingEndpointWireIndex < static_cast<int>(circuitWires.size())) {
                                Wire& w = circuitWires[draggingEndpointWireIndex];
                                Point releasePoint = draggingEndpointIsStart ? (w.routingPoints.empty() ? Point{0.0f,0.0f} : w.routingPoints.front()) : (w.routingPoints.empty() ? Point{0.0f,0.0f} : w.routingPoints.back());
                                bool connectedToPin = false; std::string foundCompId, foundPinName; Point foundPinPos = releasePoint;
                                for (const auto& comp : placedComponents) {
                                    for (const auto& pin : comp.pins) {
                                        if (pin.isHighlighted) { connectedToPin = true; foundCompId = comp.labelId; foundPinName = pin.designation; foundPinPos = pin.calculatedWorldPos; break; }
                                    }
                                    if (connectedToPin) break;
                                }

                                if (connectedToPin) {
                                    if (draggingEndpointIsStart) {
                                        w.startCompId = foundCompId; w.startPinName = foundPinName; w.startAnchor = WireAnchor::makePinLock(foundCompId, foundPinName, foundPinPos);
                                        Point fixedEnd = w.routingPoints.empty() ? foundPinPos : w.routingPoints.back(); w.rerouteSmartPreservingShape(foundPinPos, fixedEnd);
                                    } else {
                                        w.endCompId = foundCompId; w.endPinName = foundPinName; w.endAnchor = WireAnchor::makePinLock(foundCompId, foundPinName, foundPinPos);
                                        Point fixedStart = w.routingPoints.empty() ? foundPinPos : w.routingPoints.front(); w.rerouteSmartPreservingShape(fixedStart, foundPinPos);
                                    }
                                } else { float lockTolerance = 8.0f / canvas->zoom(); rehomeWireEndpointIfOnWire(w, draggingEndpointIsStart, releasePoint, lockTolerance); }
                                propagateWireUpdates(); isDraggingWireEndpoint = false; draggingEndpointWireIndex = -1;
                            }
                            else if (isSelectDragging) {
                                isSelectDragging = false;
                                Point worldStart = canvas->screenToWorld({ visualSelectBox.x - 200.0f, visualSelectBox.y - 45.0f });
                                Point worldEnd = canvas->screenToWorld({ (visualSelectBox.x + visualSelectBox.w) - 200.0f, (visualSelectBox.y + visualSelectBox.h) - 45.0f });
                                float minX = std::min(worldStart.x, worldEnd.x), maxX = std::max(worldStart.x, worldEnd.x);
                                float minY = std::min(worldStart.y, worldEnd.y), maxY = std::max(worldStart.y, worldEnd.y);

                                for (auto& comp : placedComponents) {
                                    SDL_FRect box = comp.getWorldBoundingBox();
                                    bool intersectX = minX < (box.x + box.w) && maxX > box.x; bool intersectY = minY < (box.y + box.h) && maxY > box.y;
                                    if (intersectX && intersectY) comp.isSelected = true;
                                }
                                for (auto& wire : circuitWires) {
                                    if (wire.routingPoints.empty()) continue;
                                    float wMinX = wire.routingPoints[0].x, wMaxX = wire.routingPoints[0].x, wMinY = wire.routingPoints[0].y, wMaxY = wire.routingPoints[0].y;
                                    for (const auto& pt : wire.routingPoints) { wMinX = std::min(wMinX, pt.x); wMaxX = std::max(wMaxX, pt.x); wMinY = std::min(wMinY, pt.y); wMaxY = std::max(wMaxY, pt.y); }
                                    bool intersectX = minX <= wMaxX && maxX >= wMinX; bool intersectY = minY <= wMaxY && maxY >= wMinY;
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
                                if (!comp.type.empty()) { char prefix = std::toupper(comp.type[0]); std::string prefixStr(1, prefix); componentCounters[prefixStr]++; }
                            }
                        }
                        startMenu.resetLoadProject();
                    } else compLib.setActiveList({});
                }
                startMenu.resetRequestedState();
            }
        }

        int currentW, currentH; SDL_GetWindowSize(window, &currentW, &currentH);

        switch (currentState) {
            case AppState::MainMenu: startMenu.render(renderer, font); break;
            case AppState::NewProject:
                if (canvasRenderer) {
                    SDL_Rect canvasViewport{ 200, 45, currentW - 200, currentH - 45 }; SDL_SetRenderViewport(renderer, &canvasViewport);
                    canvasRenderer->renderSDL(renderer, font, currentW - 200, currentH - 45);
                    canvasRenderer->renderWiresSDL(renderer, circuitWires, simState != SimState::Stopped);
                    canvasRenderer->renderComponentsSDL(renderer, font, placedComponents);
                    SDL_SetRenderViewport(renderer, nullptr); toolbar.render(renderer, font); compLib.render(renderer, font, selectedComponent);

                    if (simState == SimState::Running) {
                        std::string timeStr = "Time: " + std::to_string(simTimeMs / 1000.0f) + "s";
                        renderText(renderer, font, timeStr, currentW - 280.0f, 60.0f, {0, 180, 80, 255});
                    } else if (simState == SimState::Paused) {
                        std::string pauseStr = "PAUSED (Time: " + std::to_string(simTimeMs / 1000.0f) + "s)";
                        renderText(renderer, font, pauseStr, currentW - 280.0f, 60.0f, {240, 180, 40, 255});
                    }

                    if (isWireModeActive) renderText(renderer, font, "WIRE MODE [Press ESC to cancel]", currentW / 2.0f + 100.0f, 60.0f, {0, 180, 80, 255});

                    if (isSelectDragging) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(renderer, 0, 120, 215, 45); SDL_RenderFillRect(renderer, &visualSelectBox);
                        SDL_SetRenderDrawColor(renderer, 0, 120, 215, 255); SDL_RenderRect(renderer, &visualSelectBox); SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                    }

                    if (isPropertiesDialogOpen && editingComponentIndex >= 0 && editingComponentIndex < static_cast<int>(placedComponents.size())) {
                        const auto& comp = placedComponents[editingComponentIndex];
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                        SDL_FRect screenRect{0, 0, static_cast<float>(currentW), static_cast<float>(currentH)}; SDL_RenderFillRect(renderer, &screenRect);
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        float dialogH = (maxEditFields == 3) ? 320.0f : 240.0f;
                        SDL_FRect dialogRect{ currentW / 2.0f - 180.0f, currentH / 2.0f - dialogH/2.0f, 360.0f, dialogH };
                        SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255); SDL_RenderFillRect(renderer, &dialogRect);
                        SDL_SetRenderDrawColor(renderer, 100, 110, 130, 255); SDL_RenderRect(renderer, &dialogRect);

                        std::string titleStr = comp.type + " Properties"; renderText(renderer, font, titleStr, currentW / 2.0f, dialogRect.y + 15.0f, {255, 255, 255, 255});
                        renderText(renderer, font, "Designator Part Label:", dialogRect.x + 20.0f, dialogRect.y + 55.0f, {200, 210, 230, 255}, false);
                        SDL_FRect labelBoxRect{ dialogRect.x + 20.0f, dialogRect.y + 82.0f, dialogRect.w - 40.0f, 32.0f };
                        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255); SDL_RenderFillRect(renderer, &labelBoxRect);
                        SDL_SetRenderDrawColor(renderer, activeEditField == 0 ? 66 : 80, activeEditField == 0 ? 153 : 90, activeEditField == 0 ? 225 : 110, 255); SDL_RenderRect(renderer, &labelBoxRect);
                        std::string dispLabel = editLabelBuf; if (activeEditField == 0 && (SDL_GetTicks() / 500) % 2 == 0) dispLabel += "_";
                        renderText(renderer, font, dispLabel, labelBoxRect.x + 8.0f, labelBoxRect.y + 4.0f, {255, 255, 255, 255}, false);

                        std::string valPrompt = "Technical Value Specification:";
                        if (comp.type == "Resistor") valPrompt = "Resistance Value (Ohm):"; else if (comp.type == "Capacitor") valPrompt = "Capacitance (Farad):"; else if (comp.type == "DC Source" || comp.type == "AC Source") valPrompt = "Source Voltage (Volt):"; else if (comp.type == "Inductor") valPrompt = "Inductance Value (Henry):"; else if (comp.type == "Colored LED") valPrompt = "LED Visual Glow Color (Red, Green, Blue):"; else if (comp.type == "7-Segment Display") valPrompt = "Active Segment Mask Hex Byte (e.g. 0x4F):"; else if (comp.type == "ADC" || comp.type == "DAC") valPrompt = "Resolution (Bits):"; else if (comp.coreComponent && comp.coreComponent->getComponentClass() == ComponentClass::DigitalLogic) valPrompt = "Propagation Delay Parameter (ns):";

                        renderText(renderer, font, valPrompt, dialogRect.x + 20.0f, dialogRect.y + 125.0f, {200, 210, 230, 255}, false);
                        SDL_FRect valBoxRect{ dialogRect.x + 20.0f, dialogRect.y + 152.0f, dialogRect.w - 40.0f, 32.0f };
                        SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255); SDL_RenderFillRect(renderer, &valBoxRect);
                        SDL_SetRenderDrawColor(renderer, activeEditField == 1 ? 66 : 80, activeEditField == 1 ? 153 : 90, activeEditField == 1 ? 225 : 110, 255); SDL_RenderRect(renderer, &valBoxRect);
                        std::string dispVal = editValueBuf; if (activeEditField == 1 && (SDL_GetTicks() / 500) % 2 == 0) dispVal += "_";
                        renderText(renderer, font, dispVal, valBoxRect.x + 8.0f, valBoxRect.y + 4.0f, {255, 255, 255, 255}, false);

                        if (maxEditFields == 3) {
                            renderText(renderer, font, "Conversion Delay (ns):", dialogRect.x + 20.0f, dialogRect.y + 195.0f, {200, 210, 230, 255}, false);
                            SDL_FRect delayBoxRect{ dialogRect.x + 20.0f, dialogRect.y + 222.0f, dialogRect.w - 40.0f, 32.0f };
                            SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255); SDL_RenderFillRect(renderer, &delayBoxRect);
                            SDL_SetRenderDrawColor(renderer, activeEditField == 2 ? 66 : 80, activeEditField == 2 ? 153 : 90, activeEditField == 2 ? 225 : 110, 255); SDL_RenderRect(renderer, &delayBoxRect);
                            std::string dispDelay = editDelayBuf; if (activeEditField == 2 && (SDL_GetTicks() / 500) % 2 == 0) dispDelay += "_";
                            renderText(renderer, font, dispDelay, delayBoxRect.x + 8.0f, delayBoxRect.y + 4.0f, {255, 255, 255, 255}, false);
                        }

                        renderText(renderer, font, "TAB: Switch | ENTER: Confirm | ESC: Cancel", currentW / 2.0f, dialogRect.y + dialogH - 35.0f, {150, 160, 180, 255});
                    }

                    if (isSaveDialogOpen) {
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                        SDL_FRect screenRect{0, 0, static_cast<float>(currentW), static_cast<float>(currentH)}; SDL_RenderFillRect(renderer, &screenRect);
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
                        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND); SDL_SetRenderDrawColor(renderer, 0, 0, 0, 100); SDL_RenderFillRect(renderer, &shadowRect); SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

                        SDL_FRect menuBg{contextMenuPos.x, contextMenuPos.y, menuWidth, menuHeight};
                        SDL_SetRenderDrawColor(renderer, 45, 50, 60, 255); SDL_RenderFillRect(renderer, &menuBg);
                        SDL_SetRenderDrawColor(renderer, 100, 110, 130, 255); SDL_RenderRect(renderer, &menuBg);

                        for (size_t i = 0; i < contextOptions.size(); ++i) {
                            SDL_FRect itemRect{contextMenuPos.x, contextMenuPos.y + i * 35.0f, menuWidth, 35.0f};
                            if (static_cast<int>(i) == hoveredContextOption) { if (i == 4) SDL_SetRenderDrawColor(renderer, 180, 50, 50, 255); else SDL_SetRenderDrawColor(renderer, 60, 120, 180, 255); SDL_RenderFillRect(renderer, &itemRect); }
                            SDL_Color textColor = {240, 240, 250, 255}; renderText(renderer, font, contextOptions[i], itemRect.x + 15.0f, itemRect.y + 6.0f, textColor, false);
                            if (i < contextOptions.size() - 1) { SDL_SetRenderDrawColor(renderer, 65, 70, 80, 255); SDL_RenderLine(renderer, itemRect.x + 10.0f, itemRect.y + 34.0f, itemRect.x + menuWidth - 10.0f, itemRect.y + 34.0f); }
                        }
                    }
                }
                break;
            default: break;K
        }
        SDL_RenderPresent(renderer);
    }

    SDL_StopTextInput(window); TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window);
    TTF_Quit(); SDL_Quit(); return 0;
}