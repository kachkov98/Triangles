#ifndef VISUALIZER_HPP
#define VISUALIZER_HPP

#include "renderer.hpp"
#include "scene.hpp"
#include "window.hpp"

namespace render {

class Visualizer {
public:
  Visualizer(const std::string &app_name, const VertexData &data);
  void run();

private:
  Window window_;
  Renderer renderer_;
  VertexBuffer scene_;
  Camera camera_;
};

} // namespace render

#endif
