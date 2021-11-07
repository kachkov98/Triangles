#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

struct GLFWwindow;

namespace render {

class Window {
public:
  Window(unsigned width, unsigned height, const std::string &title);
  Window(const Window &) = delete;
  Window(Window &&rhs) : window_(rhs.window_) { rhs.window_ = nullptr; }
  Window &operator=(const Window &) = delete;
  Window &operator=(Window &&rhs) {
    window_ = rhs.window_;
    rhs.window_ = nullptr;
    return *this;
  }
  ~Window();

  bool shouldClose() const;
  void processEvents() const;
  vk::Extent2D getExtent() const;

  std::vector<const char *> getRequiredExtensions() const;
  vk::UniqueSurfaceKHR createSurface(vk::Instance instance) const;

private:
  GLFWwindow *window_;
};

} // namespace render

#endif
