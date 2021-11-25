#include "common.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>

int main(int argc, char *argv[]) {
  scene::TriangleIdx N;
  float MaxTime;
  std::cin >> N >> MaxTime;
  scene::DynamicScene triangles;
  triangles.reserve(N);
  for (scene::TriangleIdx i = 0; i < N; ++i) {
    scene::DynamicTriangle tri;
    std::cin >> tri;
    triangles.push_back(tri);
  }

  glfwInit();
  try {
    render::Visualizer visualizer("Dynamic triangles", N * 3);
    auto start_time = std::chrono::high_resolution_clock::now();
    while (!visualizer.shouldClose()) {
      float time = std::chrono::duration<float>(
                       std::chrono::high_resolution_clock::now() - start_time)
                       .count();
      auto cur_scene =
          scene::updateDynamicScene(triangles, std::min(time, MaxTime));
      auto vertex_data =
          getVertexData(cur_scene, scene::findIntersectingTriangles(cur_scene));
      visualizer.drawFrame(vertex_data);
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
  glfwTerminate();
  return 0;
}
