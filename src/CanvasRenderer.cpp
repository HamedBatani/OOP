//canvasRenderer.cpp
#include "CanvasRenderer.h"

#include <iomanip>
#include <ostream>

namespace {
    void printPoint(std::ostream& output, const Point& point) {
        output << '(' << point.x << ", " << point.y << ')';
    }
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
    if (columns < 3) {
        columns = 3;
    }

    if (rows < 3) {
        rows = 3;
    }

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
            if (row == centerRow && column == centerColumn) {
                output << '+';
            } else if (row == centerRow) {
                output << '-';
            } else if (column == centerColumn) {
                output << '|';
            } else if (row % 2 == 0 && column % 4 == 0) {
                output << '.';
            } else {
                output << ' ';
            }
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
// پیاده‌سازی متد renderSDL در انتهای فایل CanvasRenderer.cpp

void CanvasRenderer::renderSDL(SDL_Renderer* renderer, TTF_Font* font, int windowWidth, int windowHeight) const {
    if (!renderer) return;

    // ۱. رسم پس‌زمینه بوم (رنگ خاکستری تیره مایل به آبی)
    SDL_SetRenderDrawColor(renderer, 30, 35, 45, 255);
    SDL_RenderClear(renderer);

    // ۲. رسم خطوط شطرنجی (Grid Lines) در صورت فعال بودن
    if (canvas_.grid().isVisible()) {
        SDL_SetRenderDrawColor(renderer, 50, 58, 75, 255); // رنگ خطوط شطرنجی

        float spacing = canvas_.grid().spacing();

        // پیدا کردن گوشه‌های صفحه در فضای جهانی (World Space)
        Point topLeftWorld = canvas_.screenToWorld({0.0f, 0.0f});
        Point bottomRightWorld = canvas_.screenToWorld({static_cast<float>(windowWidth), static_cast<float>(windowHeight)});

        // رند کردن موقعیت‌ها برای هم‌ترازی با شبکه‌بندی
        float startX = std::floor(topLeftWorld.x / spacing) * spacing;
        float endX = std::ceil(bottomRightWorld.x / spacing) * spacing;
        float startY = std::floor(topLeftWorld.y / spacing) * spacing;
        float endY = std::ceil(bottomRightWorld.y / spacing) * spacing;

        // رسم خطوط عمودی
        for (float x = startX; x <= endX; x += spacing) {
            Point p1 = canvas_.worldToScreen({x, topLeftWorld.y});
            Point p2 = canvas_.worldToScreen({x, bottomRightWorld.y});
            SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
        }

        // رسم خطوط افقی
        for (float y = startY; y <= endY; y += spacing) {
            Point p1 = canvas_.worldToScreen({topLeftWorld.x, y});
            Point p2 = canvas_.worldToScreen({bottomRightWorld.x, y});
            SDL_RenderLine(renderer, p1.x, p1.y, p2.x, p2.y);
        }
    }

    // ۳. رندر متون راهنما و مختصات لحظه‌ای (Real-time Coordinates)
    if (font) {
        SDL_Color infoColor{170, 180, 200, 255};

        Point mouseWorld = canvas_.mouseWorldPosition();
        std::string coordText = "X: " + std::to_string(static_cast<int>(mouseWorld.x)) +
                                " , Y: " + std::to_string(static_cast<int>(mouseWorld.y));
        std::string zoomText = "Zoom: " + std::to_string(static_cast<int>(canvas_.zoom() * 100)) + "%";
        std::string hintText = "Hold Middle Mouse to Pan / Scroll to Zoom";

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