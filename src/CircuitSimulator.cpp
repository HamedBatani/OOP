#include "CircuitSimulator.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <string>
#include <unordered_map>

namespace {
    struct DisjointSet {
        explicit DisjointSet(std::size_t count) : parent(count), rank(count, 0) {
            std::iota(parent.begin(), parent.end(), 0);
        }
        int find(int value) {
            if (parent[value] != value) parent[value] = find(parent[value]);
            return parent[value];
        }
        void unite(int a, int b) {
            a = find(a); b = find(b);
            if (a == b) return;
            if (rank[a] < rank[b]) std::swap(a, b);
            parent[b] = a;
            if (rank[a] == rank[b]) ++rank[a];
        }
        std::vector<int> parent;
        std::vector<int> rank;
    };

    struct NetValue {
        bool driven{false};
        bool conflict{false};
        float voltage{0.0f};
        DigitalState logic{DigitalState::Undefined};
    };

    std::string pinKey(const std::string& component, const std::string& pin) {
        return component + "\x1f" + pin;
    }

    float parseValue(const std::string& value, float fallback) {
        if (value.empty()) return fallback;
        char* end = nullptr;
        const float parsed = std::strtof(value.c_str(), &end);
        return end == value.c_str() || !std::isfinite(parsed) ? fallback : parsed;
    }

    void drive(NetValue& net, float voltage, DigitalState logic) {
        if (!net.driven) {
            net.driven = true;
            net.voltage = voltage;
            net.logic = logic;
            return;
        }
        if (std::fabs(net.voltage - voltage) > 0.25f ||
            (net.logic != DigitalState::Undefined && logic != DigitalState::Undefined && net.logic != logic)) {
            net.conflict = true;
            net.logic = DigitalState::Undefined;
            return;
        }
        net.voltage = (net.voltage + voltage) * 0.5f;
        if (net.logic == DigitalState::Undefined) net.logic = logic;
    }
}

bool CircuitSimulator::clockLevelAt(std::uint64_t simulationTimeMs, const std::string& frequencyHz) {
    const float frequency = std::clamp(parseValue(frequencyHz, 1.0f), 0.05f, 1000.0f);
    const auto halfPeriodMs = static_cast<std::uint64_t>(std::max(1.0f, std::round(500.0f / frequency)));
    return (simulationTimeMs / halfPeriodMs) % 2 == 0;
}

void CircuitSimulator::step(std::vector<ComponentInstance>& components, std::vector<Wire>& wires, bool advanceSequential) {
    if (wires.empty()) {
        for (auto& component : components) {
            for (auto& pin : component.pins) { pin.isFloating = true; pin.currentVoltage = 0.0f; }
            if (component.type == "Colored LED") component.interactiveStateBool = false;
            if (component.type == "7-Segment Display") component.activeSevenSegmentByte = 0;
        }
        return;
    }

    DisjointSet sets(wires.size());
    std::unordered_map<std::string, int> wireByUid;
    std::unordered_map<std::string, int> firstWireAtPin;
    for (std::size_t i = 0; i < wires.size(); ++i) wireByUid[wires[i].uid] = static_cast<int>(i);

    auto registerPin = [&](int wireIndex, const std::string& component, const std::string& pin) {
        if (component.empty() || pin.empty()) return;
        const std::string key = pinKey(component, pin);
        auto [it, inserted] = firstWireAtPin.emplace(key, wireIndex);
        if (!inserted) sets.unite(wireIndex, it->second);
    };

    for (std::size_t i = 0; i < wires.size(); ++i) {
        registerPin(static_cast<int>(i), wires[i].startCompId, wires[i].startPinName);
        registerPin(static_cast<int>(i), wires[i].endCompId, wires[i].endPinName);
        if (wires[i].startAnchor.isWireLock()) {
            auto it = wireByUid.find(wires[i].startAnchor.lockedWireUid);
            if (it != wireByUid.end()) sets.unite(static_cast<int>(i), it->second);
        }
        if (wires[i].endAnchor.isWireLock()) {
            auto it = wireByUid.find(wires[i].endAnchor.lockedWireUid);
            if (it != wireByUid.end()) sets.unite(static_cast<int>(i), it->second);
        }
    }

    auto rawNetForPin = [&](const ComponentInstance& component, const std::string& pin) -> int {
        auto it = firstWireAtPin.find(pinKey(component.labelId, pin));
        return it == firstWireAtPin.end() ? -1 : it->second;
    };

    // Closed interactive contacts join their two electrical nets.
    for (const auto& component : components) {
        if (component.type == "Ammeter") {
            int a = rawNetForPin(component, "+"), b = rawNetForPin(component, "-");
            if (a >= 0 && b >= 0) sets.unite(a, b);
        } else if ((component.type == "Switch" || component.type == "Push Button") && component.interactiveStateBool) {
            int a = rawNetForPin(component, "1"), b = rawNetForPin(component, "2");
            if (a >= 0 && b >= 0) sets.unite(a, b);
        } else if (component.type == "Keypad 4x4" && component.activeKeypadRow >= 0 && component.activeKeypadCol >= 0) {
            int row = rawNetForPin(component, "R" + std::to_string(component.activeKeypadRow + 1));
            int column = rawNetForPin(component, "C" + std::to_string(component.activeKeypadCol + 1));
            if (row >= 0 && column >= 0) sets.unite(row, column);
        }
    }

    std::unordered_map<int, NetValue> values;
    auto netForPin = [&](const ComponentInstance& component, const std::string& pin) -> int {
        const int wire = rawNetForPin(component, pin);
        return wire < 0 ? -1 : sets.find(wire);
    };
    auto drivePin = [&](const ComponentInstance& component, const std::string& pin, float voltage, DigitalState state) {
        const int net = netForPin(component, pin);
        if (net >= 0) drive(values[net], voltage, state);
    };
    auto readPin = [&](const ComponentInstance& component, const std::string& pin) -> NetValue {
        const int net = netForPin(component, pin);
        auto it = values.find(net);
        return net < 0 || it == values.end() ? NetValue{} : it->second;
    };

    for (const auto& component : components) {
        if (component.type == "Ground") drivePin(component, "GND", 0.0f, DigitalState::Low);
        else if (component.type == "DC Source" || component.type == "Battery") {
            drivePin(component, "+", parseValue(component.valueStr, 5.0f), DigitalState::High);
            drivePin(component, "-", 0.0f, DigitalState::Low);
        } else if (component.type == "Clock Generator") {
            drivePin(component, "+", component.interactiveStateBool ? 5.0f : 0.0f,
                     component.interactiveStateBool ? DigitalState::High : DigitalState::Low);
            drivePin(component, "-", 0.0f, DigitalState::Low);
        }
    }

    // Iterate because gates may feed other gates in arbitrary drawing order.
    for (int pass = 0; pass < 12; ++pass) {
        for (auto& component : components) {
            if (auto* gate = dynamic_cast<BaseLogicGate*>(component.coreComponent.get())) {
                std::vector<DigitalState> inputs;
                if (component.type == "NOT Gate") inputs.push_back(readPin(component, "In").logic);
                else if (component.type == "Flip-Flop") {
                    const DigitalState d = readPin(component, "D").logic;
                    const DigitalState clk = readPin(component, "CLK").logic;
                    if (advanceSequential && component.lastClkState != DigitalState::High && clk == DigitalState::High && d != DigitalState::Undefined)
                        component.internalQState = d;
                    if (advanceSequential) component.lastClkState = clk;
                    drivePin(component, "Q", LogicEngine::logicToVoltage(component.internalQState), component.internalQState);
                    const DigitalState inverse = component.internalQState == DigitalState::High ? DigitalState::Low : DigitalState::High;
                    drivePin(component, "~Q", LogicEngine::logicToVoltage(inverse), inverse);
                    continue;
                } else {
                    for (int i = 0; i < component.inputCount; ++i)
                        inputs.push_back(readPin(component, "In" + std::to_string(i + 1)).logic);
                }
                const DigitalState output = gate->evaluateLogic(inputs);
                if (output != DigitalState::Undefined) drivePin(component, "Out", LogicEngine::logicToVoltage(output), output);
            } else if (component.type == "Potentiometer") {
                NetValue high = readPin(component, "+"), low = readPin(component, "-");
                const float highV = high.driven ? high.voltage : 5.0f;
                const float lowV = low.driven ? low.voltage : 0.0f;
                const float output = lowV + (highV - lowV) * component.potWiperPosition;
                drivePin(component, "Out", output, LogicEngine::voltageToLogic(output));
            } else if (component.type == "ADC") {
                const NetValue input = readPin(component, "Vin");
                const NetValue refHigh = readPin(component, "Vref+");
                const NetValue refLow = readPin(component, "Vref-");
                auto bits = AdcComponent::performConversion(input.driven ? input.voltage : 0.0f,
                                                            refHigh.driven ? refHigh.voltage : 5.0f,
                                                            refLow.driven ? refLow.voltage : 0.0f,
                                                            component.adcResolutionBits);
                for (int i = 0; i < component.adcResolutionBits; ++i)
                    drivePin(component, "D" + std::to_string(i), LogicEngine::logicToVoltage(bits[i]), bits[i]);
            } else if (component.type == "DAC") {
                std::vector<DigitalState> bits;
                for (int i = 0; i < component.dacResolutionBits; ++i) bits.push_back(readPin(component, "D" + std::to_string(i)).logic);
                const NetValue refHigh = readPin(component, "Vref+");
                const NetValue refLow = readPin(component, "Vref-");
                const float output = DacComponent::performConversion(bits,
                                                                     refHigh.driven ? refHigh.voltage : 5.0f,
                                                                     refLow.driven ? refLow.voltage : 0.0f);
                drivePin(component, "Vout", output, LogicEngine::voltageToLogic(output));
            } else if (component.type == "Microcontroller" && component.microcontroller && pass == 0) {
                std::uint8_t input = 0;
                for (int i = 0; i < 8; ++i)
                    if (readPin(component, "PB" + std::to_string(i)).logic == DigitalState::High) input |= static_cast<std::uint8_t>(1u << i);
                component.microcontroller->portB_.setExternalPins(input);
                if (advanceSequential) component.microcontroller->stepInstruction();
                const std::uint8_t output = component.microcontroller->portA_.getExternalPins();
                for (int i = 0; i < 8; ++i) {
                    const DigitalState state = (output & (1u << i)) ? DigitalState::High : DigitalState::Low;
                    drivePin(component, "PA" + std::to_string(i), LogicEngine::logicToVoltage(state), state);
                }
            }
        }
    }

    for (auto& wire : wires) {
        NetValue value = values[sets.find(static_cast<int>(&wire - wires.data()))];
        wire.currentVoltage = value.voltage;
        wire.currentLogicState = value.conflict ? DigitalState::Undefined : value.logic;
    }

    for (auto& component : components) {
        for (auto& pin : component.pins) {
            const NetValue value = readPin(component, pin.designation);
            pin.isFloating = !value.driven;
            pin.currentVoltage = value.voltage;
        }
        if (component.type == "Colored LED") {
            const NetValue a = readPin(component, "1"), k = readPin(component, "2");
            component.interactiveStateBool = a.driven && k.driven && (a.voltage - k.voltage) > 1.2f;
        } else if (component.type == "7-Segment Display") {
            component.activeSevenSegmentByte = 0;
            static const char* names[] = {"A", "B", "C", "D", "E", "F", "G", "DP"};
            for (int i = 0; i < 8; ++i)
                if (readPin(component, names[i]).logic == DigitalState::High) component.activeSevenSegmentByte |= static_cast<uint8_t>(1u << i);
        } else if (component.type == "LCD 16x2") {
            const DigitalState enable = readPin(component, "E").logic;
            const DigitalState readWrite = readPin(component, "RW").logic;
            if (advanceSequential && component.lastLcdEnableState != DigitalState::High && enable == DigitalState::High && readWrite != DigitalState::High) {
                std::uint8_t data = 0;
                for (int i = 0; i < 8; ++i)
                    if (readPin(component, "D" + std::to_string(i)).logic == DigitalState::High) data |= static_cast<std::uint8_t>(1u << i);
                component.processLcdCommand(readPin(component, "RS").logic == DigitalState::High ? 1 : 0, data);
            }
            if (advanceSequential) component.lastLcdEnableState = enable;
        } else if (component.type == "Voltmeter") {
            component.measuredValue = readPin(component, "+").voltage - readPin(component, "-").voltage;
        } else if (component.type == "Ammeter") {
            component.measuredValue = 0.0f;
            const int meterNet = netForPin(component, "+");
            for (const auto& load : components) {
                if (load.type != "Resistor") continue;
                const int aNet = netForPin(load, "1"), bNet = netForPin(load, "2");
                if (aNet != meterNet && bNet != meterNet) continue;
                const float resistance = std::max(0.001f, parseValue(load.valueStr, 1000.0f));
                component.measuredValue += std::fabs(readPin(load, "1").voltage - readPin(load, "2").voltage) / resistance;
            }
        } else if (component.type == "Oscilloscope") {
            component.measuredValue = readPin(component, "ChA").voltage;
            if (advanceSequential) {
                component.scopeSamplesA.push_back(readPin(component, "ChA").voltage);
                component.scopeSamplesB.push_back(readPin(component, "ChB").voltage);
                constexpr std::size_t maxSamples = 120;
                if (component.scopeSamplesA.size() > maxSamples) component.scopeSamplesA.erase(component.scopeSamplesA.begin());
                if (component.scopeSamplesB.size() > maxSamples) component.scopeSamplesB.erase(component.scopeSamplesB.begin());
            }
        }
    }
}

void CircuitSimulator::reset(std::vector<ComponentInstance>& components, std::vector<Wire>& wires) {
    for (auto& wire : wires) { wire.currentLogicState = DigitalState::Undefined; wire.currentVoltage = 0.0f; }
    for (auto& component : components) {
        component.lastClkState = DigitalState::Undefined;
        component.lastLcdEnableState = DigitalState::Low;
        component.internalQState = DigitalState::Low;
        if (component.type == "Clock Generator" || component.type == "Push Button" || component.type == "Colored LED")
            component.interactiveStateBool = false;
        if (component.type == "7-Segment Display") component.activeSevenSegmentByte = 0;
        component.measuredValue = 0.0f;
        component.scopeSamplesA.clear(); component.scopeSamplesB.clear();
        if (component.microcontroller) component.microcontroller->reset();
        for (auto& pin : component.pins) { pin.currentVoltage = 0.0f; pin.isFloating = true; }
    }
}
