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
#include <vector>

namespace {
    void printPoint(std::ostream& output, const Point& point) {
        output << '(' << point.x << ", " << point.y << ')';
    }
    constexpr float PI = 3.1415926535f;
}

CanvasRenderer::CanvasRenderer(const Canvas& canvas) : canvas_{canvas} {}

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
    output << "Canvas State\n------------\nCamera Offset: ";
    printPoint(output, cameraOffset);
    output << "\nZoom Level: " << canvas_.zoom() << "x\nMouse Screen Coordinate: ";
    printPoint(output, mouseScreen);
    output << "\nMouse World Coordinate: ";
    printPoint(output, mouseWorld);
    output << '\n';
}

void CanvasRenderer::renderGridPreview(std::ostream& output, int columns, int rows) const {
    if (columns < 3) columns = 3;
    if (rows < 3) rows = 3;

    const int centerColumn = columns / 2;
    const int centerRow = rows / 2;

    output << "Grid Preview\n------------\n";
    if (!canvas_.grid().isVisible()) { output << "Grid is hidden.\n"; return; }

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
}

void CanvasRenderer::renderSnapTest(std::ostream& output, const Point& rawMouseScreenPoint) const {
    const Point rawWorldPoint = canvas_.screenToWorld(rawMouseScreenPoint);
    const Point snappedWorldPoint = canvas_.snapToGrid(rawWorldPoint);

    output << "Snap To Grid Test\n-----------------\nRaw Mouse Screen: ";
    printPoint(output, rawMouseScreenPoint);
    output << "\nRaw Mouse World: ";
    printPoint(output, rawWorldPoint);
    output << "\nSnapped World: ";
    printPoint(output, snappedWorldPoint);
    output << "\nGrid Spacing: " << canvas_.grid().spacing() << '\n';
}

void CanvasRenderer::renderSDL(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight) const {
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderClear(renderer);

    if (canvas_.grid().isVisible()) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
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
            int lineIdx = static_cast<int>(std::round(x / spacing));
            if (lineIdx % 5 == 0) {
                SDL_SetRenderDrawColor(renderer, 190, 190, 190, 255);
                SDL_RenderLine(renderer, p1.x - 1, p1.y, p2.x - 1, p2.y);
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
                SDL_RenderLine(renderer, p1.x + 1, p1.y, p2.x + 1, p2.y);
            } else {
                SDL_SetRenderDrawColor(renderer, 213, 213, 213, 255);
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }

        for (float y = startY; y <= endY; y += spacing) {
            Point p1 = canvas_.worldToScreen({topLeftWorld.x, y});
            Point p2 = canvas_.worldToScreen({bottomRightWorld.x, y});
            int lineIdx = static_cast<int>(std::round(y / spacing));
            if (lineIdx % 5 == 0) {
                SDL_SetRenderDrawColor(renderer, 190, 190, 190, 255);
                SDL_RenderLine(renderer, p1.x, p1.y - 1, p2.x, p2.y - 1);
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
                SDL_RenderLine(renderer, p1.x, p1.y + 1, p2.x, p2.y + 1);
            } else {
                SDL_SetRenderDrawColor(renderer, 213, 213, 213, 255);
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    if (font) {
        SDL_Color infoColor{80, 85, 90, 255};
        Point mouseWorld = canvas_.mouseWorldPosition();
        std::string coordText = "X: " + std::to_string(static_cast<int>(mouseWorld.x)) + " , Y: " + std::to_string(static_cast<int>(mouseWorld.y));
        std::string zoomText = "Zoom: " + std::to_string(static_cast<int>(canvas_.zoom() * 100)) + "%";

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
    }
}

void CanvasRenderer::fillScreenCircle(SDL_Renderer* renderer, float cx, float cy, float r, SDL_Color c) const {
    const int segments = 24;
    std::vector<SDL_Vertex> v;
    SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
    float step = (2.0f * PI) / segments;
    for (int i = 0; i < segments; ++i) {
        v.push_back({{cx, cy}, fc, {0, 0}});
        v.push_back({{cx + r * std::cos(i * step), cy + r * std::sin(i * step)}, fc, {0, 0}});
        v.push_back({{cx + r * std::cos((i + 1) * step), cy + r * std::sin((i + 1) * step)}, fc, {0, 0}});
    }
    SDL_RenderGeometry(renderer, nullptr, v.data(), v.size(), nullptr, 0);
}

void CanvasRenderer::renderWiresSDL(SDL_Renderer* renderer, const std::vector<Wire>& wires) const {
    if (!renderer) return;

    for (const auto& wire : wires) {
        if (wire.routingPoints.size() < 2) continue;

        SDL_Color wireColor = wire.isSelected ? SDL_Color{0, 120, 215, 255} : SDL_Color{10, 110, 40, 255};
        SDL_SetRenderDrawColor(renderer, wireColor.r, wireColor.g, wireColor.b, wireColor.a);

        for (size_t i = 0; i < wire.routingPoints.size() - 1; ++i) {
            Point p1 = canvas_.worldToScreen(wire.routingPoints[i]);
            Point p2 = canvas_.worldToScreen(wire.routingPoints[i + 1]);

            SDL_RenderLine(renderer, p1.x - 1, p1.y, p2.x - 1, p2.y);
            SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            SDL_RenderLine(renderer, p1.x + 1, p1.y, p2.x + 1, p2.y);
            SDL_RenderLine(renderer, p1.x, p1.y - 1, p2.x, p2.y - 1);
            SDL_RenderLine(renderer, p1.x, p1.y + 1, p2.x, p2.y + 1);
        }

        if (wire.isCompleted) {
            Point pEnd = canvas_.worldToScreen(wire.routingPoints.back());
            fillScreenCircle(renderer, pEnd.x, pEnd.y, 4.0f, wireColor);
            Point pStart = canvas_.worldToScreen(wire.routingPoints.front());
            fillScreenCircle(renderer, pStart.x, pStart.y, 4.0f, wireColor);
        }

        SDL_Color freeEndpointColor{255, 140, 0, 255};
        if (wire.startAnchor.isFree() && !wire.routingPoints.empty()) {
            Point pFreeStart = canvas_.worldToScreen(wire.routingPoints.front());
            fillScreenCircle(renderer, pFreeStart.x, pFreeStart.y, 5.0f, freeEndpointColor);
        }
        if (wire.endAnchor.isFree() && !wire.routingPoints.empty()) {
            Point pFreeEnd = canvas_.worldToScreen(wire.routingPoints.back());
            fillScreenCircle(renderer, pFreeEnd.x, pFreeEnd.y, 5.0f, freeEndpointColor);
        }
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

        SDL_Color fillColor = comp.isSelected ? SDL_Color{200, 230, 255, 255} : SDL_Color{245, 245, 245, 255};
        SDL_Color strokeColor = comp.isSelected ? SDL_Color{0, 120, 215, 255} : SDL_Color{20, 20, 20, 255};

        if (comp.isSelected) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 120, 215, 30);
            SDL_RenderFillRect(renderer, &screenBox);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            SDL_SetRenderDrawColor(renderer, 0, 120, 215, 255);
            SDL_RenderRect(renderer, &screenBox);

            float anchorSize = 6.0f;
            SDL_FRect anchors[4] = {
                    {screenBox.x - 3, screenBox.y - 3, anchorSize, anchorSize},
                    {screenBox.x + screenBox.w - 3, screenBox.y - 3, anchorSize, anchorSize},
                    {screenBox.x - 3, screenBox.y + screenBox.h - 3, anchorSize, anchorSize},
                    {screenBox.x + screenBox.w - 3, screenBox.y + screenBox.h - 3, anchorSize, anchorSize}
            };
            for(int i = 0; i < 4; ++i) SDL_RenderFillRect(renderer, &anchors[i]);
        }

        auto transformLocal = [&](float lx, float ly) -> Point {
            if (comp.isMirroredH) lx = -lx;
            if (comp.isMirroredV) ly = -ly;
            float rx = lx, ry = ly;
            if (comp.rotationDegrees == 90) { rx = -ly; ry = lx; }
            else if (comp.rotationDegrees == 180) { rx = -lx; ry = -ly; }
            else if (comp.rotationDegrees == 270) { rx = ly; ry = -lx; }
            return canvas_.worldToScreen({comp.worldPos.x + rx, comp.worldPos.y + ry});
        };

        auto fillRectLocal = [&](float x, float y, float w, float h, SDL_Color c) {
            Point p1 = transformLocal(x, y); Point p2 = transformLocal(x + w, y);
            Point p3 = transformLocal(x + w, y + h); Point p4 = transformLocal(x, y + h);
            SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
            SDL_Vertex v[6] = {
                    {{p1.x, p1.y}, fc, {0, 0}}, {{p2.x, p2.y}, fc, {0, 0}}, {{p3.x, p3.y}, fc, {0, 0}},
                    {{p1.x, p1.y}, fc, {0, 0}}, {{p3.x, p3.y}, fc, {0, 0}}, {{p4.x, p4.y}, fc, {0, 0}}
            };
            SDL_RenderGeometry(renderer, nullptr, v, 6, nullptr, 0);
        };

        auto fillTriangleLocal = [&](float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color c) {
            Point p1 = transformLocal(x1, y1); Point p2 = transformLocal(x2, y2); Point p3 = transformLocal(x3, y3);
            SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
            SDL_Vertex v[3] = {{{p1.x, p1.y}, fc, {0, 0}}, {{p2.x, p2.y}, fc, {0, 0}}, {{p3.x, p3.y}, fc, {0, 0}}};
            SDL_RenderGeometry(renderer, nullptr, v, 3, nullptr, 0);
        };

        auto fillCircleLocal = [&](float cx, float cy, float r, SDL_Color c) {
            const int segments = 32; std::vector<SDL_Vertex> v;
            SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
            Point center = transformLocal(cx, cy);
            float step = (2.0f * PI) / segments;
            for (int i = 0; i < segments; ++i) {
                Point p2 = transformLocal(cx + r * std::cos(i * step), cy + r * std::sin(i * step));
                Point p3 = transformLocal(cx + r * std::cos((i + 1) * step), cy + r * std::sin((i + 1) * step));
                v.push_back({{center.x, center.y}, fc, {0, 0}});
                v.push_back({{p2.x, p2.y}, fc, {0, 0}});
                v.push_back({{p3.x, p3.y}, fc, {0, 0}});
            }
            SDL_RenderGeometry(renderer, nullptr, v.data(), v.size(), nullptr, 0);
        };

        auto fillSemicircleLocal = [&](float cx, float cy, float r, float startAngle, float endAngle, SDL_Color c) {
            const int segments = 16; std::vector<SDL_Vertex> v;
            SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
            Point center = transformLocal(cx, cy);
            float step = (endAngle - startAngle) / segments;
            for (int i = 0; i < segments; ++i) {
                Point p2 = transformLocal(cx + r * std::cos(startAngle + i * step), cy + r * std::sin(startAngle + i * step));
                Point p3 = transformLocal(cx + r * std::cos(startAngle + (i + 1) * step), cy + r * std::sin(startAngle + (i + 1) * step));
                v.push_back({{center.x, center.y}, fc, {0, 0}});
                v.push_back({{p2.x, p2.y}, fc, {0, 0}});
                v.push_back({{p3.x, p3.y}, fc, {0, 0}});
            }
            SDL_RenderGeometry(renderer, nullptr, v.data(), v.size(), nullptr, 0);
        };

        auto drawTransformedLine = [&](float x1, float y1, float x2, float y2) {
            Point p1 = transformLocal(x1, y1); Point p2 = transformLocal(x2, y2);
            SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
        };

        auto drawTransformedCircle = [&](float cx, float cy, float r) {
            const int segments = 32; float step = (2.0f * PI) / segments;
            for (int i = 0; i < segments; ++i) {
                Point p1 = transformLocal(cx + r * std::cos(i * step), cy + r * std::sin(i * step));
                Point p2 = transformLocal(cx + r * std::cos((i + 1) * step), cy + r * std::sin((i + 1) * step));
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        };

        for (const auto& pin : comp.pins) {
            Point pinScreen = canvas_.worldToScreen(pin.calculatedWorldPos);
            if (pin.isHighlighted) {
                SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255);
                SDL_FRect pinRect{pinScreen.x - 3.5f, pinScreen.y - 3.5f, 7.0f, 7.0f};
                SDL_RenderFillRect(renderer, &pinRect);
            } else {
                SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
                SDL_FRect pinRect{pinScreen.x - 2.5f, pinScreen.y - 2.5f, 5.0f, 5.0f};
                SDL_RenderFillRect(renderer, &pinRect);
            }
        }

        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);

        // --- PRISTINE HIGH-FIDELITY SCHEMATIC PATH BLUEPRINTS ---
        if (comp.type == "Resistor") {
            fillRectLocal(-16, -8, 32, 16, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-32, 0, -16, 0); drawTransformedLine(16, 0, 32, 0);
            drawTransformedLine(-16, -8, 16, -8); drawTransformedLine(16, -8, 16, 8);
            drawTransformedLine(16, 8, -16, 8); drawTransformedLine(-16, 8, -16, -8);
        }
        else if (comp.type == "Capacitor") {
            fillRectLocal(-6, -12, 3, 24, strokeColor); fillRectLocal(3, -12, 3, 24, strokeColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-32, 0, -6, 0); drawTransformedLine(6, 0, 32, 0);
        }
        else if (comp.type == "Inductor") {
            for(int i = 0; i < 4; ++i) fillSemicircleLocal(-15.0f + (i * 10.0f), 0.0f, 5.0f, PI, 2.0f * PI, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-32, 0, -20, 0); drawTransformedLine(20, 0, 32, 0);
            for(int i = 0; i < 4; ++i) {
                float bx = -15.0f + (i * 10.0f); float step = PI / 8.0f;
                for(int j = 0; j < 8; ++j) {
                    Point p1 = transformLocal(bx + 5.0f * std::cos(PI + j * step), 5.0f * std::sin(PI + j * step));
                    Point p2 = transformLocal(bx + 5.0f * std::cos(PI + (j+1) * step), 5.0f * std::sin(PI + (j+1) * step));
                    SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
                }
            }
        }
        else if (comp.type == "Battery") {
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-20, 0, -6, 0); drawTransformedLine(6, 0, 20, 0);
            drawTransformedLine(-6, -16, -6, 16); drawTransformedLine(-2, -8, -2, 8);
            drawTransformedLine(2, -16, 2, 16); drawTransformedLine(6, -8, 6, 8);
        }
        else if (comp.type == "Clock Generator") {
            fillCircleLocal(0, 0, 16, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedCircle(0, 0, 16);
            drawTransformedLine(-30, 0, -16, 0); drawTransformedLine(16, 0, 30, 0);
            drawTransformedLine(-10, 5, -4, 5); drawTransformedLine(-4, 5, -4, -5);
            drawTransformedLine(-4, -5, 4, -5); drawTransformedLine(4, -5, 4, 5); drawTransformedLine(4, 5, 10, 5);
        }
        else if (comp.type == "Switch") {
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-32, 0, -12, 0); drawTransformedLine(12, 0, 32, 0);
            fillCircleLocal(-12, 0, 3, strokeColor); fillCircleLocal(12, 0, 3, strokeColor);
            if (comp.interactiveStateBool) drawTransformedLine(-12, 0, 12, 0);
            else drawTransformedLine(-12, 0, 10, -12);
        }
        else if (comp.type == "Push Button") {
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-32, 0, -12, 0); drawTransformedLine(12, 0, 32, 0);
            fillCircleLocal(-12, 0, 2, strokeColor); fillCircleLocal(12, 0, 2, strokeColor);
            float btnY = comp.interactiveStateBool ? -2.0f : -8.0f;
            drawTransformedLine(-16, btnY, 16, btnY); drawTransformedLine(0, btnY - 6.0f, 0, btnY);
            fillRectLocal(-6, btnY - 8.0f, 12, 2, strokeColor);
        }
        else if (comp.type == "Colored LED") {
            SDL_Color activeColor = {210, 210, 215, 255};
            if (comp.interactiveStateBool) {
                if (comp.ledColorMode == 0) activeColor = {255, 40, 40, 255};
                else if (comp.ledColorMode == 1) activeColor = {40, 255, 40, 255};
                else activeColor = {40, 120, 255, 255};
            }
            fillTriangleLocal(-12, -12, -12, 12, 12, 0, activeColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-32, 0, -12, 0); drawTransformedLine(12, 0, 32, 0);
            drawTransformedLine(12, -12, 12, 12);
        }
        else if (comp.type == "7-Segment Display") {
            fillRectLocal(-20, -32, 40, 64, {32, 32, 36, 255});
            SDL_SetRenderDrawColor(renderer, 60, 65, 70, 255);
            drawTransformedLine(-20, -32, 20, -32); drawTransformedLine(20, -32, 20, 64);
            drawTransformedLine(20, 64, -20, 64); drawTransformedLine(-20, 64, -20, -32);
            SDL_Color glow = {255, 30, 30, 255}, off = {55, 50, 50, 255};
            fillRectLocal(-10, -24, 20, 3, (comp.activeSevenSegmentByte & 0x01) ? glow : off);
            fillRectLocal(10, -24, 3, 22, (comp.activeSevenSegmentByte & 0x02) ? glow : off);
            fillRectLocal(10, 2, 3, 22, (comp.activeSevenSegmentByte & 0x04) ? glow : off);
            fillRectLocal(-10, 24, 20, 3, (comp.activeSevenSegmentByte & 0x08) ? glow : off);
            fillRectLocal(-13, 2, 3, 22, (comp.activeSevenSegmentByte & 0x10) ? glow : off);
            fillRectLocal(-13, -24, 3, 22, (comp.activeSevenSegmentByte & 0x20) ? glow : off);
            fillRectLocal(-10, 0, 20, 3, (comp.activeSevenSegmentByte & 0x40) ? glow : off);
            fillCircleLocal(14, 24, 2.5f, (comp.activeSevenSegmentByte & 0x80) ? glow : off);
        }
        else if (comp.type == "AND Gate" || comp.type == "NAND Gate") {
            fillRectLocal(-20, -16, 20, 32, fillColor);
            fillSemicircleLocal(0, 0, 16.0f, -PI/2.0f, PI/2.0f, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-15, -16, -15, 16); drawTransformedLine(-15, -16, 0, -16); drawTransformedLine(-15, 16, 0, 16);
            drawTransformedLine(-40, -10, -20, -10); drawTransformedLine(-40, 10, -20, 10);
            if (comp.type == "NAND Gate") { drawTransformedCircle(19, 0, 3); drawTransformedLine(22, 0, 40, 0); }
            else { drawTransformedLine(16, 0, 40, 0); }
            for(int j = -6; j < 6; ++j) {
                float step = PI / 12.0f;
                Point p1 = transformLocal(16.0f * std::cos(j * step), 16.0f * std::sin(j * step));
                Point p2 = transformLocal(16.0f * std::cos((j+1) * step), 16.0f * std::sin((j+1) * step));
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }
        else if (comp.type == "OR Gate" || comp.type == "XOR Gate") {
            fillTriangleLocal(-20, -20, -10, 0, 0, -15, fillColor); fillTriangleLocal(-10, 0, 20, 0, 0, -15, fillColor);
            fillTriangleLocal(-10, 0, 0, 15, 20, 0, fillColor); fillTriangleLocal(-20, 20, 0, 15, -10, 0, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-40, -10, -14, -10); drawTransformedLine(-40, 10, -14, 10); drawTransformedLine(20, 0, 40, 0);
            drawTransformedLine(-20, -20, -10, 0); drawTransformedLine(-10, 0, -20, 20); drawTransformedLine(-20, -20, 0, -15);
            drawTransformedLine(0, -15, 20, 0); drawTransformedLine(-20, 20, 0, 15); drawTransformedLine(0, 15, 20, 0);
            if (comp.type == "XOR Gate") {
                for (float y = -18; y <= 18; y += 2.0f) {
                    Point pt = transformLocal((y * y) / 32.0f - 24.0f, y);
                    fillScreenCircle(renderer, pt.x, pt.y, 1.2f, strokeColor);
                }
            }
        }
        else if (comp.type == "NOT Gate") {
            fillTriangleLocal(-15, -15, -15, 15, 5, 0, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-40, 0, -15, 0); drawTransformedLine(-15, -15, -15, 15);
            drawTransformedLine(-15, -15, 5, 0); drawTransformedLine(-15, 15, 5, 0);
            drawTransformedCircle(8, 0, 3); drawTransformedLine(11, 0, 40, 0);
        }
        else if (comp.type == "Flip-Flop") {
            fillRectLocal(-20, -25, 40, 50, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-20, -25, 20, -25); drawTransformedLine(20, -25, 20, 25);
            drawTransformedLine(20, 25, -20, 25); drawTransformedLine(-20, 25, -20, -25);
            drawTransformedLine(-40, -15, -20, -15); drawTransformedLine(-40, 15, -20, 15);
            drawTransformedLine(20, -15, 40, -15); drawTransformedLine(20, 15, 40, 15);
            drawTransformedLine(-20, 10, -12, 15); drawTransformedLine(-12, 15, -20, 20);
        }
        else if (comp.type == "Diode") {
            fillTriangleLocal(-12, -12, -12, 12, 12, 0, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-32, 0, -12, 0); drawTransformedLine(12, 0, 32, 0);
            drawTransformedLine(-12, -12, -12, 12); drawTransformedLine(-12, -12, 12, 0);
            drawTransformedLine(-12, 12, 12, 0); drawTransformedLine(12, -12, 12, 12);
        }
        else if (comp.type == "Op-Amp") {
            fillTriangleLocal(-15, -20, -15, 20, 20, 0, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-15, -20, -15, 20); drawTransformedLine(-15, -20, 20, 0); drawTransformedLine(-15, 20, 20, 0);
            drawTransformedLine(-35, -8, -15, -8); drawTransformedLine(-35, 8, -15, 8); drawTransformedLine(20, 0, 35, 0);
            drawTransformedLine(-12, -10, -6, -10); drawTransformedLine(-12, 10, -6, 10); drawTransformedLine(-9, 7, -9, 13);
        }
        else if (comp.type == "AC Source") {
            fillCircleLocal(0, 0, 15, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(0, -30, 0, -15); drawTransformedLine(0, 15, 0, 30); drawTransformedCircle(0, 0, 15);
            for(float x = -8; x <= 8; x += 1.0f) {
                float y1 = std::sin(x * PI / 8.0f) * 5.0f; float y2 = std::sin((x+1) * PI / 8.0f) * 5.0f;
                Point p1 = transformLocal(x, -y1); Point p2 = transformLocal(x + 1, -y2);
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }
        else if (comp.type == "Voltmeter" || comp.type == "Ammeter") {
            fillCircleLocal(0, 0, 15, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(0, -30, 0, -15); drawTransformedLine(0, 15, 0, 30); drawTransformedCircle(0, 0, 15);
            if(comp.type == "Voltmeter") { drawTransformedLine(-5, -5, 0, 5); drawTransformedLine(0, 5, 5, -5); }
            else { drawTransformedLine(0, -6, -5, 5); drawTransformedLine(0, -6, 5, 5); drawTransformedLine(-3, 2, 3, 2); }
        }
        else if (comp.type == "Oscilloscope") {
            fillRectLocal(-25, -20, 50, 40, fillColor);
            SDL_Color screenColor = comp.isSelected ? SDL_Color{150, 200, 230, 255} : SDL_Color{220, 230, 220, 255};
            fillRectLocal(-20, -15, 30, 30, screenColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-25, -20, 25, -20); drawTransformedLine(25, -20, 25, 20);
            drawTransformedLine(25, 20, -25, 20); drawTransformedLine(-25, 20, -25, -20);
            drawTransformedCircle(17, -5, 3); drawTransformedCircle(17, 5, 3);
            for(float x = -18; x <= 8; x += 1.0f) {
                float y1 = std::sin((x+18) * PI / 6.0f) * 8.0f; float y2 = std::sin((x+19) * PI / 6.0f) * 8.0f;
                Point p1 = transformLocal(x, -y1); Point p2 = transformLocal(x + 1, -y2);
                SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
            }
        }
        else if (comp.type == "Ground") {
            drawTransformedLine(0, -15, 0, 0); drawTransformedLine(-12, 0, 12, 0);
            drawTransformedLine(-8, 4, 8, 4); drawTransformedLine(-4, 8, 4, 8);
        }
        else {
            fillRectLocal(-20, -15, 40, 30, fillColor);
            SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
            drawTransformedLine(-20, -15, 20, -15); drawTransformedLine(20, -15, 20, 15);
            drawTransformedLine(20, 15, -20, 15); drawTransformedLine(-20, 15, -20, -15);
            drawTransformedLine(-35, 0, -20, 0); drawTransformedLine(20, 0, 35, 0);
        }

        if (font) {
            SDL_Color textColor{30, 30, 30, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(font, comp.labelId.c_str(), 0, textColor);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    float textScale = std::min(0.65f, canvas_.zoom() * 0.65f);
                    SDL_FRect destRect{ center.x - (surface->w * textScale) / 2.0f, screenBox.y - surface->h * textScale - 3.0f, static_cast<float>(surface->w) * textScale, static_cast<float>(surface->h) * textScale };
                    SDL_RenderTexture(renderer, texture, nullptr, &destRect);
                    SDL_DestroyTexture(texture);
                }
                SDL_DestroySurface(surface);
            }

            if (!comp.valueStr.empty()) {
                SDL_Color valueColor{40, 100, 170, 255};
                SDL_Surface* valSurf = TTF_RenderText_Blended(font, comp.valueStr.c_str(), 0, valueColor);
                if (valSurf) {
                    SDL_Texture* valTex = SDL_CreateTextureFromSurface(renderer, valSurf);
                    if (valTex) {
                        float textScale = std::min(0.6f, canvas_.zoom() * 0.6f);
                        SDL_FRect destRect{ center.x - (valSurf->w * textScale) / 2.0f, screenBox.y + screenBox.h + 2.0f, static_cast<float>(valSurf->w) * textScale, static_cast<float>(valSurf->h) * textScale };
                        SDL_RenderTexture(renderer, valTex, nullptr, &destRect);
                        SDL_DestroyTexture(valTex);
                    }
                    SDL_DestroySurface(valSurf);
                }
            }
        }
    }
}