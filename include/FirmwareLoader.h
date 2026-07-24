#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdint>
#include <iomanip>

class FirmwareLoader {
public:
    struct FlashMemory {
        std::vector<uint8_t> memoryArray;

        FlashMemory(size_t defaultSize = 65536) { // پیش‌فرض 64 کیلوبایت
            memoryArray.assign(defaultSize, 0xFF);
        }
    };

    // متد اصلی برای باز کردن، خواندن، اعتبارسنجی و استخراج داده‌ها
    static bool loadHexFile(const std::string& filename, FlashMemory& flash) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[FirmwareLoader] Error: Could not open HEX file -> " << filename << "\n";
            return false;
        }

        std::string line;
        int lineNumber = 0;
        uint32_t baseAddress = 0;

        while (std::getline(file, line)) {
            lineNumber++;

            // پاک‌سازی کاراکترهای اضافی (مثل Carriage Return در ویندوز)
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;

            // --- 1. اعتبارسنجی اولیه ساختار (بررسی شروع با ':') ---
            if (line[0] != ':') {
                std::cerr << "[FirmwareLoader] Error: Invalid start code at line " << lineNumber << "\n";
                return false;
            }

            // حداقل طول یک خط HEX (بدون داده) 11 کاراکتر است: :llaaaattcc
            if (line.length() < 11) {
                std::cerr << "[FirmwareLoader] Error: Line too short at line " << lineNumber << "\n";
                return false;
            }

            // لامبدا برای تبدیل سریع رشته هگزادسیمال به عدد صحیح
            auto parseHex = [](const std::string& str) -> uint32_t {
                uint32_t val;
                std::stringstream ss;
                ss << std::hex << str;
                ss >> val;
                return val;
            };

            uint8_t byteCount = parseHex(line.substr(1, 2));

            // --- 2. بررسی تطابق طول خط با تعداد بایت اعلام شده ---
            if (line.length() != static_cast<size_t>(11 + (byteCount * 2))) {
                std::cerr << "[FirmwareLoader] Error: Length mismatch at line " << lineNumber << "\n";
                return false;
            }

            uint16_t address = parseHex(line.substr(3, 4));
            uint8_t recordType = parseHex(line.substr(7, 2));

            // --- 3. بررسی دقیق Checksum (اعتبارسنجی هگز) ---
            // مجموع تمام بایت‌های خط (از طول داده تا خود چک‌سام) باید برابر صفر (modulo 256) شود.
            uint8_t checksum = 0;
            for (size_t i = 1; i < line.length(); i += 2) {
                checksum += parseHex(line.substr(i, 2));
            }
            if (checksum != 0) {
                std::cerr << "[FirmwareLoader] Error: Checksum validation failed at line " << lineNumber << "\n";
                return false;
            }

            // --- 4. استخراج کدهای باینری (Opcodeها) و داده‌ها ---
            if (recordType == 0x00) { // Data Record (شامل کدهای اجرایی و متغیرها)
                for (int i = 0; i < byteCount; ++i) {
                    uint8_t dataByte = parseHex(line.substr(9 + (i * 2), 2));
                    uint32_t physicalAddress = baseAddress + address + i;

                    // مدیریت پویای سایز حافظه (اگر برنامه بزرگتر از پیش‌فرض بود)
                    if (physicalAddress >= flash.memoryArray.size()) {
                        flash.memoryArray.resize(physicalAddress + 2048, 0xFF);
                    }

                    // بارگذاری مستقیم درون آرایه حافظه فلش
                    flash.memoryArray[physicalAddress] = dataByte;
                }
            }
            else if (recordType == 0x01) { // End of File (EOF)
                break;
            }
            else if (recordType == 0x02) { // Extended Segment Address (آدرس‌دهی بیشتر از 64KB)
                baseAddress = parseHex(line.substr(9, 4)) * 16;
            }
            else if (recordType == 0x04) { // Extended Linear Address (آدرس‌دهی 32 بیتی)
                baseAddress = parseHex(line.substr(9, 4)) << 16;
            }
        }

        file.close();
        std::cout << "[FirmwareLoader] Success: Firmware loaded successfully. Flash size: " << flash.memoryArray.size() << " bytes.\n";
        return true;
    }
};