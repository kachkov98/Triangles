#include "geometry.hpp"
#include <iostream>

namespace geom {

std::istream &operator>>(std::istream &is, glm::vec3 &vec) {
  is >> vec.x >> vec.y >> vec.z;
  return is;
}

std::ostream &operator<<(std::ostream &os, const glm::vec3 &vec) {
  os << "[x: " << vec.x << " y: " << vec.y << " z: " << vec.z << "]";
  return os;
}
void Range::dump(std::ostream &os) const {
  os << "[" << start_ << ", " << finish_ << "]";
}

std::ostream &operator<<(std::ostream &os, const Range &range) {
  range.dump(os);
  return os;
}

void Line::dump(std::ostream &os) const {
  os << "(" << point_ << " + " << dir_ << " * t)";
}

std::ostream &operator<<(std::ostream &os, const Line &line) {
  line.dump(os);
  return os;
}

Range Triangle::getIntersectionRange(const Line &line) const {
  std::array<float, 3> projections = {line.getProjection(p_[0]),
                                      line.getProjection(p_[1]),
                                      line.getProjection(p_[2])};
  std::array<float, 3> distances = {line.getDistance(p_[0]),
                                    line.getDistance(p_[1]),
                                    line.getDistance(p_[2])};
  return Range({0.0f, 1.0f});
}

void Triangle::dump(std::ostream &os) const {
  os << "(" << p_[0] << ", " << p_[1] << ", " << p_[2] << ")";
}

void Triangle::read(std::istream &is) { is >> p_[0] >> p_[1] >> p_[2]; }

bool Intersects(const Triangle &tri1, const Triangle &tri2) {
  Plane pln1(tri1), pln2(tri2);
  if (pln1.isFront(tri2) || pln1.isBack(tri2) || pln2.isFront(tri1) ||
      pln2.isBack(tri1))
    return false;
  if (auto line = Intersect(pln1, pln2)) {
    // non-complanar triangles intersection
    Range rng1 = tri1.getIntersectionRange(*line),
          rng2 = tri2.getIntersectionRange(*line);
    return Intersects(rng1, rng2);
  }
  // TODO: complanar triangles intersection
  return false;
}

std::ostream &operator<<(std::ostream &os, const Triangle &triangle) {
  triangle.dump(os);
  return os;
}

std::istream &operator>>(std::istream &is, Triangle &triangle) {
  triangle.read(is);
  return is;
}

std::optional<Line> Plane::intersect(const Plane &other) const {
  glm::vec3 dir = glm::cross(normal_, other.normal_);
  auto det = glm::length2(dir);
  if (det < epsilon2)
    return std::nullopt;
  glm::vec3 point(glm::cross(dir, normal_) *
                      glm::dot(other.point_, other.normal_) -
                  glm::cross(dir, other.normal_) * glm::dot(point_, normal_));
  point /= det;
  return Line(point, dir);
}
} // namespace geom
