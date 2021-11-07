#include "window.hpp"
#include <GLFW/glfw3.h>

namespace render {

Window::Window(unsigned width, unsigned height, const std::string &title) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_); }
void Window::processEvents() const { glfwPollEvents(); }
vk::Extent2D Window::getExtent() const {
  int width, height;
  glfwGetFramebufferSize(window_, &width, &height);
  return vk::Extent2D{static_cast<uint32_t>(width),
                      static_cast<uint32_t>(height)};
}

std::vector<const char *> Window::getRequiredExtensions() const {
  uint32_t count;
  const char **extensions = glfwGetRequiredInstanceExtensions(&count);
  return std::vector<const char *>(extensions, extensions + count);
}

vk::UniqueSurfaceKHR Window::createSurface(vk::Instance instance) const {
  VkSurfaceKHR surface;
  if (glfwCreateWindowSurface(instance, window_, nullptr, &surface) !=
      VK_SUCCESS)
    throw std::runtime_error("failed to create window surface!");
  return vk::UniqueSurfaceKHR(surface, instance);
}

Window::~Window() {
  if (window_)
    glfwDestroyWindow(window_);
}

} // namespace render
