#pragma once

struct Vec2 {
	float x;
	float y;

	static Vec2 one() {
		return { 1, 1 };
	}

	Vec2& operator+=(const Vec2& rhs) {
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
};

inline Vec2 operator+(const Vec2& lhs, const Vec2& rhs) {
	return Vec2{ lhs.x + rhs.x, lhs.y + rhs.y };
}

inline Vec2 operator-(const Vec2& lhs, const Vec2& rhs) {
	return Vec2{ lhs.x - rhs.x, lhs.y - rhs.y };
}

inline Vec2 operator*(const float& rhs, const Vec2& lhs) {
	return Vec2{ lhs.x * rhs, lhs.y * rhs };
}

inline Vec2 operator*(const Vec2& lhs, const float& rhs) {
	return Vec2{ lhs.x * rhs, lhs.y * rhs };
}

inline Vec2 operator/(const Vec2& lhs, const float& rhs) {
	return Vec2{ lhs.x / rhs, lhs.y / rhs };
}

inline Vec2 operator*(const Vec2& lhs, const Vec2& rhs) {
	return Vec2{ lhs.x * rhs.x, lhs.y * rhs.y };
}

inline Vec2 operator/(const Vec2& lhs, const Vec2& rhs) {
	return Vec2{ lhs.x / rhs.x, lhs.y / rhs.y };
}

inline Vec2 linear_interp(Vec2 p1, Vec2 p2, float t) {
    return p1 + (p2 - p1) * t;
}

inline float linear_interp(float a, float b, float t) {
    return a + (b - a) * t;
}
