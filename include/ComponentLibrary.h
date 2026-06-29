// include/ComponentLibrary.h
#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <vector>
#include <string>

class ComponentLibrary {
public:
    ComponentLibrary(float x, float y, float width, float height);

    void handleEvent(const SDL_Event& event, std::string& selectedComponent);
    void render(SDL_Renderer* renderer, TTF_Font* font, const std::string& selectedComponent) const;

    // دو تابع جدید برای ارسال و دریافت لیست قطعات جهت ذخیره/بازیابی
    const std::vector<std::string>& getActiveList() const { return activeList_; }
    void setActiveList(const std::vector<std::string>& list) { activeList_ = list; }

private:
    struct Category {
        std::string name;
        bool isExpanded;
        std::vector<std::string> items;
    };

    float x_, y_, width_, height_;
    std::vector<Category> categories_;

    std::vector<std::string> activeList_;

    mutable float hoverX_{0};
    mutable float hoverY_{0};

    std::string searchQuery_{""};
    bool isSearchFocused_{false};

    mutable float scrollY_{0.0f};
    mutable float maxScrollY_{0.0f};

    static bool containsIgnoreCase(const std::string& text, const std::string& query);
    void renderPreviewBox(SDL_Renderer* renderer, TTF_Font* font, const std::string& compName) const;
};