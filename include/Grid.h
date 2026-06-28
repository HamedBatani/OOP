//grid.h
#pragma once

#include <cmath>

#include "Point.h"

class Grid {
public:
    explicit Grid(float spacing = 32.0f, bool visible = true, bool snapEnabled = true)
            : spacing_{spacing > 0.0f ? spacing : 1.0f},
              visible_{visible},
              snapEnabled_{snapEnabled} {}

    float spacing() const {
        return spacing_;
    }

    void setSpacing(float spacing) {
        if (spacing > 0.0f) {
            spacing_ = spacing;
        }
    }

    bool isVisible() const {
        return visible_;
    }

    void setVisible(bool visible) {
        visible_ = visible;
    }

    bool isSnapEnabled() const {
        return snapEnabled_;
    }

    void setSnapEnabled(bool enabled) {
        snapEnabled_ = enabled;
    }

    float snap(float coordinate) const {
        if (!snapEnabled_) {
            return coordinate;
        }

        return std::round(coordinate / spacing_) * spacing_;
    }

    Point snap(const Point& point) const {
        return {snap(point.x), snap(point.y)};
    }

private:
    float spacing_{32.0f};
    bool visible_{true};
    bool snapEnabled_{true};
};