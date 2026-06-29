// include/ComponentInstance.h
#pragma once

#include <string>
#include <vector>
#include <SDL3/SDL.h>
#include "Point.h"

struct ComponentPin {
    std::string designation;   // e.g., "1", "2", "In+", "Out", "GND"
    Point localOffset;         // Original relative offset from the component center point
    Point calculatedWorldPos;   // Actual absolute coordinate inside World Space
};

class ComponentInstance {
public:
    std::string type;
    std::string labelId;
    std::string valueStr;

    Point worldPos;
    Point dragStartPos{0.0f, 0.0f};
    int rotationDegrees{0};    // Increments of 0, 90, 180, 270
    bool isMirroredH{false};   // Horizontal mirror flip flag
    bool isMirroredV{false};   // Vertical mirror flip flag
    bool isSelected{false};

    float worldWidth{64.0f};
    float worldHeight{48.0f};
    std::vector<ComponentPin> pins;

    ComponentInstance(std::string typeName, std::string id, std::string val, Point worldLocation)
            : type(std::move(typeName)), labelId(std::move(id)), valueStr(std::move(val)), worldPos(worldLocation) {

        // Setup base bounding boxes scaled to component category type
        if (type == "Flip-Flop" || type == "Oscilloscope") {
            worldWidth = 80.0f; worldHeight = 60.0f;
        } else if (type == "Op-Amp" || type == "AND Gate" || type == "OR Gate") {
            worldWidth = 80.0f; worldHeight = 50.0f;
        } else if (type == "Resistor" || type == "Capacitor" || type == "Diode" || type == "Inductor") {
            worldWidth = 64.0f; worldHeight = 32.0f;
        } else if (type == "DC Source" || type == "AC Source" || type == "Voltmeter" || type == "Ammeter") {
            worldWidth = 40.0f; worldHeight = 60.0f;
        } else if (type == "Ground") {
            worldWidth = 32.0f; worldHeight = 32.0f;
        }

        initializeBasePins();
        updatePinPositions();
    }

    // Instantiates blueprint terminal pin layouts for the schematic router
    void initializeBasePins() {
        pins.clear();
        if (type == "Resistor" || type == "Capacitor" || type == "Inductor" || type == "Diode") {
            pins.push_back({"1", {-32.0f, 0.0f}, {0.0f, 0.0f}});
            pins.push_back({"2", {32.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "Op-Amp" || type == "AND Gate" || type == "OR Gate") {
            pins.push_back({"In1", {-35.0f, -8.0f}, {0.0f, 0.0f}});
            pins.push_back({"In2", {-35.0f, 8.0f}, {0.0f, 0.0f}});
            pins.push_back({"Out", {35.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "NOT Gate") {
            pins.push_back({"In", {-35.0f, 0.0f}, {0.0f, 0.0f}});
            pins.push_back({"Out", {35.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "Flip-Flop") {
            pins.push_back({"J", {-35.0f, -10.0f}, {0.0f, 0.0f}});
            pins.push_back({"CLK", {-35.0f, 0.0f}, {0.0f, 0.0f}});
            pins.push_back({"K", {-35.0f, 10.0f}, {0.0f, 0.0f}});
            pins.push_back({"Q", {35.0f, -10.0f}, {0.0f, 0.0f}});
            pins.push_back({"~Q", {35.0f, 10.0f}, {0.0f, 0.0f}});
        } else if (type == "DC Source" || type == "AC Source" || type == "Voltmeter" || type == "Ammeter") {
            pins.push_back({"+", {0.0f, -30.0f}, {0.0f, 0.0f}});
            pins.push_back({"-", {0.0f, 30.0f}, {0.0f, 0.0f}});
        } else if (type == "Oscilloscope") {
            pins.push_back({"ChA", {-35.0f, -10.0f}, {0.0f, 0.0f}});
            pins.push_back({"ChB", {-35.0f, 10.0f}, {0.0f, 0.0f}});
        } else if (type == "Ground") {
            pins.push_back({"GND", {0.0f, -15.0f}, {0.0f, 0.0f}});
        }
    }

    // Dynamic rotation matrix and mirror mapping arithmetic solver
    void updatePinPositions() {
        for (auto& pin : pins) {
            float lx = pin.localOffset.x;
            float ly = pin.localOffset.y;

            // 1. Compute Mirroring Inversions
            if (isMirroredH) lx = -lx;
            if (isMirroredV) ly = -ly;

            // 2. Compute Trig Rotation Transformations
            float rx = lx;
            float ry = ly;
            if (rotationDegrees == 90) {
                rx = -ly; ry = lx;
            } else if (rotationDegrees == 180) {
                rx = -lx; ry = -ly;
            } else if (rotationDegrees == 270) {
                rx = ly; ry = -lx;
            }

            // 3. Save absolute world coordinates out to map references
            pin.calculatedWorldPos = { worldPos.x + rx, worldPos.y + ry };
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