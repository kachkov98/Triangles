#include "window.hpp"
#include <GLFW/glfw3.h>

namespace render {

Window::Window(unsigned width, unsigned height, const std::string &title) {
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

bool Window::should_close() const { return glfwWindowShouldClose(window_); }
void Window::process_events() const { glfwPollEvents(); }

Window::~Window() {
  if (window_)
    glfwDestroyWindow(window_);
}

} // namespace render
