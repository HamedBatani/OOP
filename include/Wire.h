// include/Wire.h
#pragma once
#include "Point.h"
#include <string>
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>

class Wire {
public:
    std::string startCompId;
    std::string startPinName;
    std::string endCompId;
    std::string endPinName;

    // گره‌های مسیر سیم (برای رسم خطوط شکسته 90 درجه)
    std::vector<Point> routingPoints;
    bool isCompleted{false};
    bool isSelected{false}; // <--- وضعیت انتخاب شدن سیم (برای بخش ۵.۵)

    Wire(std::string sComp, std::string sPin, Point startPos)
            : startCompId(std::move(sComp)), startPinName(std::move(sPin)) {
        routingPoints.push_back(startPos);
    }

    // الگوریتم مسیریابی 90 درجه (Orthogonal Routing)
    void updateOrthogonalRoute(Point startPos, Point endPos) {
        routingPoints.clear();
        routingPoints.push_back(startPos);

        Point elbow{endPos.x, startPos.y};
        if (startPos.x != endPos.x && startPos.y != endPos.y) {
            routingPoints.push_back(elbow);
        }
        routingPoints.push_back(endPos);
    }

    // بررسی اینکه آیا موس روی این سیم کلیک کرده یا نه (تشخیص برخورد پاره‌خط)
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
};