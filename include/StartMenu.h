//STARTMENU.H
#pragma once

#include "AppState.h"
#include "Button.h"
#include "PageSize.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <string>
#include <vector>

class StartMenu {
public:
    StartMenu();

    void handleEvent(const SDL_Event& event);
    void updateHoverState(float mouseX, float mouseY);
    void render(SDL_Renderer* renderer, TTF_Font* font) const;

    const PageSize& getSelectedPageSize() const;
    AppState getRequestedState() const;

    void resetRequestedState();

private:
    enum class MenuView {
        Main,
        PageSizeSelection,
        RecentProjects
    };

    void initializeButtons();

    void renderMainMenu(SDL_Renderer* renderer, TTF_Font* font) const;
    void renderPageSizeSelection(SDL_Renderer* renderer, TTF_Font* font) const;
    void renderRecentProjects(SDL_Renderer* renderer, TTF_Font* font) const;

    void handleMainMenuClick(float mouseX, float mouseY);
    void handlePageSizeClick(float mouseX, float mouseY);
    void handleRecentProjectsClick(float mouseX, float mouseY);

    std::vector<Button> mainButtons_;
    std::vector<Button> pageSizeButtons_;
    std::vector<Button> recentProjectButtons_;

    PageSize selectedPageSize_;
    std::vector<std::string> recentProjects_;

    MenuView currentView_;
    AppState requestedState_;
};