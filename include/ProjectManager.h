// include/ProjectManager.h
#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <SDL3/SDL.h>

class ProjectManager {
public:
    static bool saveProject(const std::string& filename, const std::vector<std::string>& activeComponents) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            // نمایش پاپ‌آپ در صورت بروز خطا
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to save project!", nullptr);
            return false;
        }

        // نوشتن نام قطعات فعال داخل فایل متنی
        for (const auto& comp : activeComponents) {
            file << comp << '\n';
        }
        file.close();

        // نمایش پاپ‌آپ موفقیت‌آمیز بودن عملیات
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", "Project saved successfully!", nullptr);
        return true;
    }

    static bool loadProject(const std::string& filename, std::vector<std::string>& activeComponents) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to load project! File not found.", nullptr);
            return false;
        }

        activeComponents.clear();
        std::string line;

        // خواندن قطعات از فایل و اضافه کردن به لیست
        while (std::getline(file, line)) {
            if (!line.empty()) {
                activeComponents.push_back(line);
            }
        }
        file.close();

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", "Project loaded successfully!", nullptr);
        return true;
    }
};