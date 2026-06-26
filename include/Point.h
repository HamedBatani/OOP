#pragma once

struct Point {
    float x{0.0f};
    float y{0.0f};

    constexpr Point() = default;
    constexpr Point(float xValue, float yValue) : x{xValue}, y{yValue} {}

    constexpr Point operator+(const Point& other) const {
        return {x + other.x, y + other.y};
    }

    constexpr Point operator-(const Point& other) const {
        return {x - other.x, y - other.y};
    }

    constexpr Point operator*(float scalar) const {
        return {x * scalar, y * scalar};
    }

    constexpr Point operator/(float scalar) const {
        return {x / scalar, y / scalar};
    }

    Point& operator+=(const Point& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Point& operator-=(const Point& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
};