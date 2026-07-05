// include/Wire.h
#pragma once
#include "Point.h"
#include "Junction.h"
#include <string>
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>
#include <sstream>

class Wire {
public:
    // ------------------------------------------------------------------
    // اعضای اصلی قبلی -- هیچ‌کدام حذف نشده‌اند، دقیقاً مثل نسخه‌ی قبلی باقی مانده‌اند
    // ------------------------------------------------------------------
    std::string startCompId;
    std::string startPinName;
    std::string endCompId;
    std::string endPinName;

    // گره‌های مسیر سیم (برای رسم خطوط شکسته 90 درجه)
    std::vector<Point> routingPoints;
    bool isCompleted{false};
    bool isSelected{false}; // <--- وضعیت انتخاب شدن سیم (برای بخش ۵.۵)

    // ------------------------------------------------------------------
    // اعضای جدید -- برای حل مشکل جدایی junctionها از گراف منطقی واقعی
    // (بند ۱ و ۲ درخواست: وقتی وایر دومی از وسط وایر اولی شروع می‌شود،
    //  این دو باید واقعاً به هم گره بخورند نه فقط از نظر بصری هم‌مکان باشند)
    // ------------------------------------------------------------------

    // شناسه‌ی یکتای این وایر؛ مستقل از ایندکس در vector<Wire> (که ممکن است با حذف/جابجایی عوض شود)
    std::string uid;

    // انکر (گره‌ی اتصال) دو سر وایر. اگر استفاده نشوند (پیش‌فرض Free) دقیقاً معادل رفتار قبلی هستند.
    WireAnchor startAnchor;
    WireAnchor endAnchor;

    // آیا این وایر توسط کاربر آزادانه قابل جابجایی کامل است؟
    // طبق تصمیم گرفته‌شده: فقط وایری که هر دو سرش آزاد باشد (Free) قابل درگ کامل است.
    bool isFullyFree() const {
        return startAnchor.isFree() && endAnchor.isFree();
    }

    Wire(std::string sComp, std::string sPin, Point startPos)
            : startCompId(std::move(sComp)), startPinName(std::move(sPin)) {
        routingPoints.push_back(startPos);
        uid = generateUid();

        // اگر با یک پین واقعی مقداردهی اولیه شده، انکر شروع را هم قفل کن (رفتار قدیمی startCompId/startPinName حفظ می‌شود)
        if (!startCompId.empty() && !startPinName.empty()) {
            startAnchor = WireAnchor::makePinLock(startCompId, startPinName, startPos);
        } else {
            startAnchor = WireAnchor::makeFree(startPos);
        }
        endAnchor = WireAnchor::makeFree(startPos);
    }

    // سازنده‌ی پیش‌فرض هم برای سازگاری با کدهایی که ممکن است vector<Wire> بدون مقدار اولیه بسازند نگه داشته می‌شود
    Wire() {
        uid = generateUid();
    }

    // تولید یک شناسه‌ی یکتای ساده بر اساس آدرس حافظه + شمارنده‌ی استاتیک (کافی برای طول عمر این پروسه)
    static std::string generateUid() {
        static unsigned long long counter = 0;
        ++counter;
        std::ostringstream oss;
        oss << "wire_" << counter;
        return oss.str();
    }

    // الگوریتم مسیریابی 90 درجه (Orthogonal Routing) -- دقیقاً مثل قبل، بدون تغییر
    void updateOrthogonalRoute(Point startPos, Point endPos) {
        routingPoints.clear();
        routingPoints.push_back(startPos);

        Point elbow{endPos.x, startPos.y};
        if (startPos.x != endPos.x && startPos.y != endPos.y) {
            routingPoints.push_back(elbow);
        }
        routingPoints.push_back(endPos);

        // کش کردن موقعیت انکرها هم‌زمان با آپدیت مسیر (برای پایداری بین فریم‌ها)
        if (!routingPoints.empty()) {
            startAnchor.cachedWorldPos = routingPoints.front();
            endAnchor.cachedWorldPos = routingPoints.back();
        }
    }

    // نسخه‌ی جدید: آپدیت مسیر با حفظ انکر شروع یا پایان (بدون بازنویسی نوع انکر)
    // این متد جدید است و متد قبلی updateOrthogonalRoute دست‌نخورده باقی مانده است.
    void updateOrthogonalRouteKeepAnchors(Point startPos, Point endPos) {
        updateOrthogonalRoute(startPos, endPos);
    }

    // ------------------------------------------------------------------
    // متد جدید: rerouteSmartPreservingShape
    // حل مشکل «به‌هم‌ریختن روتینگ»: به‌جای بازسازی کامل مسیر با یک خم
    // ساده‌ی ثابت (کاری که updateOrthogonalRoute انجام می‌دهد)، این متد
    // "شکل نسبی" مسیر قبلی را حفظ می‌کند -- یعنی اگر وایر قبلاً از سه
    // بخش تشکیل شده بود (عمودی-افقی-عمودی، طبق مثال دقیقی که در گفتگو
    // توضیح داده شد: از X=0 بالا رفته، روی Y=50 افقی رفته، بعد از X=90
    // پایین آمده)، وقتی فقط یکی از دو سر جابجا می‌شود، تنها بخش نزدیک
    // به همان سر تغییر می‌کند و بقیه‌ی بخش‌ها (segmentهای میانی) دست‌نخورده
    // می‌مانند مگر اینکه واقعاً لازم باشد.
    //
    // منطق: اگر تعداد routingPoints فعلی >= 4 باشد (یعنی حداقل یک
    // segment میانی افقی/عمودی وجود دارد که هر دو سرش به startPos/endPos
    // قدیمی وصل نبوده)، فقط segment اول (نزدیک start) و segment آخر
    // (نزدیک end) با توجه به موقعیت جدید کشیده می‌شوند و segmentهای
    // میانی با همان "سطح" (همان مختصات x یا y ثابتشان) حفظ می‌شوند.
    // ------------------------------------------------------------------
    void rerouteSmartPreservingShape(Point newStartPos, Point newEndPos) {
        // اگر مسیر قبلی خیلی ساده بود (کمتر از ۴ نقطه، یعنی صفر یا یک خم)،
        // همان رفتار قدیمی ساده کافی و درست است.
        if (routingPoints.size() < 4) {
            updateOrthogonalRoute(newStartPos, newEndPos);
            return;
        }

        // ذخیره‌ی موقعیت‌های قدیمی برای مقایسه
        Point oldStartPos = routingPoints.front();
        Point oldEndPos = routingPoints.back();

        bool startMoved = std::hypot(newStartPos.x - oldStartPos.x, newStartPos.y - oldStartPos.y) > 0.5f;
        bool endMoved = std::hypot(newEndPos.x - oldEndPos.x, newEndPos.y - oldEndPos.y) > 0.5f;

        // اگر هیچ‌کدام حرکت نکرده‌اند، کاری لازم نیست
        if (!startMoved && !endMoved) {
            return;
        }

        // اگر هر دو سر حرکت کرده‌اند (مثلاً کل وایر با هم جابجا شده)،
        // ساده‌ترین و امن‌ترین کار این است که کل مسیر را با همان دلتای
        // یکسان جابجا کنیم (شکل کاملاً حفظ می‌شود چون هندسه‌ی نسبی عوض نشده)
        if (startMoved && endMoved) {
            Point deltaStart{newStartPos.x - oldStartPos.x, newStartPos.y - oldStartPos.y};
            Point deltaEnd{newEndPos.x - oldEndPos.x, newEndPos.y - oldEndPos.y};
            bool sameDelta = std::hypot(deltaStart.x - deltaEnd.x, deltaStart.y - deltaEnd.y) < 0.5f;
            if (sameDelta) {
                for (auto& pt : routingPoints) {
                    pt.x += deltaStart.x;
                    pt.y += deltaStart.y;
                }
                startAnchor.cachedWorldPos = routingPoints.front();
                endAnchor.cachedWorldPos = routingPoints.back();
                return;
            }
            // اگر دلتاها متفاوت باشند (کشش نامتقارن)، به روش امن‌تر (بازسازی کامل) برمی‌گردیم
            updateOrthogonalRoute(newStartPos, newEndPos);
            return;
        }

        // حالت اصلی مورد نظر کاربر: فقط یکی از دو سر حرکت کرده است.
        // مثال دقیق از گفتگو: مسیر routingPoints = [P0(start), P1, P2, P3(end)]
        // که segment اول عمودی از P0 به P1 است (x ثابت = x شروع)،
        // segment میانی افقی از P1 به P2 است (y ثابت = سطح مشترک)،
        // segment آخر عمودی از P2 به P3 است (x ثابت = x پایان).
        // وقتی فقط start حرکت کند، فقط P0 و P1 باید آپدیت شوند (P1 با
        // همان y سطح مشترک قبلی ولی x جدید بر اساس newStartPos)،
        // و P2, P3 (بخش دوم و سوم طبق توضیح کاربر) دست‌نخورده می‌مانند.
        if (startMoved && !endMoved) {
            // سطح مشترک قدیمی (y یا x segment میانی) را از P1 می‌گیریم
            Point oldP1 = routingPoints[1];
            Point p1New;

            // تشخیص جهت segment اول (عمودی یا افقی بوده)
            bool firstSegmentWasVertical = std::abs(oldStartPos.x - oldP1.x) < 0.5f;

            if (firstSegmentWasVertical) {
                // segment اول عمودی بود: x باید با newStartPos.x برابر شود،
                // y همان سطح مشترک قبلی (oldP1.y) باقی می‌ماند
                p1New = { newStartPos.x, oldP1.y };
            } else {
                // segment اول افقی بود: y باید با newStartPos.y برابر شود،
                // x همان سطح مشترک قبلی (oldP1.x) باقی می‌ماند
                p1New = { oldP1.x, newStartPos.y };
            }

            routingPoints[0] = newStartPos;
            routingPoints[1] = p1New;
            // بقیه‌ی نقاط (P2, P3, ...) دست‌نخورده می‌مانند -- این دقیقاً
            // همان چیزی است که کاربر خواسته بود: «بخش اول و دوم که میشه
            // از X۰ تا X۱ و از X۹۰ تا X۱۰۰ باید ثابت بمونن»
            startAnchor.cachedWorldPos = routingPoints.front();
            return;
        }

        if (!startMoved && endMoved) {
            size_t lastIdx = routingPoints.size() - 1;
            size_t secondLastIdx = lastIdx - 1;
            Point oldPSecondLast = routingPoints[secondLastIdx];

            bool lastSegmentWasVertical = std::abs(oldEndPos.x - oldPSecondLast.x) < 0.5f;

            Point pSecondLastNew;
            if (lastSegmentWasVertical) {
                pSecondLastNew = { newEndPos.x, oldPSecondLast.y };
            } else {
                pSecondLastNew = { oldPSecondLast.x, newEndPos.y };
            }

            routingPoints[lastIdx] = newEndPos;
            routingPoints[secondLastIdx] = pSecondLastNew;
            // نقاط ابتدایی (P0, P1, ...) دست‌نخورده می‌مانند
            endAnchor.cachedWorldPos = routingPoints.back();
            return;
        }
    }

    // بررسی اینکه آیا موس روی این سیم کلیک کرده یا نه (تشخیص برخورد پاره‌خط) -- بدون تغییر
    bool containsPoint(const Point& p, float tolerance = 5.0f) const {
        if (routingPoints.size() < 2) return false;

        for (size_t i = 0; i < routingPoints.size() - 1; ++i) {
            Point a = routingPoints[i];
            Point b = routingPoints[i+1];

            // محاسبه مجذور طول پاره‌خط
            float l2 = (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
            if (l2 == 0.0f) {
                if (std::hypot(p.x - a.x, p.y - a.y) <= tolerance) return true;
                continue;
            }

            // پیدا کردن نزدیک‌ترین نقطه روی پاره‌خط به موس (Projection)
            float t = std::max(0.0f, std::min(1.0f, ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2));
            float projX = a.x + t * (b.x - a.x);
            float projY = a.y + t * (b.y - a.y);

            // اگر فاصله موس تا سیم کمتر از تلورانس بود، یعنی کلیک شده!
            if (std::hypot(p.x - projX, p.y - projY) <= tolerance) {
                return true;
            }
        }
        return false;
    }

    // ------------------------------------------------------------------
    // متدهای جدید کمکی برای پیدا کردن نزدیک‌ترین نقطه روی این وایر به یک نقطه‌ی دلخواه
    // (لازم برای ساخت WireLock هنگام اتصال یک وایر جدید به وسط این وایر)
    // خروجی: segmentIndex و t و خود نقطه‌ی روی خط، به‌اضافه فاصله
    // ------------------------------------------------------------------
    struct ClosestPointResult {
        bool found{false};
        int segmentIndex{0};
        float t{0.0f};
        Point point{0.0f, 0.0f};
        float distance{0.0f};
    };

    ClosestPointResult findClosestPointOnWire(const Point& p) const {
        ClosestPointResult best;
        if (routingPoints.size() < 2) return best;

        best.distance = 1e9f;

        for (size_t i = 0; i < routingPoints.size() - 1; ++i) {
            Point a = routingPoints[i];
            Point b = routingPoints[i + 1];

            float l2 = (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
            float t = 0.0f;
            Point proj = a;

            if (l2 > 0.0f) {
                t = std::max(0.0f, std::min(1.0f, ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2));
                proj = { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
            }

            float dist = std::hypot(p.x - proj.x, p.y - proj.y);
            if (dist < best.distance) {
                best.found = true;
                best.distance = dist;
                best.segmentIndex = static_cast<int>(i);
                best.t = t;
                best.point = proj;
            }
        }

        return best;
    }

    // محاسبه‌ی موقعیت واقعی یک انکر از نوع WireLock بر اساس segmentIndex/t فعلی این وایر
    // (فراخوانی می‌شود از روی وایر مقصد -- یعنی خود همین Wire همان "وایر میزبان" جانکشن است)
    Point resolvePointOnSegment(int segmentIndex, float t) const {
        if (routingPoints.empty()) return {0.0f, 0.0f};
        if (segmentIndex < 0) segmentIndex = 0;
        if (segmentIndex >= static_cast<int>(routingPoints.size()) - 1) {
            segmentIndex = static_cast<int>(routingPoints.size()) - 2;
        }
        if (segmentIndex < 0) return routingPoints.front();

        const Point& a = routingPoints[segmentIndex];
        const Point& b = routingPoints[segmentIndex + 1];
        return { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
    }
};