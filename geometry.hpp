#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include "glm/common.hpp"
#include "glm/geometric.hpp"
#include "glm/gtx/norm.hpp"
#include "glm/vec3.hpp"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <iosfwd>
#include <optional>

namespace geom {

constexpr auto epsilon = glm::epsilon<float>();
constexpr auto epsilon2 = epsilon * epsilon;

std::istream &operator>>(std::istream &is, glm::vec3 &vec);
std::ostream &operator<<(std::ostream &os, const glm::vec3 &vec);

class Range {
public:
  Range(std::initializer_list<float> points)
      : start_(std::min(points)), finish_(std::max(points)) {}
  bool intersects(const Range &other) const {
    if (other.finish_ < start_ || other.start_ > finish_)
      return false;
    return true;
  }
  void dump(std::ostream &os) const;

private:
  float start_, finish_;
};

inline bool Intersects(const Range &range1, const Range &range2) {
  return range1.intersects(range2);
}

std::ostream &operator<<(std::ostream &os, const Range &range);

class Line {
public:
  Line(glm::vec3 point, glm::vec3 dir) : point_(point), dir_(dir) {
    assert(glm::length2(dir_) >= epsilon2);
  }
  float getProjection(glm::vec3 point) const {
    return glm::dot(point - point_, dir_);
  }
  float getDistance(glm::vec3 point) const {
    return glm::length(glm::cross(point - point_, dir_));
  }
  void dump(std::ostream &os) const;

private:
  glm::vec3 point_, dir_;
};

std::ostream &operator<<(std::ostream &os, const Line &line);

class Triangle {
public:
  Triangle() = default;
  Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) : p_({p1, p2, p3}) {}
  glm::vec3 getPoint(unsigned idx) const { return p_[idx]; }
  glm::vec3 getNormal() const {
    return glm::cross(p_[1] - p_[0], p_[2] - p_[0]);
  }
  bool isDegenerative() const { return glm::length2(getNormal()) <= epsilon2; }
  Range getIntersectionRange(const Line &line) const;
  void dump(std::ostream &os) const;
  void read(std::istream &is);

private:
  std::array<glm::vec3, 3> p_;
};

bool Intersects(const Triangle &tri1, const Triangle &tri2);

std::ostream &operator<<(std::ostream &os, const Triangle &triangle);
std::istream &operator>>(std::istream &is, Triangle &triangle);

template <class Derived> class PlaneBase {
public:
  bool isFront(glm::vec3 point) const {
    return static_cast<const Derived *>(this)->getDistance(point) > epsilon;
  };
  bool isBack(glm::vec3 point) const {
    return static_cast<const Derived *>(this)->getDistance(point) < -epsilon;
  };
  bool isCoplanar(glm::vec3 point) const {
    return glm::abs(static_cast<const Derived *>(this)->getDistance(point)) <=
           epsilon;
  }
  bool isFront(const Triangle &tri) const {
    return isFront(tri.getPoint(0)) && isFront(tri.getPoint(1)) &&
           isFront(tri.getPoint(2));
  }
  bool isBack(const Triangle &tri) const {
    return isBack(tri.getPoint(0)) && isBack(tri.getPoint(1)) &&
           isBack(tri.getPoint(2));
  }
  bool isCoplanar(const Triangle &tri) const {
    return isCoplanar(tri.getPoint(0)) && isCoplanar(tri.getPoint(1)) &&
           isCoplanar(tri.getPoint(2));
  }
};

class AAPlane : public PlaneBase<AAPlane> {
public:
  enum class Axis : uint8_t { X, Y, Z };
  AAPlane(float pos, Axis axis) : pos_(pos), axis_(axis) {}
  float getDistance(glm::vec3 point) const {
    return point[static_cast<uint8_t>(axis_)] - pos_;
  }

private:
  float pos_;
  Axis axis_;
};

class Plane : public PlaneBase<Plane> {
public:
  Plane(glm::vec3 point, glm::vec3 normal) : point_(point), normal_(normal) {
    assert(glm::length2(normal) > epsilon2);
  }
  Plane(const Triangle &tri)
      : point_(tri.getPoint(0)), normal_(tri.getNormal()) {
    assert(!tri.isDegenerative());
  }
  float getDistance(glm::vec3 point) const {
    return glm::dot(point - point_, normal_);
  }
  std::optional<Line> intersect(const Plane &other) const;

private:
  glm::vec3 point_, normal_;
};

inline std::optional<Line> Intersect(const Plane &pln1, const Plane &pln2) {
  return pln1.intersect(pln2);
}

} // namespace geom

#endif
