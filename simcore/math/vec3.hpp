#pragma once
#include <cmath>
#include <algorithm>
#include <ostream>

namespace minerva {

struct Vec3 {
  double x{}, y{}, z{};
  constexpr Vec3() = default;
  constexpr Vec3(double X, double Y, double Z) : x(X), y(Y), z(Z) {}

  static constexpr Vec3 zero() { return Vec3(0.0, 0.0, 0.0); }
  static constexpr Vec3 unit_x() { return Vec3(1.0, 0.0, 0.0); }
  static constexpr Vec3 unit_y() { return Vec3(0.0, 1.0, 0.0); }
  static constexpr Vec3 unit_z() { return Vec3(0.0, 0.0, 1.0); }

  Vec3 operator+(const Vec3& b) const { return {x+b.x, y+b.y, z+b.z}; }
  Vec3 operator-(const Vec3& b) const { return {x-b.x, y-b.y, z-b.z}; }
  Vec3 operator*(double s) const { return {x*s, y*s, z*s}; }
  Vec3 operator/(double s) const { return {x/s, y/s, z/s}; }
  Vec3& operator+=(const Vec3& b){ x+=b.x; y+=b.y; z+=b.z; return *this; }
  Vec3& operator-=(const Vec3& b){ x-=b.x; y-=b.y; z-=b.z; return *this; }
  Vec3& operator*=(double s){ x*=s; y*=s; z*=s; return *this; }

  double dot(const Vec3& b) const { return x*b.x + y*b.y + z*b.z; }
  Vec3 cross(const Vec3& b) const {
    return { y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x };
  }
  double norm2() const { return this->dot(*this); }
  double norm() const { return std::sqrt(norm2()); }
  Vec3 normalized() const { double n = norm(); return n>0?(*this)/n:*this; }
};

inline Vec3 operator*(double s, const Vec3& v) { return v*s; }
inline std::ostream& operator<<(std::ostream& os, const Vec3& v){
  return os << "(" << v.x << "," << v.y << "," << v.z << ")";
}

} // namespace minerva
