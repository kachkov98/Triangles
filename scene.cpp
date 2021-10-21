#include "scene.hpp"
#include "geometry.hpp"
#include <iostream>
#include <numeric>

namespace scene {
TreeNode::TreeNode(const Triangles &tris, const Scene &scene) {
  // Find separating plane
  // TODO: better heuristic
  glm::vec3 min{geom::pos_inf, geom::pos_inf, geom::pos_inf},
      max{geom::neg_inf, geom::neg_inf, geom::neg_inf};
  for (auto idx : tris) {
    const geom::Triangle &tri = scene[idx];
    for (auto p : {tri.getPoint(0), tri.getPoint(1), tri.getPoint(2)}) {
      min.x = std::min(min.x, p.x);
      min.y = std::min(min.y, p.y);
      min.z = std::min(min.z, p.z);
      max.x = std::max(max.x, p.x);
      max.y = std::max(max.y, p.y);
      max.z = std::max(max.z, p.z);
    }
  }
  geom::AAPlane::Axis axis;
  if (max.x - min.x > max.y - min.y) {
    if (max.x - min.x > max.z - min.z)
      axis = geom::AAPlane::Axis::X;
    else
      axis = geom::AAPlane::Axis::Z;
  } else {
    if (max.y - min.y > max.z - min.z)
      axis = geom::AAPlane::Axis::Y;
    else
      axis = geom::AAPlane::Axis::Z;
  }
  geom::AAPlane plane(
      (min[static_cast<unsigned>(axis)] + max[static_cast<unsigned>(axis)]) *
          0.5f,
      axis);
  // Do separation
  for (auto idx : tris) {
    if (plane.isFront(scene[idx])) {
      children_tris_.first.push_back(idx);
      continue;
    }
    if (plane.isBack(scene[idx])) {
      children_tris_.second.push_back(idx);
      continue;
    }
    tris_.push_back(idx);
  }
  if (!children_tris_.first.empty())
    children_.first = std::make_unique<TreeNode>(children_tris_.first, scene);
  if (!children_tris_.second.empty())
    children_.second = std::make_unique<TreeNode>(children_tris_.second, scene);
}

Collisions TreeNode::testCollisions(const Scene &scene) {
  Collisions res;
  auto do_test = [&](TriangleIdx idx1, TriangleIdx idx2) {
    if (res.count(idx1) && res.count(idx2))
      return;
#ifndef NDEBUG
    std::cerr << "Testing tris " << idx1 << " and " << idx2 << '\n';
#endif
    if (geom::Intersects(scene[idx1], scene[idx2]))
      res.insert({idx1, idx2});
  };
  // Test all triangles on current hierarchy level with each other and with
  // children triangles
  for (unsigned i = 0; i < tris_.size(); ++i) {
    for (unsigned j = i + 1; j < tris_.size(); ++j)
      do_test(tris_[i], tris_[j]);
    for (unsigned j = 0; j < children_tris_.first.size(); ++j)
      do_test(tris_[i], children_tris_.first[j]);
    for (unsigned j = 0; j < children_tris_.second.size(); ++j)
      do_test(tris_[i], children_tris_.second[j]);
  }
  if (children_.first)
    res.merge(children_.first->testCollisions(scene));
  if (children_.second)
    res.merge(children_.second->testCollisions(scene));
  return res;
}

Collisions findIntersectingTriangles(const Scene &scene) {
  Triangles tris(scene.size());
  std::iota(tris.begin(), tris.end(), 0);
  return TreeNode(tris, scene).testCollisions(scene);
}
} // namespace scene
