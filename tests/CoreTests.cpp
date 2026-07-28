#include "Canvas.h"
#include "CircuitSimulator.h"
#include "ComponentLibrary.h"
#include "DesignRuleChecker.h"
#include "ProjectHistory.h"
#include "ProjectManager.h"
#include "SmartWireRouter.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <fstream>

namespace {
    void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }

    Wire connect(const ComponentInstance& from, const char* fromPin,
                 const ComponentInstance& to, const char* toPin) {
        Point start{}, end{};
        for (const auto& pin : from.pins) if (pin.designation == fromPin) start = pin.calculatedWorldPos;
        for (const auto& pin : to.pins) if (pin.designation == toPin) end = pin.calculatedWorldPos;
        Wire wire(from.labelId, fromPin, start);
        wire.endCompId = to.labelId;
        wire.endPinName = toPin;
        wire.endAnchor = WireAnchor::makePinLock(to.labelId, toPin, end);
        wire.isCompleted = true;
        wire.updateOrthogonalRoute(start, end);
        return wire;
    }
}

int main() {
    try {
        Canvas canvas(800.0f, 600.0f);
        canvas.setCameraPosition({40.0f, 30.0f});
        const Point cursor{250.0f, 180.0f};
        const Point before = canvas.screenToWorld(cursor);
        canvas.zoomAt(cursor, 1.75f);
        const Point after = canvas.screenToWorld(cursor);
        require(std::fabs(before.x - after.x) < 0.001f && std::fabs(before.y - after.y) < 0.001f,
                "cursor-centred zoom changed the world point under the cursor");

        ComponentInstance resistor("Resistor", "R1", "1k", {100.0f, 100.0f});
        resistor.rotationDegrees = 90;
        resistor.updatePinPositions();
        require(std::fabs(resistor.pins[0].calculatedWorldPos.x - 100.0f) < 0.001f,
                "rotated pin X coordinate is incorrect");
        require(std::fabs(resistor.pins[0].calculatedWorldPos.y - 68.0f) < 0.001f,
                "rotated pin Y coordinate is incorrect");

        const auto smartRoute = SmartWireRouter::route({100.0f, 100.0f}, {0.0f, 0.0f},
                                                        {1.0f, 0.0f}, {0.0f, 1.0f}, {});
        require(smartRoute.size() >= 4 && smartRoute[1].x > smartRoute[0].x &&
                std::fabs(smartRoute[1].y - smartRoute[0].y) < 0.001f,
                "smart router did not leave the source pin in its outward direction");
        require(std::fabs(smartRoute[smartRoute.size() - 2].x - smartRoute.back().x) < 0.001f &&
                smartRoute[smartRoute.size() - 2].y > smartRoute.back().y,
                "smart router did not approach the destination from its outward side");

        // Regression: a connection travelling right-to-left must not reach a
        // left-hand pin by crossing through the destination symbol and then
        // doubling back.  Endpoint component bodies participate in routing.
        const std::vector<SDL_FRect> endpointObstacles{
            {260.0f, 75.0f, 40.0f, 50.0f}, {100.0f, 84.0f, 64.0f, 32.0f}
        };
        const auto aroundEndpoints = SmartWireRouter::route({300.0f, 100.0f}, {100.0f, 100.0f},
                                                             {1.0f, 0.0f}, {-1.0f, 0.0f},
                                                             endpointObstacles);
        require(aroundEndpoints.size() >= 6 && aroundEndpoints[1].x > aroundEndpoints.front().x,
                "smart router did not leave the source symbol outwards");
        require(aroundEndpoints[aroundEndpoints.size() - 2].x < aroundEndpoints.back().x,
                "smart router approached a left-hand destination pin from behind");
        for (std::size_t i = 2; i + 2 < aroundEndpoints.size(); ++i) {
            const Point a = aroundEndpoints[i - 1], b = aroundEndpoints[i];
            for (const auto& raw : endpointObstacles) {
                const SDL_FRect box{raw.x - 8.0f, raw.y - 8.0f, raw.w + 16.0f, raw.h + 16.0f};
                const bool horizontalHit = std::fabs(a.y - b.y) < 0.01f && a.y > box.y && a.y < box.y + box.h &&
                                           std::max(a.x, b.x) > box.x && std::min(a.x, b.x) < box.x + box.w;
                const bool verticalHit = std::fabs(a.x - b.x) < 0.01f && a.x > box.x && a.x < box.x + box.w &&
                                         std::max(a.y, b.y) > box.y && std::min(a.y, b.y) < box.y + box.h;
                require(!horizontalHit && !verticalHit, "smart route crosses a component clearance area");
            }
        }

        std::vector<ComponentInstance> components;
        components.emplace_back("DC Source", "V1", "5", Point{0.0f, 0.0f});
        components.emplace_back("DC Source", "V2", "5", Point{0.0f, 100.0f});
        components.emplace_back("AND Gate", "U1", "", Point{150.0f, 50.0f});
        std::vector<Wire> wires;
        wires.push_back(connect(components[0], "+", components[2], "In1"));
        wires.push_back(connect(components[1], "+", components[2], "In2"));
        Wire output("U1", "Out", components[2].pins.back().calculatedWorldPos);
        output.updateOrthogonalRoute(components[2].pins.back().calculatedWorldPos, {240.0f, 50.0f});
        output.isCompleted = true;
        wires.push_back(output);
        CircuitSimulator::step(components, wires);
        require(wires.back().currentLogicState == DigitalState::High,
                "AND gate did not propagate two high inputs");

        components.emplace_back("Colored LED", "LED1", "Red", Point{300.0f, 50.0f});
        wires.push_back(connect(components[2], "Out", components[3], "1"));
        components.emplace_back("Ground", "GND1", "", Point{300.0f, 120.0f});
        wires.push_back(connect(components[4], "GND", components[3], "2"));
        CircuitSimulator::step(components, wires);
        require(components[3].interactiveStateBool, "LED did not respond to the simulated voltage difference");

        CircuitSimulator::reset(components, wires);
        require(wires.front().currentLogicState == DigitalState::Undefined,
                "simulation reset left stale wire state");

        ComponentLibrary library(0.0f, 45.0f, 300.0f, 900.0f);
        std::string selected = "None";
        SDL_Event motion{};
        motion.type = SDL_EVENT_MOUSE_MOTION;
        motion.motion.x = 120.0f;
        motion.motion.y = 240.0f;
        library.handleEvent(motion, selected);
        SDL_Event wheel{};
        wheel.type = SDL_EVENT_MOUSE_WHEEL;
        wheel.wheel.y = -1.0f;
        require(library.handleEvent(wheel, selected),
                "library wheel input was not consumed inside its scrollable area");

        // Part 9: a digital voltmeter reports the live difference between its terminals.
        std::vector<ComponentInstance> measured;
        measured.emplace_back("DC Source", "VS", "5", Point{0.0f, 0.0f});
        measured.emplace_back("Voltmeter", "VM1", "", Point{100.0f, 0.0f});
        std::vector<Wire> measuredWires;
        measuredWires.push_back(connect(measured[0], "+", measured[1], "+"));
        measuredWires.push_back(connect(measured[0], "-", measured[1], "-"));
        CircuitSimulator::step(measured, measuredWires);
        require(std::fabs(measured[1].measuredValue - 5.0f) < 0.01f, "voltmeter did not report the source voltage");

        // Part 10: serialization restores advanced settings and exact routed geometry.
        measured[0].rotationDegrees = 90;
        measuredWires[0].routingPoints.insert(measuredWires[0].routingPoints.begin() + 1, {40.0f, 25.0f});
        const std::string snapshot = ProjectManager::serialize({"Voltmeter"}, measured, measuredWires);
        require(!snapshot.empty() && snapshot.front() == '{' && snapshot.find("\"format\": \"OOP_CIRCUIT\"") != std::string::npos,
                "project serialization is not JSON");
        { std::ofstream jsonProof("json_roundtrip_test.json", std::ios::binary); jsonProof << snapshot; }
        std::vector<std::string> restoredActive; std::vector<ComponentInstance> restored; std::vector<Wire> restoredWires;
        require(ProjectManager::deserialize(snapshot, restoredActive, restored, restoredWires), "project snapshot could not be restored");
        require(restored.size() == 2 && restored[0].rotationDegrees == 90, "component settings were lost during restore");
        require(restoredWires[0].routingPoints.size() == measuredWires[0].routingPoints.size(), "wire route was lost during restore");

        ProjectHistory history;
        history.reset(restoredActive, restored, restoredWires);
        restored[0].worldPos.x = 500.0f; history.record(restoredActive, restored, restoredWires);
        require(history.undo(restoredActive, restored, restoredWires) && restored[0].worldPos.x != 500.0f, "undo did not restore the previous design");
        require(history.redo(restoredActive, restored, restoredWires) && restored[0].worldPos.x == 500.0f, "redo did not restore the newer design");

        // Part 11: conflicting sources and floating sensitive inputs block simulation.
        std::vector<ComponentInstance> invalid;
        invalid.emplace_back("DC Source", "VHIGH", "5", Point{0.0f, 0.0f});
        invalid.emplace_back("DC Source", "VLOW", "0", Point{100.0f, 0.0f});
        invalid.emplace_back("AND Gate", "BAD_GATE", "", Point{200.0f, 0.0f});
        std::vector<Wire> invalidWires{connect(invalid[0], "+", invalid[1], "+")};
        const DrcReport invalidReport = DesignRuleChecker::inspect(invalid, invalidWires);
        require(!invalidReport.canRun && invalidReport.errorCount >= 3,
                "DRC did not block conflicting sources and floating gate inputs");
        std::vector<ComponentInstance> floatingMcu;
        floatingMcu.emplace_back("Microcontroller", "MCU_FLOAT", "", Point{0.0f, 0.0f});
        const DrcReport mcuReport = DesignRuleChecker::inspect(floatingMcu, {});
        require(!mcuReport.canRun && mcuReport.errorCount == 8,
                "DRC did not classify the MCU Port B input bank correctly");

        // Shipped verification circuits must remain readable and demonstrate
        // one valid measurement design plus both mandatory DRC failures.
        auto loadExample = [](const char* path, std::vector<std::string>& active,
                              std::vector<ComponentInstance>& comps, std::vector<Wire>& exampleWires) {
            return ProjectManager::loadProject(path, active, comps, exampleWires);
        };
        std::vector<std::string> exampleActive; std::vector<ComponentInstance> exampleComponents; std::vector<Wire> exampleWires;
        require(loadExample("../parts9_11_demo.circuit", exampleActive, exampleComponents, exampleWires), "measurement example is invalid");
        require(DesignRuleChecker::inspect(exampleComponents, exampleWires).canRun, "measurement example does not pass DRC");
        require(loadExample("../parts9_11_drc_short.circuit", exampleActive, exampleComponents, exampleWires), "short-circuit example is invalid");
        require(!DesignRuleChecker::inspect(exampleComponents, exampleWires).canRun, "short-circuit example was not blocked");
        require(loadExample("../parts9_11_drc_floating.circuit", exampleActive, exampleComponents, exampleWires), "floating-input example is invalid");
        require(!DesignRuleChecker::inspect(exampleComponents, exampleWires).canRun, "floating-input example was not blocked");

        require(loadExample("../parts9_oscilloscope_clock.circuit", exampleActive, exampleComponents, exampleWires), "oscilloscope example is invalid");
        require(DesignRuleChecker::inspect(exampleComponents, exampleWires).canRun, "oscilloscope example does not pass DRC");
        exampleComponents[0].interactiveStateBool = false;
        CircuitSimulator::step(exampleComponents, exampleWires, true);
        exampleComponents[0].interactiveStateBool = true;
        CircuitSimulator::step(exampleComponents, exampleWires, true);
        require(exampleComponents[2].scopeSamplesA.size() == 2 &&
                exampleComponents[2].scopeSamplesA.front() < 0.1f && exampleComponents[2].scopeSamplesA.back() > 4.9f,
                "oscilloscope example did not capture the clock transition");
        require(CircuitSimulator::clockLevelAt(250, "1") && !CircuitSimulator::clockLevelAt(250, "2"),
                "changing clock frequency did not change its output timing");
    } catch (const std::exception& error) {
        std::cerr << "Core test failure: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Core tests passed\n";
    return 0;
}
