#pragma once

struct Vec2 {
    float x;
    float y;

    Vec2 operator+(const Vec2& other) {
        return Vec2{x + other.x, y + other.y};
    }

    Vec2 operator+(const float& other) {
        return Vec2{x + other, y + other};
    }

    Vec2& operator+=(const Vec2& rhs) {
        this->x += rhs.x;
        this->y += rhs.y;
        return *this;
    }
    
    Vec2 operator-(const Vec2& other) {
        return Vec2{x - other.x, y - other.y};
    }

    Vec2 operator*(const float& other) {
        return Vec2{x * other, y * other};
    }

    Vec2 operator/(const float& other) {
        return Vec2{x / other, y / other};
    }

    Vec2 operator/(const Vec2& other) {
        return Vec2{x / other.x, y / other.y};
    }

    Vec2 operator*(const Vec2& other) {
        return Vec2{x * other.x, y * other.y};
    }
};