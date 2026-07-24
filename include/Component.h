// include/Component.h
#pragma once

#include <string>
#include <vector>
#include <iostream>
#include <cmath>

enum class ComponentClass {
    MainSource, Passive, InteractiveOutput, DigitalLogic, Advanced
};

enum class DigitalState { Low, High, Undefined };

class LogicEngine {
public:
    static DigitalState voltageToLogic(float voltage, bool isFloating = false) {
        if (isFloating) {
            std::cout << "Warning: Floating input detected.\n";
            return DigitalState::Undefined;
        }
        if (voltage >= 3.5f) return DigitalState::High;
        if (voltage <= 1.5f) return DigitalState::Low;
        return DigitalState::Undefined;
    }

    static float logicToVoltage(DigitalState state) {
        if (state == DigitalState::High) return 5.0f;
        if (state == DigitalState::Low) return 0.0f;
        return 0.0f;
    }
};

class Component {
public:
    virtual ~Component() = default;
    virtual std::string getComponentName() const = 0;
    virtual ComponentClass getComponentClass() const = 0;
    virtual std::string getMathematicalEquation() const = 0;
    virtual std::string getDescription() const = 0;
};

class GroundComponent : public Component {
public:
    std::string getComponentName() const override { return "Ground (GND)"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V = 0V"; }
    std::string getDescription() const override { return "Circuit voltage reference node (0V)."; }
};

class DcSourceComponent : public Component {
public:
    std::string getComponentName() const override { return "DC Voltage Source"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V = Constant"; }
    std::string getDescription() const override { return "Ideal Direct Current power supply."; }
};

class BatteryComponent : public Component {
public:
    std::string getComponentName() const override { return "Battery"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V_term = E - I * R_int"; }
    std::string getDescription() const override { return "DC voltage source modeled with internal resistance."; }
};

class ClockGeneratorComponent : public Component {
public:
    std::string getComponentName() const override { return "Clock Generator"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V(t) = SquareWave(0V to 5V)"; }
    std::string getDescription() const override { return "Digital signal generator pulsing between Low (0V) and High (5V)."; }
};

class ResistorComponent : public Component {
public:
    std::string getComponentName() const override { return "Resistor"; }
    ComponentClass getComponentClass() const override { return ComponentClass::Passive; }
    std::string getMathematicalEquation() const override { return "V = I * R"; }
    std::string getDescription() const override { return "Linear passive component implementing Ohm's Law."; }
};

class CapacitorComponent : public Component {
public:
    std::string getComponentName() const override { return "Capacitor"; }
    ComponentClass getComponentClass() const override { return ComponentClass::Passive; }
    std::string getMathematicalEquation() const override { return "I = C * (dV / dt)"; }
    std::string getDescription() const override { return "Electrostatic energy storage component."; }
};

class InductorComponent : public Component {
public:
    std::string getComponentName() const override { return "Inductor"; }
    ComponentClass getComponentClass() const override { return ComponentClass::Passive; }
    std::string getMathematicalEquation() const override { return "V = L * (dI / dt)"; }
    std::string getDescription() const override { return "Electromagnetic energy storage component."; }
};

class SwitchComponent : public Component {
public:
    std::string getComponentName() const override { return "Switch"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "R = Open ? Inf : 0"; }
    std::string getDescription() const override { return "Interactive toggle switch."; }
};

class PushButtonComponent : public Component {
public:
    std::string getComponentName() const override { return "Push Button"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "Pressed ? HIGH(5V) : LOW(0V)"; }
    std::string getDescription() const override { return "Momentary tactile button."; }
};

class PotentiometerComponent : public Component {
public:
    std::string getComponentName() const override { return "Potentiometer"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "Vout = V- + Wiper * (V+ - V-)"; }
    std::string getDescription() const override { return "Interactive Variable Resistor (Voltage Divider)."; }
};

class LedComponent : public Component {
public:
    std::string getComponentName() const override { return "Colored LED"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "Emit Light if V > V_th"; }
    std::string getDescription() const override { return "Light Emitting Diode (Red, Green, Blue)."; }
};

class SevenSegmentComponent : public Component {
public:
    std::string getComponentName() const override { return "7-Segment Display"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "Display = f(Pins A-G, DP)"; }
    std::string getDescription() const override { return "LED cluster configuration mapping digits."; }
};

class Lcd16x2Component : public Component {
public:
    std::string getComponentName() const override { return "LCD 16x2"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "Display = f(RS, RW, E, D0-D7)"; }
    std::string getDescription() const override { return "Standard 16x2 Character LCD (HD44780 compatible)."; }
};

class KeypadComponent : public Component {
public:
    std::string getComponentName() const override { return "Keypad 4x4"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "Row[i] & Col[j] shorted if Key(i,j) pressed"; }
    std::string getDescription() const override { return "4x4 Matrix Keypad. Allows multiplexed scanning of rows and columns."; }
};

class BaseLogicGate : public Component {
public:
    ComponentClass getComponentClass() const override { return ComponentClass::DigitalLogic; }
    virtual DigitalState evaluateLogic(const std::vector<DigitalState>& inputs) const = 0;
};

class AndGateComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "AND Gate"; }
    std::string getMathematicalEquation() const override { return "Out = A & B"; }
    std::string getDescription() const override { return "Multi-input digital AND logic gate."; }
    DigitalState evaluateLogic(const std::vector<DigitalState>& inputs) const override {
        for (auto state : inputs) {
            if (state == DigitalState::Undefined) return DigitalState::Undefined;
            if (state == DigitalState::Low) return DigitalState::Low;
        }
        return DigitalState::High;
    }
};

class OrGateComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "OR Gate"; }
    std::string getMathematicalEquation() const override { return "Out = A | B"; }
    std::string getDescription() const override { return "Multi-input digital OR logic gate."; }
    DigitalState evaluateLogic(const std::vector<DigitalState>& inputs) const override {
        for (auto state : inputs) {
            if (state == DigitalState::Undefined) return DigitalState::Undefined;
            if (state == DigitalState::High) return DigitalState::High;
        }
        return DigitalState::Low;
    }
};

class NotGateComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "NOT Gate"; }
    std::string getMathematicalEquation() const override { return "Out = ~In"; }
    std::string getDescription() const override { return "Single-input logic inverter."; }
    DigitalState evaluateLogic(const std::vector<DigitalState>& inputs) const override {
        if (inputs.empty() || inputs[0] == DigitalState::Undefined) return DigitalState::Undefined;
        return (inputs[0] == DigitalState::High) ? DigitalState::Low : DigitalState::High;
    }
};

class XorGateComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "XOR Gate"; }
    std::string getMathematicalEquation() const override { return "Out = A ^ B"; }
    std::string getDescription() const override { return "Exclusive-OR digital logic node."; }
    DigitalState evaluateLogic(const std::vector<DigitalState>& inputs) const override {
        if (inputs.size() < 2 || inputs[0] == DigitalState::Undefined || inputs[1] == DigitalState::Undefined) return DigitalState::Undefined;
        return (inputs[0] != inputs[1]) ? DigitalState::High : DigitalState::Low;
    }
};

class NandGateComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "NAND Gate"; }
    std::string getMathematicalEquation() const override { return "Out = ~(A & B)"; }
    std::string getDescription() const override { return "Inverted AND digital logic block."; }
    DigitalState evaluateLogic(const std::vector<DigitalState>& inputs) const override {
        for (auto state : inputs) {
            if (state == DigitalState::Undefined) return DigitalState::Undefined;
            if (state == DigitalState::Low) return DigitalState::High;
        }
        return DigitalState::Low;
    }
};

class FlipFlopDComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "Flip-Flop D"; }
    std::string getMathematicalEquation() const override { return "Q(t+1) = D on CLK Edge Rising"; }
    std::string getDescription() const override { return "Sequential edge-triggered storage register."; }
    DigitalState evaluateLogic(const std::vector<DigitalState>& inputs) const override {
        return inputs.empty() ? DigitalState::Undefined : inputs[0];
    }
};

class AdcComponent : public Component {
public:
    std::string getComponentName() const override { return "Analog-to-Digital Converter"; }
    ComponentClass getComponentClass() const override { return ComponentClass::Advanced; }
    std::string getMathematicalEquation() const override { return "Output = ((Vin - Vref-)/(Vref+ - Vref-)) * (2^N - 1)"; }
    std::string getDescription() const override { return "Ideal N-bit ADC with Conversion Delay and Saturation."; }

    static std::vector<DigitalState> performConversion(float vin, float vrefPlus, float vrefMinus, int bits) {
        std::vector<DigitalState> output(bits, DigitalState::Low);
        if (vrefPlus <= vrefMinus) return output;
        float maxVal = static_cast<float>((1 << bits) - 1);
        float rawVal = ((vin - vrefMinus) / (vrefPlus - vrefMinus)) * maxVal;

        int digitalVal = static_cast<int>(std::round(rawVal));
        if (digitalVal < 0) digitalVal = 0;
        if (digitalVal > maxVal) digitalVal = static_cast<int>(maxVal);

        for (int i = 0; i < bits; ++i) {
            output[i] = ((digitalVal >> i) & 1) ? DigitalState::High : DigitalState::Low;
        }
        return output;
    }
};

class DacComponent : public Component {
public:
    std::string getComponentName() const override { return "Digital-to-Analog Converter"; }
    ComponentClass getComponentClass() const override { return ComponentClass::Advanced; }
    std::string getMathematicalEquation() const override { return "Vout = Vref- + (Code / (2^N - 1)) * (Vref+ - Vref-)"; }
    std::string getDescription() const override { return "Ideal N-bit DAC with Conversion Delay."; }

    static float performConversion(const std::vector<DigitalState>& digitalInputs, float vrefPlus, float vrefMinus) {
        if (vrefPlus <= vrefMinus || digitalInputs.empty()) return 0.0f;
        int bits = digitalInputs.size();
        int code = 0;
        for (int i = 0; i < bits; ++i) {
            if (digitalInputs[i] == DigitalState::High) {
                code |= (1 << i);
            }
        }
        float maxVal = static_cast<float>((1 << bits) - 1);
        return vrefMinus + (static_cast<float>(code) / maxVal) * (vrefPlus - vrefMinus);
    }
};