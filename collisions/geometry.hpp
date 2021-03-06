#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include <array>
#include <cassert>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/vec3.hpp>
#include <iosfwd>
#include <optional>

namespace geom {

constexpr auto epsilon = glm::epsilon<float>();
constexpr auto epsilon2 = epsilon * epsilon;
constexpr auto pos_inf = std::numeric_limits<float>::infinity();
constexpr auto neg_inf = -pos_inf;

std::istream &operator>>(std::istream &is, glm::vec3 &vec);
std::ostream &operator<<(std::ostream &os, const glm::vec3 &vec);

std::ostream &operator<<(std::ostream &os, const glm::vec2 &vec);

class Range {
public:
  Range(float min, float max) : min_(min), max_(max) { assert(min_ <= max_); }
  bool intersects(const Range &other) const {
    if (other.max_ < min_ || other.min_ > max_)
      return false;
    return true;
  }
  void dump(std::ostream &os) const;

private:
  float min_, max_;
};

inline bool Intersects(const Range &range1, const Range &range2) {
  return range1.intersects(range2);
}

std::ostream &operator<<(std::ostream &os, const Range &range);

using Edge = std::pair<glm::vec3, glm::vec3>;
using Edge2D = std::pair<glm::vec2, glm::vec2>;
std::ostream &operator<<(std::ostream &os, const Edge &edge);
std::ostream &operator<<(std::ostream &os, const Edge2D &edge);

class Plane;

class Line {
public:
  Line() = default;
  Line(glm::vec3 point, glm::vec3 dir) : point_(point), dir_(dir) {
    assert(glm::length2(dir_) >= epsilon2);
  }
  glm::vec3 rotatePoint(glm::vec3 point, float angle) const;
  float getProjection(glm::vec3 point) const {
    return glm::dot(point - point_, dir_);
  }
  std::optional<float> getEdgeIntersection(const Edge &edge,
                                           const Plane &plane) const;
  void dump(std::ostream &os) const;
  void read(std::istream &is);

private:
  glm::vec3 point_, dir_;
};

std::ostream &operator<<(std::ostream &os, const Line &line);
std::istream &operator>>(std::istream &is, Line &line);

class Triangle {
public:
  Triangle() = default;
  Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) : p_({p1, p2, p3}) {}
  glm::vec3 getPoint(unsigned idx) const { return p_[idx]; }
  glm::vec3 getNormal() const {
    return glm::cross(p_[1] - p_[0], p_[2] - p_[0]);
  }
  bool isDegenerative() const { return glm::length2(getNormal()) <= epsilon2; }
  Range getIntersectionRange(const Line &line, const Plane &plane) const;
  void dump(std::ostream &os) const;
  void read(std::istream &is);

private:
  std::array<glm::vec3, 3> p_;
};

bool Intersects(const Triangle &tri1, const Triangle &tri2);

std::ostream &operator<<(std::ostream &os, const Triangle &triangle);
std::istream &operator>>(std::istream &is, Triangle &triangle);

class Triangle2D {
public:
  Triangle2D(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3) : p_({p1, p2, p3}) {}
  glm::vec2 getPoint(unsigned idx) const { return p_[idx]; }
  bool isInner(glm::vec2 p) const;
  bool isInner(const Triangle2D &other) const {
    return isInner(other.getPoint(0)) && isInner(other.getPoint(1)) &&
           isInner(other.getPoint(2));
  }
  void dump(std::ostream &os) const;

private:
  std::array<glm::vec2, 3> p_;
};

bool Intersects(const Triangle2D &tri1, const Triangle2D &tri2);

std::ostream &operator<<(std::ostream &os, const Triangle2D &triangle);

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
  enum class Axis : unsigned char { X, Y, Z };
  AAPlane(float pos, Axis axis) : pos_(pos), axis_(axis) {}
  float getDistance(glm::vec3 point) const {
    return point[static_cast<uint8_t>(axis_)] - pos_;
  }
  glm::vec2 getProjection(glm::vec3 p) {
    switch (axis_) {
    case Axis::X:
      return glm::vec2(p.y, p.z);
    case Axis::Y:
      return glm::vec2(p.x, p.z);
    case Axis::Z:
      return glm::vec2(p.x, p.y);
    }
    return glm::vec2{};
  }
  Triangle2D getProjection(const Triangle &tri) {
    return Triangle2D(getProjection(tri.getPoint(0)),
                      getProjection(tri.getPoint(1)),
                      getProjection(tri.getPoint(2)));
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
  glm::vec3 getNormal() const { return normal_; }
  std::optional<Line> intersect(const Plane &other) const;

private:
  glm::vec3 point_, normal_;
};

inline std::optional<Line> Intersect(const Plane &pln1, const Plane &pln2) {
  return pln1.intersect(pln2);
}

} // namespace geom

#endif
