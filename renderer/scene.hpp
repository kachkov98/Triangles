#ifndef RENDERER_SCENE_HPP
#define RENDERER_SCENE_HPP

#include <array>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace render {

struct Vertex {
  glm::vec3 position;
  glm::vec3 color;
  glm::vec3 normal;

  static vk::VertexInputBindingDescription getBindingDescription();
  static std::array<vk::VertexInputAttributeDescription, 3>
  getAttributeDescriptions();
};

using VertexData = std::vector<Vertex>;

struct CameraData {
  glm::mat4 view;
  glm::mat4 proj;
};

uint32_t
findMemoryTypeIndex(const vk::PhysicalDeviceMemoryProperties &memory_properties,
                    const vk::MemoryRequirements &memory_requirements,
                    vk::MemoryPropertyFlags memory_type);

class Buffer {
public:
  Buffer() = default;
  Buffer(vk::Device device, vk::PhysicalDevice physical_device, size_t size,
         vk::BufferUsageFlags usage, vk::MemoryPropertyFlags memory_type);
  vk::Buffer get() const { return *buffer_; }
  template <typename T> void upload(vk::Device device, const T &data) const {
    size_t size = sizeof(T);
    void *mem = device.mapMemory(*memory_, 0, size, {});
    memcpy(mem, &data, size);
    device.unmapMemory(*memory_);
  }
  template <typename T>
  void upload(vk::Device device, const std::vector<T> &data) const {
    size_t size = sizeof(T) * data.size();
    void *mem = device.mapMemory(*memory_, 0, size, {});
    memcpy(mem, data.data(), size);
    device.unmapMemory(*memory_);
  }

protected:
  vk::UniqueBuffer buffer_;
  vk::UniqueDeviceMemory memory_;
};

class VertexBuffer final : public Buffer {
public:
  VertexBuffer() = default;
  VertexBuffer(vk::Device device, vk::PhysicalDevice physical_device,
               const VertexData &data)
      : Buffer(device, physical_device, sizeof(Vertex) * data.size(),
               vk::BufferUsageFlagBits::eVertexBuffer,
               vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent),
        num_vertices_(data.size()) {
    upload(device, data);
  }
  unsigned getNumVertices() const { return num_vertices_; }

private:
  unsigned num_vertices_;
};

class CameraBuffer final : public Buffer {
public:
  CameraBuffer() = default;
  CameraBuffer(vk::Device device, vk::PhysicalDevice physical_device,
               const CameraData &data)
      : Buffer(device, physical_device, sizeof(CameraData),
               vk::BufferUsageFlagBits::eUniformBuffer,
               vk::MemoryPropertyFlagBits::eHostVisible |
                   vk::MemoryPropertyFlagBits::eHostCoherent) {
    upload(device, data);
  }
};

class Camera {
public:
  Camera(glm::vec3 center, float radius, float aspect, float fovy = 90.f,
         float near = 0.1f, float far = 100.f)
      : center_(center), radius_(radius), azimuthal_(0.f), polar_(0.f),
        aspect_(aspect), fovy_(fovy), near_(near), far_(far) {}
  void pan(glm::vec2 dir);
  void rotate(glm::vec2 dir);
  void zoom(float offset, float scale = 1.125f);
  void setAspect(float aspect) { aspect_ = aspect; }
  CameraData getData() const {
    return CameraData{getViewMatrix(), getProjectionMatrix()};
  }
  glm::mat4 getViewMatrix() const;
  glm::mat4 getProjectionMatrix() const;
  glm::vec3 getDirection() const;

private:
  static constexpr glm::vec3 up_{0.f, 1.f, 0.f};
  glm::vec3 center_;
  float radius_, azimuthal_, polar_;
  float aspect_, fovy_, near_, far_;
};

} // namespace render

#endif
