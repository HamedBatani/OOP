#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class ExternalMemory {
public:
    explicit ExternalMemory(std::size_t size = 256) : bytes_(size, 0) {}

    std::uint8_t read(std::size_t address) const {
        return address < bytes_.size() ? bytes_[address] : 0;
    }

    bool write(std::size_t address, std::uint8_t value) {
        if (address >= bytes_.size()) return false;
        bytes_[address] = value;
        return true;
    }

    std::size_t size() const { return bytes_.size(); }

private:
    std::vector<std::uint8_t> bytes_;
};
