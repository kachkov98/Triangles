#include "scene.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  scene::TriangleIdx N;
  std::cin >> N;
  scene::Scene triangles;
  triangles.reserve(N);
  for (scene::TriangleIdx i = 0; i < N; ++i) {
    geom::Triangle tri;
    std::cin >> tri;
    triangles.push_back(tri);
  }
  auto res = scene::findIntersectingTriangles(triangles);
  for (auto idx : res)
    std::cout << idx << " ";
  std::cout << std::endl;
  return 0;
}
