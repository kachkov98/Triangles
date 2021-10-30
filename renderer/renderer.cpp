#include "renderer.hpp"

namespace render {

Renderer::Renderer() : window_(1280, 720, "Triangles"){};

void Renderer::run() const {
  while (!window_.should_close()) {
    window_.process_events();
    draw_frame();
  }
  // device_->waitIdle();
}

void Renderer::draw_frame() const {}

} // namespace render
