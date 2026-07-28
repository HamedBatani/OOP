#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "ProjectManager.h"

class ProjectHistory {
public:
    void reset(const std::vector<std::string>& active, const std::vector<ComponentInstance>& components, const std::vector<Wire>& wires) {
        states_.assign(1, ProjectManager::serialize(active, components, wires)); index_ = 0;
    }
    void record(const std::vector<std::string>& active, const std::vector<ComponentInstance>& components, const std::vector<Wire>& wires) {
        const std::string next = ProjectManager::serialize(active, components, wires);
        if (!states_.empty() && states_[index_] == next) return;
        states_.erase(states_.begin() + static_cast<std::ptrdiff_t>(index_ + 1), states_.end());
        states_.push_back(next); index_ = states_.size() - 1;
        if (states_.size() > 100) { states_.erase(states_.begin()); --index_; }
    }
    bool canUndo() const { return !states_.empty() && index_ > 0; }
    bool canRedo() const { return !states_.empty() && index_ + 1 < states_.size(); }
    bool undo(std::vector<std::string>& active, std::vector<ComponentInstance>& components, std::vector<Wire>& wires) {
        if (!canUndo()) return false;
        --index_;
        return ProjectManager::deserialize(states_[index_], active, components, wires);
    }
    bool redo(std::vector<std::string>& active, std::vector<ComponentInstance>& components, std::vector<Wire>& wires) {
        if (!canRedo()) return false;
        ++index_;
        return ProjectManager::deserialize(states_[index_], active, components, wires);
    }
private:
    std::vector<std::string> states_; std::size_t index_{0};
};
