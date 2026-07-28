#include "Canvas.h"
#include "CircuitSimulator.h"
#include "ComponentLibrary.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

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
    } catch (const std::exception& error) {
        std::cerr << "Core test failure: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Core tests passed\n";
    return 0;
}
