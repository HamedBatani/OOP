// src/StartMenu.cpp
#include "StartMenu.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>

namespace {
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;
    constexpr int ButtonWidth = 280;
    constexpr int ButtonHeight = 46;
    constexpr int ButtonSpacing = 16;

    // تم رنگی فوق مدرن و تاریک
    constexpr SDL_Color BackgroundColor{18, 20, 26, 255};
    constexpr SDL_Color TitleColor{240, 245, 255, 255};
    constexpr SDL_Color SubtitleColor{140, 150, 170, 255};
    constexpr SDL_Color TextColor{220, 225, 235, 255};

    // رنگ دکمه‌های دیفالت (همونی که گفتی)
    constexpr SDL_Color BtnNormal{70, 82, 109, 255};   // #46526D
    constexpr SDL_Color BtnHover{88, 105, 141, 255};   // #58698D

    // دکمه‌های خاص
    constexpr SDL_Color BtnBlueNormal{43, 108, 196, 255}; // #2B6CC4
    constexpr SDL_Color BtnBlueHover{58, 130, 228, 255};
    constexpr SDL_Color BtnRedNormal{179, 74, 74, 255};   // #B34A4A
    constexpr SDL_Color BtnRedHover{209, 94, 94, 255};

    constexpr SDL_Color ButtonTextColor{255, 255, 255, 255};

    void renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, float x, float y, SDL_Color color, bool centered = false, float scale = 1.0f) {
        if (!renderer || !font || text.empty()) return;
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
        if (!surface) return;
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_FRect dest{ centered ? x - (surface->w * scale) / 2.0f : x, y, static_cast<float>(surface->w) * scale, static_cast<float>(surface->h) * scale };
            SDL_RenderTexture(renderer, texture, nullptr, &dest);
            SDL_DestroyTexture(texture);
        }
        SDL_DestroySurface(surface);
    }

    std::string pageSizeToDisplayText(const PageSize& pageSize) {
        std::ostringstream stream; stream << pageSizeTypeToString(pageSize.type) << " (" << pageSize.width << " x " << pageSize.height << " mm)";
        return stream.str();
    }

    void drawFaintGrid(SDL_Renderer* renderer) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 8); // 3% opacity
        for(int x = 0; x < WindowWidth; x += 30) SDL_RenderLine(renderer, x, 0, x, WindowHeight);
        for(int y = 0; y < WindowHeight; y += 30) SDL_RenderLine(renderer, 0, y, WindowWidth, y);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

    void drawLightningLogo(SDL_Renderer* renderer, float cx, float cy) {
        SDL_FColor c = {1.0f, 0.65f, 0.1f, 1.0f}; // نارنجی برق‌آسا
        SDL_Vertex v[6] = {
                {{cx + 4, cy - 22}, c, {0,0}}, {{cx - 12, cy + 2}, c, {0,0}}, {{cx + 2, cy + 2}, c, {0,0}},
                {{cx + 2, cy + 2}, c, {0,0}}, {{cx - 6, cy + 22}, c, {0,0}}, {{cx + 12, cy - 4}, c, {0,0}}
        };
        SDL_RenderGeometry(renderer, nullptr, v, 6, nullptr, 0);
    }
}

const char* pageSizeTypeToString(PageSizeType type) {
    switch (type) { case PageSizeType::A4: return "A4"; case PageSizeType::A3: return "A3"; case PageSizeType::Custom: return "Custom"; default: return "Unknown"; }
}

StartMenu::StartMenu() : selectedPageSize_{210.0, 297.0, PageSizeType::A4}, recentProjects_{"circuit.txt"}, currentView_(MenuView::Main), requestedState_(AppState::MainMenu), shouldLoadProject_(false) {
    initializeButtons();
}

bool StartMenu::shouldLoadProject() const { return shouldLoadProject_; }
void StartMenu::resetLoadProject() { shouldLoadProject_ = false; }
std::string StartMenu::getSelectedProjectFile() const { return selectedProjectFile_; }

void StartMenu::addSavedProject(const std::string& filename) {
    if (std::find(recentProjects_.begin(), recentProjects_.end(), filename) == recentProjects_.end()) {
        recentProjects_.insert(recentProjects_.begin(), filename);
        initializeButtons();
    }
}

void StartMenu::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_MOUSE_MOTION) { updateHoverState(event.motion.x, event.motion.y); return; }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        float mx = event.button.x, my = event.button.y;
        switch (currentView_) {
            case MenuView::Main: handleMainMenuClick(mx, my); break;
            case MenuView::PageSizeSelection: handlePageSizeClick(mx, my); break;
            case MenuView::RecentProjects: handleRecentProjectsClick(mx, my); break;
            case MenuView::OpenProject: handleOpenProjectClick(mx, my); break;
        }
        updateHoverState(mx, my);
    }
}

void StartMenu::updateHoverState(float mouseX, float mouseY) {
    auto updateBtns = [mouseX, mouseY](std::vector<Button>& buttons) { for (auto& btn : buttons) btn.setHovered(btn.contains(mouseX, mouseY)); };
    updateBtns(mainButtons_); updateBtns(pageSizeButtons_); updateBtns(recentProjectButtons_); updateBtns(openProjectButtons_);
}

void StartMenu::render(SDL_Renderer* renderer, TTF_Font* font) const {
    if (!renderer) return;
    SDL_SetRenderDrawColor(renderer, BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
    SDL_RenderClear(renderer);

    drawFaintGrid(renderer); // اضافه شدن Pattern محو مهندسی

    // لوگو و عنوان بزرگ بالای همه منوها
    drawLightningLogo(renderer, WindowWidth / 2.0f, 60.0f);
    renderText(renderer, font, "Circuit Design Application", WindowWidth / 2.0f, 95.0f, TitleColor, true, 1.4f);
    renderText(renderer, font, "Professional Circuit Design Environment", WindowWidth / 2.0f, 138.0f, SubtitleColor, true, 0.85f);

    switch (currentView_) {
        case MenuView::Main: renderMainMenu(renderer, font); break;
        case MenuView::PageSizeSelection: renderPageSizeSelection(renderer, font); break;
        case MenuView::RecentProjects: renderRecentProjects(renderer, font); break;
        case MenuView::OpenProject: renderOpenProject(renderer, font); break;
    }

    // فوتر شیک و حرفه‌ای
    renderText(renderer, font, "Version 1.0.0", 25.0f, WindowHeight - 35.0f, SubtitleColor, false, 0.75f);
    renderText(renderer, font, "Open Source Circuit Simulator (c) 2026", WindowWidth - 280.0f, WindowHeight - 35.0f, SubtitleColor, false, 0.75f);
}

const PageSize& StartMenu::getSelectedPageSize() const { return selectedPageSize_; }
AppState StartMenu::getRequestedState() const { return requestedState_; }
void StartMenu::resetRequestedState() { requestedState_ = AppState::MainMenu; }

void StartMenu::initializeButtons() {
    const float x = static_cast<float>((WindowWidth - ButtonWidth) / 2);
    float y = 190.0f;

    // اختصاص رنگ‌های متمایز و آیکون به دکمه‌های منوی اصلی
    mainButtons_.clear();
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "New Project", BtnBlueNormal, BtnBlueHover, ButtonTextColor, IconType::NewFile); y += ButtonHeight + ButtonSpacing;
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Open Project", BtnNormal, BtnHover, ButtonTextColor, IconType::Folder); y += ButtonHeight + ButtonSpacing;
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Select Page Size", BtnNormal, BtnHover, ButtonTextColor, IconType::Ruler); y += ButtonHeight + ButtonSpacing;
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Recent Projects", BtnNormal, BtnHover, ButtonTextColor, IconType::Clock); y += ButtonHeight + ButtonSpacing;
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Exit", BtnRedNormal, BtnRedHover, ButtonTextColor, IconType::ExitIcon);

    y = 260.0f;
    pageSizeButtons_.clear();
    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "A4", BtnNormal, BtnHover, ButtonTextColor); y += ButtonHeight + ButtonSpacing;
    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "A3", BtnNormal, BtnHover, ButtonTextColor); y += ButtonHeight + ButtonSpacing;
    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Custom", BtnNormal, BtnHover, ButtonTextColor); y += ButtonHeight + ButtonSpacing;
    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Back", BtnRedNormal, BtnRedHover, ButtonTextColor, IconType::Menu);

    recentProjectButtons_.clear(); float rpY = 200.0f;
    for (const auto& proj : recentProjects_) {
        recentProjectButtons_.emplace_back(SDL_FRect{x, rpY, ButtonWidth, ButtonHeight}, proj, BtnNormal, BtnHover, ButtonTextColor, IconType::NewFile); rpY += ButtonHeight + ButtonSpacing;
    }
    recentProjectButtons_.emplace_back(SDL_FRect{x, 470.0f, ButtonWidth, ButtonHeight}, "Back", BtnRedNormal, BtnRedHover, ButtonTextColor, IconType::Menu);

    openProjectButtons_.clear(); float opY = 200.0f;
    for (const auto& proj : recentProjects_) {
        openProjectButtons_.emplace_back(SDL_FRect{x, opY, ButtonWidth, ButtonHeight}, proj, BtnNormal, BtnHover, ButtonTextColor, IconType::Folder); opY += ButtonHeight + ButtonSpacing;
    }
    openProjectButtons_.emplace_back(SDL_FRect{x, 470.0f, ButtonWidth, ButtonHeight}, "Back", BtnRedNormal, BtnRedHover, ButtonTextColor, IconType::Menu);
}

void StartMenu::renderMainMenu(SDL_Renderer* renderer, TTF_Font* font) const {
    for (const auto& button : mainButtons_) button.render(renderer, font);
}

void StartMenu::renderPageSizeSelection(SDL_Renderer* renderer, TTF_Font* font) const {
    // رسم "کارت" شیک برای نمایش سایز صفحه بجای متن ساده
    SDL_FRect cardRect{WindowWidth/2.0f - 140.0f, 175.0f, 280.0f, 65.0f};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 45, 55, 75, 150); SDL_RenderFillRect(renderer, &cardRect);
    SDL_SetRenderDrawColor(renderer, BtnBlueHover.r, BtnBlueHover.g, BtnBlueHover.b, 255); SDL_RenderRect(renderer, &cardRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    renderText(renderer, font, "Active Canvas Size", WindowWidth / 2.0f, 185.0f, SubtitleColor, true, 0.7f);
    renderText(renderer, font, pageSizeToDisplayText(selectedPageSize_), WindowWidth / 2.0f, 205.0f, TitleColor, true, 0.9f);

    for (const auto& button : pageSizeButtons_) button.render(renderer, font);
}

void StartMenu::renderRecentProjects(SDL_Renderer* renderer, TTF_Font* font) const {
    if (recentProjects_.empty()) renderText(renderer, font, "No recent projects found.", WindowWidth / 2.0f, 200.0f, SubtitleColor, true);
    for (const auto& button : recentProjectButtons_) button.render(renderer, font);
}

void StartMenu::renderOpenProject(SDL_Renderer* renderer, TTF_Font* font) const {
    renderText(renderer, font, "Select a file to load:", WindowWidth / 2.0f, 170.0f, SubtitleColor, true, 0.85f);
    for (const auto& button : openProjectButtons_) button.render(renderer, font);
}

void StartMenu::handleMainMenuClick(float mouseX, float mouseY) {
    for (const auto& button : mainButtons_) {
        if (!button.contains(mouseX, mouseY)) continue;
        const std::string& label = button.getLabel();
        if (label == "New Project") requestedState_ = AppState::NewProject;
        else if (label == "Open Project") currentView_ = MenuView::OpenProject;
        else if (label == "Select Page Size") currentView_ = MenuView::PageSizeSelection;
        else if (label == "Recent Projects") currentView_ = MenuView::RecentProjects;
        else if (label == "Exit") requestedState_ = AppState::Exit;
        return;
    }
}

void StartMenu::handlePageSizeClick(float mouseX, float mouseY) {
    for (const auto& button : pageSizeButtons_) {
        if (!button.contains(mouseX, mouseY)) continue;
        const std::string& label = button.getLabel();
        if (label == "A4") selectedPageSize_ = PageSize{210.0, 297.0, PageSizeType::A4};
        else if (label == "A3") selectedPageSize_ = PageSize{297.0, 420.0, PageSizeType::A3};
        else if (label == "Custom") selectedPageSize_ = PageSize{500.0, 350.0, PageSizeType::Custom};
        else if (label == "Back") currentView_ = MenuView::Main;
        return;
    }
}

void StartMenu::handleRecentProjectsClick(float mouseX, float mouseY) {
    for (const auto& button : recentProjectButtons_) {
        if (!button.contains(mouseX, mouseY)) continue;
        if (button.getLabel() == "Back") currentView_ = MenuView::Main;
        else { selectedProjectFile_ = button.getLabel(); shouldLoadProject_ = true; requestedState_ = AppState::NewProject; }
        return;
    }
}

void StartMenu::handleOpenProjectClick(float mouseX, float mouseY) {
    for (const auto& button : openProjectButtons_) {
        if (!button.contains(mouseX, mouseY)) continue;
        if (button.getLabel() == "Back") currentView_ = MenuView::Main;
        else { selectedProjectFile_ = button.getLabel(); shouldLoadProject_ = true; requestedState_ = AppState::NewProject; }
        return;
    }
}