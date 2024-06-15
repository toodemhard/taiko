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