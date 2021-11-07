#include "visualizer.hpp"

namespace render {

Visualizer::Visualizer(const std::string &app_name, const VertexData &data)
    : window_(1280, 720, app_name), renderer_(window_, app_name) {}

void Visualizer::run() {
  while (!window_.shouldClose()) {
    window_.processEvents();
    renderer_.draw(window_);
  }
  renderer_.waitIdle();
}

} // namespace render
