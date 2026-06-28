#include "CanvasRenderer.h"

#include <iomanip>
#include <ostream>

namespace {
    void printPoint(std::ostream& output, const Point& point) {
        output << '(' << point.x << ", " << point.y << ')';
    }
}

CanvasRenderer::CanvasRenderer(const Canvas& canvas)
        : canvas_{canvas} {}

void CanvasRenderer::render(std::ostream& output) const {
    renderState(output);
    output << '\n';
    renderGridPreview(output);
    output << '\n';
    renderSnapTest(output, canvas_.mouseScreenPosition());
}

void CanvasRenderer::renderState(std::ostream& output) const {
    const Point cameraOffset = canvas_.cameraPosition();
    const Point mouseScreen = canvas_.mouseScreenPosition();
    const Point mouseWorld = canvas_.mouseWorldPosition();

    output << std::fixed << std::setprecision(2);
    output << "Canvas State\n";
    output << "------------\n";
    output << "Camera Offset: ";
    printPoint(output, cameraOffset);
    output << '\n';
    output << "Zoom Level: " << canvas_.zoom() << "x\n";
    output << "Mouse Screen Coordinate: ";
    printPoint(output, mouseScreen);
    output << '\n';
    output << "Mouse World Coordinate: ";
    printPoint(output, mouseWorld);
    output << '\n';
}

void CanvasRenderer::renderGridPreview(std::ostream& output, int columns, int rows) const {
    if (columns < 3) {
        columns = 3;
    }

    if (rows < 3) {
        rows = 3;
    }

    const int centerColumn = columns / 2;
    const int centerRow = rows / 2;

    output << "Grid Preview\n";
    output << "------------\n";

    if (!canvas_.grid().isVisible()) {
        output << "Grid is hidden.\n";
        return;
    }

    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            if (row == centerRow && column == centerColumn) {
                output << '+';
            } else if (row == centerRow) {
                output << '-';
            } else if (column == centerColumn) {
                output << '|';
            } else if (row % 2 == 0 && column % 4 == 0) {
                output << '.';
            } else {
                output << ' ';
            }
        }
        output << '\n';
    }

    output << "Each dot represents a visible grid intersection.\n";
}

void CanvasRenderer::renderSnapTest(std::ostream& output, const Point& rawMouseScreenPoint) const {
    const Point rawWorldPoint = canvas_.screenToWorld(rawMouseScreenPoint);
    const Point snappedWorldPoint = canvas_.snapToGrid(rawWorldPoint);
    const Point snappedScreenPoint = canvas_.worldToScreen(snappedWorldPoint);

    output << "Snap To Grid Test\n";
    output << "-----------------\n";
    output << "Raw Mouse Screen: ";
    printPoint(output, rawMouseScreenPoint);
    output << '\n';
    output << "Raw Mouse World: ";
    printPoint(output, rawWorldPoint);
    output << '\n';
    output << "Snapped World: ";
    printPoint(output, snappedWorldPoint);
    output << '\n';
    output << "Snapped Screen: ";
    printPoint(output, snappedScreenPoint);
    output << '\n';
    output << "Grid Spacing: " << canvas_.grid().spacing() << '\n';
}