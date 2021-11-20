#include "collisions/scene.hpp"
#include "renderer/visualizer.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>

render::VertexData getVertexData(const scene::Scene &scene, const scene::Collisions &collisions) {
  render::VertexData data;
  data.reserve(scene.size() * 3);
  for (scene::TriangleIdx i = 0; i < scene.size(); ++i) {
    glm::vec3 color = collisions.count(i) ?
      glm::vec3(1.f, 0.f, 0.f) : glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 normal = glm::normalize(scene[i].getNormal());
    data.push_back(render::Vertex{scene[i].getPoint(0), color, normal});
    data.push_back(render::Vertex{scene[i].getPoint(1), color, normal});
    data.push_back(render::Vertex{scene[i].getPoint(2), color, normal});
  }
  return data;
}

int main(int argc, char *argv[]) {
  scene::TriangleIdx N;
  float Time;
  std::cin >> N >> Time;
  scene::DynamicScene triangles;
  triangles.reserve(N);
  for (scene::TriangleIdx i = 0; i < N; ++i) {
    scene::DynamicTriangle tri;
    std::cin >> tri;
    triangles.push_back(tri);
  }

  glfwInit();
  try {
    render::Visualizer visualizer("Triangles", N * 3);
    auto start_time = std::chrono::high_resolution_clock::now();
    while (!visualizer.shouldClose()) {
      float time = std::chrono::duration<float>(
          std::chrono::high_resolution_clock::now() - start_time).count();
      auto cur_scene = scene::updateDynamicScene(triangles, time);
      auto vertex_data = getVertexData(cur_scene, scene::findIntersectingTriangles(cur_scene));
      visualizer.drawFrame(vertex_data);
    }
  }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
  glfwTerminate();
  return 0;
}
