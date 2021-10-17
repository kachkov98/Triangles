#ifndef SCENE_HPP
#define SCENE_HPP

#include "geometry.hpp"
#include <set>
#include <vector>

namespace scene {
using Triangles = std::vector<geom::Triangle>;
using TriangleIdx = uint32_t;

std::set<TriangleIdx> findIntersectingTriangles(const Triangles &triangles);
} // namespace scene

#endif
