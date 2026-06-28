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
        hoverX_ = event.motion.x;
        hoverY_ = event.motion.y;
    }

    if (isSearchFocused_) {
        if (event.type == SDL_EVENT_TEXT_INPUT) {
            searchQuery_ += event.text.text;
            return;
        } else if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_BACKSPACE && !searchQuery_.empty()) {
                searchQuery_.pop_back();
                return;
            }
        }
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        bool isLeftClick = (event.button.button == SDL_BUTTON_LEFT);
        bool isRightClick = (event.button.button == SDL_BUTTON_RIGHT);

        if (!isLeftClick && !isRightClick) return;

        float mx = event.button.x;
        float my = event.button.y;

        if (mx < x_ || mx > x_ + width_ || my < y_ || my > y_ + height_ - 150.0f) {
            if (isLeftClick && !(mx >= x_ && mx <= x_ + width_ && my >= y_ && my <= y_ + height_)) {
                isSearchFocused_ = false;
            }
            return;
        }

        if (isLeftClick) {
            SDL_FRect searchRect{x_ + 5.0f, y_ + 5.0f, width_ - 10.0f, 30.0f};
            if (mx >= searchRect.x && mx <= searchRect.x + searchRect.w &&
                my >= searchRect.y && my <= searchRect.y + searchRect.h) {
                isSearchFocused_ = true;
                return;
            } else {
                isSearchFocused_ = false;
            }
        }

        float activeY = y_ + 70.0f;
        if (activeList_.empty()) {
            activeY += 25.0f;
        } else {
            for (size_t i = 0; i < activeList_.size(); ++i) {
                SDL_FRect itemRect{x_ + 10.0f, activeY, width_ - 20.0f, 25.0f};
                if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w &&
                    my >= itemRect.y && my <= itemRect.y + itemRect.h) {
                    if (isLeftClick) {
                        // بررسی کلیک روی علامت X در سمت راست یا دبل-کلیک
                        if (mx >= itemRect.x + itemRect.w - 25.0f || event.button.clicks >= 2) {
                            activeList_.erase(activeList_.begin() + i);
                        } else {
                            selectedComponent = activeList_[i];
                        }
                    } else if (isRightClick) {
                        activeList_.erase(activeList_.begin() + i);
                    }
                    return;
                }
                activeY += 30.0f;
            }
        }

        float currentY = activeY + 30.0f;

        for (auto& cat : categories_) {
            bool catMatch = containsIgnoreCase(cat.name, searchQuery_);
            std::vector<std::string> matchingItems;

            for (const auto& item : cat.items) {
                if (catMatch || containsIgnoreCase(item, searchQuery_)) {
                    matchingItems.push_back(item);
                }
            }

            if (!searchQuery_.empty() && !catMatch && matchingItems.empty()) continue;

            SDL_FRect catRect{x_ + 5.0f, currentY, width_ - 10.0f, 30.0f};
            if (mx >= catRect.x && mx <= catRect.x + catRect.w &&
                my >= catRect.y && my <= catRect.y + catRect.h) {
                if (isLeftClick) cat.isExpanded = !cat.isExpanded;
                return;
            }
            currentY += 35.0f;

            bool shouldExpand = cat.isExpanded || (!searchQuery_.empty() && !matchingItems.empty());

            if (shouldExpand) {
                for (const auto& item : matchingItems) {
                    SDL_FRect itemRect{x_ + 20.0f, currentY, width_ - 30.0f, 25.0f};
                    if (mx >= itemRect.x && mx <= itemRect.x + itemRect.w &&
                        my >= itemRect.y && my <= itemRect.y + itemRect.h) {
                        if (isLeftClick) {
                            // بررسی کلیک روی علامت + در سمت راست یا دبل-کلیک
                            if (mx >= itemRect.x + itemRect.w - 25.0f || event.button.clicks >= 2) {
                                if (std::find(activeList_.begin(), activeList_.end(), item) == activeList_.end()) {
                                    activeList_.push_back(item);
                                }
                            } else {
                                selectedComponent = item;
                            }
                        } else if (isRightClick) {
                            if (std::find(activeList_.begin(), activeList_.end(), item) == activeList_.end()) {
                                activeList_.push_back(item);
                            }
                        }
                        return;
                    }
                    currentY += 30.0f;
                }
            }
        }
    }
}

void ComponentLibrary::render(SDL_Renderer* renderer, TTF_Font* font, const std::string& selectedComponent) const {
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer, 30, 36, 48, 255);
    SDL_FRect bgRect{x_, y_, width_, height_};
    SDL_RenderFillRect(renderer, &bgRect);

    if (!font) return;

    SDL_FRect searchRect{x_ + 5.0f, y_ + 5.0f, width_ - 10.0f, 30.0f};
    if (isSearchFocused_) {
        SDL_SetRenderDrawColor(renderer, 66, 153, 225, 255);
        SDL_RenderRect(renderer, &searchRect);
        SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255);
    } else {
        SDL_SetRenderDrawColor(renderer, 40, 48, 65, 255);
    }
    SDL_RenderFillRect(renderer, &searchRect);

    std::string displayText = searchQuery_.empty() ? "Search..." : searchQuery_;
    SDL_Color searchTextColor = searchQuery_.empty() ? SDL_Color{120, 130, 150, 255} : SDL_Color{255, 255, 255, 255};
    SDL_Surface* searchSurf = TTF_RenderText_Blended(font, displayText.c_str(), 0, searchTextColor);
    if (searchSurf) {
        SDL_Texture* searchTex = SDL_CreateTextureFromSurface(renderer, searchSurf);
        SDL_FRect textRect{searchRect.x + 10.0f, searchRect.y + (30.0f - searchSurf->h) / 2.0f, static_cast<float>(searchSurf->w), static_cast<float>(searchSurf->h)};
        SDL_RenderTexture(renderer, searchTex, nullptr, &textRect);
        SDL_DestroyTexture(searchTex);
        SDL_DestroySurface(searchSurf);
    }

    SDL_Color headerColor{150, 160, 180, 255};
    SDL_Surface* activeTitleSurf = TTF_RenderText_Blended(font, "Active List:", 0, headerColor);
    if (activeTitleSurf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, activeTitleSurf);
        SDL_FRect dest{x_ + 10.0f, y_ + 45.0f, static_cast<float>(activeTitleSurf->w)*0.8f, static_cast<float>(activeTitleSurf->h)*0.8f};
        SDL_RenderTexture(renderer, tex, nullptr, &dest);
        SDL_DestroyTexture(tex);
        SDL_DestroySurface(activeTitleSurf);
    }

    float currentY = y_ + 70.0f;

    if (activeList_.empty()) {
        SDL_Color emptyColor{100, 110, 130, 255};
        SDL_Surface* emptySurf = TTF_RenderText_Blended(font, "(Double-Click to add)", 0, emptyColor);
        if (emptySurf) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, emptySurf);
            SDL_FRect dest{x_ + 15.0f, currentY, static_cast<float>(emptySurf->w)*0.7f, static_cast<float>(emptySurf->h)*0.7f};
            SDL_RenderTexture(renderer, tex, nullptr, &dest);
            SDL_DestroyTexture(tex);
            SDL_DestroySurface(emptySurf);
        }
        currentY += 25.0f;
    } else {
        for (const auto& item : activeList_) {
            SDL_FRect itemRect{x_ + 10.0f, currentY, width_ - 20.0f, 25.0f};
            bool isItemHovered = (hoverX_ >= itemRect.x && hoverX_ <= itemRect.x + itemRect.w &&
                                  hoverY_ >= itemRect.y && hoverY_ <= itemRect.y + itemRect.h);

            if (item == selectedComponent) SDL_SetRenderDrawColor(renderer, 40, 100, 160, 255);
            else if (isItemHovered) SDL_SetRenderDrawColor(renderer, 80, 70, 90, 255);
            else SDL_SetRenderDrawColor(renderer, 45, 50, 65, 255);

            SDL_RenderFillRect(renderer, &itemRect);

            SDL_Color itemTextColor{220, 230, 255, 255};
            SDL_Surface* itemSurf = TTF_RenderText_Blended(font, item.c_str(), 0, itemTextColor);
            if (itemSurf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, itemSurf);
                SDL_FRect textRect{itemRect.x + 10.0f, itemRect.y + (25.0f - itemSurf->h)/2.0f, static_cast<float>(itemSurf->w), static_cast<float>(itemSurf->h)};
                SDL_RenderTexture(renderer, tex, nullptr, &textRect);
                SDL_DestroyTexture(tex);
                SDL_DestroySurface(itemSurf);
            }

            // رسم آیکون قرمز X در زمان هاور برای راهنمایی بصری حذف
            if (isItemHovered) {
                SDL_Color xColor{255, 100, 100, 255};
                SDL_Surface* xSurf = TTF_RenderText_Blended(font, "X", 0, xColor);
                if (xSurf) {
                    SDL_Texture* xTex = SDL_CreateTextureFromSurface(renderer, xSurf);
                    SDL_FRect xRect{itemRect.x + itemRect.w - 18.0f, itemRect.y + (25.0f - xSurf->h)/2.0f, static_cast<float>(xSurf->w), static_cast<float>(xSurf->h)};
                    SDL_RenderTexture(renderer, xTex, nullptr, &xRect);
                    SDL_DestroyTexture(xTex);
                    SDL_DestroySurface(xSurf);
                }
            }
            currentY += 30.0f;
        }
    }

    currentY += 5.0f;
    SDL_Surface* libTitleSurf = TTF_RenderText_Blended(font, "Library:", 0, headerColor);
    if (libTitleSurf) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, libTitleSurf);
        SDL_FRect dest{x_ + 10.0f, currentY, static_cast<float>(libTitleSurf->w)*0.8f, static_cast<float>(libTitleSurf->h)*0.8f};
        SDL_RenderTexture(renderer, tex, nullptr, &dest);
        SDL_DestroyTexture(tex);
        SDL_DestroySurface(libTitleSurf);
    }
    currentY += 25.0f;

    float remainingHeight = (y_ + height_ - 150.0f) - currentY;
    if (remainingHeight > 0) {
        SDL_Rect listClipRect{ static_cast<int>(x_), static_cast<int>(currentY), static_cast<int>(width_), static_cast<int>(remainingHeight) };
        SDL_SetRenderClipRect(renderer, &listClipRect);

        int totalMatches = 0;

        for (const auto& cat : categories_) {
            bool catMatch = containsIgnoreCase(cat.name, searchQuery_);
            std::vector<std::string> matchingItems;

            for (const auto& item : cat.items) {
                if (catMatch || containsIgnoreCase(item, searchQuery_)) {
                    matchingItems.push_back(item);
                }
            }

            if (!searchQuery_.empty() && !catMatch && matchingItems.empty()) continue;

            totalMatches++;

            SDL_FRect catRect{x_ + 5.0f, currentY, width_ - 10.0f, 30.0f};
            bool isCatHovered = (hoverX_ >= catRect.x && hoverX_ <= catRect.x + catRect.w &&
                                 hoverY_ >= catRect.y && hoverY_ <= catRect.y + catRect.h);

            if (isCatHovered) SDL_SetRenderDrawColor(renderer, 70, 85, 110, 255);
            else SDL_SetRenderDrawColor(renderer, 45, 55, 75, 255);

            SDL_RenderFillRect(renderer, &catRect);

            bool isExpanded = cat.isExpanded || (!searchQuery_.empty() && !matchingItems.empty());
            std::string prefix = isExpanded ? "v  " : ">  ";
            SDL_Color catTextColor{240, 245, 255, 255};

            SDL_Surface* catSurf = TTF_RenderText_Blended(font, (prefix + cat.name).c_str(), 0, catTextColor);
            if (catSurf) {
                SDL_Texture* catTex = SDL_CreateTextureFromSurface(renderer, catSurf);
                SDL_FRect textRect{catRect.x + 10.0f, catRect.y + (30.0f - catSurf->h) / 2.0f, static_cast<float>(catSurf->w), static_cast<float>(catSurf->h)};
                SDL_RenderTexture(renderer, catTex, nullptr, &textRect);
                SDL_DestroyTexture(catTex);
                SDL_DestroySurface(catSurf);
            }

            currentY += 35.0f;

            if (isExpanded) {
                for (const auto& item : matchingItems) {
                    totalMatches++;
                    SDL_FRect itemRect{x_ + 20.0f, currentY, width_ - 30.0f, 25.0f};
                    bool isItemHovered = (hoverX_ >= itemRect.x && hoverX_ <= itemRect.x + itemRect.w &&
                                          hoverY_ >= itemRect.y && hoverY_ <= itemRect.y + itemRect.h);

                    if (item == selectedComponent) {
                        SDL_SetRenderDrawColor(renderer, 40, 100, 160, 255);
                    } else if (isItemHovered) {
                        SDL_SetRenderDrawColor(renderer, 60, 120, 180, 255);
                    } else {
                        SDL_SetRenderDrawColor(renderer, 38, 45, 60, 255);
                    }

                    SDL_RenderFillRect(renderer, &itemRect);

                    SDL_Color itemTextColor{180, 190, 210, 255};
                    SDL_Surface* itemSurf = TTF_RenderText_Blended(font, item.c_str(), 0, itemTextColor);
                    if (itemSurf) {
                        SDL_Texture* itemTex = SDL_CreateTextureFromSurface(renderer, itemSurf);
                        SDL_FRect textRect{itemRect.x + 10.0f, itemRect.y + (25.0f - itemSurf->h) / 2.0f, static_cast<float>(itemSurf->w), static_cast<float>(itemSurf->h)};
                        SDL_RenderTexture(renderer, itemTex, nullptr, &textRect);
                        SDL_DestroyTexture(itemTex);
                        SDL_DestroySurface(itemSurf);
                    }

                    // رسم آیکون سبز + در زمان هاور برای راهنمایی افزودن قطعه
                    if (isItemHovered) {
                        SDL_Color plusColor{100, 255, 100, 255};
                        SDL_Surface* plusSurf = TTF_RenderText_Blended(font, "+", 0, plusColor);
                        if (plusSurf) {
                            SDL_Texture* plusTex = SDL_CreateTextureFromSurface(renderer, plusSurf);
                            SDL_FRect plusRect{itemRect.x + itemRect.w - 18.0f, itemRect.y + (25.0f - plusSurf->h)/2.0f, static_cast<float>(plusSurf->w), static_cast<float>(plusSurf->h)};
                            SDL_RenderTexture(renderer, plusTex, nullptr, &plusRect);
                            SDL_DestroyTexture(plusTex);
                            SDL_DestroySurface(plusSurf);
                        }
                    }
                    currentY += 30.0f;
                }
            }
        }

        if (totalMatches == 0 && !searchQuery_.empty()) {
            SDL_Color errorColor{255, 100, 100, 255};
            SDL_Surface* errorSurf = TTF_RenderText_Blended(font, "No component!", 0, errorColor);
            if (errorSurf) {
                SDL_Texture* errorTex = SDL_CreateTextureFromSurface(renderer, errorSurf);
                SDL_FRect textRect{x_ + 10.0f, currentY + 10.0f, static_cast<float>(errorSurf->w), static_cast<float>(errorSurf->h)};
                SDL_RenderTexture(renderer, errorTex, nullptr, &textRect);
                SDL_DestroyTexture(errorTex);
                SDL_DestroySurface(errorSurf);
            }
        }
    }

    SDL_SetRenderClipRect(renderer, nullptr);
    renderPreviewBox(renderer, font, selectedComponent);
}

void ComponentLibrary::renderPreviewBox(SDL_Renderer* renderer, TTF_Font* font, const std::string& compName) const {
    SDL_FRect previewBg{x_, y_ + height_ - 150.0f, width_, 150.0f};
    SDL_SetRenderDrawColor(renderer, 24, 28, 36, 255);
    SDL_RenderFillRect(renderer, &previewBg);

    SDL_SetRenderDrawColor(renderer, 80, 90, 110, 255);
    SDL_RenderLine(renderer, x_, y_ + height_ - 150.0f, x_ + width_, y_ + height_ - 150.0f);

    SDL_Color titleColor{150, 160, 180, 255};
    SDL_Surface* titleSurf = TTF_RenderText_Blended(font, "Preview:", 0, titleColor);
    if (titleSurf) {
        SDL_Texture* titleTex = SDL_CreateTextureFromSurface(renderer, titleSurf);
        SDL_FRect textRect{x_ + 10.0f, previewBg.y + 10.0f, static_cast<float>(titleSurf->w)*0.8f, static_cast<float>(titleSurf->h)*0.8f};
        SDL_RenderTexture(renderer, titleTex, nullptr, &textRect);
        SDL_DestroyTexture(titleTex);
        SDL_DestroySurface(titleSurf);
    }

    if (compName == "None" || compName.empty()) return;

    float cx = x_ + width_ / 2.0f;
    float cy = previewBg.y + 85.0f;

    SDL_SetRenderDrawColor(renderer, 220, 80, 80, 255);

    auto drawCircle = [&](float x, float y, float r) {
        const int segments = 24;
        float step = (2.0f * PI) / segments;
        for (int i = 0; i < segments; ++i) {
            SDL_RenderLine(renderer,
                           x + r * std::cos(i * step), y + r * std::sin(i * step),
                           x + r * std::cos((i + 1) * step), y + r * std::sin((i + 1) * step));
        }
    };

    if (compName == "Resistor") {
        SDL_RenderLine(renderer, cx - 40, cy, cx - 20, cy);
        SDL_RenderLine(renderer, cx + 20, cy, cx + 40, cy);
        SDL_FRect body{cx - 20, cy - 10, 40, 20};
        SDL_RenderRect(renderer, &body);
    }
    else if (compName == "Capacitor") {
        SDL_RenderLine(renderer, cx - 40, cy, cx - 10, cy);
        SDL_RenderLine(renderer, cx + 10, cy, cx + 40, cy);
        SDL_RenderLine(renderer, cx - 10, cy - 15, cx - 10, cy + 15);
        SDL_RenderLine(renderer, cx + 10, cy - 15, cx + 10, cy + 15);
    }
    else if (compName == "Inductor") {
        SDL_RenderLine(renderer, cx - 40, cy, cx - 20, cy);
        SDL_RenderLine(renderer, cx + 20, cy, cx + 40, cy);
        for(int i = 0; i < 4; ++i) {
            float bx = cx - 15 + (i * 10);
            float step = PI / 8.0f;
            for(int j = 0; j < 8; ++j) {
                SDL_RenderLine(renderer,
                               bx + 5 * std::cos(PI + j * step), cy + 5 * std::sin(PI + j * step),
                               bx + 5 * std::cos(PI + (j+1) * step), cy + 5 * std::sin(PI + (j+1) * step));
            }
        }
    }
    else if (compName == "Diode") {
        SDL_RenderLine(renderer, cx - 40, cy, cx - 15, cy);
        SDL_RenderLine(renderer, cx + 15, cy, cx + 40, cy);
        SDL_RenderLine(renderer, cx - 15, cy - 15, cx - 15, cy + 15);
        SDL_RenderLine(renderer, cx - 15, cy - 15, cx + 15, cy);
        SDL_RenderLine(renderer, cx - 15, cy + 15, cx + 15, cy);
        SDL_RenderLine(renderer, cx + 15, cy - 15, cx + 15, cy + 15);
    }
    else if (compName == "Op-Amp") {
        SDL_RenderLine(renderer, cx - 15, cy - 25, cx - 15, cy + 25);
        SDL_RenderLine(renderer, cx - 15, cy - 25, cx + 25, cy);
        SDL_RenderLine(renderer, cx - 15, cy + 25, cx + 25, cy);
        SDL_RenderLine(renderer, cx - 40, cy - 10, cx - 15, cy - 10);
        SDL_RenderLine(renderer, cx - 40, cy + 10, cx - 15, cy + 10);
        SDL_RenderLine(renderer, cx + 25, cy, cx + 40, cy);
        SDL_RenderLine(renderer, cx - 12, cy - 10, cx - 6, cy - 10);
        SDL_RenderLine(renderer, cx - 12, cy + 10, cx - 6, cy + 10);
        SDL_RenderLine(renderer, cx - 9, cy + 7, cx - 9, cy + 13);
    }
    else if (compName == "AND Gate") {
        SDL_RenderLine(renderer, cx - 40, cy - 10, cx - 15, cy - 10);
        SDL_RenderLine(renderer, cx - 40, cy + 10, cx - 15, cy + 10);
        SDL_RenderLine(renderer, cx + 15, cy, cx + 40, cy);
        SDL_RenderLine(renderer, cx - 15, cy - 20, cx - 15, cy + 20);
        SDL_RenderLine(renderer, cx - 15, cy - 20, cx, cy - 20);
        SDL_RenderLine(renderer, cx - 15, cy + 20, cx, cy + 20);
        float step = PI / 12.0f;
        for(int j = -6; j < 6; ++j) {
            SDL_RenderLine(renderer,
                           cx + 20 * std::cos(j * step), cy + 20 * std::sin(j * step),
                           cx + 20 * std::cos((j+1) * step), cy + 20 * std::sin((j+1) * step));
        }
    }
    else if (compName == "OR Gate") {
        SDL_RenderLine(renderer, cx - 40, cy - 10, cx - 10, cy - 10);
        SDL_RenderLine(renderer, cx - 40, cy + 10, cx - 10, cy + 10);
        SDL_RenderLine(renderer, cx + 25, cy, cx + 40, cy);
        SDL_RenderLine(renderer, cx - 15, cy - 20, cx - 5, cy);
        SDL_RenderLine(renderer, cx - 5, cy, cx - 15, cy + 20);
        SDL_RenderLine(renderer, cx - 15, cy - 20, cx + 5, cy - 15);
        SDL_RenderLine(renderer, cx + 5, cy - 15, cx + 25, cy);
        SDL_RenderLine(renderer, cx - 15, cy + 20, cx + 5, cy + 15);
        SDL_RenderLine(renderer, cx + 5, cy + 15, cx + 25, cy);
    }
    else if (compName == "NOT Gate") {
        SDL_RenderLine(renderer, cx - 40, cy, cx - 15, cy);
        SDL_RenderLine(renderer, cx + 15, cy, cx + 40, cy);
        SDL_RenderLine(renderer, cx - 15, cy - 15, cx - 15, cy + 15);
        SDL_RenderLine(renderer, cx - 15, cy - 15, cx + 5, cy);
        SDL_RenderLine(renderer, cx - 15, cy + 15, cx + 5, cy);
        drawCircle(cx + 10, cy, 5);
    }
    else if (compName == "Flip-Flop") {
        SDL_FRect body{cx - 20, cy - 25, 40, 50};
        SDL_RenderRect(renderer, &body);
        SDL_RenderLine(renderer, cx - 40, cy - 10, cx - 20, cy - 10);
        SDL_RenderLine(renderer, cx - 40, cy + 10, cx - 20, cy + 10);
        SDL_RenderLine(renderer, cx + 20, cy - 10, cx + 40, cy - 10);
        SDL_RenderLine(renderer, cx + 20, cy + 10, cx + 40, cy + 10);
        SDL_RenderLine(renderer, cx - 20, cy + 5, cx - 12, cy + 10);
        SDL_RenderLine(renderer, cx - 12, cy + 10, cx - 20, cy + 15);
        drawCircle(cx + 24, cy + 10, 4);
    }
    else if (compName == "DC Source") {
        SDL_RenderLine(renderer, cx, cy - 30, cx, cy - 10);
        SDL_RenderLine(renderer, cx, cy + 10, cx, cy + 30);
        SDL_RenderLine(renderer, cx - 15, cy - 10, cx + 15, cy - 10);
        SDL_RenderLine(renderer, cx - 8, cy + 10, cx + 8, cy + 10);
        SDL_RenderLine(renderer, cx + 20, cy - 15, cx + 26, cy - 15);
        SDL_RenderLine(renderer, cx + 23, cy - 18, cx + 23, cy - 12);
    }
    else if (compName == "AC Source") {
        SDL_RenderLine(renderer, cx, cy - 30, cx, cy - 15);
        SDL_RenderLine(renderer, cx, cy + 15, cx, cy + 30);
        drawCircle(cx, cy, 15);
        for(float x = -8; x <= 8; x += 1.0f) {
            float y1 = std::sin(x * PI / 8.0f) * 5.0f;
            float y2 = std::sin((x+1) * PI / 8.0f) * 5.0f;
            SDL_RenderLine(renderer, cx + x, cy - y1, cx + x + 1, cy - y2);
        }
    }
    else if (compName == "Voltmeter" || compName == "Ammeter") {
        SDL_RenderLine(renderer, cx, cy - 30, cx, cy - 15);
        SDL_RenderLine(renderer, cx, cy + 15, cx, cy + 30);
        drawCircle(cx, cy, 15);
        if(compName == "Voltmeter") {
            SDL_RenderLine(renderer, cx - 5, cy - 5, cx, cy + 5);
            SDL_RenderLine(renderer, cx, cy + 5, cx + 5, cy - 5);
        } else {
            SDL_RenderLine(renderer, cx, cy - 6, cx - 5, cy + 5);
            SDL_RenderLine(renderer, cx, cy - 6, cx + 5, cy + 5);
            SDL_RenderLine(renderer, cx - 3, cy + 2, cx + 3, cy + 2);
        }
    }
    else if (compName == "Oscilloscope") {
        SDL_FRect outer{cx - 25, cy - 20, 50, 40};
        SDL_RenderRect(renderer, &outer);
        SDL_FRect screen{cx - 20, cy - 15, 30, 30};
        SDL_RenderRect(renderer, &screen);
        drawCircle(cx + 17, cy - 5, 3);
        drawCircle(cx + 17, cy + 5, 3);
        for(float x = -18; x <= 8; x += 1.0f) {
            float y1 = std::sin((x+18) * PI / 6.0f) * 8.0f;
            float y2 = std::sin((x+19) * PI / 6.0f) * 8.0f;
            SDL_RenderLine(renderer, cx + x, cy - y1, cx + x + 1, cy - y2);
        }
    }
    else if (compName == "Ground") {
        SDL_RenderLine(renderer, cx, cy - 20, cx, cy);
        SDL_RenderLine(renderer, cx - 15, cy, cx + 15, cy);
        SDL_RenderLine(renderer, cx - 10, cy + 6, cx + 10, cy + 6);
        SDL_RenderLine(renderer, cx - 5, cy + 12, cx + 5, cy + 12);
    }
    else {
        SDL_FRect body{cx - 25, cy - 15, 50, 30};
        SDL_RenderRect(renderer, &body);
        SDL_RenderLine(renderer, cx - 45, cy, cx - 25, cy);
        SDL_RenderLine(renderer, cx + 25, cy, cx + 45, cy);
    }
}