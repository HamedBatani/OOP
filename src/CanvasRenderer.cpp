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
        std::string hintText = "Middle Mouse: Pan | Scroll: Zoom | Right-Click/ESC: Cancel Placement";

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

        // رسم حاشیه انتخاب کاربر
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

        // رنگ قرمز اصیل مدار الکتریکی
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);

        auto drawCanvasCircle = [&](float x, float y, float r) {
            const int segments = 24;
            float step = (2.0f * PI) / segments;
            for (int i = 0; i < segments; ++i) {
                SDL_RenderLine(renderer,
                               x + r * std::cos(i * step), y + r * std::sin(i * step),
                               x + r * std::cos((i + 1) * step), y + r * std::sin((i + 1) * step));
            }
        };

        // ==========================================
        // شاخه قطعات ANALOG
        // ==========================================
        if (comp.type == "Resistor") {
            SDL_RenderLine(renderer, center.x - 32 * zoom, center.y, center.x - 16 * zoom, center.y);
            SDL_RenderLine(renderer, center.x + 16 * zoom, center.y, center.x + 32 * zoom, center.y);
            SDL_FRect body{center.x - 16 * zoom, center.y - 8 * zoom, 32 * zoom, 16 * zoom};
            SDL_RenderRect(renderer, &body);
        }
        else if (comp.type == "Capacitor") {
            SDL_RenderLine(renderer, center.x - 32 * zoom, center.y, center.x - 6 * zoom, center.y);
            SDL_RenderLine(renderer, center.x + 6 * zoom, center.y, center.x + 32 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 6 * zoom, center.y - 12 * zoom, center.x - 6 * zoom, center.y + 12 * zoom);
            SDL_RenderLine(renderer, center.x + 6 * zoom, center.y - 12 * zoom, center.x + 6 * zoom, center.y + 12 * zoom);
        }
        else if (comp.type == "Inductor") {
            SDL_RenderLine(renderer, center.x - 32 * zoom, center.y, center.x - 20 * zoom, center.y);
            SDL_RenderLine(renderer, center.x + 20 * zoom, center.y, center.x + 32 * zoom, center.y);
            for(int i = 0; i < 4; ++i) {
                float bx = center.x - (15 - i * 10) * zoom;
                for(int j = 0; j < 8; ++j) {
                    float step = PI / 8.0f;
                    SDL_RenderLine(renderer,
                                   bx + 5 * zoom * std::cos(PI + j * step), center.y + 5 * zoom * std::sin(PI + j * step),
                                   bx + 5 * zoom * std::cos(PI + (j+1) * step), center.y + 5 * zoom * std::sin(PI + (j+1) * step));
                }
            }
        }
        else if (comp.type == "Diode") {
            SDL_RenderLine(renderer, center.x - 32 * zoom, center.y, center.x - 12 * zoom, center.y);
            SDL_RenderLine(renderer, center.x + 12 * zoom, center.y, center.x + 32 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 12 * zoom, center.y - 12 * zoom, center.x - 12 * zoom, center.y + 12 * zoom);
            SDL_RenderLine(renderer, center.x - 12 * zoom, center.y - 12 * zoom, center.x + 12 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 12 * zoom, center.y + 12 * zoom, center.x + 12 * zoom, center.y);
            SDL_RenderLine(renderer, center.x + 12 * zoom, center.y - 12 * zoom, center.x + 12 * zoom, center.y + 12 * zoom);
        }
        else if (comp.type == "Op-Amp") {
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y - 20 * zoom, center.x - 15 * zoom, center.y + 20 * zoom);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y - 20 * zoom, center.x + 20 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y + 20 * zoom, center.x + 20 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y - 8 * zoom, center.x - 15 * zoom, center.y - 8 * zoom);
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y + 8 * zoom, center.x - 15 * zoom, center.y + 8 * zoom);
            SDL_RenderLine(renderer, center.x + 20 * zoom, center.y, center.x + 35 * zoom, center.y);
        }
            // ==========================================
            // شاخه قطعات DIGITAL
            // ==========================================
        else if (comp.type == "AND Gate") {
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y - 8 * zoom, center.x - 15 * zoom, center.y - 8 * zoom);
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y + 8 * zoom, center.x - 15 * zoom, center.y + 8 * zoom);
            SDL_RenderLine(renderer, center.x + 15 * zoom, center.y, center.x + 35 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y - 16 * zoom, center.x - 15 * zoom, center.y + 16 * zoom);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y - 16 * zoom, center.x, center.y - 16 * zoom);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y + 16 * zoom, center.x, center.y + 16 * zoom);
            for(int j = -6; j < 6; ++j) {
                float step = PI / 12.0f;
                SDL_RenderLine(renderer, center.x + 15 * zoom * std::cos(j * step), center.y + 16 * zoom * std::sin(j * step),
                               center.x + 15 * zoom * std::cos((j+1) * step), center.y + 16 * zoom * std::sin((j+1) * step));
            }
        }
        else if (comp.type == "OR Gate") {
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y - 8 * zoom, center.x - 10 * zoom, center.y - 8 * zoom);
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y + 8 * zoom, center.x - 10 * zoom, center.y + 8 * zoom);
            SDL_RenderLine(renderer, center.x + 25 * zoom, center.y, center.x + 35 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y - 20 * zoom, center.x - 5 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 5 * zoom, center.y, center.x - 15 * zoom, center.y + 20 * zoom);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y - 20 * zoom, center.x + 5 * zoom, center.y - 15 * zoom);
            SDL_RenderLine(renderer, center.x + 5 * zoom, center.y - 15 * zoom, center.x + 25 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y + 20 * zoom, center.x + 5 * zoom, center.y + 15 * zoom);
            SDL_RenderLine(renderer, center.x + 5 * zoom, center.y + 15 * zoom, center.x + 25 * zoom, center.y);
        }
        else if (comp.type == "NOT Gate") {
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y, center.x - 15 * zoom, center.y);
            SDL_RenderLine(renderer, center.x + 15 * zoom, center.y, center.x + 35 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y - 15 * zoom, center.x - 15 * zoom, center.y + 15 * zoom);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y - 15 * zoom, center.x + 5 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y + 15 * zoom, center.x + 5 * zoom, center.y);
            drawCanvasCircle(center.x + 10 * zoom, center.y, 5 * zoom);
        }
        else if (comp.type == "Flip-Flop") {
            SDL_FRect ffBody{center.x - 20 * zoom, center.y - 25 * zoom, 40 * zoom, 50 * zoom};
            SDL_RenderRect(renderer, &ffBody);
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y - 10 * zoom, center.x - 20 * zoom, center.y - 10 * zoom);
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y + 10 * zoom, center.x - 20 * zoom, center.y + 10 * zoom);
            SDL_RenderLine(renderer, center.x + 20 * zoom, center.y - 10 * zoom, center.x + 35 * zoom, center.y - 10 * zoom);
            SDL_RenderLine(renderer, center.x + 20 * zoom, center.y + 10 * zoom, center.x + 35 * zoom, center.y + 10 * zoom);
            SDL_RenderLine(renderer, center.x - 20 * zoom, center.y + 5 * zoom, center.x - 12 * zoom, center.y + 10 * zoom);
            SDL_RenderLine(renderer, center.x - 12 * zoom, center.y + 10 * zoom, center.x - 20 * zoom, center.y + 15 * zoom);
            drawCanvasCircle(center.x + 24 * zoom, center.y + 10 * zoom, 4 * zoom);
        }
            // ==========================================
            // شاخه منابع POWER
            // ==========================================
        else if (comp.type == "DC Source") {
            SDL_RenderLine(renderer, center.x, center.y - 30 * zoom, center.x, center.y - 10 * zoom);
            SDL_RenderLine(renderer, center.x, center.y + 10 * zoom, center.x, center.y + 30 * zoom);
            SDL_RenderLine(renderer, center.x - 15 * zoom, center.y - 10 * zoom, center.x + 15 * zoom, center.y - 10 * zoom);
            SDL_RenderLine(renderer, center.x - 8 * zoom, center.y + 10 * zoom, center.x + 8 * zoom, center.y + 10 * zoom);
        }
        else if (comp.type == "AC Source") {
            SDL_RenderLine(renderer, center.x, center.y - 30 * zoom, center.x, center.y - 15 * zoom);
            SDL_RenderLine(renderer, center.x, center.y + 15 * zoom, center.x, center.y + 30 * zoom);
            drawCanvasCircle(center.x, center.y, 15 * zoom);
            for(float x = -8; x <= 8; x += 1.0f) {
                float y1 = std::sin(x * PI / 8.0f) * 5.0f * zoom;
                float y2 = std::sin((x+1) * PI / 8.0f) * 5.0f * zoom;
                SDL_RenderLine(renderer, center.x + x * zoom, center.y - y1, center.x + (x + 1) * zoom, center.y - y2);
            }
        }
        else if (comp.type == "Ground") {
            SDL_RenderLine(renderer, center.x, center.y - 15 * zoom, center.x, center.y);
            SDL_RenderLine(renderer, center.x - 12 * zoom, center.y, center.x + 12 * zoom, center.y);
            SDL_RenderLine(renderer, center.x - 8 * zoom, center.y + 4 * zoom, center.x + 8 * zoom, center.y + 4 * zoom);
            SDL_RenderLine(renderer, center.x - 4 * zoom, center.y + 8 * zoom, center.x + 4 * zoom, center.y + 8 * zoom);
        }
            // ==========================================
            // شاخه ابزارهای MEASUREMENT
            // ==========================================
        else if (comp.type == "Voltmeter" || comp.type == "Ammeter") {
            SDL_RenderLine(renderer, center.x, center.y - 30 * zoom, center.x, center.y - 15 * zoom);
            SDL_RenderLine(renderer, center.x, center.y + 15 * zoom, center.x, center.y + 30 * zoom);
            drawCanvasCircle(center.x, center.y, 15 * zoom);
            if(comp.type == "Voltmeter") {
                SDL_RenderLine(renderer, center.x - 5 * zoom, center.y - 5 * zoom, center.x, center.y + 5 * zoom);
                SDL_RenderLine(renderer, center.x, center.y + 5 * zoom, center.x + 5 * zoom, center.y - 5 * zoom);
            } else {
                SDL_RenderLine(renderer, center.x, center.y - 6 * zoom, center.x - 5 * zoom, center.y + 5 * zoom);
                SDL_RenderLine(renderer, center.x, center.y - 6 * zoom, center.x + 5 * zoom, center.y + 5 * zoom);
                SDL_RenderLine(renderer, center.x - 3 * zoom, center.y + 2 * zoom, center.x + 3 * zoom, center.y + 2 * zoom);
            }
        }
        else if (comp.type == "Oscilloscope") {
            SDL_FRect oscOuter{center.x - 25 * zoom, center.y - 20 * zoom, 50 * zoom, 40 * zoom};
            SDL_RenderRect(renderer, &oscOuter);
            SDL_FRect oscScreen{center.x - 20 * zoom, center.y - 15 * zoom, 30 * zoom, 30 * zoom};
            SDL_RenderRect(renderer, &oscScreen);
            drawCanvasCircle(center.x + 17 * zoom, center.y - 5 * zoom, 3 * zoom);
            drawCanvasCircle(center.x + 17 * zoom, center.y + 5 * zoom, 3 * zoom);
            for(float x = -18; x <= 8; x += 1.0f) {
                float y1 = std::sin((x+18) * PI / 6.0f) * 8.0f * zoom;
                float y2 = std::sin((x+19) * PI / 6.0f) * 8.0f * zoom;
                SDL_RenderLine(renderer, center.x + x * zoom, center.y - y1, center.x + (x + 1) * zoom, center.y - y2);
            }
        }
            // کادر پیش‌فرض برای موارد پیش‌بینی نشده
        else {
            SDL_SetRenderDrawColor(renderer, 90, 100, 110, 255);
            SDL_FRect boundaryBox{center.x - 20 * zoom, center.y - 15 * zoom, 40 * zoom, 30 * zoom};
            SDL_RenderRect(renderer, &boundaryBox);
            SDL_RenderLine(renderer, center.x - 35 * zoom, center.y, center.x - 20 * zoom, center.y);
            SDL_RenderLine(renderer, center.x + 20 * zoom, center.y, center.x + 35 * zoom, center.y);
        }

        // ۴. چاپ متون شناسه قطعات متناسب با فاکتور زوم بوم نقاشی
        if (font) {
            std::string textString = comp.labelId;
            SDL_Color textColor{35, 45, 60, 255};
            SDL_Surface* surface = TTF_RenderText_Blended(font, textString.c_str(), 0, textColor);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    float textScale = std::min(0.7f, canvas_.zoom() * 0.7f);
                    SDL_FRect destRect{
                            center.x - (surface->w * textScale) / 2.0f,
                            screenBox.y - surface->h * textScale - 2.0f,
                            static_cast<float>(surface->w) * textScale,
                            static_cast<float>(surface->h) * textScale
                    };
                    SDL_RenderTexture(renderer, texture, nullptr, &destRect);
                    SDL_DestroyTexture(texture);
                }
                SDL_DestroySurface(surface);
            }
        }
    }
}