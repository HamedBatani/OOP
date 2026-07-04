// src/ComponentLibrary.cpp
#include "ComponentLibrary.h"
#include <algorithm>
#include <cctype>
#include <cmath>

namespace {
    constexpr float PI = 3.14159265358979323846f;
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
    categories_.push_back({"Analog", true, {"Resistor", "Capacitor", "Inductor", "Diode", "Op-Amp"}});
    categories_.push_back({"Digital", false, {"AND Gate", "OR Gate", "NOT Gate", "Flip-Flop"}});
    categories_.push_back({"Power", false, {"DC Source", "AC Source", "Ground"}});
    categories_.push_back({"Measurement", false, {"Voltmeter", "Ammeter", "Oscilloscope"}});
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

    // --- توابع رنگ‌امیزی داخلی مختص بخش پیش‌نمایش ---
    auto fillTrianglePreview = [&](float x1, float y1, float x2, float y2, float x3, float y3, SDL_Color c) {
        SDL_FColor fc = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
        SDL_Vertex v[3] = {{{cx + x1, cy + y1}, fc, {0, 0}}, {{cx + x2, cy + y2}, fc, {0, 0}}, {{cx + x3, cy + y3}, fc, {0, 0}}};
        SDL_RenderGeometry(renderer, nullptr, v, 3, nullptr, 0);
    };

    auto fillCirclePreview = [&](float x, float y, float r, SDL_Color c) {
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

    auto fillSemicirclePreview = [&](float x, float y, float r, float startAngle, float endAngle, SDL_Color c) {
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

    auto drawCircle = [&](float x, float y, float r) {
        const int segments = 24; float step = (2.0f * PI) / segments;
        for (int i = 0; i < segments; ++i) {
            SDL_RenderLine(renderer, x + r * std::cos(i * step), y + r * std::sin(i * step), x + r * std::cos((i + 1) * step), y + r * std::sin((i + 1) * step));
        }
    };

    SDL_Color fillC = {250, 220, 220, 255}; // قرمز خیلی ملایم و توپر
    SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255); // قرمز تیره برای حاشیه

    if (compName == "Resistor") {
        SDL_FRect body{cx - 16, cy - 8, 32, 16};
        SDL_SetRenderDrawColor(renderer, fillC.r, fillC.g, fillC.b, 255);
        SDL_RenderFillRect(renderer, &body);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);
        SDL_RenderRect(renderer, &body);
        SDL_RenderLine(renderer, cx - 32, cy, cx - 16, cy);
        SDL_RenderLine(renderer, cx + 16, cy, cx + 32, cy);
    }
    else if (compName == "Capacitor") {
        SDL_FRect plate1{cx - 7, cy - 12, 3, 24}; SDL_FRect plate2{cx + 4, cy - 12, 3, 24};
        SDL_SetRenderDrawColor(renderer, fillC.r, fillC.g, fillC.b, 255);
        SDL_RenderFillRect(renderer, &plate1); SDL_RenderFillRect(renderer, &plate2);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);
        SDL_RenderRect(renderer, &plate1); SDL_RenderRect(renderer, &plate2);
        SDL_RenderLine(renderer, cx - 32, cy, cx - 7, cy); SDL_RenderLine(renderer, cx + 7, cy, cx + 32, cy);
    }
    else if (compName == "Inductor") {
        for(int i = 0; i < 4; ++i) fillSemicirclePreview(-15.0f + (i * 10.0f), 0.0f, 5.0f, PI, 2.0f * PI, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);
        SDL_RenderLine(renderer, cx - 32, cy, cx - 20, cy); SDL_RenderLine(renderer, cx + 20, cy, cx + 32, cy);
        for(int i = 0; i < 4; ++i) {
            float bx = cx - 15.0f + (i * 10.0f); float step = PI / 8.0f;
            for(int j = 0; j < 8; ++j) {
                SDL_RenderLine(renderer, bx + 5.0f * std::cos(PI + j * step), cy + 5.0f * std::sin(PI + j * step), bx + 5.0f * std::cos(PI + (j+1) * step), cy + 5.0f * std::sin(PI + (j+1) * step));
            }
        }
    }
    else if (compName == "Diode") {
        fillTrianglePreview(-12, -12, -12, 12, 12, 0, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);
        SDL_RenderLine(renderer, cx - 32, cy, cx - 12, cy); SDL_RenderLine(renderer, cx + 12, cy, cx + 32, cy);
        SDL_RenderLine(renderer, cx - 12, cy - 12, cx - 12, cy + 12); SDL_RenderLine(renderer, cx - 12, cy - 12, cx + 12, cy);
        SDL_RenderLine(renderer, cx - 12, cy + 12, cx + 12, cy); SDL_RenderLine(renderer, cx + 12, cy - 12, cx + 12, cy + 12);
    }
    else if (compName == "Op-Amp") {
        fillTrianglePreview(-15, -20, -15, 20, 20, 0, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);
        SDL_RenderLine(renderer, cx - 15, cy - 20, cx - 15, cy + 20); SDL_RenderLine(renderer, cx - 15, cy - 20, cx + 20, cy);
        SDL_RenderLine(renderer, cx - 15, cy + 20, cx + 20, cy); SDL_RenderLine(renderer, cx - 35, cy - 8, cx - 15, cy - 8);
        SDL_RenderLine(renderer, cx - 35, cy + 8, cx - 15, cy + 8); SDL_RenderLine(renderer, cx + 20, cy, cx + 35, cy);
        SDL_RenderLine(renderer, cx - 12, cy - 10, cx - 6, cy - 10); SDL_RenderLine(renderer, cx - 12, cy + 10, cx - 6, cy + 10);
        SDL_RenderLine(renderer, cx - 9, cy + 7, cx - 9, cy + 13);
    }
    else if (compName == "AND Gate") {
        SDL_FRect body{cx - 15, cy - 16, 15, 32};
        SDL_SetRenderDrawColor(renderer, fillC.r, fillC.g, fillC.b, 255); SDL_RenderFillRect(renderer, &body);
        fillSemicirclePreview(0, 0, 16.0f, -PI / 2.0f, PI / 2.0f, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);
        SDL_RenderLine(renderer, cx - 35, cy - 8, cx - 15, cy - 8); SDL_RenderLine(renderer, cx - 35, cy + 8, cx - 15, cy + 8);
        SDL_RenderLine(renderer, cx + 16, cy, cx + 35, cy); SDL_RenderLine(renderer, cx - 15, cy - 16, cx - 15, cy + 16);
        SDL_RenderLine(renderer, cx - 15, cy - 16, cx, cy - 16); SDL_RenderLine(renderer, cx - 15, cy + 16, cx, cy + 16);
        for(int j = -6; j < 6; ++j) { float step = PI / 12.0f; SDL_RenderLine(renderer, cx + 16.0f * std::cos(j * step), cy + 16.0f * std::sin(j * step), cx + 16.0f * std::cos((j+1) * step), cy + 16.0f * std::sin((j+1) * step)); }
    }
    else if (compName == "OR Gate") {
        fillTrianglePreview(-15, -20, -5, 0, 5, -15, fillC); fillTrianglePreview(-5, 0, 25, 0, 5, -15, fillC);
        fillTrianglePreview(-5, 0, 5, 15, 25, 0, fillC); fillTrianglePreview(-15, 20, 5, 15, -5, 0, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);
        SDL_RenderLine(renderer, cx - 35, cy - 8, cx - 10, cy - 8); SDL_RenderLine(renderer, cx - 35, cy + 8, cx - 10, cy + 8);
        SDL_RenderLine(renderer, cx + 25, cy, cx + 35, cy); SDL_RenderLine(renderer, cx - 15, cy - 20, cx - 5, cy);
        SDL_RenderLine(renderer, cx - 5, cy, cx - 15, cy + 20); SDL_RenderLine(renderer, cx - 15, cy - 20, cx + 5, cy - 15);
        SDL_RenderLine(renderer, cx + 5, cy - 15, cx + 25, cy); SDL_RenderLine(renderer, cx - 15, cy + 20, cx + 5, cy + 15);
        SDL_RenderLine(renderer, cx + 5, cy + 15, cx + 25, cy);
    }
    else if (compName == "NOT Gate") {
        fillTrianglePreview(-15, -15, -15, 15, 5, 0, fillC); fillCirclePreview(10, 0, 5, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255);
        SDL_RenderLine(renderer, cx - 35, cy, cx - 15, cy); SDL_RenderLine(renderer, cx + 15, cy, cx + 35, cy);
        SDL_RenderLine(renderer, cx - 15, cy - 15, cx - 15, cy + 15); SDL_RenderLine(renderer, cx - 15, cy - 15, cx + 5, cy);
        SDL_RenderLine(renderer, cx - 15, cy + 15, cx + 5, cy); drawCircle(cx + 10, cy, 5);
    }
    else if (compName == "Flip-Flop") {
        SDL_FRect body{cx - 20, cy - 25, 40, 50};
        SDL_SetRenderDrawColor(renderer, fillC.r, fillC.g, fillC.b, 255); SDL_RenderFillRect(renderer, &body);
        fillCirclePreview(24, 10, 4, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255); SDL_RenderRect(renderer, &body);
        SDL_RenderLine(renderer, cx - 40, cy - 10, cx - 20, cy - 10); SDL_RenderLine(renderer, cx - 40, cy + 10, cx - 20, cy + 10);
        SDL_RenderLine(renderer, cx + 20, cy - 10, cx + 40, cy - 10); SDL_RenderLine(renderer, cx + 20, cy + 10, cx + 40, cy + 10);
        SDL_RenderLine(renderer, cx - 20, cy + 5, cx - 12, cy + 10); SDL_RenderLine(renderer, cx - 12, cy + 10, cx - 20, cy + 15);
        drawCircle(cx + 24, cy + 10, 4);
    }
    else if (compName == "DC Source") {
        fillCirclePreview(0, 0, 20, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255); drawCircle(cx, cy, 20);
        SDL_RenderLine(renderer, cx, cy - 30, cx, cy - 20); SDL_RenderLine(renderer, cx, cy + 20, cx, cy + 30);
        SDL_RenderLine(renderer, cx - 12, cy - 8, cx + 12, cy - 8); SDL_RenderLine(renderer, cx - 6, cy + 8, cx + 6, cy + 8);
        SDL_RenderLine(renderer, cx - 12, cy - 16, cx - 6, cy - 16); SDL_RenderLine(renderer, cx - 9, cy - 19, cx - 9, cy - 13);
    }
    else if (compName == "AC Source") {
        fillCirclePreview(0, 0, 15, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255); drawCircle(cx, cy, 15);
        SDL_RenderLine(renderer, cx, cy - 30, cx, cy - 15); SDL_RenderLine(renderer, cx, cy + 15, cx, cy + 30);
        for(float x = -8; x <= 8; x += 1.0f) { float y1 = std::sin(x * PI / 8.0f) * 5.0f; float y2 = std::sin((x+1) * PI / 8.0f) * 5.0f; SDL_RenderLine(renderer, cx + x, cy - y1, cx + x + 1, cy - y2); }
    }
    else if (compName == "Voltmeter" || compName == "Ammeter") {
        fillCirclePreview(0, 0, 15, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255); drawCircle(cx, cy, 15);
        SDL_RenderLine(renderer, cx, cy - 30, cx, cy - 15); SDL_RenderLine(renderer, cx, cy + 15, cx, cy + 30);
        if(compName == "Voltmeter") { SDL_RenderLine(renderer, cx - 5, cy - 5, cx, cy + 5); SDL_RenderLine(renderer, cx, cy + 5, cx + 5, cy - 5); }
        else { SDL_RenderLine(renderer, cx, cy - 6, cx - 5, cy + 5); SDL_RenderLine(renderer, cx, cy - 6, cx + 5, cy + 5); SDL_RenderLine(renderer, cx - 3, cy + 2, cx + 3, cy + 2); }
    }
    else if (compName == "Oscilloscope") {
        SDL_FRect outer{cx - 25, cy - 20, 50, 40}; SDL_FRect screen{cx - 20, cy - 15, 30, 30};
        SDL_SetRenderDrawColor(renderer, fillC.r, fillC.g, fillC.b, 255); SDL_RenderFillRect(renderer, &outer);
        SDL_SetRenderDrawColor(renderer, fillC.r*0.9f, fillC.g*0.9f, fillC.b*0.9f, 255); SDL_RenderFillRect(renderer, &screen);
        fillCirclePreview(17, -5, 3, fillC); fillCirclePreview(17, 5, 3, fillC);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255); SDL_RenderRect(renderer, &outer); SDL_RenderRect(renderer, &screen);
        drawCircle(cx + 17, cy - 5, 3); drawCircle(cx + 17, cy + 5, 3);
        for(float x = -18; x <= 8; x += 1.0f) { float y1 = std::sin((x+18) * PI / 6.0f) * 8.0f; float y2 = std::sin((x+19) * PI / 6.0f) * 8.0f; SDL_RenderLine(renderer, cx + x, cy - y1, cx + x + 1, cy - y2); }
    }
    else if (compName == "Ground") {
        SDL_RenderLine(renderer, cx, cy - 15, cx, cy); SDL_RenderLine(renderer, cx - 12, cy, cx + 12, cy);
        SDL_RenderLine(renderer, cx - 8, cy + 4, cx + 8, cy + 4); SDL_RenderLine(renderer, cx - 4, cy + 8, cx + 4, cy + 8);
    }
    else {
        SDL_FRect body{cx - 20, cy - 15, 40, 30};
        SDL_SetRenderDrawColor(renderer, fillC.r, fillC.g, fillC.b, 255); SDL_RenderFillRect(renderer, &body);
        SDL_SetRenderDrawColor(renderer, 180, 40, 40, 255); SDL_RenderRect(renderer, &body);
        SDL_RenderLine(renderer, cx - 35, cy, cx - 20, cy); SDL_RenderLine(renderer, cx + 20, cy, cx + 35, cy);
    }
}