// include/Wire.h
#pragma once
#include "Point.h"
#include "Junction.h"
#include "Component.h"
#include <string>
#include <vector>
#include <utility>
#include <cmath>
#include <algorithm>
#include <sstream>

class Wire {
public:
    std::string startCompId;
    std::string startPinName;
    std::string endCompId;
    std::string endPinName;

    std::vector<Point> routingPoints;
    bool isCompleted{false};
    bool isSelected{false};

    // وضعیت‌های شبیه‌سازی زنده (دیجیتال و آنالوگ)
    DigitalState currentLogicState{DigitalState::Undefined};
    float currentVoltage{0.0f};

    std::string uid;
    WireAnchor startAnchor;
    WireAnchor endAnchor;

    bool isFullyFree() const {
        return startAnchor.isFree() && endAnchor.isFree();
    }

    Wire(std::string sComp, std::string sPin, Point startPos)
            : startCompId(std::move(sComp)), startPinName(std::move(sPin)) {
        routingPoints.push_back(startPos);
        uid = generateUid();

        if (!startCompId.empty() && !startPinName.empty()) {
            startAnchor = WireAnchor::makePinLock(startCompId, startPinName, startPos);
        } else {
            startAnchor = WireAnchor::makeFree(startPos);
        }
        endAnchor = WireAnchor::makeFree(startPos);
    }

    Wire() {
        uid = generateUid();
    }

    static std::string generateUid() {
        static unsigned long long counter = 0;
        ++counter;
        std::ostringstream oss;
        oss << "wire_" << counter;
        return oss.str();
    }

    void updateOrthogonalRoute(Point startPos, Point endPos) {
        routingPoints.clear();
        routingPoints.push_back(startPos);
        Point elbow{endPos.x, startPos.y};
        if (startPos.x != endPos.x && startPos.y != endPos.y) {
            routingPoints.push_back(elbow);
        }
        routingPoints.push_back(endPos);

        if (!routingPoints.empty()) {
            startAnchor.cachedWorldPos = routingPoints.front();
            endAnchor.cachedWorldPos = routingPoints.back();
        }
    }

    void updateOrthogonalRouteKeepAnchors(Point startPos, Point endPos) {
        updateOrthogonalRoute(startPos, endPos);
    }

    void rerouteSmartPreservingShape(Point newStartPos, Point newEndPos) {
        if (routingPoints.size() < 4) {
            updateOrthogonalRoute(newStartPos, newEndPos);
            return;
        }

        Point oldStartPos = routingPoints.front();
        Point oldEndPos = routingPoints.back();

        bool startMoved = std::hypot(newStartPos.x - oldStartPos.x, newStartPos.y - oldStartPos.y) > 0.5f;
        bool endMoved = std::hypot(newEndPos.x - oldEndPos.x, newEndPos.y - oldEndPos.y) > 0.5f;

        if (!startMoved && !endMoved) return;

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
            updateOrthogonalRoute(newStartPos, newEndPos);
            return;
        }

        if (startMoved && !endMoved) {
            Point oldP1 = routingPoints[1];
            Point p1New;
            bool firstSegmentWasVertical = std::abs(oldStartPos.x - oldP1.x) < 0.5f;
            if (firstSegmentWasVertical) p1New = { newStartPos.x, oldP1.y };
            else p1New = { oldP1.x, newStartPos.y };

            routingPoints[0] = newStartPos;
            routingPoints[1] = p1New;
            startAnchor.cachedWorldPos = routingPoints.front();
            return;
        }

        if (!startMoved && endMoved) {
            size_t lastIdx = routingPoints.size() - 1;
            size_t secondLastIdx = lastIdx - 1;
            Point oldPSecondLast = routingPoints[secondLastIdx];
            bool lastSegmentWasVertical = std::abs(oldEndPos.x - oldPSecondLast.x) < 0.5f;

            Point pSecondLastNew;
            if (lastSegmentWasVertical) pSecondLastNew = { newEndPos.x, oldPSecondLast.y };
            else pSecondLastNew = { oldPSecondLast.x, newEndPos.y };

            routingPoints[lastIdx] = newEndPos;
            routingPoints[secondLastIdx] = pSecondLastNew;
            endAnchor.cachedWorldPos = routingPoints.back();
            return;
        }
    }

    bool containsPoint(const Point& p, float tolerance = 5.0f) const {
        if (routingPoints.size() < 2) return false;
        for (size_t i = 0; i < routingPoints.size() - 1; ++i) {
            Point a = routingPoints[i]; Point b = routingPoints[i+1];
            float l2 = (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
            if (l2 == 0.0f) { if (std::hypot(p.x - a.x, p.y - a.y) <= tolerance) return true; continue; }
            float t = std::max(0.0f, std::min(1.0f, ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2));
            float projX = a.x + t * (b.x - a.x); float projY = a.y + t * (b.y - a.y);
            if (std::hypot(p.x - projX, p.y - projY) <= tolerance) return true;
        }
        return false;
    }

    struct ClosestPointResult {
        bool found{false}; int segmentIndex{0}; float t{0.0f}; Point point{0.0f, 0.0f}; float distance{0.0f};
    };

    ClosestPointResult findClosestPointOnWire(const Point& p) const {
        ClosestPointResult best;
        if (routingPoints.size() < 2) return best;
        best.distance = 1e9f;

        for (size_t i = 0; i < routingPoints.size() - 1; ++i) {
            Point a = routingPoints[i]; Point b = routingPoints[i + 1];
            float l2 = (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
            float t = 0.0f; Point proj = a;
            if (l2 > 0.0f) {
                t = std::max(0.0f, std::min(1.0f, ((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2));
                proj = { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
            }
            float dist = std::hypot(p.x - proj.x, p.y - proj.y);
            if (dist < best.distance) { best.found = true; best.distance = dist; best.segmentIndex = static_cast<int>(i); best.t = t; best.point = proj; }
        }
        return best;
    }

    Point resolvePointOnSegment(int segmentIndex, float t) const {
        if (routingPoints.empty()) return {0.0f, 0.0f};
        if (segmentIndex < 0) segmentIndex = 0;
        if (segmentIndex >= static_cast<int>(routingPoints.size()) - 1) segmentIndex = static_cast<int>(routingPoints.size()) - 2;
        if (segmentIndex < 0) return routingPoints.front();
        const Point& a = routingPoints[segmentIndex]; const Point& b = routingPoints[segmentIndex + 1];
        return { a.x + t * (b.x - a.x), a.y + t * (b.y - a.y) };
    }
};