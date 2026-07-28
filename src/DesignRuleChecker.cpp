#include "DesignRuleChecker.h"

#include <cmath>
#include <cstdlib>
#include <numeric>
#include <unordered_map>

namespace {
struct Sets {
    explicit Sets(std::size_t n) : parent(n) { std::iota(parent.begin(), parent.end(), 0); }
    int find(int x) { return parent[x] == x ? x : parent[x] = find(parent[x]); }
    void join(int a, int b) { a = find(a); b = find(b); if (a != b) parent[b] = a; }
    std::vector<int> parent;
};
std::string key(const std::string& component, const std::string& pin) { return component + "\x1f" + pin; }
float number(const std::string& text, float fallback) {
    char* end = nullptr; const float value = std::strtof(text.c_str(), &end);
    return end == text.c_str() || !std::isfinite(value) ? fallback : value;
}
bool sensitiveInput(const ComponentInstance& c, const std::string& pin) {
    if (c.type == "AND Gate" || c.type == "OR Gate" || c.type == "XOR Gate" || c.type == "NAND Gate") return pin.rfind("In", 0) == 0;
    if (c.type == "NOT Gate") return pin == "In";
    if (c.type == "Flip-Flop") return pin == "D" || pin == "CLK";
    if (c.type == "ADC") return pin == "Vin" || pin == "Vref+" || pin == "Vref-";
    if (c.type == "DAC") return pin.rfind("D", 0) == 0 || pin == "Vref+" || pin == "Vref-";
    if (c.type == "LCD 16x2" || c.type == "Colored LED" || c.type == "7-Segment Display") return true;
    if (c.type == "Microcontroller") return pin.rfind("PB", 0) == 0;
    return false;
}
}

DrcReport DesignRuleChecker::inspect(const std::vector<ComponentInstance>& components, const std::vector<Wire>& wires) {
    DrcReport report;
    Sets sets(wires.size());
    std::unordered_map<std::string, int> atPin, byUid;
    for (std::size_t i = 0; i < wires.size(); ++i) byUid[wires[i].uid] = static_cast<int>(i);
    auto attach = [&](int wire, const std::string& component, const std::string& pin) {
        if (component.empty() || pin.empty()) return;
        auto [it, inserted] = atPin.emplace(key(component, pin), wire);
        if (!inserted) sets.join(wire, it->second);
    };
    for (std::size_t i = 0; i < wires.size(); ++i) {
        attach(static_cast<int>(i), wires[i].startCompId, wires[i].startPinName);
        attach(static_cast<int>(i), wires[i].endCompId, wires[i].endPinName);
        if (wires[i].startAnchor.isWireLock()) { auto it = byUid.find(wires[i].startAnchor.lockedWireUid); if (it != byUid.end()) sets.join(static_cast<int>(i), it->second); }
        if (wires[i].endAnchor.isWireLock()) { auto it = byUid.find(wires[i].endAnchor.lockedWireUid); if (it != byUid.end()) sets.join(static_cast<int>(i), it->second); }
    }
    struct Driver { float voltage; std::string name; };
    std::unordered_map<int, std::vector<Driver>> drivers;
    auto addDriver = [&](const ComponentInstance& c, const char* pin, float voltage) {
        auto it = atPin.find(key(c.labelId, pin));
        if (it != atPin.end()) drivers[sets.find(it->second)].push_back({voltage, c.labelId + "." + pin});
    };
    for (const auto& c : components) {
        if (c.type == "Ground") addDriver(c, "GND", 0.0f);
        else if (c.type == "DC Source" || c.type == "Battery") { addDriver(c, "+", number(c.valueStr, 5.0f)); addDriver(c, "-", 0.0f); }
        else if (c.type == "Clock Generator") { addDriver(c, "+", c.interactiveStateBool ? 5.0f : 0.0f); addDriver(c, "-", 0.0f); }
    }
    for (const auto& entry : drivers) {
        const auto& list = entry.second;
        for (std::size_t i = 0; i < list.size(); ++i) for (std::size_t j = i + 1; j < list.size(); ++j) {
            if (std::fabs(list[i].voltage - list[j].voltage) > 0.25f) {
                report.messages.push_back({DrcSeverity::Error, "Short/conflicting drivers: " + list[i].name + " and " + list[j].name});
                ++report.errorCount;
            }
        }
    }
    for (const auto& c : components) for (const auto& p : c.pins) {
        if (sensitiveInput(c, p.designation) && atPin.find(key(c.labelId, p.designation)) == atPin.end()) {
            report.messages.push_back({DrcSeverity::Error, "Floating input: " + c.labelId + "." + p.designation});
            ++report.errorCount;
        }
    }
    report.canRun = report.errorCount == 0;
    if (report.messages.empty()) report.messages.push_back({DrcSeverity::Info, "DRC passed: no electrical errors found."});
    return report;
}
