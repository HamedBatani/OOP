#pragma once

#include <algorithm>

#include "Grid.h"
#include "Point.h"

class Canvas {
public:
    Canvas(float width, float height, const Grid& grid = Grid{})
            : width_{width},
              height_{height},
              grid_{grid} {}

    float width() const {
        return width_;
    }

    float height() const {
        return height_;
    }

    void setSize(float width, float height) {
        width_ = width;
        height_ = height;
    }

    float zoom() const {
        return zoom_;
    }

    void setZoom(float zoom) {
        zoom_ = std::max(zoom, minZoom_);
    }

    void zoomBy(float factor) {
        if (factor > 0.0f) {
            setZoom(zoom_ * factor);
        }
    }

    Point cameraPosition() const {
        return cameraPosition_;
    }

    void setCameraPosition(const Point& position) {
        cameraPosition_ = position;
    }

    void pan(const Point& delta) {
        cameraPosition_ += delta;
    }

    Point mouseScreenPosition() const {
        return mouseScreenPosition_;
    }

    void setMouseScreenPosition(const Point& position) {
        mouseScreenPosition_ = position;
    }

    Point mouseWorldPosition() const {
        return screenToWorld(mouseScreenPosition_);
    }

    const Grid& grid() const {
        return grid_;
    }

    Grid& grid() {
        return grid_;
    }

    void setGrid(const Grid& grid) {
        grid_ = grid;
    }

    Point screenToWorld(const Point& screenPoint) const {
        return (screenPoint / zoom_) + cameraPosition_;
    }

    Point worldToScreen(const Point& worldPoint) const {
        return (worldPoint - cameraPosition_) * zoom_;
    }

    Point snapToGrid(const Point& worldPoint) const {
        return grid_.snap(worldPoint);
    }

    float snapToGrid(float worldCoordinate) const {
        return grid_.snap(worldCoordinate);
    }

private:
    static constexpr float minZoom_{0.01f};

    float width_{0.0f};
    float height_{0.0f};
    float zoom_{1.0f};
    Point cameraPosition_{0.0f, 0.0f};
    Point mouseScreenPosition_{0.0f, 0.0f};
    Grid grid_{};
};