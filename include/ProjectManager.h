// include/ProjectManager.h
#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <SDL3/SDL.h>
#include "ComponentInstance.h"
#include "Wire.h"

class ProjectManager {
public:
    static bool saveProject(const std::string& filename, const std::vector<std::string>& activeComponents, const std::vector<ComponentInstance>& placedComponents, const std::vector<Wire>& wires) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to save project!", nullptr);
            return false;
        }

        file << "[ACTIVE_LIST]\n";
        for (const auto& comp : activeComponents) {
            file << comp << '\n';
        }

        file << "[COMPONENTS]\n";
        for (const auto& comp : placedComponents) {
            file << comp.type << '|'
                 << comp.labelId << '|'
                 << comp.valueStr << '|'
                 << comp.worldPos.x << '|'
                 << comp.worldPos.y << '|'
                 << comp.rotationDegrees << '|'
                 << comp.isMirroredH << '|'
                 << comp.isMirroredV << '\n';
        }

        // ذخیره سیم‌های تکمیل شده در فایل
        file << "[WIRES]\n";
        for (const auto& wire : wires) {
            if (wire.isCompleted) {
                file << wire.startCompId << '|'
                     << wire.startPinName << '|'
                     << wire.endCompId << '|'
                     << wire.endPinName << '\n';
            }
        }

        file.close();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", "Project saved successfully!", nullptr);
        return true;
    }

    static bool loadProject(const std::string& filename, std::vector<std::string>& activeComponents, std::vector<ComponentInstance>& placedComponents, std::vector<Wire>& wires) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Failed to load project! File not found.", nullptr);
            return false;
        }

        activeComponents.clear();
        placedComponents.clear();
        wires.clear();

        std::string line;
        enum ParseMode { NONE, ACTIVE_LIST, COMPONENTS, WIRES } mode = NONE;

        while (std::getline(file, line)) {
            if (line.empty()) continue;
            if (line == "[ACTIVE_LIST]") { mode = ACTIVE_LIST; continue; }
            if (line == "[COMPONENTS]") { mode = COMPONENTS; continue; }
            if (line == "[WIRES]") { mode = WIRES; continue; }

            if (mode == ACTIVE_LIST) {
                activeComponents.push_back(line);
            } else if (mode == COMPONENTS) {
                std::stringstream ss(line);
                std::string type, labelId, valueStr, wx, wy, rot, mh, mv;

                std::getline(ss, type, '|');
                std::getline(ss, labelId, '|');
                std::getline(ss, valueStr, '|');
                std::getline(ss, wx, '|');
                std::getline(ss, wy, '|');
                std::getline(ss, rot, '|');
                std::getline(ss, mh, '|');
                std::getline(ss, mv, '|');

                Point pos{ std::stof(wx), std::stof(wy) };
                ComponentInstance comp(type, labelId, valueStr, pos);
                comp.rotationDegrees = std::stoi(rot);
                comp.isMirroredH = std::stoi(mh) != 0;
                comp.isMirroredV = std::stoi(mv) != 0;
                comp.updatePinPositions();

                placedComponents.push_back(comp);
            } else if (mode == WIRES) {
                // بازیابی سیم‌کشی‌ها و اتصال مجدد آن‌ها به قطعات
                std::stringstream ss(line);
                std::string sComp, sPin, eComp, ePin;

                std::getline(ss, sComp, '|');
                std::getline(ss, sPin, '|');
                std::getline(ss, eComp, '|');
                std::getline(ss, ePin, '|');

                Point startP{0,0}, endP{0,0};
                bool foundS = false, foundE = false;

                for (const auto& c : placedComponents) {
                    if (c.labelId == sComp) {
                        for (const auto& p : c.pins) {
                            if (p.designation == sPin) { startP = p.calculatedWorldPos; foundS = true; break; }
                        }
                    }
                    if (c.labelId == eComp) {
                        for (const auto& p : c.pins) {
                            if (p.designation == ePin) { endP = p.calculatedWorldPos; foundE = true; break; }
                        }
                    }
                }

                if (foundS && foundE) {
                    Wire w(sComp, sPin, startP);
                    w.endCompId = eComp;
                    w.endPinName = ePin;
                    w.isCompleted = true;
                    w.updateOrthogonalRoute(startP, endP);
                    wires.push_back(w);
                }
            }
        }
        file.close();

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Success", "Project loaded successfully!", nullptr);
        return true;
    }
};