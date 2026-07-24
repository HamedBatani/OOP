// src/ComponentLibrary.cpp
#include "ComponentLibrary.h"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace {
    constexpr float PI = 3.1415926535f;
}

bool ComponentLibrary::containsIgnoreCase(const std::string& text, const std::string& query) {
    if (query.empty()) return true;
    auto it = std::search(
            text.begin(), text.end(),
            query.begin(), query.end(),
            [](char ch1, char ch2) { return std::tolower((unsigned char)ch1) == std::tolower((unsigned char)ch2); }
    );
    return (it != text.end());
}

ComponentLibrary::ComponentLibrary(float x, float y, float width, float height)
        : x_(x), y_(y), width_(width), height_(height) {
    categories_.push_back({"Main Sources", true, {"Ground", "DC Source", "Battery", "Clock Generator", "AC Source"}});
    categories_.push_back({"Passive Components", true, {"Resistor", "Capacitor", "Inductor", "Diode", "Op-Amp"}});
    categories_.push_back({"Interactive & Outputs", false, {"Switch", "Push Button", "Potentiometer", "Colored LED", "7-Segment Display", "LCD 16x2", "Keypad 4x4"}});
    categories_.push_back({"Digital Logic Gates", false, {"AND Gate", "OR Gate", "NOT Gate", "XOR Gate", "NAND Gate", "Flip-Flop"}});
    categories_.push_back({"Measurement", false, {"Voltmeter", "Ammeter", "Oscilloscope"}});
    categories_.push_back({"Advanced Components", false, {"ADC", "DAC"}});
}

void ComponentLibrary::handleEvent(const SDL_Event& event, std::string& selectedComponent) {
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        hoverX_ = event.motion.x; hoverY_ = event.motion.y;
    }

    float mx = hoverX_, my = hoverY_;
    float previewHeight = 240.0f;

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        if (mx >= x_ && mx <= x_ + width_ && my >= y_ + 40.0f && my <= y_ + height_ - previewHeight) {
            scrollY_ -= event.wheel.y * 30.0f;
            if (scrollY_ < 0.0f) scrollY_ = 0.0f;
            if (scrollY_ > maxScrollY_) scrollY_ = maxScrollY_;
            return;
        }
    }

    if (isSearchFocused_) {
        if (event.type == SDL_EVENT_TEXT_INPUT) {
            searchQuery_ += event.text.text; return;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_BACKSPACE && !searchQuery_.empty()) {
                searchQuery_.pop_back(); return;
            }
        }
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        bool isLeftClick = (event.button.button == SDL_BUTTON_LEFT);
        bool isRightClick = (event.button.button == SDL_BUTTON_RIGHT);

        if (!isLeftClick && !isRightClick) return;

        if (mx < x_ || mx > x_ + width_ || my < y_ || my > y_ + height_ - previewHeight) {
            if (isLeftClick && !(mx >= x_ && mx <= x_ + width_ && my >= y_ && my <= y_ + height_)) {
                isSearchFocused_ = false;
            }
            return;
        }

        if (isLeftClick) {
            SDL_FRect searchRect{x_ + 10.0f, y_ + 10.0f, width_ - 20.0f, 30.0f};
            if (mx >= searchRect.x && mx <= searchRect.x + searchRect.w && my >= searchRect.y && my <= searchRect.y + searchRect.h) {
                isSearchFocused_ = true; return;
            } else {
                isSearchFocused_ = false;
            }
        }

        if (my < y_ + 50.0f) return;

        float currentY = y_ + 55.0f - scrollY_;

        if (!activeList_.empty()) {
            currentY += 25.0f;
            for (size_t i = 0; i < activeList_.size(); ++i) {
                SDL_FRect itemRect{x_ + 5.0f, currentY, width_ - 10.0f, 26.0f};
                if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w && my >= itemRect.y && my <= itemRect.y + itemRect.h) {
                    if (isLeftClick) {
                        if (mx >= itemRect.x + itemRect.w - 20.0f || event.button.clicks >= 2) activeList_.erase(activeList_.begin() + i);
                        else selectedComponent = activeList_[i];
                    } else if (isRightClick) {
                        activeList_.erase(activeList_.begin() + i);
                    }
                    return;
                }
                currentY += 26.0f;
            }
            currentY += 15.0f;
        }

        currentY += 25.0f;

        for (auto& cat : categories_) {
            bool catMatch = containsIgnoreCase(cat.name, searchQuery_);
            std::vector<std::string> matchingItems;
            for (const auto& item : cat.items) {
                if (catMatch || containsIgnoreCase(item, searchQuery_)) matchingItems.push_back(item);
            }
            if (!searchQuery_.empty() && !catMatch && matchingItems.empty()) continue;

            SDL_FRect catRect{x_ + 5.0f, currentY, width_ - 10.0f, 28.0f};
            if (mx >= catRect.x && mx <= catRect.x + catRect.w && my >= catRect.y && my <= catRect.y + catRect.h) {
                if (isLeftClick) cat.isExpanded = !cat.isExpanded; return;
            }
            currentY += 28.0f;

            bool shouldExpand = cat.isExpanded || (!searchQuery_.empty() && !matchingItems.empty());
            if (shouldExpand) {
                for (const auto& item : matchingItems) {
                    SDL_FRect itemRect{x_ + 5.0f, currentY, width_ - 10.0f, 26.0f};
                    if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w && my >= itemRect.y && my <= itemRect.y + itemRect.h) {
                        if (isLeftClick) {
                            if (mx >= itemRect.x + itemRect.w - 20.0f || event.button.clicks >= 2) {
                                if (std::find(activeList_.begin(), activeList_.end(), item) == activeList_.end()) activeList_.push_back(item);
                            } else {
                                selectedComponent = item;
                            }
                        } else if (isRightClick) {
                            if (std::find(activeList_.begin(), activeList_.end(), item) == activeList_.end()) activeList_.push_back(item);
                        }
                        return;
                    }
                    currentY += 26.0f;
                }
            }
        }
    }
}

void ComponentLibrary::render(SDL_Renderer* renderer, TTF_Font* font, const std::string& selectedComponent) const {
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer, 24, 24, 24, 255);
    SDL_FRect bgRect{x_, y_, width_, height_};
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_SetRenderDrawColor(renderer, 45, 45, 45, 255);
    SDL_RenderLine(renderer, x_ + width_ - 1.0f, y_, x_ + width_ - 1.0f, y_ + height_);

    if (!font) return;

    SDL_FRect searchRect{x_ + 10.0f, y_ + 10.0f, width_ - 20.0f, 28.0f};
    if (isSearchFocused_) {
        SDL_SetRenderDrawColor(renderer, 0, 120, 215, 255);
        SDL_RenderRect(renderer, &searchRect);
        SDL_SetRenderDrawColor(renderer, 35, 35, 35, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    }
    SDL_RenderFillRect(renderer, &searchRect);

    std::string displayText = searchQuery_.empty() ? "Search..." : searchQuery_;
    SDL_Color searchTextColor = searchQuery_.empty() ? SDL_Color{140, 140, 140, 255} : SDL_Color{230, 230, 230, 255};
    SDL_Surface* searchSurf = TTF_RenderText_Blended(font, displayText.c_str(), 0, searchTextColor);
    if (searchSurf) {
        SDL_Texture* searchTex = SDL_CreateTextureFromSurface(renderer, searchSurf);
        SDL_FRect textRect{searchRect.x + 8.0f, searchRect.y + (28.0f - searchSurf->h) / 2.0f, static_cast<float>(searchSurf->w)*0.85f, static_cast<float>(searchSurf->h)*0.85f};
        SDL_RenderTexture(renderer, searchTex, nullptr, &textRect);
        SDL_DestroyTexture(searchTex);
        SDL_DestroySurface(searchSurf);
    }

    float previewHeight = 240.0f;
    SDL_Rect listClipRect{ static_cast<int>(x_), static_cast<int>(y_ + 45.0f), static_cast<int>(width_), static_cast<int>(height_ - previewHeight - 45.0f) };
    SDL_SetRenderClipRect(renderer, &listClipRect);

    SDL_Color headerColor{150, 150, 150, 255};
    SDL_Color itemTextColor{200, 200, 200, 255};
    SDL_Color selectedTextColor{255, 255, 255, 255};

    float currentY = y_ + 55.0f - scrollY_;

    if (!activeList_.empty()) {
        SDL_Surface* activeTitleSurf = TTF_RenderText_Blended(font, "ACTIVE", 0, headerColor);
        if (activeTitleSurf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, activeTitleSurf);
            SDL_FRect dest{x_ + 10.0f, currentY, static_cast<float>(activeTitleSurf->w)*0.75f, static_cast<float>(activeTitleSurf->h)*0.75f};
            SDL_RenderTexture(renderer, tex, nullptr, &dest);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(activeTitleSurf);
        }
        currentY += 25.0f;

        for (const auto& item : activeList_) {
            SDL_FRect itemRect{x_ + 5.0f, currentY, width_ - 10.0f, 26.0f};
            bool isItemHovered = (hoverX_ >= itemRect.x && hoverX_ <= itemRect.x + itemRect.w && hoverY_ >= itemRect.y && hoverY_ <= itemRect.y + itemRect.h);

            if (item == selectedComponent) {
                SDL_SetRenderDrawColor(renderer, 0, 120, 215, 180);
                SDL_RenderFillRect(renderer, &itemRect);
            } else if (isItemHovered) {
                SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                SDL_RenderFillRect(renderer, &itemRect);
            }

            std::string bulletItem = "  . " + item;
            SDL_Surface* itemSurf = TTF_RenderText_Blended(font, bulletItem.c_str(), 0, (item == selectedComponent) ? selectedTextColor : itemTextColor);
            if (itemSurf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, itemSurf);
                SDL_FRect textRect{itemRect.x + 8.0f, itemRect.y + (26.0f - itemSurf->h)/2.0f, static_cast<float>(itemSurf->w)*0.85f, static_cast<float>(itemSurf->h)*0.85f};
                SDL_RenderTexture(renderer, tex, nullptr, &textRect);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(itemSurf);
            }
            currentY += 26.0f;
        }
        currentY += 15.0f;
    }

    SDL_Surface* libTitleSurf = TTF_RenderText_Blended(font, "LIBRARY", 0, headerColor);
    if (libTitleSurf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, libTitleSurf);
        SDL_FRect dest{x_ + 10.0f, currentY, static_cast<float>(libTitleSurf->w)*0.75f, static_cast<float>(libTitleSurf->h)*0.75f};
        SDL_RenderTexture(renderer, tex, nullptr, &dest);
        SDL_DestroyTexture(tex);
        SDL_DestroySurface(libTitleSurf);
    }
    currentY += 25.0f;

    for (const auto& cat : categories_) {
        bool catMatch = containsIgnoreCase(cat.name, searchQuery_);
        std::vector<std::string> matchingItems;
        for (const auto& item : cat.items) {
            if (catMatch || containsIgnoreCase(item, searchQuery_)) matchingItems.push_back(item);
        }
        if (!searchQuery_.empty() && !catMatch && matchingItems.empty()) continue;

        SDL_FRect catRect{x_ + 5.0f, currentY, width_ - 10.0f, 28.0f};
        bool isCatHovered = (hoverX_ >= catRect.x && hoverX_ <= catRect.x + catRect.w && hoverY_ >= catRect.y && hoverY_ <= catRect.y + catRect.h);

        if (isCatHovered) SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
        else SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &catRect);

        bool isExpanded = cat.isExpanded || (!searchQuery_.empty() && !matchingItems.empty());
        std::string prefix = isExpanded ? "v  " : ">  ";

        SDL_Surface* catSurf = TTF_RenderText_Blended(font, (prefix + cat.name).c_str(), 0, {230, 230, 230, 255});
        if (catSurf) {
            SDL_Texture* catTex = SDL_CreateTextureFromSurface(renderer, catSurf);
            SDL_FRect textRect{catRect.x + 6.0f, catRect.y + (28.0f - catSurf->h) / 2.0f, static_cast<float>(catSurf->w)*0.85f, static_cast<float>(catSurf->h)*0.85f};
            SDL_RenderTexture(renderer, catTex, nullptr, &textRect);
            SDL_DestroyTexture(catTex);
            SDL_DestroySurface(catSurf);
        }
        currentY += 28.0f;

        if (isExpanded) {
            for (const auto& item : matchingItems) {
                SDL_FRect itemRect{x_ + 5.0f, currentY, width_ - 10.0f, 26.0f};
                bool isItemHovered = (hoverX_ >= itemRect.x && hoverX_ <= itemRect.x + itemRect.w && hoverY_ >= itemRect.y && hoverY_ <= itemRect.y + itemRect.h);

                if (item == selectedComponent) {
                    SDL_SetRenderDrawColor(renderer, 0, 120, 215, 180);
                    SDL_RenderFillRect(renderer, &itemRect);
                } else if (isItemHovered) {
                    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
                    SDL_RenderFillRect(renderer, &itemRect);
                }

                std::string bulletItem = "   . " + item;
                SDL_Surface* itemSurf = TTF_RenderText_Blended(font, bulletItem.c_str(), 0, (item == selectedComponent) ? selectedTextColor : itemTextColor);
                if (itemSurf) {
                    SDL_Texture* itemTex = SDL_CreateTextureFromSurface(renderer, itemSurf);
                    SDL_FRect textRect{itemRect.x + 8.0f, itemRect.y + (26.0f - itemSurf->h) / 2.0f, static_cast<float>(itemSurf->w)*0.85f, static_cast<float>(itemSurf->h)*0.85f};
                    SDL_RenderTexture(renderer, itemTex, nullptr, &textRect);
                    SDL_DestroyTexture(itemTex);
                    SDL_DestroySurface(itemSurf);
                }
                currentY += 26.0f;
            }
        }
    }

    SDL_SetRenderClipRect(renderer, nullptr);

    float visibleHeight = height_ - previewHeight - 45.0f;
    float contentHeight = (currentY + scrollY_) - (y_ + 45.0f);
    maxScrollY_ = std::max(0.0f, contentHeight - visibleHeight);
    if (scrollY_ > maxScrollY_) scrollY_ = maxScrollY_;

    if (maxScrollY_ > 0) {
        float scrollbarHeight = std::max(20.0f, (visibleHeight / contentHeight) * visibleHeight);
        float scrollbarY = y_ + 45.0f + (scrollY_ / maxScrollY_) * (visibleHeight - scrollbarHeight);
        SDL_FRect scrollbar{x_ + width_ - 4.0f, scrollbarY, 2.0f, scrollbarHeight};
        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
        SDL_RenderFillRect(renderer, &scrollbar);
    }

    renderPreviewBox(renderer, font, selectedComponent);
}

void ComponentLibrary::renderPreviewBox(SDL_Renderer* renderer, TTF_Font* font, const std::string& compName) const {
    float previewHeight = 240.0f;
    SDL_FRect previewBg{x_, y_ + height_ - previewHeight, width_, previewHeight};

    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderFillRect(renderer, &previewBg);

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderLine(renderer, x_, previewBg.y, x_ + width_, previewBg.y);

    SDL_Color titleColor{150, 150, 150, 255};
    SDL_Surface* titleSurf = TTF_RenderText_Blended(font, "Preview", 0, titleColor);
    if (titleSurf) {
        SDL_Texture* titleTex = SDL_CreateTextureFromSurface(renderer, titleSurf);
        SDL_FRect textRect{x_ + 10.0f, previewBg.y + 10.0f, static_cast<float>(titleSurf->w)*0.75f, static_cast<float>(titleSurf->h)*0.75f};
        SDL_RenderTexture(renderer, titleTex, nullptr, &textRect);
        SDL_DestroyTexture(titleTex);
        SDL_DestroySurface(titleSurf);
    }

    if (compName == "None" || compName.empty()) return;

    float cx = x_ + width_ / 2.0f;
    float cy = previewBg.y + (previewHeight / 2.0f) + 10.0f;

    float scale = 1.8f;
    SDL_Color fillColor = {250, 252, 255, 255};
    SDL_Color strokeColor = {25, 35, 45, 255};

    auto fillRectLocal = [&](float x, float y, float w, float h, SDL_Color c) {
        x *= scale; y *= scale; w *= scale; h *= scale;
        SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
        SDL_Vertex v[6] = {
                {{cx + x, cy + y}, fc, {0, 0}}, {{cx + x + w, cy + y}, fc, {0, 0}}, {{cx + x + w, cy + y + h}, fc, {0, 0}},
                {{cx + x, cy + y}, fc, {0, 0}}, {{cx + x + w, cy + y + h}, fc, {0, 0}}, {{cx + x, cy + y + h}, fc, {0, 0}}
        };
        SDL_RenderGeometry(renderer, nullptr, v, 6, nullptr, 0);
    };

    auto fillTriangleLocal = [&](float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color c) {
        x1 *= scale; y1 *= scale; x2 *= scale; y2 *= scale; x3 *= scale; y3 *= scale;
        SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
        SDL_Vertex v[3] = {{{cx + x1, cy + y1}, fc, {0, 0}}, {{cx + x2, cy + y2}, fc, {0, 0}}, {{cx + x3, cy + y3}, fc, {0, 0}}};
        SDL_RenderGeometry(renderer, nullptr, v, 3, nullptr, 0);
    };

    auto fillCircleLocal = [&](float x, float y, float r, SDL_Color c) {
        x *= scale; y *= scale; r *= scale;
        const int segments = 32; std::vector<SDL_Vertex> v;
        SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
        float step = (2.0f * PI) / segments;
        for (int i = 0; i < segments; ++i) {
            v.push_back({{cx + x, cy + y}, fc, {0, 0}});
            v.push_back({{cx + x + r * std::cos(i * step), cy + y + r * std::sin(i * step)}, fc, {0, 0}});
            v.push_back({{cx + x + r * std::cos((i + 1) * step), cy + y + r * std::sin((i + 1) * step)}, fc, {0, 0}});
        }
        SDL_RenderGeometry(renderer, nullptr, v.data(), v.size(), nullptr, 0);
    };

    auto fillSemicircleLocal = [&](float x, float y, float r, float startAngle, float endAngle, SDL_Color c) {
        x *= scale; y *= scale; r *= scale;
        const int segments = 16; std::vector<SDL_Vertex> v;
        SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
        float step = (endAngle - startAngle) / segments;
        for (int i = 0; i < segments; ++i) {
            v.push_back({{cx + x, cy + y}, fc, {0, 0}});
            v.push_back({{cx + x + r * std::cos(startAngle + i * step), cy + y + r * std::sin(startAngle + i * step)}, fc, {0, 0}});
            v.push_back({{cx + x + r * std::cos(startAngle + (i + 1) * step), cy + y + r * std::sin(startAngle + (i + 1) * step)}, fc, {0, 0}});
        }
        SDL_RenderGeometry(renderer, nullptr, v.data(), v.size(), nullptr, 0);
    };

    auto drawTransformedLine = [&](float x1, float y1, float x2, float y2) {
        x1 *= scale; y1 *= scale; x2 *= scale; y2 *= scale;
        SDL_RenderLine(renderer, cx + x1, cy + y1, cx + x2, cy + y2);
    };

    auto drawTransformedCircle = [&](float cx_c, float cy_c, float r) {
        cx_c *= scale; cy_c *= scale; r *= scale;
        const int segments = 32; float step = (2.0f * PI) / segments;
        for (int i = 0; i < segments; ++i) {
            SDL_RenderLine(renderer, cx + cx_c + r * std::cos(i * step), cy + cy_c + r * std::sin(i * step), cx + cx_c + r * std::cos((i + 1) * step), cy + cy_c + r * std::sin((i + 1) * step));
        }
    };

    SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);

    if (compName == "Resistor") {
        fillRectLocal(-16, -8, 32, 16, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-32, 0, -16, 0); drawTransformedLine(16, 0, 32, 0);
        drawTransformedLine(-16, -8, 16, -8); drawTransformedLine(16, -8, 16, 8);
        drawTransformedLine(16, 8, -16, 8); drawTransformedLine(-16, 8, -16, -8);
    }
    else if (compName == "Capacitor") {
        fillRectLocal(-6, -12, 3, 24, strokeColor); fillRectLocal(3, -12, 3, 24, strokeColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-32, 0, -6, 0); drawTransformedLine(6, 0, 32, 0);
    }
    else if (compName == "Potentiometer") {
        fillRectLocal(-16, -16, 16, 32, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-32, -10, -16, -10); drawTransformedLine(-32, 10, -16, 10);
        drawTransformedLine(-16, -16, 0, -16); drawTransformedLine(0, -16, 0, 16); drawTransformedLine(0, 16, -16, 16); drawTransformedLine(-16, 16, -16, -16);
        drawTransformedLine(16, 0, 32, 0);
        drawTransformedLine(16, 0, 5, 0); drawTransformedLine(5, 0, 10, -5); drawTransformedLine(5, 0, 10, 5);
    }
    else if (compName == "Inductor") {
        for(int i = 0; i < 4; ++i) fillSemicircleLocal(-15.0f + (i * 10.0f), 0.0f, 5.0f, PI, 2.0f * PI, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-32, 0, -20, 0); drawTransformedLine(20, 0, 32, 0);
        for(int i = 0; i < 4; ++i) {
            float bx = -15.0f + (i * 10.0f); float step = PI / 8.0f;
            for(int j = 0; j < 8; ++j) {
                drawTransformedLine(bx + 5.0f * std::cos(PI + j * step), 5.0f * std::sin(PI + j * step), bx + 5.0f * std::cos(PI + (j+1) * step), 5.0f * std::sin(PI + (j+1) * step));
            }
        }
    }
    else if (compName == "Battery") {
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-20, 0, -6, 0); drawTransformedLine(6, 0, 20, 0);
        drawTransformedLine(-6, -16, -6, 16); drawTransformedLine(-2, -8, -2, 8);
        drawTransformedLine(2, -16, 2, 16); drawTransformedLine(6, -8, 6, 8);
    }
    else if (compName == "Clock Generator") {
        fillCircleLocal(0, 0, 16, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedCircle(0, 0, 16);
        drawTransformedLine(-30, 0, -16, 0); drawTransformedLine(16, 0, 30, 0);
        drawTransformedLine(-10, 5, -4, 5); drawTransformedLine(-4, 5, -4, -5);
        drawTransformedLine(-4, -5, 4, -5); drawTransformedLine(4, -5, 4, 5); drawTransformedLine(4, 5, 10, 5);
    }
    else if (compName == "Switch") {
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-32, 0, -12, 0); drawTransformedLine(12, 0, 32, 0);
        fillCircleLocal(-12, 0, 3, strokeColor); fillCircleLocal(12, 0, 3, strokeColor);
        drawTransformedLine(-12, 0, 10, -12);
    }
    else if (compName == "Push Button") {
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-32, 0, -12, 0); drawTransformedLine(12, 0, 32, 0);
        fillCircleLocal(-12, 0, 2, strokeColor); fillCircleLocal(12, 0, 2, strokeColor);
        drawTransformedLine(-16, -8, 16, -8); drawTransformedLine(0, -14, 0, -8);
        fillRectLocal(-6, -16, 12, 2, strokeColor);
    }
    else if (compName == "Colored LED") {
        fillTriangleLocal(-12, -12, -12, 12, 12, 0, {210, 210, 215, 255});
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-32, 0, -12, 0); drawTransformedLine(12, 0, 32, 0);
        drawTransformedLine(12, -12, 12, 12);
    }
    else if (compName == "7-Segment Display") {
        fillRectLocal(-20, -32, 40, 64, {32, 32, 36, 255});
        SDL_SetRenderDrawColor(renderer, 60, 65, 70, 255);
        drawTransformedLine(-20, -32, 20, -32); drawTransformedLine(20, -32, 20, 64);
        drawTransformedLine(20, 64, -20, 64); drawTransformedLine(-20, 64, -20, -32);
        SDL_Color off = {55, 50, 50, 255};
        fillRectLocal(-10, -24, 20, 3, off); fillRectLocal(10, -24, 3, 22, off);
        fillRectLocal(10, 2, 3, 22, off); fillRectLocal(-10, 24, 20, 3, off);
        fillRectLocal(-13, 2, 3, 22, off); fillRectLocal(-13, -24, 3, 22, off);
        fillRectLocal(-10, 0, 20, 3, off); fillCircleLocal(14, 24, 2.5f, off);
    }
    else if (compName == "LCD 16x2") {
        SDL_FRect block{cx - 35 * scale, cy - 15 * scale, 70 * scale, 30 * scale};
        SDL_SetRenderDrawColor(renderer, 40, 45, 50, 255); SDL_RenderFillRect(renderer, &block);
        SDL_FRect screen{cx - 25 * scale, cy - 8 * scale, 50 * scale, 16 * scale};
        SDL_SetRenderDrawColor(renderer, 130, 180, 50, 255); SDL_RenderFillRect(renderer, &screen);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255); SDL_RenderRect(renderer, &block);
        drawTransformedLine(-40, -5, -35, -5); drawTransformedLine(-40, 0, -35, 0); drawTransformedLine(-40, 5, -35, 5);
        for(int i=0; i<4; ++i) drawTransformedLine(35, -5 + i*3.3f, 40, -5 + i*3.3f);
    }
    else if (compName == "Keypad 4x4") {
        SDL_FRect block{cx - 25 * scale, cy - 30 * scale, 50 * scale, 60 * scale};
        SDL_SetRenderDrawColor(renderer, fillColor.r, fillColor.g, fillColor.b, 255); SDL_RenderFillRect(renderer, &block);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255); SDL_RenderRect(renderer, &block);
        for(int r = 0; r < 4; ++r) {
            for(int c = 0; c < 4; ++c) {
                fillRectLocal(-20.0f + c * 10.0f, -25.0f + r * 15.0f, 8.0f, 10.0f, {200, 200, 200, 255});
            }
        }
        for(int i = 0; i < 4; ++i) drawTransformedLine(-30, -20.0f + i * 13.0f, -25, -20.0f + i * 13.0f);
        for(int i = 0; i < 4; ++i) drawTransformedLine(-15.0f + i * 10.0f, 30, -15.0f + i * 10.0f, 35);
    }
    else if (compName == "AND Gate" || compName == "NAND Gate") {
        fillRectLocal(-20, -16, 20, 32, fillColor);
        fillSemicircleLocal(0, 0, 16.0f, -PI/2.0f, PI/2.0f, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-15, -16, -15, 16); drawTransformedLine(-15, -16, 0, -16); drawTransformedLine(-15, 16, 0, 16);
        drawTransformedLine(-40, -10, -20, -10); drawTransformedLine(-40, 10, -20, 10);
        if (compName == "NAND Gate") { drawTransformedCircle(19, 0, 3); drawTransformedLine(22, 0, 40, 0); }
        else { drawTransformedLine(16, 0, 40, 0); }
        for(int j = -6; j < 6; ++j) {
            float step = PI / 12.0f;
            drawTransformedLine(16.0f * std::cos(j * step), 16.0f * std::sin(j * step), 16.0f * std::cos((j+1) * step), 16.0f * std::sin((j+1) * step));
        }
    }
    else if (compName == "OR Gate" || compName == "XOR Gate") {
        fillTriangleLocal(-20, -20, -10, 0, 0, -15, fillColor); fillTriangleLocal(-10, 0, 20, 0, 0, -15, fillColor);
        fillTriangleLocal(-10, 0, 0, 15, 20, 0, fillColor); fillTriangleLocal(-20, 20, 0, 15, -10, 0, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-40, -10, -14, -10); drawTransformedLine(-40, 10, -14, 10); drawTransformedLine(20, 0, 40, 0);
        drawTransformedLine(-20, -20, -10, 0); drawTransformedLine(-10, 0, -20, 20); drawTransformedLine(-20, -20, 0, -15);
        drawTransformedLine(0, -15, 20, 0); drawTransformedLine(-20, 20, 0, 15); drawTransformedLine(0, 15, 20, 0);
        if (compName == "XOR Gate") {
            for (float y = -18; y <= 18; y += 2.0f) fillCircleLocal((y * y) / 32.0f - 24.0f, y, 1.2f, strokeColor);
        }
    }
    else if (compName == "NOT Gate") {
        fillTriangleLocal(-15, -15, -15, 15, 5, 0, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-40, 0, -15, 0); drawTransformedLine(-15, -15, -15, 15);
        drawTransformedLine(-15, -15, 5, 0); drawTransformedLine(-15, 15, 5, 0);
        drawTransformedCircle(8, 0, 3); drawTransformedLine(11, 0, 40, 0);
    }
    else if (compName == "Flip-Flop") {
        fillRectLocal(-20, -25, 40, 50, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-20, -25, 20, -25); drawTransformedLine(20, -25, 20, 25);
        drawTransformedLine(20, 25, -20, 25); drawTransformedLine(-20, 25, -20, -25);
        drawTransformedLine(-40, -15, -20, -15); drawTransformedLine(-40, 15, -20, 15);
        drawTransformedLine(20, -15, 40, -15); drawTransformedLine(20, 15, 40, 15);
        drawTransformedLine(-20, 10, -12, 15); drawTransformedLine(-12, 15, -20, 20);
    }
    else if (compName == "Diode") {
        fillTriangleLocal(-12, -12, -12, 12, 12, 0, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-32, 0, -12, 0); drawTransformedLine(12, 0, 32, 0);
        drawTransformedLine(-12, -12, -12, 12); drawTransformedLine(-12, -12, 12, 0);
        drawTransformedLine(-12, 12, 12, 0); drawTransformedLine(12, -12, 12, 12);
    }
    else if (compName == "Op-Amp") {
        fillTriangleLocal(-15, -20, -15, 20, 20, 0, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        drawTransformedLine(-15, -20, -15, 20); drawTransformedLine(-15, -20, 20, 0); drawTransformedLine(-15, 20, 20, 0);
        drawTransformedLine(-35, -8, -15, -8); drawTransformedLine(-35, 8, -15, 8); drawTransformedLine(20, 0, 35, 0);
        drawTransformedLine(-12, -10, -6, -10); drawTransformedLine(-12, 10, -6, 10); drawTransformedLine(-9, 7, -9, 13);
    }
    else if (compName == "ADC" || compName == "DAC") {
        fillRectLocal(-20, -35, 40, 70, fillColor);
        SDL_SetRenderDrawColor(renderer, strokeColor.r, strokeColor.g, strokeColor.b, 255);
        SDL_FRect block = {-20, -35, 40, 70};
        drawTransformedLine(block.x, block.y, block.x + block.w, block.y);
        drawTransformedLine(block.x + block.w, block.y, block.x + block.w, block.y + block.h);
        drawTransformedLine(block.x + block.w, block.y + block.h, block.x, block.y + block.h);
        drawTransformedLine(block.x, block.y + block.h, block.x, block.y);

        if (compName == "ADC") {
            drawTransformedLine(-30, -25, -20, -25); drawTransformedLine(-30, 0, -20, 0); drawTransformedLine(-30, 25, -20, 25);
            for(int i = 0; i < 8; ++i) drawTransformedLine(20, -26.0f + i * 7.5f, 30, -26.0f + i * 7.5f);
        } else {
            drawTransformedLine(0, -40, 0, -35); drawTransformedLine(0, 35, 0, 40); drawTransformedLine(20, 0, 30, 0);
            for(int i = 0; i < 8; ++i) drawTransformedLine(-30, -26.0f + i * 7.5f, -20, -26.0f + i * 7.5f);
        }
    }
    else if (compName == "Ground") {
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
}