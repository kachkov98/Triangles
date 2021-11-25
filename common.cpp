#include "common.hpp"

render::VertexData getVertexData(const scene::Scene &scene,
                                 const scene::Collisions &collisions) {
  render::VertexData data;
  data.reserve(scene.size() * 3);
  for (scene::TriangleIdx i = 0; i < scene.size(); ++i) {
    glm::vec3 color = collisions.count(i) ? glm::vec3(1.f, 0.f, 0.f)
                                          : glm::vec3(0.f, 0.f, 1.f);
    glm::vec3 normal = glm::normalize(scene[i].getNormal());
    data.push_back(render::Vertex{scene[i].getPoint(0), color, normal});
    data.push_back(render::Vertex{scene[i].getPoint(1), color, normal});
    data.push_back(render::Vertex{scene[i].getPoint(2), color, normal});
  }
  return data;
}
