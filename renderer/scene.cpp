#include "scene.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

namespace render {

uint32_t
findMemoryTypeIndex(const vk::PhysicalDeviceMemoryProperties &memory_properties,
                    const vk::MemoryRequirements &memory_requirements,
                    vk::MemoryPropertyFlags memory_type) {
  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    if ((memory_requirements.memoryTypeBits & (1 << i)) &&
        (memory_properties.memoryTypes[i].propertyFlags & memory_type) ==
            memory_type)
      return i;
  throw std::runtime_error("can not find suitable memory type!");
}

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
  return vk::VertexInputBindingDescription{0, sizeof(Vertex),
                                           vk::VertexInputRate::eVertex};
}

std::array<vk::VertexInputAttributeDescription, 3>
Vertex::getAttributeDescriptions() {
  return {vk::VertexInputAttributeDescription{
              0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
          vk::VertexInputAttributeDescription{
              1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)},
          vk::VertexInputAttributeDescription{
              2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)}};
}

Buffer::Buffer(vk::Device device, vk::PhysicalDevice physical_device,
               size_t size, vk::BufferUsageFlags usage,
               vk::MemoryPropertyFlags memory_type) {
  buffer_ = device.createBufferUnique(
      vk::BufferCreateInfo{{}, size, usage, vk::SharingMode::eExclusive});
  auto memory_properties = physical_device.getMemoryProperties();
  auto memory_requirements = device.getBufferMemoryRequirements(*buffer_);
  auto memory_index =
      findMemoryTypeIndex(memory_properties, memory_requirements, memory_type);
  memory_ = device.allocateMemoryUnique(
      vk::MemoryAllocateInfo{memory_requirements.size, memory_index});
  device.bindBufferMemory(*buffer_, *memory_, 0);
}

void Camera::pan(glm::vec2 dir) {
  glm::vec3 right = glm::normalize(glm::cross(getDirection(), up_));
  glm::vec3 up = glm::cross(getDirection(), right);
  center_ += right * dir.x + up * dir.y;
}

void Camera::rotate(glm::vec2 dir) {
  azimuthal_ = fmod(azimuthal_ - dir.x, 2 * glm::pi<float>());
  polar_ =
      glm::clamp(polar_ - dir.y, -glm::half_pi<float>() + glm::epsilon<float>(),
                 glm::half_pi<float>() - glm::epsilon<float>());
}

void Camera::zoom(float offset, float scale) { radius_ /= pow(scale, offset); }

glm::mat4 Camera::getViewMatrix() const {
  return glm::lookAt(center_ + getDirection() * radius_, center_, up_);
}

glm::mat4 Camera::getProjectionMatrix() const {
  return glm::perspective(fovy_, aspect_, near_, far_);
}

glm::vec3 Camera::getDirection() const {
  return glm::vec3{glm::sin(azimuthal_) * glm::cos(polar_), glm::sin(polar_),
                   glm::cos(azimuthal_) * glm::cos(polar_)};
}

} // namespace render
