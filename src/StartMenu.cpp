//STARTMENU.CPP
#include "StartMenu.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <sstream>
#include <string>

namespace {
    constexpr int WindowWidth = 800;
    constexpr int ButtonWidth = 260;
    constexpr int ButtonHeight = 48;
    constexpr int ButtonSpacing = 14;

    constexpr SDL_Color BackgroundColor{24, 28, 36, 255};
    constexpr SDL_Color TitleColor{235, 240, 255, 255};
    constexpr SDL_Color TextColor{225, 230, 240, 255};
    constexpr SDL_Color MutedTextColor{165, 172, 186, 255};
    constexpr SDL_Color ButtonNormalColor{58, 70, 92, 255};
    constexpr SDL_Color ButtonHoverColor{82, 101, 136, 255};
    constexpr SDL_Color ButtonTextColor{255, 255, 255, 255};

    void renderText(SDL_Renderer* renderer,
                    TTF_Font* font,
                    const std::string& text,
                    float x,
                    float y,
                    SDL_Color color,
                    bool centered = false) {
        if (!renderer || !font || text.empty()) {
            return;
        }

        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
        if (!surface) {
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (!texture) {
            SDL_DestroySurface(surface);
            return;
        }

        SDL_FRect destination{
                centered ? x - static_cast<float>(surface->w) / 2.0f : x,
                y,
                static_cast<float>(surface->w),
                static_cast<float>(surface->h)
        };

        SDL_RenderTexture(renderer, texture, nullptr, &destination);

        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);
    }

    std::string pageSizeToDisplayText(const PageSize& pageSize) {
        std::ostringstream stream;
        stream << "Selected page size: " << pageSizeTypeToString(pageSize.type)
               << " (" << pageSize.width << " x " << pageSize.height << " mm)";
        return stream.str();
    }
}

const char* pageSizeTypeToString(PageSizeType type) {
    switch (type) {
        case PageSizeType::A4:
            return "A4";
        case PageSizeType::A3:
            return "A3";
        case PageSizeType::Custom:
            return "Custom";
        default:
            return "Unknown";
    }
}

StartMenu::StartMenu()
        : selectedPageSize_{210.0, 297.0, PageSizeType::A4},
          recentProjects_{"Amplifier Board", "Power Supply Module", "LED Matrix Controller"},
          currentView_(MenuView::Main),
          requestedState_(AppState::MainMenu) {
    initializeButtons();
}

void StartMenu::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_MOUSE_MOTION) {
        updateHoverState(event.motion.x, event.motion.y);
        return;
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        const float mouseX = event.button.x;
        const float mouseY = event.button.y;

        switch (currentView_) {
            case MenuView::Main:
                handleMainMenuClick(mouseX, mouseY);
                break;
            case MenuView::PageSizeSelection:
                handlePageSizeClick(mouseX, mouseY);
                break;
            case MenuView::RecentProjects:
                handleRecentProjectsClick(mouseX, mouseY);
                break;
        }

        updateHoverState(mouseX, mouseY);
    }
}

void StartMenu::updateHoverState(float mouseX, float mouseY) {
    auto updateButtons = [mouseX, mouseY](std::vector<Button>& buttons) {
        for (Button& button : buttons) {
            button.setHovered(button.contains(mouseX, mouseY));
        }
    };

    updateButtons(mainButtons_);
    updateButtons(pageSizeButtons_);
    updateButtons(recentProjectButtons_);
}

void StartMenu::render(SDL_Renderer* renderer, TTF_Font* font) const {
    if (!renderer) {
        return;
    }

    SDL_SetRenderDrawColor(renderer,
                           BackgroundColor.r,
                           BackgroundColor.g,
                           BackgroundColor.b,
                           BackgroundColor.a);
    SDL_RenderClear(renderer);

    switch (currentView_) {
        case MenuView::Main:
            renderMainMenu(renderer, font);
            break;
        case MenuView::PageSizeSelection:
            renderPageSizeSelection(renderer, font);
            break;
        case MenuView::RecentProjects:
            renderRecentProjects(renderer, font);
            break;
    }
}

const PageSize& StartMenu::getSelectedPageSize() const {
    return selectedPageSize_;
}

AppState StartMenu::getRequestedState() const {
    return requestedState_;
}

void StartMenu::resetRequestedState() {
    requestedState_ = AppState::MainMenu;
}

void StartMenu::initializeButtons() {
    const float x = static_cast<float>((WindowWidth - ButtonWidth) / 2);
    float y = 210.0f;

    mainButtons_.clear();
    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight},
                              "New Project",
                              ButtonNormalColor,
                              ButtonHoverColor,
                              ButtonTextColor);
    y += ButtonHeight + ButtonSpacing;

    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight},
                              "Open Project",
                              ButtonNormalColor,
                              ButtonHoverColor,
                              ButtonTextColor);
    y += ButtonHeight + ButtonSpacing;

    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight},
                              "Select Page Size",
                              ButtonNormalColor,
                              ButtonHoverColor,
                              ButtonTextColor);
    y += ButtonHeight + ButtonSpacing;

    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight},
                              "Recent Projects",
                              ButtonNormalColor,
                              ButtonHoverColor,
                              ButtonTextColor);
    y += ButtonHeight + ButtonSpacing;

    mainButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight},
                              "Exit",
                              ButtonNormalColor,
                              ButtonHoverColor,
                              ButtonTextColor);

    y = 240.0f;
    pageSizeButtons_.clear();
    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight},
                                  "A4",
                                  ButtonNormalColor,
                                  ButtonHoverColor,
                                  ButtonTextColor);
    y += ButtonHeight + ButtonSpacing;

    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight},
                                  "A3",
                                  ButtonNormalColor,
                                  ButtonHoverColor,
                                  ButtonTextColor);
    y += ButtonHeight + ButtonSpacing;

    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight},
                                  "Custom",
                                  ButtonNormalColor,
                                  ButtonHoverColor,
                                  ButtonTextColor);
    y += ButtonHeight + ButtonSpacing;

    pageSizeButtons_.emplace_back(SDL_FRect{x, y, ButtonWidth, ButtonHeight},
                                  "Back",
                                  ButtonNormalColor,
                                  ButtonHoverColor,
                                  ButtonTextColor);

    recentProjectButtons_.clear();
    recentProjectButtons_.emplace_back(SDL_FRect{x, 470.0f, ButtonWidth, ButtonHeight},
                                       "Back",
                                       ButtonNormalColor,
                                       ButtonHoverColor,
                                       ButtonTextColor);
}

void StartMenu::renderMainMenu(SDL_Renderer* renderer, TTF_Font* font) const {
    renderText(renderer,
               font,
               "Circuit Design Application",
               WindowWidth / 2.0f,
               80.0f,
               TitleColor,
               true);

    renderText(renderer,
               font,
               pageSizeToDisplayText(selectedPageSize_),
               WindowWidth / 2.0f,
               145.0f,
               MutedTextColor,
               true);

    for (const Button& button : mainButtons_) {
        button.render(renderer, font);
    }
}

void StartMenu::renderPageSizeSelection(SDL_Renderer* renderer, TTF_Font* font) const {
    renderText(renderer,
               font,
               "Select Page Size",
               WindowWidth / 2.0f,
               90.0f,
               TitleColor,
               true);

    renderText(renderer,
               font,
               pageSizeToDisplayText(selectedPageSize_),
               WindowWidth / 2.0f,
               155.0f,
               MutedTextColor,
               true);

    for (const Button& button : pageSizeButtons_) {
        button.render(renderer, font);
    }
}

void StartMenu::renderRecentProjects(SDL_Renderer* renderer, TTF_Font* font) const {
    renderText(renderer,
               font,
               "Recent Projects",
               WindowWidth / 2.0f,
               90.0f,
               TitleColor,
               true);

    float y = 175.0f;

    if (recentProjects_.empty()) {
        renderText(renderer,
                   font,
                   "No recent projects found.",
                   WindowWidth / 2.0f,
                   y,
                   MutedTextColor,
                   true);
    } else {
        for (std::size_t index = 0; index < recentProjects_.size(); ++index) {
            renderText(renderer,
                       font,
                       std::to_string(index + 1) + ". " + recentProjects_[index],
                       260.0f,
                       y,
                       TextColor);
            y += 42.0f;
        }
    }

    for (const Button& button : recentProjectButtons_) {
        button.render(renderer, font);
    }
}

void StartMenu::handleMainMenuClick(float mouseX, float mouseY) {
    for (const Button& button : mainButtons_) {
        if (!button.contains(mouseX, mouseY)) {
            continue;
        }

        const std::string& label = button.getLabel();

        if (label == "New Project") {
            requestedState_ = AppState::NewProject;
        } else if (label == "Open Project") {
            requestedState_ = AppState::OpenProject;
        } else if (label == "Select Page Size") {
            currentView_ = MenuView::PageSizeSelection;
        } else if (label == "Recent Projects") {
            currentView_ = MenuView::RecentProjects;
        } else if (label == "Exit") {
            requestedState_ = AppState::Exit;
        }

        return;
    }
}

void StartMenu::handlePageSizeClick(float mouseX, float mouseY) {
    for (const Button& button : pageSizeButtons_) {
        if (!button.contains(mouseX, mouseY)) {
            continue;
        }

        const std::string& label = button.getLabel();

        if (label == "A4") {
            selectedPageSize_ = PageSize{210.0, 297.0, PageSizeType::A4};
        } else if (label == "A3") {
            selectedPageSize_ = PageSize{297.0, 420.0, PageSizeType::A3};
        } else if (label == "Custom") {
            selectedPageSize_ = PageSize{500.0, 350.0, PageSizeType::Custom};
        } else if (label == "Back") {
            currentView_ = MenuView::Main;
        }

        return;
    }
}

void StartMenu::handleRecentProjectsClick(float mouseX, float mouseY) {
    for (const Button& button : recentProjectButtons_) {
        if (button.contains(mouseX, mouseY) && button.getLabel() == "Back") {
            currentView_ = MenuView::Main;
            return;
        }
    }
}