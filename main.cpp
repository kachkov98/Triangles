#include "collisions/scene.hpp"
#include "renderer/visualizer.hpp"
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
  auto res = scene::findIntersectingTriangles(triangles);

  render::VertexData scene;
  scene.reserve(triangles.size() * 3);
  for (scene::TriangleIdx i = 0; i < triangles.size(); ++i) {
    glm::vec3 color = res.count(i) ?
      glm::vec3(1.f, 0.f, 0.f) : glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 normal = glm::normalize(triangles[i].getNormal());
    scene.push_back(render::Vertex{triangles[i].getPoint(0), color, normal});
    scene.push_back(render::Vertex{triangles[i].getPoint(1), color, normal});
    scene.push_back(render::Vertex{triangles[i].getPoint(2), color, normal});
  }
  glfwInit();
  try {
    render::Visualizer visualizer("Triangles", scene);
    visualizer.run();
  }
  catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
  glfwTerminate();
  return 0;
}
