#ifndef VISUALIZER_HPP
#define VISUALIZER_HPP

#include "renderer.hpp"
#include "window.hpp"

namespace render {

class Visualizer {
public:
  Visualizer(const std::string &app_name, const VertexData &data);
  void run();

private:
  Window window_;
  Camera camera_;
  Renderer renderer_;
};

} // namespace render

#endif
