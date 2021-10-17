#include "scene.hpp"
#include "geometry.hpp"

namespace scene {
std::set<TriangleIdx> findIntersectingTriangles(const Triangles &triangles) {
  std::set<TriangleIdx> indices;
  for (TriangleIdx fst = 0; fst < triangles.size(); ++fst) {
    if (indices.count(fst))
      continue;
    for (TriangleIdx snd = fst + 1; snd < triangles.size(); ++snd)
      if (geom::Intersects(triangles[fst], triangles[snd])) {
        indices.insert(fst);
        indices.insert(snd);
      }
  }
  return indices;
}
} // namespace scene
