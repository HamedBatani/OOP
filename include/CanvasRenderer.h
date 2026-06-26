#pragma once

#include <iosfwd>

#include "Canvas.h"
#include "Point.h"

class CanvasRenderer {
public:
    explicit CanvasRenderer(const Canvas& canvas);

    void render(std::ostream& output) const;
    void renderState(std::ostream& output) const;
    void renderGridPreview(std::ostream& output, int columns = 21, int rows = 11) const;
    void renderSnapTest(std::ostream& output, const Point& rawMouseScreenPoint) const;

private:
    const Canvas& canvas_;
};