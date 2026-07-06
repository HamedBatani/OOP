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
    bool isHighlighted{false};
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

    // --- Interactive State Registers (Section 3.6 Requirements) ---
    bool interactiveStateBool{false};     // Tracks Switch (Open/Closed) or PushButton states
    int ledColorMode{0};                  // 0 = Red, 1 = Green, 2 = Blue
    uint8_t activeSevenSegmentByte{0x00}; // Bitwise flags for segments [A,B,C,D,E,F,G,DP]

    std::shared_ptr<Component> coreComponent;

    ComponentInstance(std::string typeName, std::string id, std::string val, Point worldLocation)
            : type(std::move(typeName)), labelId(std::move(id)), valueStr(std::move(val)), worldPos(worldLocation) {

        // Polymorphic Backend Instantiation Context Matching
        if (type == "Ground") coreComponent = std::make_shared<GroundComponent>();
        else if (type == "DC Source") coreComponent = std::make_shared<DcSourceComponent>();
        else if (type == "Battery") coreComponent = std::make_shared<BatteryComponent>();
        else if (type == "Clock Generator") coreComponent = std::make_shared<ClockGeneratorComponent>();
        else if (type == "Resistor") coreComponent = std::make_shared<ResistorComponent>();
        else if (type == "Capacitor") coreComponent = std::make_shared<CapacitorComponent>();
        else if (type == "Inductor") coreComponent = std::make_shared<InductorComponent>();
        else if (type == "Switch") coreComponent = std::make_shared<SwitchComponent>();
        else if (type == "Push Button") coreComponent = std::make_shared<PushButtonComponent>();
        else if (type == "Colored LED") coreComponent = std::make_shared<LedComponent>();
        else if (type == "7-Segment Display") coreComponent = std::make_shared<SevenSegmentComponent>();
        else if (type == "AND Gate") coreComponent = std::make_shared<AndGateComponent>();
        else if (type == "OR Gate") coreComponent = std::make_shared<OrGateComponent>();
        else if (type == "NOT Gate") coreComponent = std::make_shared<NotGateComponent>();
        else if (type == "XOR Gate") coreComponent = std::make_shared<XorGateComponent>();
        else if (type == "NAND Gate") coreComponent = std::make_shared<NandGateComponent>();
        else if (type == "Flip-Flop") coreComponent = std::make_shared<FlipFlopDComponent>();

        // Establish absolute spatial aspect configurations
        if (type == "Flip-Flop" || type == "Oscilloscope") {
            worldWidth = 80.0f; worldHeight = 60.0f;
        } else if (type == "Op-Amp" || type == "AND Gate" || type == "OR Gate" || type == "XOR Gate" || type == "NAND Gate") {
            worldWidth = 80.0f; worldHeight = 50.0f;
        } else if (type == "Resistor" || type == "Capacitor" || type == "Diode" || type == "Inductor" || type == "Switch" || type == "Push Button" || type == "Colored LED") {
            worldWidth = 64.0f; worldHeight = 32.0f;
        } else if (type == "DC Source" || type == "AC Source" || type == "Voltmeter" || type == "Ammeter" || type == "Battery" || type == "Clock Generator") {
            worldWidth = 40.0f; worldHeight = 60.0f;
        } else if (type == "7-Segment Display") {
            worldWidth = 50.0f; worldHeight = 75.0f;
        } else if (type == "Ground") {
            worldWidth = 32.0f; worldHeight = 32.0f;
        }

        initializeBasePins();
        updatePinPositions();
    }

    void initializeBasePins() {
        pins.clear();
        if (type == "Resistor" || type == "Capacitor" || type == "Inductor" || type == "Diode" || type == "Switch" || type == "Push Button" || type == "Colored LED") {
            pins.push_back({"1", {-32.0f, 0.0f}, {0.0f, 0.0f}});
            pins.push_back({"2", {32.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "Op-Amp" || type == "AND Gate" || type == "OR Gate" || type == "XOR Gate" || type == "NAND Gate") {
            pins.push_back({"In1", {-40.0f, -10.0f}, {0.0f, 0.0f}});
            pins.push_back({"In2", {-40.0f, 10.0f}, {0.0f, 0.0f}});
            pins.push_back({"Out", {40.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "NOT Gate") {
            pins.push_back({"In", {-40.0f, 0.0f}, {0.0f, 0.0f}});
            pins.push_back({"Out", {40.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "Flip-Flop") {
            pins.push_back({"D", {-40.0f, -15.0f}, {0.0f, 0.0f}});
            pins.push_back({"CLK", {-40.0f, 15.0f}, {0.0f, 0.0f}});
            pins.push_back({"Q", {40.0f, -15.0f}, {0.0f, 0.0f}});
            pins.push_back({"~Q", {40.0f, 15.0f}, {0.0f, 0.0f}});
        } else if (type == "7-Segment Display") {
            // Explicit 8-Terminal grid system layout allocation maps (Section 3.6 Requirement)
            pins.push_back({"A", {-25.0f, -30.0f}, {0.0f, 0.0f}});
            pins.push_back({"B", {-25.0f, -10.0f}, {0.0f, 0.0f}});
            pins.push_back({"C", {-25.0f, 10.0f}, {0.0f, 0.0f}});
            pins.push_back({"D", {-25.0f, 30.0f}, {0.0f, 0.0f}});
            pins.push_back({"E", {25.0f, -30.0f}, {0.0f, 0.0f}});
            pins.push_back({"F", {25.0f, -10.0f}, {0.0f, 0.0f}});
            pins.push_back({"G", {25.0f, 10.0f}, {0.0f, 0.0f}});
            pins.push_back({"DP", {25.0f, 30.0f}, {0.0f, 0.0f}});
        } else if (type == "DC Source" || type == "AC Source" || type == "Voltmeter" || type == "Ammeter" || type == "Battery" || type == "Clock Generator") {
            pins.push_back({"+", {0.0f, -30.0f}, {0.0f, 0.0f}});
            pins.push_back({"-", {0.0f, 30.0f}, {0.0f, 0.0f}});
        } else if (type == "Oscilloscope") {
            pins.push_back({"ChA", {-40.0f, -10.0f}, {0.0f, 0.0f}});
            pins.push_back({"ChB", {-40.0f, 10.0f}, {0.0f, 0.0f}});
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

    void checkPinHover(const Point& mouseWorldPos, float sensitivityRadius = 8.0f) {
        for (auto& pin : pins) {
            pin.isHighlighted = (pin.calculatedWorldPos.distanceTo(mouseWorldPos) <= sensitivityRadius);
        }
    }

    SDL_FRect getWorldBoundingBox() const {
        float actualW = (rotationDegrees % 180 == 0) ? worldWidth : worldHeight;
        float actualH = (rotationDegrees % 180 == 0) ? worldHeight : worldWidth;
        return SDL_FRect{ worldPos.x - (actualW / 2.0f), worldPos.y - (actualH / 2.0f), actualW, actualH };
    }
};