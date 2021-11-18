#include "window.hpp"
#include "scene.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <optional>

namespace render {

Window::Window(unsigned width, unsigned height, const std::string &title,
               void *user_data) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  glfwSetWindowUserPointer(window_, user_data);
  glfwSetWindowSizeCallback(window_, WindowSizeCallback);
  glfwSetCursorPosCallback(window_, CursorPosCallback);
  glfwSetScrollCallback(window_, ScrollCallback);
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_); }
void Window::processEvents() const { glfwPollEvents(); }
vk::Extent2D Window::getExtent() const {
  int width, height;
  glfwGetFramebufferSize(window_, &width, &height);
  return vk::Extent2D{static_cast<uint32_t>(width),
                      static_cast<uint32_t>(height)};
}

float Window::getAspectRatio() const {
  int width, height;
  glfwGetFramebufferSize(window_, &width, &height);
  return static_cast<float>(width) / static_cast<float>(height);
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

void Window::WindowSizeCallback(GLFWwindow *window, int width, int height) {
  Camera *camera = reinterpret_cast<Camera *>(glfwGetWindowUserPointer(window));
  camera->setAspect(static_cast<float>(width) / static_cast<float>(height));
}

void Window::CursorPosCallback(GLFWwindow *window, double xpos, double ypos) {
  Camera *camera = reinterpret_cast<Camera *>(glfwGetWindowUserPointer(window));
  static std::optional<glm::vec2> prev_pos;
  glm::vec2 cur_pos{static_cast<float>(xpos), static_cast<float>(ypos)};
  bool left_button =
      glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  bool right_button =
      glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
  if (left_button || right_button) {
    if (prev_pos) {
      glm::vec2 diff = cur_pos - *prev_pos;
      if (left_button)
        camera->rotate(diff / 128.f);
      else if (right_button)
        camera->pan(diff / 128.f);
    }
    prev_pos = cur_pos;
  } else
    prev_pos = std::nullopt;
}

void Window::ScrollCallback(GLFWwindow *window, double xoffset,
                            double yoffset) {
  Camera *camera = reinterpret_cast<Camera *>(glfwGetWindowUserPointer(window));
  camera->zoom(static_cast<float>(yoffset));
}

} // namespace render
