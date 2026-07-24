// include/IOPort.h
#pragma once
#include <cstdint>
#include <string>
#include <utility>

class IOPort {
private:
    std::string portName_;
    uint8_t outputRegister_{0}; // رجیستر خروجی (چیزی که میکرو می‌نویسد)
    uint8_t inputRegister_{0};  // رجیستر ورودی (چیزی که از پایه‌های فیزیکی خوانده می‌شود)

public:
    explicit IOPort(std::string name) : portName_(std::move(name)) {}

    // ----------------------------------------------------
    // متدهای سمت میکروکنترلر (دنیای داخل آی‌سی)
    // ----------------------------------------------------
    void write(uint8_t data) {
        outputRegister_ = data;
        // در چرخه شبیه‌سازی بوم (Canvas)، این مقدار روی پین‌های گرافیکی قطعه اعمال می‌شود
    }

    uint8_t read() const {
        return inputRegister_;
    }

    // ----------------------------------------------------
    // متدهای سمت شبیه‌ساز گرافیکی (دنیای خارج آی‌سی)
    // ----------------------------------------------------
    void setExternalPins(uint8_t externalData) {
        inputRegister_ = externalData;
    }

    uint8_t getExternalPins() const {
        return outputRegister_;
    }

    const std::string& getName() const { return portName_; }
};