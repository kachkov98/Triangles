#include "geometry.hpp"
#include "glm/gtc/random.hpp"
#include "scene.hpp"
#include <gtest/gtest.h>
#include <iostream>

TEST(Geometry, Triangles) {
  geom::Triangle tri{glm::vec3{5.f, 6.f, 7.f}, glm::vec3{6.f, 5.f, 4.f},
                     glm::vec3{1.f, 2.f, 3.f}};
  std::vector<std::pair<geom::Triangle, bool>> tris = {
      {{glm::vec3{-1.f, 5.f, 0.f}, glm::vec3{2.f, 2.f, -3.f},
        glm::vec3{5.f, 5.f, 0.f}},
       false},
      {{glm::vec3{-1.f, -1.f, 0.f}, glm::vec3{0.f, 1.f, 0.f},
        glm::vec3{1.f, -1.f, 0.f}},
       false},
      {{glm::vec3{-1.f, -5.f, 0.f}, glm::vec3{2.f, -2.f, -3.f},
        glm::vec3{5.f, -5.f, 0.f}},
       false},
      {{glm::vec3{5.f, 6.f, 7.f}, glm::vec3{6.f, 5.f, 4.f},
        glm::vec3{1.f, 2.f, 3.f}},
       true}};
  for (const auto &[other_tri, answer] : tris)
    EXPECT_TRUE(geom::Intersects(tri, other_tri) == answer);
}

TEST(Scene, RandomScene) {
  constexpr unsigned N = 1000000;
  scene::Scene triangles;
  for (unsigned i = 0; i < N; ++i) {
    glm::vec3 center = glm::linearRand(glm::vec3(-1000.f, -1000.f, -1000.f),
                                       glm::vec3(1000.f, 1000.f, 1000.f));
    triangles.emplace_back(center + glm::ballRand(1.f),
                           center + glm::ballRand(1.f),
                           center + glm::ballRand(1.f));
  }
  auto res = scene::findIntersectingTriangles(triangles);
  for (auto idx : res)
    std::cout << idx << " ";
  std::cout << std::endl;
  EXPECT_TRUE(true);
}
