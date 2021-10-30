#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>

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
  bool should_close() const;
  void process_events() const;

private:
  GLFWwindow *window_;
};

} // namespace render

#endif
