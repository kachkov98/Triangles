#include "geometry.hpp"
#include <glm/gtc/quaternion.hpp>
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

std::ostream &operator<<(std::ostream &os, const glm::vec2 &vec) {
  os << "[x: " << vec.x << " y: " << vec.y << "]";
  return os;
}

std::ostream &operator<<(std::ostream &os, const Edge &edge) {
  os << "{" << edge.first << ", " << edge.second << "}";
  return os;
}

std::ostream &operator<<(std::ostream &os, const Edge2D &edge) {
  os << "{" << edge.first << ", " << edge.second << "}";
  return os;
}

void Range::dump(std::ostream &os) const {
  os << "[" << min_ << ", " << max_ << "]";
}

std::ostream &operator<<(std::ostream &os, const Range &range) {
  range.dump(os);
  return os;
}

glm::vec3 Line::rotatePoint(glm::vec3 point, float angle) const {
  auto quat = glm::normalize(glm::angleAxis(angle, dir_));
  return point_ + quat * (point - point_);
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

void Line::read(std::istream &is) {
  glm::vec3 p1, p2;
  is >> p1 >> p2;
  Line tmp(p1, p2 - p1);
  std::swap(*this, tmp);
}

std::ostream &operator<<(std::ostream &os, const Line &line) {
  line.dump(os);
  return os;
}

std::istream &operator>>(std::istream &is, Line &line) {
  line.read(is);
  return is;
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
  // coplanar triangles intersection
  auto normal = pln1.getNormal();
  AAPlane::Axis axis;
  if (std::abs(normal.x) > std::abs(normal.y)) {
    if (std::abs(normal.x) > std::abs(normal.z))
      axis = AAPlane::Axis::X;
    else
      axis = AAPlane::Axis::Z;
  } else {
    if (std::abs(normal.y) > std::abs(normal.z))
      axis = AAPlane::Axis::Y;
    else
      axis = AAPlane::Axis::Z;
  }
  AAPlane aa_plane(0.0f, axis);
  auto tri1_prj = aa_plane.getProjection(tri1),
       tri2_prj = aa_plane.getProjection(tri2);
#ifndef NDEBUG
  std::cerr << "2D triangles:\n" << tri1_prj << '\n' << tri2_prj << '\n';
#endif
  return Intersects(tri1_prj, tri2_prj);
}

std::ostream &operator<<(std::ostream &os, const Triangle &triangle) {
  triangle.dump(os);
  return os;
}

std::istream &operator>>(std::istream &is, Triangle &triangle) {
  triangle.read(is);
  return is;
}

static float GetOrientation(glm::vec2 p, const Edge2D &edge) {
  return (edge.second.x - p.x) * (edge.second.y - edge.first.y) -
         (edge.second.x - edge.first.x) * (edge.second.y - p.y);
}

bool Triangle2D::isInner(glm::vec2 p) const {
  auto d1 = GetOrientation(p, Edge2D{p_[0], p_[1]}),
       d2 = GetOrientation(p, Edge2D{p_[1], p_[2]}),
       d3 = GetOrientation(p, Edge2D{p_[2], p_[0]});
  return ((d1 >= 0.0f) && (d2 >= 0.0f) && (d3 >= 0.0f)) ||
         ((d1 <= 0.0f) && (d2 <= 0.0f) && (d3 <= 0.0f));
}

void Triangle2D::dump(std::ostream &os) const {
  os << "(" << p_[0] << ", " << p_[1] << ", " << p_[2] << ")";
}

static bool IsEdgesIntersect(const Edge2D &edge1, const Edge2D &edge2) {
  auto orient11 = GetOrientation(edge2.first, edge1),
       orient12 = GetOrientation(edge2.second, edge1),
       orient21 = GetOrientation(edge1.first, edge2),
       orient22 = GetOrientation(edge1.second, edge2);
  if ((std::abs(orient11) < epsilon) && (std::abs(orient12) < epsilon) &&
      (std::abs(orient21) < epsilon) && (std::abs(orient22) < epsilon)) {
    Range x_projection1{std::min(edge1.first.x, edge1.second.x),
                        std::max(edge1.first.x, edge1.second.x)};
    Range x_projection2{std::min(edge2.first.x, edge2.second.x),
                        std::max(edge2.first.x, edge2.second.x)};
    Range y_projection1{std::min(edge1.first.y, edge1.second.y),
                        std::max(edge1.first.y, edge1.second.y)};
    Range y_projection2{std::min(edge2.first.y, edge2.second.y),
                        std::max(edge2.first.y, edge2.second.y)};
    return Intersects(x_projection1, x_projection2) &&
           Intersects(y_projection1, y_projection2);
  }
  if (((orient11 >= epsilon && orient12 <= -epsilon) ||
       (orient11 <= -epsilon && orient12 >= epsilon)) &&
      ((orient21 >= epsilon && orient22 <= -epsilon) ||
       (orient21 <= -epsilon && orient22 >= epsilon)))
    return true;
  return false;
}

bool Intersects(const Triangle2D &tri1, const Triangle2D &tri2) {
  auto get_edges = [](const Triangle2D &tri) -> std::array<Edge2D, 3> {
    return {Edge2D{tri.getPoint(0), tri.getPoint(1)},
            Edge2D{tri.getPoint(1), tri.getPoint(2)},
            Edge2D{tri.getPoint(2), tri.getPoint(0)}};
  };
  for (auto edge1 : get_edges(tri1))
    for (auto edge2 : get_edges(tri2))
      if (IsEdgesIntersect(edge1, edge2)) {
#ifndef NDEBUG
        std::cerr << "Edges: " << edge1 << " and " << edge2 << " intersect\n";
#endif
        return true;
      }
  return tri1.isInner(tri2) || tri2.isInner(tri1);
}

std::ostream &operator<<(std::ostream &os, const Triangle2D &triangle) {
  triangle.dump(os);
  return os;
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
