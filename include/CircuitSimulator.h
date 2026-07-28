#pragma once

#include <vector>
#include <string>
#include <cstdint>

#include "ComponentInstance.h"
#include "Wire.h"

// Resolves the drawable wires into electrical nets and advances the simplified
// mixed-signal model used by parts 6-8 of the project.
class CircuitSimulator {
public:
    static bool clockLevelAt(std::uint64_t simulationTimeMs, const std::string& frequencyHz);

    static void step(std::vector<ComponentInstance>& components,
                     std::vector<Wire>& wires,
                     bool advanceSequential = true);

    static void reset(std::vector<ComponentInstance>& components,
                      std::vector<Wire>& wires);
};
