#ifndef RENDERER_HPP
#define RENDERER_HPP

#include "window.hpp"
#include <vulkan/vulkan.hpp>

namespace render {

class Renderer {
public:
  Renderer();
  void run() const;

private:
  void draw_frame() const;

  Window window_;
  vk::UniqueInstance instance_;
  vk::PhysicalDevice physical_device_;
  vk::UniqueDevice device_;
};

} // namespace render

#endif
