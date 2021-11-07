#ifndef RENDERER_SCENE_HPP
#define RENDERER_SCENE_HPP

#include <glm/vec3.hpp>
#include <vector>

namespace render {

struct Vertex {
  glm::vec3 position;
  glm::vec3 color;
  glm::vec3 normal;
};

using VertexData = std::vector<Vertex>;

class VertexBuffer {
#if 0
public:
  VertexBuffer(vk::Device device, size_t size);
  VertexBuffer(vk::Device device, const VertexData &data)
      : vertexBuffer(device, sizeof(Vertex) * data.size()) {
    upload(data);
  }
  void upload(const std::vector<Vertex> &data);
  vk::buffer getBuffer() const { return vertex_buffer_; }

private:
  vk::DeviceMemory buffer_mem_;
  vk::BufferUnique vertex_buffer_;
#endif
};

class Camera {};

} // namespace render

#endif
