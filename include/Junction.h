// include/Junction.h
// این فایل جدید است و برای حل مشکل "جدا بودن نقطه junction از گره منطقی واقعی" اضافه شده.
// هیچ فایل موجودی توسط این فایل حذف یا کم نمی‌شود؛ صرفاً یک ساختار داده جدید برای Wire.h فراهم می‌کند.
#pragma once

#include "Point.h"
#include <string>
#include <utility>

// نوع اتصال دو سر یک وایر:
// - Free      : نقطه آزاد (مثل قبل، فقط یک Point خام؛ رفتار قدیمی حفظ شده)
// - PinLock   : قفل شده روی یک Pin واقعی از یک ComponentInstance (با کامپوننت‌آیدی + نام پین)
// - WireLock  : قفل شده روی یک نقطه از یک Wire دیگر (اتصال گره به گره / Node-to-Node)
//               که با ایندکس وایر مقصد + پارامتر مکان (index سگمنت و ضریب t) شناسایی می‌شود.
enum class AnchorKind {
    Free,
    PinLock,
    WireLock
};

struct WireAnchor {
    AnchorKind kind{AnchorKind::Free};

    // برای PinLock
    std::string lockedCompId;
    std::string lockedPinName;

    // برای WireLock: به کدام وایر (با شناسه‌ی یکتای خودش، نه ایندکس -- چون ایندکس با حذف/جابجایی بی‌اعتبار می‌شود)
    std::string lockedWireUid;
    // موقعیت روی آن وایر: ایندکس سگمنت (بین routingPoints[segmentIndex] و routingPoints[segmentIndex+1])
    // و پارامتر t در بازه [0,1] که نسبت فاصله از ابتدای سگمنت را مشخص می‌کند.
    int lockedSegmentIndex{0};
    float lockedSegmentT{0.0f};

    // آخرین موقعیت resolve شده (کش می‌شود تا در صورت نیاز به موقعیت فعلی بدون دسترسی مستقیم به لیست وایرها هم چیزی موجود باشد)
    Point cachedWorldPos{0.0f, 0.0f};

    WireAnchor() = default;

    static WireAnchor makeFree(const Point& p) {
        WireAnchor a;
        a.kind = AnchorKind::Free;
        a.cachedWorldPos = p;
        return a;
    }

    static WireAnchor makePinLock(std::string compId, std::string pinName, const Point& p) {
        WireAnchor a;
        a.kind = AnchorKind::PinLock;
        a.lockedCompId = std::move(compId);
        a.lockedPinName = std::move(pinName);
        a.cachedWorldPos = p;
        return a;
    }

    static WireAnchor makeWireLock(std::string wireUid, int segmentIndex, float t, const Point& p) {
        WireAnchor a;
        a.kind = AnchorKind::WireLock;
        a.lockedWireUid = std::move(wireUid);
        a.lockedSegmentIndex = segmentIndex;
        a.lockedSegmentT = t;
        a.cachedWorldPos = p;
        return a;
    }

    bool isFree() const { return kind == AnchorKind::Free; }
    bool isPinLock() const { return kind == AnchorKind::PinLock; }
    bool isWireLock() const { return kind == AnchorKind::WireLock; }
};