#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>
#include <SDL3/SDL.h>
#include "Point.h"

// Orthogonal obstacle-avoiding router.  It builds a visibility graph from the
// clearance edges of every symbol, then chooses the shortest route while
// mildly penalising bends.  This permits any number of bends and is therefore
// not dependent on a catalogue of special L/Z-shaped cases.
class SmartWireRouter {
public:
    static std::vector<Point> route(Point start, Point end, Point startDirection, Point endDirection,
                                    const std::vector<SDL_FRect>& obstacles, float clearance = 18.0f) {
        constexpr float margin = 8.0f;
        const bool hasStartDirection = directionPresent(startDirection);
        const bool hasEndDirection = directionPresent(endDirection);
        const Point startStub = hasStartDirection
            ? Point{start.x + startDirection.x * clearance, start.y + startDirection.y * clearance} : start;
        const Point endStub = hasEndDirection
            ? Point{end.x + endDirection.x * clearance, end.y + endDirection.y * clearance} : end;

        std::vector<SDL_FRect> expanded;
        std::vector<float> xs{startStub.x, endStub.x};
        std::vector<float> ys{startStub.y, endStub.y};
        float minX = std::min(startStub.x, endStub.x), maxX = std::max(startStub.x, endStub.x);
        float minY = std::min(startStub.y, endStub.y), maxY = std::max(startStub.y, endStub.y);
        for (const auto& box : obstacles) {
            SDL_FRect e{box.x - margin, box.y - margin, box.w + 2.0f * margin, box.h + 2.0f * margin};
            expanded.push_back(e);
            xs.push_back(e.x); xs.push_back(e.x + e.w);
            ys.push_back(e.y); ys.push_back(e.y + e.h);
            minX = std::min(minX, e.x); maxX = std::max(maxX, e.x + e.w);
            minY = std::min(minY, e.y); maxY = std::max(maxY, e.y + e.h);
        }
        const float outer = std::max(40.0f, clearance * 2.0f);
        xs.push_back(minX - outer); xs.push_back(maxX + outer);
        ys.push_back(minY - outer); ys.push_back(maxY + outer);
        sortUnique(xs); sortUnique(ys);

        const int width = static_cast<int>(xs.size());
        const int height = static_cast<int>(ys.size());
        const int nodeCount = width * height;
        auto node = [width](int xi, int yi) { return yi * width + xi; };
        auto pointFor = [&](int n) { return Point{xs[n % width], ys[n / width]}; };
        std::vector<bool> valid(nodeCount, false);
        for (int yi = 0; yi < height; ++yi)
            for (int xi = 0; xi < width; ++xi)
                valid[node(xi, yi)] = !insideAny({xs[xi], ys[yi]}, expanded);

        const int startNode = node(indexOf(xs, startStub.x), indexOf(ys, startStub.y));
        const int endNode = node(indexOf(xs, endStub.x), indexOf(ys, endStub.y));
        // A free preview endpoint can temporarily sit over a component.  Keep
        // routing responsive there; locked pins have their stubs outside.
        valid[startNode] = true; valid[endNode] = true;

        struct Entry { float cost; int state; bool operator>(const Entry& rhs) const { return cost > rhs.cost; } };
        constexpr int none = 0, horizontal = 1, vertical = 2;
        const int stateCount = nodeCount * 3;
        std::vector<float> distance(stateCount, std::numeric_limits<float>::infinity());
        std::vector<int> previous(stateCount, -1);
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> queue;
        const int initialState = startNode * 3 + none;
        distance[initialState] = 0.0f; queue.push({0.0f, initialState});

        auto relax = [&](int state, int nextNode, int nextDirection, float length) {
            const int currentNode = state / 3;
            const int oldDirection = state % 3;
            const Point delta{pointFor(nextNode).x - pointFor(currentNode).x,
                              pointFor(nextNode).y - pointFor(currentNode).y};
            if (currentNode == startNode && hasStartDirection && dot(delta, startDirection) < -0.01f) return;
            if (nextNode == endNode && hasEndDirection) {
                const Point outward{-delta.x, -delta.y};
                if (dot(outward, endDirection) < -0.01f) return;
            }
            const float bendCost = oldDirection != none && oldDirection != nextDirection ? 12.0f : 0.0f;
            const int nextState = nextNode * 3 + nextDirection;
            const float candidate = distance[state] + length + bendCost;
            if (candidate + 0.001f < distance[nextState]) {
                distance[nextState] = candidate; previous[nextState] = state;
                queue.push({candidate, nextState});
            }
        };

        while (!queue.empty()) {
            const Entry current = queue.top(); queue.pop();
            if (current.cost > distance[current.state] + 0.001f) continue;
            const int currentNode = current.state / 3;
            const int xi = currentNode % width, yi = currentNode / width;
            const Point from = pointFor(currentNode);
            const int dx[4]{-1, 1, 0, 0}, dy[4]{0, 0, -1, 1};
            for (int k = 0; k < 4; ++k) {
                const int nx = xi + dx[k], ny = yi + dy[k];
                if (nx < 0 || nx >= width || ny < 0 || ny >= height) continue;
                const int next = node(nx, ny);
                if (!valid[next]) continue;
                const Point to = pointFor(next);
                if (!segmentClear(from, to, expanded)) continue;
                const int dir = dx[k] != 0 ? horizontal : vertical;
                relax(current.state, next, dir, std::fabs(to.x - from.x) + std::fabs(to.y - from.y));
            }
        }

        int finalState = -1;
        for (int dir : {none, horizontal, vertical}) {
            const int candidate = endNode * 3 + dir;
            if (finalState < 0 || distance[candidate] < distance[finalState]) finalState = candidate;
        }
        if (finalState < 0 || !std::isfinite(distance[finalState]))
            return fallback(start, end, startStub, endStub, minX - outer, minY - outer);

        std::vector<Point> middle;
        for (int state = finalState; state >= 0; state = previous[state]) middle.push_back(pointFor(state / 3));
        std::reverse(middle.begin(), middle.end());
        std::vector<Point> result;
        result.push_back(start);
        result.insert(result.end(), middle.begin(), middle.end());
        result.push_back(end);
        simplify(result, hasStartDirection, hasEndDirection);
        return result;
    }

private:
    static bool directionPresent(Point d) { return std::fabs(d.x) + std::fabs(d.y) > 0.01f; }
    static float dot(Point a, Point b) { return a.x * b.x + a.y * b.y; }
    static void sortUnique(std::vector<float>& values) {
        std::sort(values.begin(), values.end());
        values.erase(std::unique(values.begin(), values.end(), [](float a, float b) { return std::fabs(a - b) < 0.01f; }), values.end());
    }
    static int indexOf(const std::vector<float>& values, float value) {
        for (int i = 0; i < static_cast<int>(values.size()); ++i) if (std::fabs(values[i] - value) < 0.01f) return i;
        return 0;
    }
    static bool strictlyInside(Point p, const SDL_FRect& r) {
        constexpr float eps = 0.01f;
        return p.x > r.x + eps && p.x < r.x + r.w - eps && p.y > r.y + eps && p.y < r.y + r.h - eps;
    }
    static bool insideAny(Point p, const std::vector<SDL_FRect>& boxes) {
        for (const auto& box : boxes) if (strictlyInside(p, box)) return true;
        return false;
    }
    static bool segmentClear(Point a, Point b, const std::vector<SDL_FRect>& boxes) {
        constexpr float eps = 0.01f;
        for (const auto& r : boxes) {
            if (std::fabs(a.y - b.y) < eps) {
                if (a.y > r.y + eps && a.y < r.y + r.h - eps &&
                    std::max(a.x, b.x) > r.x + eps && std::min(a.x, b.x) < r.x + r.w - eps) return false;
            } else if (std::fabs(a.x - b.x) < eps) {
                if (a.x > r.x + eps && a.x < r.x + r.w - eps &&
                    std::max(a.y, b.y) > r.y + eps && std::min(a.y, b.y) < r.y + r.h - eps) return false;
            } else return false;
        }
        return true;
    }
    static std::vector<Point> fallback(Point start, Point end, Point startStub, Point endStub, float outsideX, float outsideY) {
        std::vector<Point> points{start, startStub, {outsideX, startStub.y}, {outsideX, outsideY},
                                  {endStub.x, outsideY}, endStub, end};
        simplify(points, directionPresent({startStub.x - start.x, startStub.y - start.y}),
                 directionPresent({endStub.x - end.x, endStub.y - end.y}));
        return points;
    }
    static void simplify(std::vector<Point>& points, bool preserveStartStub, bool preserveEndStub) {
        points.erase(std::unique(points.begin(), points.end(), [](Point a, Point b) { return a.distanceTo(b) < 0.01f; }), points.end());
        for (std::size_t i = 1; i + 1 < points.size();) {
            if ((preserveStartStub && i == 1) || (preserveEndStub && i + 2 == points.size())) { ++i; continue; }
            const bool collinear = (std::fabs(points[i - 1].x - points[i].x) < 0.01f && std::fabs(points[i].x - points[i + 1].x) < 0.01f) ||
                                   (std::fabs(points[i - 1].y - points[i].y) < 0.01f && std::fabs(points[i].y - points[i + 1].y) < 0.01f);
            if (collinear) points.erase(points.begin() + static_cast<std::ptrdiff_t>(i)); else ++i;
        }
    }
};
