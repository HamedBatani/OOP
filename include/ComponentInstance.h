// include/ComponentInstance.h
#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <SDL3/SDL.h>
#include "Point.h"
#include "Component.h"

struct ComponentPin {
    std::string designation;
    Point localOffset;
    Point calculatedWorldPos;
    bool isHighlighted{false}; // <--- متغیر جدید برای تشخیص هاور شدن پین
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

    // Object-Oriented base component representation (Section 6 Polymorphism Requirement)
    std::shared_ptr<Component> coreComponent;

    ComponentInstance(std::string typeName, std::string id, std::string val, Point worldLocation)
            : type(std::move(typeName)), labelId(std::move(id)), valueStr(std::move(val)), worldPos(worldLocation) {

        // Instantiate appropriate derived polymorphic object class context
        if (type == "Ground") coreComponent = std::make_shared<GroundComponent>();
        else if (type == "DC Source") coreComponent = std::make_shared<DcSourceComponent>();
        else if (type == "Battery") coreComponent = std::make_shared<BatteryComponent>();
        else if (type == "Clock Generator") coreComponent = std::make_shared<ClockGeneratorComponent>();
        else if (type == "Resistor") coreComponent = std::make_shared<ResistorComponent>();
        else if (type == "Capacitor") coreComponent = std::make_shared<CapacitorComponent>();
        else if (type == "Inductor") coreComponent = std::make_shared<InductorComponent>();

        if (type == "Flip-Flop" || type == "Oscilloscope") {
            worldWidth = 80.0f; worldHeight = 60.0f;
        } else if (type == "Op-Amp" || type == "AND Gate" || type == "OR Gate") {
            worldWidth = 80.0f; worldHeight = 50.0f;
        } else if (type == "Resistor" || type == "Capacitor" || type == "Diode" || type == "Inductor") {
            worldWidth = 64.0f; worldHeight = 32.0f;
        } else if (type == "DC Source" || type == "AC Source" || type == "Voltmeter" || type == "Ammeter" || type == "Battery" || type == "Clock Generator") {
            worldWidth = 40.0f; worldHeight = 60.0f;
        } else if (type == "Ground") {
            worldWidth = 32.0f; worldHeight = 32.0f;
        }

        initializeBasePins();
        updatePinPositions();
    }

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
        } else if (type == "DC Source" || type == "AC Source" || type == "Voltmeter" || type == "Ammeter" || type == "Battery" || type == "Clock Generator") {
            pins.push_back({"+", {0.0f, -30.0f}, {0.0f, 0.0f}});
            pins.push_back({"-", {0.0f, 30.0f}, {0.0f, 0.0f}});
        } else if (type == "Oscilloscope") {
            pins.push_back({"ChA", {-35.0f, -10.0f}, {0.0f, 0.0f}});
            pins.push_back({"ChB", {-35.0f, 10.0f}, {0.0f, 0.0f}});
        } else if (type == "Ground") {
            pins.push_back({"GND", {0.0f, -15.0f}, {0.0f, 0.0f}});
        }
    }

    void updatePinPositions() {
        for (auto& pin : pins) {
            float lx = pin.localOffset.x;
            float ly = pin.localOffset.y;

            if (isMirroredH) lx = -lx;
            if (isMirroredV) ly = -ly;

            float rx = lx;
            float ry = ly;
            if (rotationDegrees == 90) {
                rx = -ly; ry = lx;
            } else if (rotationDegrees == 180) {
                rx = -lx; ry = -ly;
            } else if (rotationDegrees == 270) {
                rx = ly; ry = -lx;
            }
            pin.calculatedWorldPos = { worldPos.x + rx, worldPos.y + ry };
        }
    }

    // تابع اصلی برای بررسی اینکه آیا موس روی پین قرار دارد یا خیر
    void checkPinHover(const Point& mouseWorldPos, float sensitivityRadius = 8.0f) {
        for (auto& pin : pins) {
            if (pin.calculatedWorldPos.distanceTo(mouseWorldPos) <= sensitivityRadius) {
                pin.isHighlighted = true;
            } else {
                pin.isHighlighted = false;
            }
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