// include/ProjectManager.h
#pragma once
#include <string>
#include <fstream>
#include <iostream>

class ProjectManager {
public:
    static bool saveProject(const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) return false;

        // اینجا در آینده مختصات قطعات اضافه شده به مدار نوشته می‌شود
        file << "SAMPLE_CIRCUIT_DATA\n";
        file.close();
        return true;
    }

    static bool loadProject(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        std::string line;
        std::getline(file, line); // خواندن دیتا
        file.close();
        return true;
    }
};