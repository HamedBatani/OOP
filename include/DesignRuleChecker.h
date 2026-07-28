#pragma once

#include <string>
#include <vector>
#include "ComponentInstance.h"
#include "Wire.h"

enum class DrcSeverity { Info, Warning, Error };
struct DrcMessage { DrcSeverity severity{DrcSeverity::Info}; std::string text; };
struct DrcReport {
    std::vector<DrcMessage> messages;
    bool canRun{true};
    int errorCount{0};
    int warningCount{0};
};

class DesignRuleChecker {
public:
    static DrcReport inspect(const std::vector<ComponentInstance>& components,
                             const std::vector<Wire>& wires);
};
