// include/MicrocontrollerCore.h
#pragma once

#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include "FirmwareLoader.h"
#include "IOPort.h"

// کدهای عملیاتی آپدیت شده شامل دستورات ورودی و خروجی پورت‌ها
enum Opcodes : uint8_t {
    MOV_REG_IMM = 0x10,
    MOV_REG_REG = 0x11,
    ADD_ACC_IMM = 0x20,
    ADD_ACC_REG = 0x21,
    JMP         = 0x30,
    SETB        = 0x40,
    CLR         = 0x41,
    OUT_PORTA   = 0x50, // OUT PortA, Reg (نوشتن مقدار ثبات روی پورت A)
    IN_PORTA    = 0x51  // IN Reg, PortA  (خواندن پورت A و ذخیره در ثبات)
};

class MicrocontrollerCore {
private:
    uint16_t pc_{0};
    uint8_t accumulator_{0};
    std::vector<uint8_t> generalPurposeRegs_;
    std::vector<uint8_t> ram_;
    std::vector<bool> ioPins_;
    FirmwareLoader::FlashMemory flashMemory_;
    bool isRunning_{false};
    bool firmwareLoaded_{false};

public:
    // --- پیاده‌سازی شیءگرا پورت‌ها (بند 6.7) ---
    IOPort portA_{"Port A"};
    IOPort portB_{"Port B"};

    explicit MicrocontrollerCore(size_t ramSize = 2048, size_t numRegisters = 32, size_t numPins = 16)
            : generalPurposeRegs_(numRegisters, 0), ram_(ramSize, 0), ioPins_(numPins, false) {}

    uint16_t getPC() const { return pc_; }
    void setPC(uint16_t address) { pc_ = address; }

    uint8_t getAccumulator() const { return accumulator_; }
    void setAccumulator(uint8_t val) { accumulator_ = val; }

    uint8_t getRegister(size_t index) const {
        if (index < generalPurposeRegs_.size()) return generalPurposeRegs_[index];
        return 0;
    }

    void setRegister(size_t index, uint8_t val) {
        if (index < generalPurposeRegs_.size()) generalPurposeRegs_[index] = val;
    }

    bool getPinState(size_t pinIndex) const {
        if (pinIndex < ioPins_.size()) return ioPins_[pinIndex];
        return false;
    }

    bool loadHexFirmware(const std::string& hexFilePath) {
        bool success = FirmwareLoader::loadHexFile(hexFilePath, flashMemory_);
        firmwareLoaded_ = success;
        if (success) reset();
        return success;
    }

    void start() { if (firmwareLoaded_) isRunning_ = true; }
    void stop() { isRunning_ = false; }
    bool isRunning() const { return isRunning_; }
    bool hasFirmware() const { return firmwareLoaded_; }

    void reset() {
        pc_ = 0;
        accumulator_ = 0;
        std::fill(generalPurposeRegs_.begin(), generalPurposeRegs_.end(), 0);
        std::fill(ram_.begin(), ram_.end(), 0);
        std::fill(ioPins_.begin(), ioPins_.end(), false);
        portA_.write(0); // ریست کردن پورت خروجی
        portB_.write(0);
        isRunning_ = false;
        std::cout << "[MCU] System Reset. Ready to execute.\n";
    }

    void stepInstruction() {
        if (!isRunning_ || pc_ >= flashMemory_.memoryArray.size()) {
            isRunning_ = false;
            return;
        }

        uint8_t opcode = flashMemory_.memoryArray[pc_];

        switch (opcode) {
            case MOV_REG_IMM: {
                if (static_cast<size_t>(pc_) + 2 >= flashMemory_.memoryArray.size()) { stop(); return; }
                uint8_t destReg = flashMemory_.memoryArray[pc_ + 1];
                uint8_t value   = flashMemory_.memoryArray[pc_ + 2];
                setRegister(destReg, value);
                pc_ += 3;
                break;
            }
            case MOV_REG_REG: {
                if (static_cast<size_t>(pc_) + 2 >= flashMemory_.memoryArray.size()) { stop(); return; }
                uint8_t destReg = flashMemory_.memoryArray[pc_ + 1];
                uint8_t srcReg  = flashMemory_.memoryArray[pc_ + 2];
                setRegister(destReg, getRegister(srcReg));
                pc_ += 3;
                break;
            }
            case ADD_ACC_IMM: {
                if (static_cast<size_t>(pc_) + 1 >= flashMemory_.memoryArray.size()) { stop(); return; }
                accumulator_ += flashMemory_.memoryArray[pc_ + 1];
                pc_ += 2;
                break;
            }
            case ADD_ACC_REG: {
                if (static_cast<size_t>(pc_) + 1 >= flashMemory_.memoryArray.size()) { stop(); return; }
                accumulator_ += getRegister(flashMemory_.memoryArray[pc_ + 1]);
                pc_ += 2;
                break;
            }
            case JMP: {
                if (static_cast<size_t>(pc_) + 2 >= flashMemory_.memoryArray.size()) { stop(); return; }
                pc_ = (flashMemory_.memoryArray[pc_ + 1] << 8) | flashMemory_.memoryArray[pc_ + 2];
                break;
            }
            case SETB: {
                if (static_cast<size_t>(pc_) + 1 >= flashMemory_.memoryArray.size()) { stop(); return; }
                uint8_t targetPin = flashMemory_.memoryArray[pc_ + 1];
                if (targetPin < ioPins_.size()) ioPins_[targetPin] = true;
                pc_ += 2;
                break;
            }
            case CLR: {
                if (static_cast<size_t>(pc_) + 1 >= flashMemory_.memoryArray.size()) { stop(); return; }
                uint8_t targetPin = flashMemory_.memoryArray[pc_ + 1];
                if (targetPin < ioPins_.size()) ioPins_[targetPin] = false;
                pc_ += 2;
                break;
            }
                // ----------------------------------------------------
                // پیاده‌سازی دستورات مدیریت پورت (بند 6.7)
                // ----------------------------------------------------
            case OUT_PORTA: {
                if (static_cast<size_t>(pc_) + 1 >= flashMemory_.memoryArray.size()) { stop(); return; }
                uint8_t srcReg = flashMemory_.memoryArray[pc_ + 1];
                portA_.write(getRegister(srcReg)); // مقدار ثبات را روی پورت A قرار می‌دهد
                pc_ += 2;
                break;
            }
            case IN_PORTA: {
                if (static_cast<size_t>(pc_) + 1 >= flashMemory_.memoryArray.size()) { stop(); return; }
                uint8_t destReg = flashMemory_.memoryArray[pc_ + 1];
                setRegister(destReg, portA_.read()); // پین‌های فیزیکی را خوانده و در ثبات ذخیره می‌کند
                pc_ += 2;
                break;
            }
            default: {
                std::cerr << "[MCU] Warning: Unknown Opcode 0x" << std::hex << (int)opcode << " at PC: 0x" << pc_ << "\n";
                pc_ += 1;
                break;
            }
        }
    }
};
