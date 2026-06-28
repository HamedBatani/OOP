//canvas.cpp
#pragma once

#include "Grid.h"
#include "Point.h"

class Canvas {
public:
    Canvas(float width, float height, const Grid& grid = Grid{});

    float width() const;
    float height() const;
    void setSize(float width, float height);

    float zoom() const;
    void setZoom(float zoom);
    void zoomBy(float factor);
    void zoomAt(const Point& screenPoint, float factor);

    Point cameraPosition() const;
    void setCameraPosition(const Point& position);
    void setCameraOffset(const Point& offset);
    void pan(const Point& screenDelta);

    Point mouseScreenPosition() const;
    void setMouseScreenPosition(const Point& position);
    Point mouseWorldPosition() const;

    const Grid& grid() const;
    Grid& grid();
    void setGrid(const Grid& grid);

    Point screenToWorld(const Point& screenPoint) const;
    Point worldToScreen(const Point& worldPoint) const;

    Point snapToGrid(const Point& worldPoint) const;
    float snapToGrid(float worldCoordinate) const;

private:
    static constexpr float minZoom_{0.01f};

    float width_{0.0f};
    float height_{0.0f};
    float zoom_{1.0f};
    Point cameraPosition_{0.0f, 0.0f};
    Point mouseScreenPosition_{0.0f, 0.0f};
    Grid grid_{};
};