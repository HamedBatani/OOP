// include/Component.h
#pragma once

#include <string>
#include <vector>

enum class ComponentClass {
    MainSource,
    Passive,
    InteractiveOutput,
    DigitalLogic
};

// Global Digital Logic State Engine Configuration (Section 4.6 Requirements)
enum class DigitalState {
    Low,       // Modelled as 0V
    High,      // Modelled as 5V
    Undefined  // Modelled as Floating/Short-circuit Warning
};

class Component {
public:
    virtual ~Component() = default;
    virtual std::string getComponentName() const = 0;
    virtual ComponentClass getComponentClass() const = 0;
    virtual std::string getMathematicalEquation() const = 0;
    virtual std::string getDescription() const = 0;
};

// --- 6.1 MAIN SOURCES ---
class GroundComponent : public Component {
public:
    std::string getComponentName() const override { return "Ground (GND)"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V = 0V"; }
    std::string getDescription() const override { return "Circuit voltage reference node (0V). Required for simulation."; }
};

class DcSourceComponent : public Component {
public:
    std::string getComponentName() const override { return "DC Voltage Source"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V = Constant"; }
    std::string getDescription() const override { return "Ideal Direct Current power supply."; }
};

class BatteryComponent : public Component {
private:
    float internalResistance_{0.1f};
public:
    std::string getComponentName() const override { return "Battery"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V_term = E - I * R_int"; }
    std::string getDescription() const override { return "DC voltage source modeled with internal series source resistance."; }
    float getInternalResistance() const { return internalResistance_; }
    void setInternalResistance(float r) { internalResistance_ = r; }
};

class ClockGeneratorComponent : public Component {
private:
    float frequencyHz_{1000.0f};
public:
    std::string getComponentName() const override { return "Clock Generator"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V(t) = SquareWave(t)"; }
    std::string getDescription() const override { return "Digital signal generator pulsing between Low (0V) and High (5V)."; }
    float getFrequency() const { return frequencyHz_; }
    void setFrequency(float f) { frequencyHz_ = f; }
};

// --- 6.2 PASSIVE COMPONENTS ---
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

// --- 6.3 INTERACTIVE COMPONENTS & SIMPLE OUTPUTS ---
class SwitchComponent : public Component {
public:
    std::string getComponentName() const override { return "Switch"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "R = Open ? Inf : 0"; }
    std::string getDescription() const override { return "Interactive toggle switch. Click to open or close the contact pathway."; }
};

class PushButtonComponent : public Component {
public:
    std::string getComponentName() const override { return "Push Button"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "Pressed ? Closed : Open"; }
    std::string getDescription() const override { return "Momentary tactile button. Outputs High (5V) only while held down."; }
};

class LedComponent : public Component {
public:
    std::string getComponentName() const override { return "Colored LED"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "Emit Light if V > V_th"; }
    std::string getDescription() const override { return "Light Emitting Diode. Illuminates in Red, Green, or Blue when forward biased."; }
};

class SevenSegmentComponent : public Component {
public:
    std::string getComponentName() const override { return "7-Segment Display"; }
    ComponentClass getComponentClass() const override { return ComponentClass::InteractiveOutput; }
    std::string getMathematicalEquation() const override { return "Display = f(Pins A-G, DP)"; }
    std::string getDescription() const override { return "Common-cathode LED cluster configuration mapping standard alphanumeric digits."; }
};

// --- 6.4 DIGITAL LOGIC GATES ---
class BaseLogicGate : public Component {
protected:
    float propagationDelayNs_{5.0f};
    int inputCount_{2};
public:
    ComponentClass getComponentClass() const override { return ComponentClass::DigitalLogic; }
    float getDelay() const { return propagationDelayNs_; }
    void setDelay(float ns) { propagationDelayNs_ = ns; }
    int getInputCount() const { return inputCount_; }
    void setInputCount(int count) { inputCount_ = count; }
};

class AndGateComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "AND Gate"; }
    std::string getMathematicalEquation() const override { return "Out = A & B"; }
    std::string getDescription() const override { return "Multi-input digital AND logic gate with instance propagation metrics."; }
};

class OrGateComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "OR Gate"; }
    std::string getMathematicalEquation() const override { return "Out = A | B"; }
    std::string getDescription() const override { return "Multi-input digital OR logic gate with instance propagation metrics."; }
};

class NotGateComponent : public BaseLogicGate {
public:
    NotGateComponent() { inputCount_ = 1; }
    std::string getComponentName() const override { return "NOT Gate"; }
    std::string getMathematicalEquation() const override { return "Out = ~In"; }
    std::string getDescription() const override { return "Single-input logic inverter."; }
};

class XorGateComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "XOR Gate"; }
    std::string getMathematicalEquation() const override { return "Out = A ^ B"; }
    std::string getDescription() const override { return "Exclusive-OR digital logic node."; }
};

class NandGateComponent : public BaseLogicGate {
public:
    std::string getComponentName() const override { return "NAND Gate"; }
    std::string getMathematicalEquation() const override { return "Out = ~(A & B)"; }
    std::string getDescription() const override { return "Inverted AND digital logic block."; }
};

class FlipFlopDComponent : public BaseLogicGate {
public:
    FlipFlopDComponent() { inputCount_ = 2; } // D and CLK
    std::string getComponentName() const override { return "Flip-Flop D"; }
    std::string getMathematicalEquation() const override { return "Q(t+1) = D on CLK Edge Rising"; }
    std::string getDescription() const override { return "Sequential edge-triggered storage register component."; }
};