// include/Component.h
#pragma once

#include <string>

enum class ComponentClass {
    MainSource,
    Passive
};

// Base polymorphic class as specified in Section 6 documentation
class Component {
public:
    virtual ~Component() = default;
    virtual std::string getComponentName() const = 0;
    virtual ComponentClass getComponentClass() const = 0;
    virtual std::string getMathematicalEquation() const = 0;
    virtual std::string getDescription() const = 0;
};

// --- 1.6 MAIN SOURCES (منابع اصلی) ---

class GroundComponent : public Component {
public:
    std::string getComponentName() const override { return "Ground (GND)"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V = 0V"; }
    std::string getDescription() const override {
        return "Circuit voltage reference node. Required for simulation execution.";
    }
};

class DcSourceComponent : public Component {
public:
    std::string getComponentName() const override { return "DC Voltage Source"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V = Constant"; }
    std::string getDescription() const override {
        return "Ideal direct current voltage supply generating constant potential difference.";
    }
};

class BatteryComponent : public Component {
private:
    float internalResistance_{0.1f}; // Real-world simulation property
public:
    std::string getComponentName() const override { return "Battery"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V_terminal = E - I * R_internal"; }
    std::string getDescription() const override {
        return "DC voltage source modeled with real-world internal source resistance attributes.";
    }
    float getInternalResistance() const { return internalResistance_; }
    void setInternalResistance(float r) { internalResistance_ = r; }
};

class ClockGeneratorComponent : public Component {
private:
    float frequencyHz_{1000.0f};
    float highVoltage_{5.0f};
public:
    std::string getComponentName() const override { return "Clock Generator (Pulse)"; }
    ComponentClass getComponentClass() const override { return ComponentClass::MainSource; }
    std::string getMathematicalEquation() const override { return "V(t) = SquareWave(t, Freq)"; }
    std::string getDescription() const override {
        return "Digital signal source alternating uniformly between Logic '0' (0V) and Logic '1' (5V).";
    }
};

// --- 2.6 PASSIVE COMPONENTS (قطعات غیرفعال) ---

class ResistorComponent : public Component {
public:
    std::string getComponentName() const override { return "Resistor"; }
    ComponentClass getComponentClass() const override { return ComponentClass::Passive; }
    std::string getMathematicalEquation() const override { return "V = I * R"; }
    std::string getDescription() const override {
        return "Linear passive component implementing Ohm's Law relationship.";
    }
};

class CapacitorComponent : public Component {
public:
    std::string getComponentName() const override { return "Capacitor"; }
    ComponentClass getComponentClass() const override { return ComponentClass::Passive; }
    std::string getMathematicalEquation() const override { return "I = C * (dV / dt)"; }
    std::string getDescription() const override {
        return "Energy storage unit maintaining linear current-to-voltage derivative profiles.";
    }
};

class InductorComponent : public Component {
public:
    std::string getComponentName() const override { return "Inductor"; }
    ComponentClass getComponentClass() const override { return ComponentClass::Passive; }
    std::string getMathematicalEquation() const override { return "V = L * (dI / dt)"; }
    std::string getDescription() const override {
        return "Magnetic energy storage unit maintaining linear voltage-to-current derivative profiles.";
    }
};