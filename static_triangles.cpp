#include "common.hpp"
#include <GLFW/glfw3.h>
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
  auto collisions = scene::findIntersectingTriangles(triangles);
  auto vertex_data = getVertexData(triangles, collisions);

  glfwInit();
  try {
    render::Visualizer visualizer("Static triangles", N * 3);
    while (!visualizer.shouldClose())
      visualizer.drawFrame(vertex_data);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
  glfwTerminate();
  return 0;
}
