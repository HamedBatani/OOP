// src/CanvasRenderer.cpp
#include "CanvasRenderer.h"
#include "ComponentInstance.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iomanip>
#include <ostream>
#include <string>
#include <cmath>
#include <algorithm>

namespace {
    void printPoint(std::ostream& output, const Point& point) {
        output << '(' << point.x << ", " << point.y << ')';
    }
    constexpr float PI = 3.1415926535f;
}

CanvasRenderer::CanvasRenderer(const Canvas& canvas)
        : canvas_{canvas} {}

void CanvasRenderer::render(std::ostream& output) const {
    renderState(output);
    output << '\n';
    renderGridPreview(output);
    output << '\n';
    renderSnapTest(output, canvas_.mouseScreenPosition());
}

void CanvasRenderer::renderState(std::ostream& output) const {
    const Point cameraOffset = canvas_.cameraPosition();
    const Point mouseScreen = canvas_.mouseScreenPosition();
    const Point mouseWorld = canvas_.mouseWorldPosition();

    output << std::fixed << std::setprecision(2);
    output << "Canvas State\n";
    output << "------------\n";
    output << "Camera Offset: ";
    printPoint(output, cameraOffset);
    output << '\n';
    output << "Zoom Level: " << canvas_.zoom() << "x\n";
    output << "Mouse Screen Coordinate: ";
    printPoint(output, mouseScreen);
    output << '\n';
    output << "Mouse World Coordinate: ";
    printPoint(output, mouseWorld);
    output << '\n';
}

void CanvasRenderer::renderGridPreview(std::ostream& output, int columns, int rows) const {
    if (columns < 3) columns = 3;
    if (rows < 3) rows = 3;

    const int centerColumn = columns / 2;
    const int centerRow = rows / 2;

    output << "Grid Preview\n";
    output << "------------\n";

    if (!canvas_.grid().isVisible()) {
        output << "Grid is hidden.\n";
        return;
    }

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            if (row == centerRow && column == centerColumn) output << '+';
            else if (row == centerRow) output << '-';
            else if (column == centerColumn) output << '|';
            else if (row % 2 == 0 && column % 4 == 0) output << '.';
            else output << ' ';
        }
        output << '\n';
    }
    output << "Each dot represents a visible grid intersection.\n";
}

void CanvasRenderer::renderSnapTest(std::ostream& output, const Point& rawMouseScreenPoint) const {
    const Point rawWorldPoint = canvas_.screenToWorld(rawMouseScreenPoint);
    const Point snappedWorldPoint = canvas_.snapToGrid(rawWorldPoint);
    const Point snappedScreenPoint = canvas_.worldToScreen(snappedWorldPoint);

    output << "Snap To Grid Test\n";
    output << "-----------------\n";
    output << "Raw Mouse Screen: ";
    printPoint(output, rawMouseScreenPoint);
    output << '\n';
    output << "Raw Mouse World: ";
    printPoint(output, rawWorldPoint);
    output << '\n';
    output << "Snapped World: ";
    printPoint(output, snappedWorldPoint);
    output << '\n';
    output << "Snapped Screen: ";
    printPoint(output, snappedScreenPoint);
    output << '\n';
    output << "Grid Spacing: " << canvas_.grid().spacing() << '\n';
}

void CanvasRenderer::renderSDL(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight) const {
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer, 236, 240, 235, 255);
    SDL_RenderClear(renderer);

    if (canvas_.grid().isVisible()) {
        SDL_SetRenderDrawColor(renderer, 205, 210, 205, 255);

        float spacing = canvas_.grid().spacing();
        Point topLeftWorld = canvas_.screenToWorld({0.0f, 0.0f});
        Point bottomRightWorld = canvas_.screenToWorld({static_cast<float>(windowWidth), static_cast<float>(windowHeight)});

        float startX = std::floor(topLeftWorld.x / spacing) * spacing;
        float endX = std::ceil(bottomRightWorld.x / spacing) * spacing;
        float startY = std::floor(topLeftWorld.y / spacing) * spacing;
        float endY = std::ceil(bottomRightWorld.y / spacing) * spacing;

        for (float x = startX; x <= endX; x += spacing) {
            Point p1 = canvas_.worldToScreen({x, topLeftWorld.y});
            Point p2 = canvas_.worldToScreen({x, bottomRightWorld.y});
            SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
        }

        for (float y = startY; y <= endY; y += spacing) {
            Point p1 = canvas_.worldToScreen({topLeftWorld.x, y});
            Point p2 = canvas_.worldToScreen({bottomRightWorld.x, y});
            SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
        }
    }

    if (font) {
        SDL_Color infoColor{60, 65, 75, 255};

        Point mouseWorld = canvas_.mouseWorldPosition();
        std::string coordText = "X: " + std::to_string(static_cast<int>(mouseWorld.x)) +
                                " , Y: " + std::to_string(static_cast<int>(mouseWorld.y));
        std::string zoomText = "Zoom: " + std::to_string(static_cast<int>(canvas_.zoom() * 100)) + "%";
        std::string hintText = "Middle Mouse: Pan | Scroll: Zoom | Double-Click component: Open Properties Panel";

        auto drawLocalText = [&](const std::string& text, float x, float y) {
            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), infoColor);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    SDL_FRect dest{x, y, static_cast<float>(surface->w), static_cast<float>(surface->h)};
                    SDL_RenderTexture(renderer, texture, nullptr, &dest);
                    SDL_DestroyTexture(texture);
                }
                SDL_DestroySurface(surface);
            }
        };

        drawLocalText(coordText, 20.0f, windowHeight - 40.0f);
        drawLocalText(zoomText, windowWidth - 140.0f, windowHeight - 40.0f);
        drawLocalText(hintText, 20.0f, 20.0f);
    }
}

void CanvasRenderer::renderComponentsSDL(SDL_Renderer* renderer, TTF_Font* font, const std::vector<ComponentInstance>& components) const {
    if (!renderer) return;

    for (const auto& comp : components) {
        SDL_FRect worldBox = comp.getWorldBoundingBox();
        Point screenTopLeft = canvas_.worldToScreen({worldBox.x, worldBox.y});
        float screenW = worldBox.w * canvas_.zoom();
        float screenH = worldBox.h * canvas_.zoom();
        SDL_FRect screenBox{screenTopLeft.x, screenTopLeft.y, screenW, screenH};

        Point center = canvas_.worldToScreen(comp.worldPos);
        float zoom = canvas_.zoom();

        if (comp.isSelected) {
            SDL_SetRenderDrawColor(renderer, 0, 136, 238, 45);
            SDL_RenderFillRect(renderer, &screenBox);
            SDL_SetRenderDrawColor(renderer, 0, 136, 238, 255);
            SDL_RenderRect(renderer, &screenBox);

            float anchorSize = 5.0f;
            SDL_FRect anchors[4] = {
                    {screenBox.x - 2, screenBox.y - 2, anchorSize, anchorSize},
                    {screenBox.x + screenBox.w - 3, screenBox.y - 2, anchorSize, anchorSize},
                    {screenBox.x - 2, screenBox.y + screenBox.h - 3, anchorSize, anchorSize},
                    {screenBox.x + screenBox.w - 3, screenBox.y + screenBox.h - 3, anchorSize, anchorSize}
            };
            SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255);
            for(int i = 0; i < 4; ++i) SDL_RenderFillRect(renderer, &anchors[i]);
        }

        auto transformLocal = [&](float lx, float ly) -> Point {
            if (comp.isMirroredH) lx = -lx;
            if (comp.isMirroredV) ly = -ly;

            float rx = lx;
            float ry = ly;
            if (comp.rotationDegrees == 90) {
                rx = -ly; ry = lx;
            } else if (comp.rotationDegrees == 180) {
                rx = -lx; ry = -ly;
            } else if (comp.rotationDegrees == 270) {
                rx = ly; ry = -lx;
            }
            return canvas_.worldToScreen({comp.worldPos.x + rx, comp.worldPos.y + ry});
        };

        auto drawTransformedLine = [&](float x1, float y1, float x2, float y2) {
            Point p1 = transformLocal(x1, y1);
            Point p2 = transformLocal(x2, y2);
            SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
        };

        auto drawTransformedCircle = [&](float cx, float cy, float r) {
            const int segments = 24;
            float step = (2.0f * PI) / segments;
            for (int i = 0; i < segments; ++i) {
                Point p1 = transformLocal(cx + r * std::cos(i * step), cy + r * std::sin(i * step));
                Point p2 = transformLocal(cx + r * std::cos((i + 1) * step), cy + r * std::sin((i + 1) * step));
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        };

        SDL_SetRenderDrawColor(renderer, 45, 55, 72, 255);
        for (const auto& pin : comp.pins) {
            Point pinScreen = canvas_.worldToScreen(pin.calculatedWorldPos);
            SDL_FRect pinRect{pinScreen.x - 2.0f, pinScreen.y - 2.0f, 4.0f, 4.0f};
            SDL_RenderFillRect(renderer, &pinRect);
        }

        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);

        if (comp.type == "Resistor") {
            drawTransformedLine(-32, 0, -16, 0);
            drawTransformedLine(16, 0, 32, 0);
            drawTransformedLine(-16, -8, 16, -8);
            drawTransformedLine(16, -8, 16, 8);
            drawTransformedLine(16, 8, -16, 8);
            drawTransformedLine(-16, 8, -16, -8);
        }
        else if (comp.type == "Capacitor") {
            drawTransformedLine(-32, 0, -6, 0);
            drawTransformedLine(6, 0, 32, 0);
            drawTransformedLine(-6, -12, -6, 12);
            drawTransformedLine(6, -12, 6, 12);
        }
        else if (comp.type == "Inductor") {
            drawTransformedLine(-32, 0, -20, 0);
            drawTransformedLine(20, 0, 32, 0);
            for(int i = 0; i < 4; ++i) {
                float bx = -15.0f + (i * 10.0f);
                float step = PI / 8.0f;
                for(int j = 0; j < 8; ++j) {
                    Point p1 = transformLocal(bx + 5.0f * std::cos(PI + j * step), 5.0f * std::sin(PI + j * step));
                    Point p2 = transformLocal(bx + 5.0f * std::cos(PI + (j+1) * step), 5.0f * std::sin(PI + (j+1) * step));
                    SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
                }
            }
        }
        else if (comp.type == "Diode") {
            drawTransformedLine(-32, 0, -12, 0);
            drawTransformedLine(12, 0, 32, 0);
            drawTransformedLine(-12, -12, -12, 12);
            drawTransformedLine(-12, -12, 12, 0);
            drawTransformedLine(-12, 12, 12, 0);
            drawTransformedLine(12, -12, 12, 12);
        }
        else if (comp.type == "Op-Amp") {
            drawTransformedLine(-15, -20, -15, 20);
            drawTransformedLine(-15, -20, 20, 0);
            drawTransformedLine(-15, 20, 20, 0);
            drawTransformedLine(-35, -8, -15, -8);
            drawTransformedLine(-35, 8, -15, 8);
            drawTransformedLine(20, 0, 35, 0);
        }
        else if (comp.type == "AND Gate") {
            drawTransformedLine(-35, -8, -15, -8);
            drawTransformedLine(-35, 8, -15, 8);
            drawTransformedLine(15, 0, 35, 0);
            drawTransformedLine(-15, -16, -15, 16);
            drawTransformedLine(-15, -16, 0, -16);
            drawTransformedLine(-15, 16, 0, 16);
            for(int j = -6; j < 6; ++j) {
                float step = PI / 12.0f;
                Point p1 = transformLocal(15.0f * std::cos(j * step), 16.0f * std::sin(j * step));
                Point p2 = transformLocal(15.0f * std::cos((j+1) * step), 16.0f * std::sin((j+1) * step));
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }
        else if (comp.type == "OR Gate") {
            drawTransformedLine(-35, -8, -10, -8);
            drawTransformedLine(-35, 8, -10, 8);
            drawTransformedLine(25, 0, 35, 0);
            drawTransformedLine(-15, -20, -5, 0);
            drawTransformedLine(-5, 0, -15, 20);
            drawTransformedLine(-15, -20, 5, -15);
            drawTransformedLine(5, -15, 25, 0);
            drawTransformedLine(-15, 20, 5, 15);
            drawTransformedLine(5, 15, 25, 0);
        }
        else if (comp.type == "NOT Gate") {
            drawTransformedLine(-35, 0, -15, 0);
            drawTransformedLine(15, 0, 35, 0);
            drawTransformedLine(-15, -15, -15, 15);
            drawTransformedLine(-15, -15, 5, 0);
            drawTransformedLine(-15, 15, 5, 0);
            drawTransformedCircle(10, 0, 5);
        }
        else if (comp.type == "Flip-Flop") {
            drawTransformedLine(-20, -25, 20, -25);
            drawTransformedLine(20, -25, 20, 25);
            drawTransformedLine(20, 25, -20, 25);
            drawTransformedLine(-20, 25, -20, -25);
            drawTransformedLine(-35, -10, -20, -10);
            drawTransformedLine(-35, 10, -20, 10);
            drawTransformedLine(20, -10, 35, -10);
            drawTransformedLine(20, 10, 35, 10);
            drawTransformedLine(-20, 5, -12, 10);
            drawTransformedLine(-12, 10, -20, 15);
            drawTransformedCircle(24, 10, 4);
        }
        else if (comp.type == "DC Source") {
            drawTransformedLine(0, -30, 0, -10);
            drawTransformedLine(0, 10, 0, 30);
            drawTransformedLine(-15, -10, 15, -10);
            drawTransformedLine(-8, 10, 8, 10);
        }
        else if (comp.type == "AC Source") {
            drawTransformedLine(0, -30, 0, -15);
            drawTransformedLine(0, 15, 0, 30);
            drawTransformedCircle(0, 0, 15);
            for(float x = -8; x <= 8; x += 1.0f) {
                float y1 = std::sin(x * PI / 8.0f) * 5.0f;
                float y2 = std::sin((x+1) * PI / 8.0f) * 5.0f;
                Point p1 = transformLocal(x, -y1);
                Point p2 = transformLocal(x + 1, -y2);
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }
        else if (comp.type == "Ground") {
            drawTransformedLine(0, -15, 0, 0);
            drawTransformedLine(-12, 0, 12, 0);
            drawTransformedLine(-8, 4, 8, 4);
            drawTransformedLine(-4, 8, 4, 8);
        }
        else if (comp.type == "Voltmeter" || comp.type == "Ammeter") {
            drawTransformedLine(0, -30, 0, -15);
            drawTransformedLine(0, 15, 0, 30);
            drawTransformedCircle(0, 0, 15);
            if(comp.type == "Voltmeter") {
                drawTransformedLine(-5, -5, 0, 5);
                drawTransformedLine(0, 5, 5, -5);
            } else {
                drawTransformedLine(0, -6, -5, 5);
                drawTransformedLine(0, -6, 5, 5);
                drawTransformedLine(-3, 2, 3, 2);
            }
        }
        else if (comp.type == "Oscilloscope") {
            drawTransformedLine(-25, -20, 25, -20);
            drawTransformedLine(25, -20, 25, 20);
            drawTransformedLine(25, 20, -25, 20);
            drawTransformedLine(-25, 20, -25, -20);
            drawTransformedLine(-20, -15, 10, -15);
            drawTransformedLine(10, -15, 10, 15);
            drawTransformedLine(10, 15, -20, 15);
            drawTransformedLine(-20, 15, -20, -15);
            drawTransformedCircle(17, -5, 3);
            drawTransformedCircle(17, 5, 3);
            for(float x = -18; x <= 8; x += 1.0f) {
                float y1 = std::sin((x+18) * PI / 6.0f) * 8.0f;
                float y2 = std::sin((x+19) * PI / 6.0f) * 8.0f;
                Point p1 = transformLocal(x, -y1);
                Point p2 = transformLocal(x + 1, -y2);
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }
        else {
            drawTransformedLine(-20, -15, 20, -15);
            drawTransformedLine(20, -15, 20, 15);
            drawTransformedLine(20, 15, -20, 15);
            drawTransformedLine(-20, 15, -20, -15);
            drawTransformedLine(-35, 0, -20, 0);
            drawTransformedLine(20, 0, 35, 0);
        }

        // ۴. رندر متن شناسه مدار (Label ID) و مقدار الکترونیکی (Value String) به صورت پویا
        if (font) {
            // نمایش شناسه قطعه (مثال: R1) بالای قطعه
            std::string labelText = comp.labelId;
            SDL_Color textColor{35, 45, 60, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(font, labelText.c_str(), 0, textColor);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    float textScale = std::min(0.65f, canvas_.zoom() * 0.65f);
                    SDL_FRect destRect{
                            center.x - (surface->w * textScale) / 2.0f,
                            screenBox.y - surface->h * textScale - 3.0f,
                            static_cast<float>(surface->w) * textScale,
                            static_cast<float>(surface->h) * textScale
                    };
                    SDL_RenderTexture(renderer, texture, nullptr, &destRect);
                    SDL_DestroyTexture(texture);
                }
                SDL_DestroySurface(surface);
            }

            // نمایش مقدار الکترونیکی تنظیم شده (مثال: 10k) زیر قطعه در صورت وجود
            if (!comp.valueStr.empty()) {
                SDL_Color valueColor{40, 100, 170, 255}; // آبی تیره برای مقادیر فنی
                SDL_Surface* valSurf = TTF_RenderText_Blended(font, comp.valueStr.c_str(), 0, valueColor);
                if (valSurf) {
                    SDL_Texture* valTex = SDL_CreateTextureFromSurface(renderer, valSurf);
                    if (valTex) {
                        float textScale = std::min(0.6f, canvas_.zoom() * 0.6f);
                        SDL_FRect destRect{
                                center.x - (valSurf->w * textScale) / 2.0f,
                                screenBox.y + screenBox.h + 2.0f,
                                static_cast<float>(valSurf->w) * textScale,
                                static_cast<float>(valSurf->h) * textScale
                        };
                        SDL_RenderTexture(renderer, valTex, nullptr, &destRect);
                        SDL_DestroyTexture(valTex);
                    }
                    SDL_DestroySurface(valSurf);
                }
            }
        }
    }
}