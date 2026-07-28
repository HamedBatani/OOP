// include/ComponentInstance.h
#pragma once

#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <algorithm>
#include <SDL3/SDL.h>
#include "Point.h"
#include "Component.h"
#include "ExternalMemory.h"
#include "MicrocontrollerCore.h"

struct ComponentPin {
    std::string designation;
    Point localOffset;
    Point calculatedWorldPos;
    bool isHighlighted{false};
    float currentVoltage{0.0f};
    bool isFloating{true};
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

    bool interactiveStateBool{false};
    int ledColorMode{0};
    uint8_t activeSevenSegmentByte{0x00};

    float propagationDelayNs{5.0f};
    int inputCount{2};
    DigitalState lastClkState{DigitalState::Undefined};
    DigitalState internalQState{DigitalState::Low};

    int adcResolutionBits{8};
    float adcConversionDelayNs{10.0f};
    int dacResolutionBits{8};
    float dacConversionDelayNs{10.0f};

    std::vector<std::string> lcdLines{std::string(16, ' '), std::string(16, ' ')};
    int lcdCursorRow{0};
    int lcdCursorCol{0};

    int activeKeypadRow{-1};
    int activeKeypadCol{-1};
    DigitalState lastLcdEnableState{DigitalState::Low};

    float potWiperPosition{0.5f}; // وضعیت لغزنده پتانسیومتر
    float measuredValue{0.0f};
    std::vector<float> scopeSamplesA;
    std::vector<float> scopeSamplesB;

    std::shared_ptr<Component> coreComponent;
    std::shared_ptr<MicrocontrollerCore> microcontroller;
    std::shared_ptr<ExternalMemory> externalMemory;

    ComponentInstance(std::string typeName, std::string id, std::string val, Point worldLocation)
            : type(std::move(typeName)), labelId(std::move(id)), valueStr(std::move(val)), worldPos(worldLocation) {

        if (type == "Ground") coreComponent = std::make_shared<GroundComponent>();
        else if (type == "DC Source") coreComponent = std::make_shared<DcSourceComponent>();
        else if (type == "Battery") coreComponent = std::make_shared<BatteryComponent>();
        else if (type == "Clock Generator") coreComponent = std::make_shared<ClockGeneratorComponent>();
        else if (type == "Resistor") coreComponent = std::make_shared<ResistorComponent>();
        else if (type == "Capacitor") coreComponent = std::make_shared<CapacitorComponent>();
        else if (type == "Inductor") coreComponent = std::make_shared<InductorComponent>();
        else if (type == "Switch") coreComponent = std::make_shared<SwitchComponent>();
        else if (type == "Push Button") coreComponent = std::make_shared<PushButtonComponent>();
        else if (type == "Potentiometer") coreComponent = std::make_shared<PotentiometerComponent>();
        else if (type == "Colored LED") coreComponent = std::make_shared<LedComponent>();
        else if (type == "7-Segment Display") coreComponent = std::make_shared<SevenSegmentComponent>();
        else if (type == "LCD 16x2") coreComponent = std::make_shared<Lcd16x2Component>();
        else if (type == "Keypad 4x4") coreComponent = std::make_shared<KeypadComponent>();
        else if (type == "AND Gate") coreComponent = std::make_shared<AndGateComponent>();
        else if (type == "OR Gate") coreComponent = std::make_shared<OrGateComponent>();
        else if (type == "NOT Gate") { coreComponent = std::make_shared<NotGateComponent>(); inputCount = 1; }
        else if (type == "XOR Gate") coreComponent = std::make_shared<XorGateComponent>();
        else if (type == "NAND Gate") coreComponent = std::make_shared<NandGateComponent>();
        else if (type == "Flip-Flop") { coreComponent = std::make_shared<FlipFlopDComponent>(); inputCount = 2; }
        else if (type == "ADC") { coreComponent = std::make_shared<AdcComponent>(); }
        else if (type == "DAC") { coreComponent = std::make_shared<DacComponent>(); }
        else if (type == "Microcontroller") { microcontroller = std::make_shared<MicrocontrollerCore>(); }
        else if (type == "External Memory") { externalMemory = std::make_shared<ExternalMemory>(); }

        if (type == "Flip-Flop" || type == "Oscilloscope") { worldWidth = 80.0f; worldHeight = 60.0f; }
        else if (type == "Op-Amp" || type == "AND Gate" || type == "OR Gate" || type == "XOR Gate" || type == "NAND Gate") { worldWidth = 80.0f; worldHeight = 50.0f; }
        else if (type == "LCD 16x2") { worldWidth = 150.0f; worldHeight = 70.0f; }
        else if (type == "Keypad 4x4") { worldWidth = 90.0f; worldHeight = 110.0f; }
        else if (type == "Potentiometer") { worldWidth = 64.0f; worldHeight = 48.0f; }
        else if (type == "Resistor" || type == "Capacitor" || type == "Diode" || type == "Inductor" || type == "Switch" || type == "Push Button" || type == "Colored LED") { worldWidth = 64.0f; worldHeight = 32.0f; }
        else if (type == "DC Source" || type == "AC Source" || type == "Voltmeter" || type == "Ammeter" || type == "Battery" || type == "Clock Generator") { worldWidth = 40.0f; worldHeight = 60.0f; }
        else if (type == "7-Segment Display") { worldWidth = 50.0f; worldHeight = 75.0f; }
        else if (type == "Ground") { worldWidth = 32.0f; worldHeight = 32.0f; }
        else if (type == "ADC") { worldWidth = 90.0f; worldHeight = std::max(70.0f, static_cast<float>(adcResolutionBits) * 15.0f); }
        else if (type == "DAC") { worldWidth = 90.0f; worldHeight = std::max(70.0f, static_cast<float>(dacResolutionBits) * 15.0f); }
        else if (type == "Microcontroller") { worldWidth = 120.0f; worldHeight = 180.0f; }
        else if (type == "External Memory") { worldWidth = 100.0f; worldHeight = 150.0f; }

        initializeBasePins();
        updatePinPositions();
    }

    void initializeBasePins() {
        pins.clear();
        if (type == "Resistor" || type == "Capacitor" || type == "Inductor" || type == "Diode" || type == "Switch" || type == "Push Button" || type == "Colored LED") {
            pins.push_back({"1", {-32.0f, 0.0f}, {0.0f, 0.0f}}); pins.push_back({"2", {32.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "Potentiometer") {
            pins.push_back({"+", {-32.0f, -15.0f}, {0.0f, 0.0f}});
            pins.push_back({"-", {-32.0f, 15.0f}, {0.0f, 0.0f}});
            pins.push_back({"Out", {32.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "Op-Amp" || type == "AND Gate" || type == "OR Gate" || type == "XOR Gate" || type == "NAND Gate") {
            float yOffsetStart = -10.0f * (inputCount - 1) / 2.0f;
            for(int i = 0; i < inputCount; ++i) { pins.push_back({"In" + std::to_string(i+1), {-40.0f, yOffsetStart + (i * 10.0f)}, {0.0f, 0.0f}}); }
            pins.push_back({"Out", {40.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "NOT Gate") {
            pins.push_back({"In", {-40.0f, 0.0f}, {0.0f, 0.0f}}); pins.push_back({"Out", {40.0f, 0.0f}, {0.0f, 0.0f}});
        } else if (type == "Flip-Flop") {
            pins.push_back({"D", {-40.0f, -15.0f}, {0.0f, 0.0f}}); pins.push_back({"CLK", {-40.0f, 15.0f}, {0.0f, 0.0f}});
            pins.push_back({"Q", {40.0f, -15.0f}, {0.0f, 0.0f}});  pins.push_back({"~Q", {40.0f, 15.0f}, {0.0f, 0.0f}});
        } else if (type == "LCD 16x2") {
            pins.push_back({"RS", {-75.0f, -20.0f}, {0.0f, 0.0f}}); pins.push_back({"RW", {-75.0f, 0.0f}, {0.0f, 0.0f}}); pins.push_back({"E",  {-75.0f, 20.0f}, {0.0f, 0.0f}});
            for(int i=0; i<8; ++i) { pins.push_back({"D" + std::to_string(i), {75.0f, -25.0f + (i * 7.5f)}, {0.0f, 0.0f}}); }
        } else if (type == "Keypad 4x4") {
            pins.push_back({"R1", {-45.0f, -30.0f}, {0.0f, 0.0f}}); pins.push_back({"R2", {-45.0f, -10.0f}, {0.0f, 0.0f}}); pins.push_back({"R3", {-45.0f,  10.0f}, {0.0f, 0.0f}}); pins.push_back({"R4", {-45.0f,  30.0f}, {0.0f, 0.0f}});
            pins.push_back({"C1", {-30.0f,  55.0f}, {0.0f, 0.0f}}); pins.push_back({"C2", {-10.0f,  55.0f}, {0.0f, 0.0f}}); pins.push_back({"C3", { 10.0f,  55.0f}, {0.0f, 0.0f}}); pins.push_back({"C4", { 30.0f,  55.0f}, {0.0f, 0.0f}});
        } else if (type == "7-Segment Display") {
            pins.push_back({"A", {-25.0f, -30.0f}, {0.0f, 0.0f}}); pins.push_back({"B", {-25.0f, -10.0f}, {0.0f, 0.0f}}); pins.push_back({"C", {-25.0f, 10.0f}, {0.0f, 0.0f}});  pins.push_back({"D", {-25.0f, 30.0f}, {0.0f, 0.0f}});
            pins.push_back({"E", {25.0f, -30.0f}, {0.0f, 0.0f}});  pins.push_back({"F", {25.0f, -10.0f}, {0.0f, 0.0f}}); pins.push_back({"G", {25.0f, 10.0f}, {0.0f, 0.0f}});   pins.push_back({"DP", {25.0f, 30.0f}, {0.0f, 0.0f}});
        } else if (type == "DC Source" || type == "AC Source" || type == "Voltmeter" || type == "Ammeter" || type == "Battery" || type == "Clock Generator") {
            pins.push_back({"+", {0.0f, -30.0f}, {0.0f, 0.0f}}); pins.push_back({"-", {0.0f, 30.0f}, {0.0f, 0.0f}});
        } else if (type == "Oscilloscope") {
            pins.push_back({"ChA", {-40.0f, -10.0f}, {0.0f, 0.0f}}); pins.push_back({"ChB", {-40.0f, 10.0f}, {0.0f, 0.0f}});
        } else if (type == "Ground") {
            pins.push_back({"GND", {0.0f, -15.0f}, {0.0f, 0.0f}});
        } else if (type == "ADC") {
            pins.push_back({"Vref+", {-45.0f, -worldHeight/2.0f + 15.0f}, {0.0f, 0.0f}}); pins.push_back({"Vin", {-45.0f, 0.0f}, {0.0f, 0.0f}}); pins.push_back({"Vref-", {-45.0f, worldHeight/2.0f - 15.0f}, {0.0f, 0.0f}});
            float startY = -((adcResolutionBits - 1) * 15.0f) / 2.0f;
            for(int i = 0; i < adcResolutionBits; ++i) { pins.push_back({"D" + std::to_string(i), {45.0f, startY + (i * 15.0f)}, {0.0f, 0.0f}}); }
        } else if (type == "DAC") {
            pins.push_back({"Vref+", {0.0f, -worldHeight/2.0f}, {0.0f, 0.0f}}); pins.push_back({"Vout",  {45.0f, 0.0f}, {0.0f, 0.0f}}); pins.push_back({"Vref-", {0.0f, worldHeight/2.0f}, {0.0f, 0.0f}});
            float startY = -((dacResolutionBits - 1) * 15.0f) / 2.0f;
            for(int i = 0; i < dacResolutionBits; ++i) { pins.push_back({"D" + std::to_string(i), {-45.0f, startY + (i * 15.0f)}, {0.0f, 0.0f}}); }
        } else if (type == "Microcontroller") {
            for (int i = 0; i < 8; ++i) {
                const float y = -70.0f + i * 20.0f;
                pins.push_back({"PB" + std::to_string(i), {-60.0f, y}, {0.0f, 0.0f}});
                pins.push_back({"PA" + std::to_string(i), {60.0f, y}, {0.0f, 0.0f}});
            }
        } else if (type == "External Memory") {
            for (int i = 0; i < 8; ++i) {
                const float y = -60.0f + i * 17.0f;
                pins.push_back({"A" + std::to_string(i), {-50.0f, y}, {0.0f, 0.0f}});
                pins.push_back({"D" + std::to_string(i), {50.0f, y}, {0.0f, 0.0f}});
            }
            pins.push_back({"CS", {-15.0f, -75.0f}, {0.0f, 0.0f}});
            pins.push_back({"WE", {15.0f, -75.0f}, {0.0f, 0.0f}});
        }
    }

    void processLcdCommand(uint8_t rs, uint8_t data) {
        if (rs == 0) {
            if (data == 0x01) { lcdLines[0] = std::string(16, ' '); lcdLines[1] = std::string(16, ' '); lcdCursorRow = 0; lcdCursorCol = 0; }
            else if (data == 0x02) { lcdCursorRow = 0; lcdCursorCol = 0; }
            else if (data >= 0x80) {
                uint8_t addr = data & 0x7F;
                if (addr >= 0x40) { lcdCursorRow = 1; lcdCursorCol = addr - 0x40; } else { lcdCursorRow = 0; lcdCursorCol = addr; }
                if (lcdCursorCol > 15) lcdCursorCol = 15;
            }
        } else if (rs == 1) {
            if (lcdCursorRow < 2 && lcdCursorCol < 16) {
                lcdLines[lcdCursorRow][lcdCursorCol] = static_cast<char>(data);
                lcdCursorCol++;
                if (lcdCursorCol >= 16) { lcdCursorCol = 0; lcdCursorRow = (lcdCursorRow + 1) % 2; }
            }
        }
    }

    void updatePinPositions() {
        for (auto& pin : pins) {
            float lx = pin.localOffset.x, ly = pin.localOffset.y;
            if (isMirroredH) lx = -lx;
            if (isMirroredV) ly = -ly;
            float rx = lx, ry = ly;
            if (rotationDegrees == 90) { rx = -ly; ry = lx; } else if (rotationDegrees == 180) { rx = -lx; ry = -ly; } else if (rotationDegrees == 270) { rx = ly; ry = -lx; }
            pin.calculatedWorldPos = { worldPos.x + rx, worldPos.y + ry };
        }
    }

    void checkPinHover(const Point& mouseWorldPos, float sensitivityRadius = 8.0f) {
        for (auto& pin : pins) pin.isHighlighted = (pin.calculatedWorldPos.distanceTo(mouseWorldPos) <= sensitivityRadius);
    }

    SDL_FRect getWorldBoundingBox() const {
        float actualW = (rotationDegrees % 180 == 0) ? worldWidth : worldHeight;
        float actualH = (rotationDegrees % 180 == 0) ? worldHeight : worldWidth;
        return SDL_FRect{ worldPos.x - (actualW / 2.0f), worldPos.y - (actualH / 2.0f), actualW, actualH };
    }
};
