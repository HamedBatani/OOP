// src/StartMenu.cpp
#include "StartMenu.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <filesystem>
#include <system_error>

namespace {
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

    void drawFaintGrid(SDL_Renderer* renderer, int width, int height) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 8); // 3% opacity
        for(int x = 0; x < width; x += 30) SDL_RenderLine(renderer, x, 0, x, height);
        for(int y = 0; y < height; y += 30) SDL_RenderLine(renderer, 0, y, width, y);
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

StartMenu::StartMenu(SDL_Window* window) : selectedPageSize_{210.0, 297.0, PageSizeType::A4}, currentView_(MenuView::Main), requestedState_(AppState::MainMenu), shouldLoadProject_(false), window_(window) {
    const std::filesystem::path workingDirectory = std::filesystem::current_path();
    const std::filesystem::path searchRoots[] = {workingDirectory, workingDirectory.parent_path()};
    std::error_code error;
    for (const auto& root : searchRoots) {
        if (!std::filesystem::is_directory(root, error)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(root, error)) {
            if (error) break;
            const std::string extension = entry.path().extension().string();
            if (!entry.is_regular_file(error) || (extension != ".json" && extension != ".circuit")) continue;
            const std::filesystem::path relative = entry.path().lexically_relative(workingDirectory);
            const std::string path = relative.empty() ? entry.path().string() : relative.string();
            if (std::find(recentProjects_.begin(), recentProjects_.end(), path) == recentProjects_.end()) recentProjects_.push_back(path);
        }
    }
    std::sort(recentProjects_.begin(), recentProjects_.end());
    initializeButtons();
}

void SDLCALL StartMenu::openDialogCallback(void* userdata, const char* const* filelist, int) {
    auto* menu = static_cast<StartMenu*>(userdata);
    if (!menu || !filelist || !filelist[0]) return;
    std::lock_guard<std::mutex> lock(menu->dialogMutex_);
    menu->pendingOpenPath_ = filelist[0];
}

void StartMenu::showNativeOpenDialog() {
    static const SDL_DialogFileFilter filters[] = {
        {"Circuit projects", "json;circuit;txt"},
        {"All files", "*"}
    };
    SDL_ShowOpenFileDialog(&StartMenu::openDialogCallback, this, window_, filters, 2, nullptr, false);
}

void StartMenu::setViewportSize(int width, int height) {
    width = std::max(width, 640);
    height = std::max(height, 520);
    if (width == viewportWidth_ && height == viewportHeight_) return;
    viewportWidth_ = width;
    viewportHeight_ = height;
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

    drawFaintGrid(renderer, viewportWidth_, viewportHeight_);

    // لوگو و عنوان بزرگ بالای همه منوها
    const float centerX = viewportWidth_ / 2.0f;
    const float topOffset = std::max(0.0f, (viewportHeight_ - 600.0f) * 0.12f);
    drawLightningLogo(renderer, centerX, 60.0f + topOffset);
    renderText(renderer, font, "Circuit Design Application", centerX, 95.0f + topOffset, TitleColor, true, 1.4f);
    renderText(renderer, font, "Professional Circuit Design Environment", centerX, 138.0f + topOffset, SubtitleColor, true, 0.85f);

    switch (currentView_) {
        case MenuView::Main: renderMainMenu(renderer, font); break;
        case MenuView::PageSizeSelection: renderPageSizeSelection(renderer, font); break;
        case MenuView::RecentProjects: renderRecentProjects(renderer, font); break;
        case MenuView::OpenProject: renderOpenProject(renderer, font); break;
    }

    // فوتر شیک و حرفه‌ای
    renderText(renderer, font, "Version 1.0.0", 25.0f, viewportHeight_ - 35.0f, SubtitleColor, false, 0.75f);
    renderText(renderer, font, "Open Source Circuit Simulator (c) 2026", viewportWidth_ - 330.0f, viewportHeight_ - 35.0f, SubtitleColor, false, 0.75f);
}

const PageSize& StartMenu::getSelectedPageSize() const { return selectedPageSize_; }
AppState StartMenu::getRequestedState() {
    std::lock_guard<std::mutex> lock(dialogMutex_);
    if (!pendingOpenPath_.empty()) {
        selectedProjectFile_ = pendingOpenPath_;
        pendingOpenPath_.clear();
        shouldLoadProject_ = true;
        requestedState_ = AppState::NewProject;
    }
    return requestedState_;
}
void StartMenu::resetRequestedState() { requestedState_ = AppState::MainMenu; }

void StartMenu::initializeButtons() {
    const float x = static_cast<float>((viewportWidth_ - ButtonWidth) / 2);
    const float topOffset = std::max(0.0f, (viewportHeight_ - 600.0f) * 0.12f);
    float y = 190.0f + topOffset;

    // اختصاص رنگ‌های متمایز و آیکون به دکمه‌های منوی اصلی
    mainButtons_.clear();
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "New Project", BtnBlueNormal, BtnBlueHover, ButtonTextColor, IconType::NewFile); y += ButtonHeight + ButtonSpacing;
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Open Project", BtnNormal, BtnHover, ButtonTextColor, IconType::Folder); y += ButtonHeight + ButtonSpacing;
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Select Page Size", BtnNormal, BtnHover, ButtonTextColor, IconType::Ruler); y += ButtonHeight + ButtonSpacing;
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Recent Projects", BtnNormal, BtnHover, ButtonTextColor, IconType::Clock); y += ButtonHeight + ButtonSpacing;
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Exit", BtnRedNormal, BtnRedHover, ButtonTextColor, IconType::ExitIcon);

    y = 260.0f + topOffset;
    pageSizeButtons_.clear();
    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "A4", BtnNormal, BtnHover, ButtonTextColor); y += ButtonHeight + ButtonSpacing;
    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "A3", BtnNormal, BtnHover, ButtonTextColor); y += ButtonHeight + ButtonSpacing;
    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Custom", BtnNormal, BtnHover, ButtonTextColor); y += ButtonHeight + ButtonSpacing;
    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight}, "Back", BtnRedNormal, BtnRedHover, ButtonTextColor, IconType::Menu);

    recentProjectButtons_.clear(); float rpY = 200.0f + topOffset;
    for (const auto& proj : recentProjects_) {
        recentProjectButtons_.emplace_back(SDL_FRect{x, rpY, ButtonWidth, ButtonHeight}, proj, BtnNormal, BtnHover, ButtonTextColor, IconType::NewFile); rpY += ButtonHeight + ButtonSpacing;
    }
    recentProjectButtons_.emplace_back(SDL_FRect{x, viewportHeight_ - 130.0f, ButtonWidth, ButtonHeight}, "Back", BtnRedNormal, BtnRedHover, ButtonTextColor, IconType::Menu);

    openProjectButtons_.clear(); float opY = 200.0f + topOffset;
    for (const auto& proj : recentProjects_) {
        openProjectButtons_.emplace_back(SDL_FRect{x, opY, ButtonWidth, ButtonHeight}, proj, BtnNormal, BtnHover, ButtonTextColor, IconType::Folder); opY += ButtonHeight + ButtonSpacing;
    }
    openProjectButtons_.emplace_back(SDL_FRect{x, viewportHeight_ - 130.0f, ButtonWidth, ButtonHeight}, "Back", BtnRedNormal, BtnRedHover, ButtonTextColor, IconType::Menu);
}

void StartMenu::renderMainMenu(SDL_Renderer* renderer, TTF_Font* font) const {
    for (const auto& button : mainButtons_) button.render(renderer, font);
}

void StartMenu::renderPageSizeSelection(SDL_Renderer* renderer, TTF_Font* font) const {
    // رسم "کارت" شیک برای نمایش سایز صفحه بجای متن ساده
    const float centerX = viewportWidth_ / 2.0f;
    const float topOffset = std::max(0.0f, (viewportHeight_ - 600.0f) * 0.12f);
    SDL_FRect cardRect{centerX - 140.0f, 175.0f + topOffset, 280.0f, 65.0f};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 45, 55, 75, 150); SDL_RenderFillRect(renderer, &cardRect);
    SDL_SetRenderDrawColor(renderer, BtnBlueHover.r, BtnBlueHover.g, BtnBlueHover.b, 255); SDL_RenderRect(renderer, &cardRect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    renderText(renderer, font, "Active Canvas Size", centerX, 185.0f + topOffset, SubtitleColor, true, 0.7f);
    renderText(renderer, font, pageSizeToDisplayText(selectedPageSize_), centerX, 205.0f + topOffset, TitleColor, true, 0.9f);

    for (const auto& button : pageSizeButtons_) button.render(renderer, font);
}

void StartMenu::renderRecentProjects(SDL_Renderer* renderer, TTF_Font* font) const {
    if (recentProjects_.empty()) renderText(renderer, font, "No recent projects found.", viewportWidth_ / 2.0f, 200.0f, SubtitleColor, true);
    for (const auto& button : recentProjectButtons_) button.render(renderer, font);
}

void StartMenu::renderOpenProject(SDL_Renderer* renderer, TTF_Font* font) const {
    renderText(renderer, font, "Select a file to load:", viewportWidth_ / 2.0f, 170.0f, SubtitleColor, true, 0.85f);
    for (const auto& button : openProjectButtons_) button.render(renderer, font);
}

void StartMenu::handleMainMenuClick(float mouseX, float mouseY) {
    for (const auto& button : mainButtons_) {
        if (!button.contains(mouseX, mouseY)) continue;
        const std::string& label = button.getLabel();
        if (label == "New Project") requestedState_ = AppState::NewProject;
        else if (label == "Open Project") showNativeOpenDialog();
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
