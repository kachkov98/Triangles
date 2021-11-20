#ifndef COLLISIONS_SCENE_HPP
#define COLLISIONS_SCENE_HPP

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

class DynamicTriangle {
public:
  DynamicTriangle() = default;
  DynamicTriangle(const geom::Triangle &tri, const geom::Line axis, float speed)
      : tri_(tri), axis_(axis), speed_(speed) {}
  geom::Triangle get(float time) const;
  void dump(std::ostream &os) const;
  void read(std::istream &is);

private:
  geom::Triangle tri_;
  geom::Line axis_;
  float speed_;
};

std::ostream &operator<<(std::ostream &os, const DynamicTriangle &tri);
std::istream &operator>>(std::istream &is, DynamicTriangle &tri);
;

using DynamicScene = std::vector<DynamicTriangle>;

Scene updateDynamicScene(const DynamicScene &scene, float time);

} // namespace scene

#endif
