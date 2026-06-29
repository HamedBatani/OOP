// include/ComponentInstance.h
#pragma once

#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include "Point.h"

struct ComponentPin {
    std::string designation;
    Point localOffset;
};

class ComponentInstance {
public:
    std::string type;
    std::string labelId;
    std::string valueStr;

    Point worldPos;
    Point dragStartPos{0.0f, 0.0f};
    int rotationDegrees{0};
    bool isMirroredH{false};
    bool isMirroredV{false};
    bool isSelected{false};

    float worldWidth{64.0f};
    float worldHeight{48.0f};
    std::vector<ComponentPin> pins;

    ComponentInstance(std::string typeName, std::string id, std::string val, Point worldLocation)
            : type(std::move(typeName)), labelId(std::move(id)), valueStr(std::move(val)), worldPos(worldLocation) {

        if (type == "Flip-Flop" || type == "Oscilloscope") {
            worldWidth = 80.0f; worldHeight = 60.0f;
        } else if (type == "Op-Amp" || type == "AND Gate" || type == "OR Gate") {
            worldWidth = 80.0f; worldHeight = 50.0f;
        } else if (type == "Resistor" || type == "Capacitor" || type == "Diode") {
            worldWidth = 64.0f; worldHeight = 32.0f;
        }
    }

    SDL_FRect getWorldBoundingBox() const {
        float actualW = (rotationDegrees % 180 == 0) ? worldWidth : worldHeight;
        float actualH = (rotationDegrees % 180 == 0) ? worldHeight : worldWidth;

        return SDL_FRect{
                worldPos.x - (actualW / 2.0f),
                worldPos.y - (actualH / 2.0f),
                actualW,
                actualH
        };
    }
};