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
  os << "[" << min_ << ", " << max_ << "]";
}

std::ostream &operator<<(std::ostream &os, const Range &range) {
  range.dump(os);
  return os;
}

std::optional<float> Line::getEdgeIntersection(const Edge &edge,
                                               const Plane &plane) const {
  float fstDistance = plane.getDistance(edge.first),
        sndDistance = plane.getDistance(edge.second);

  if (std::abs(fstDistance) < epsilon && std::abs(sndDistance) < epsilon)
    return std::nullopt;
  if ((fstDistance > epsilon && sndDistance > epsilon) ||
      (fstDistance < -epsilon && sndDistance < -epsilon))
    return std::nullopt;
  float fstProjection = getProjection(edge.first),
        sndProjection = getProjection(edge.second);
  return (fstProjection * sndDistance - sndProjection * fstDistance) /
         (sndDistance - fstDistance);
}

void Line::dump(std::ostream &os) const {
  os << "(" << point_ << " + " << dir_ << " * t)";
}

std::ostream &operator<<(std::ostream &os, const Line &line) {
  line.dump(os);
  return os;
}

Range Triangle::getIntersectionRange(const Line &line,
                                     const Plane &plane) const {
  float min = pos_inf, max = neg_inf;
  for (auto edge : {Edge{p_[0], p_[1]}, Edge{p_[1], p_[2]}, Edge{p_[2], p_[0]}})
    if (auto intersectionPt = line.getEdgeIntersection(edge, plane)) {
#ifndef NDEBUG
      std::cerr << "Edge (" << edge.first << ", " << edge.second
                << ") intersects in point " << *intersectionPt << '\n';
#endif
      min = std::min(min, *intersectionPt);
      max = std::max(max, *intersectionPt);
    }
  return Range(min, max);
}

void Triangle::dump(std::ostream &os) const {
  os << "(" << p_[0] << ", " << p_[1] << ", " << p_[2] << ")";
}

void Triangle::read(std::istream &is) { is >> p_[0] >> p_[1] >> p_[2]; }

bool Intersects(const Triangle &tri1, const Triangle &tri2) {
#ifndef NDEBUG
  std::cerr << "Checking triangles:\n" << tri1 << '\n' << tri2 << '\n';
#endif
  Plane pln1(tri1), pln2(tri2);
  if (pln1.isFront(tri2) || pln1.isBack(tri2) || pln2.isFront(tri1) ||
      pln2.isBack(tri1)) {
#ifndef NDEBUG
    std::cerr << "Fully front or back, not intersecting\n";
#endif
    return false;
  }
  if (auto line = Intersect(pln1, pln2)) {
    // non-complanar triangles intersection
#ifndef NDEBUG
    std::cerr << "Non-complanar, intersection line: " << *line << '\n';
#endif
    Range rng1 = tri1.getIntersectionRange(*line, pln2),
          rng2 = tri2.getIntersectionRange(*line, pln1);
#ifndef NDEBUG
    std::cerr << "fst range: " << rng1 << '\n' << "snd range: " << rng2 << '\n';
#endif
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
