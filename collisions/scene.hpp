#ifndef SCENE_HPP
#define SCENE_HPP

#include "geometry.hpp"
#include <memory>
#include <set>
#include <vector>

namespace scene {
using Scene = std::vector<geom::Triangle>;
using TriangleIdx = uint32_t;
using Triangles = std::vector<TriangleIdx>;
using Collisions = std::set<TriangleIdx>;

class TreeNode {
public:
  TreeNode(const Triangles &tris, const Scene &scene);
  Collisions testCollisions(const Scene &scene);

private:
  Triangles tris_;
  std::pair<Triangles, Triangles> children_tris_;
  std::pair<std::unique_ptr<TreeNode>, std::unique_ptr<TreeNode>> children_;
};

Collisions findIntersectingTriangles(const Scene &scene);
} // namespace scene

#endif
