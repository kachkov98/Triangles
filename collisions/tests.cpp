#include "geometry.hpp"
#include "scene.hpp"
#include <glm/gtc/random.hpp>
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

static std::pair<glm::vec3, glm::vec3> getBasis(glm::vec3 n) {
  glm::vec3 t;
  if (std::abs(n.x) < std::abs(n.y)) {
    if (std::abs(n.x) < std::abs(n.z))
      t = glm::vec3{1.f, 0.f, 0.f};
    else
      t = glm::vec3{0.f, 0.f, 1.f};
  } else {
    if (std::abs(n.y) < std::abs(n.z))
      t = glm::vec3{0.f, 1.f, 0.f};
    else
      t = glm::vec3{0.f, 0.f, 1.f};
  }
  glm::vec3 u = glm::normalize(glm::cross(n, t)), v = glm::cross(n, u);
  return {u, v};
}

static geom::Triangle generateRandomTri(glm::vec3 center, glm::vec3 normal) {
  constexpr float pi = glm::pi<float>();
  float angle1 = glm::linearRand(0.f, 2 * pi);
  float angle2 = glm::linearRand(0.f, pi);
  float angle3 = glm::linearRand(-pi, -pi + angle2);
  auto generateVertex = [](float angle) {
    float radius = glm::linearRand(0.0f, 1.0f);
    return glm::vec2{radius * std::cos(angle), radius * std::sin(angle)};
  };
  std::array<glm::vec2, 3> vertices = {generateVertex(angle1),
                                       generateVertex(angle1 + angle2),
                                       generateVertex(angle1 + angle3)};
  auto [u, v] = getBasis(normal);
  return geom::Triangle{center + u * vertices[0].x + v * vertices[0].y,
                        center + u * vertices[1].x + v * vertices[1].y,
                        center + u * vertices[2].x + v * vertices[2].y};
}

TEST(Geometry, IntersectingNonComplanarTriangles) {
  constexpr unsigned N = 100;
  for (unsigned i = 0; i < N; ++i) {
    glm::vec3 common_point = glm::linearRand(glm::vec3(-10.f, -10.f, -10.f),
                                             glm::vec3(10.f, 10.f, 10.f));
    glm::vec3 normal1 = glm::sphericalRand(1.f),
              normal2 = glm::sphericalRand(1.f);
    auto tri1 = generateRandomTri(common_point, normal1),
         tri2 = generateRandomTri(common_point, normal2);
    EXPECT_TRUE(geom::Intersects(tri1, tri2));
  }
}

TEST(Geometry, IntersectingComplanarTriangles) {
  constexpr unsigned N = 100;
  for (unsigned i = 0; i < N; ++i) {
    glm::vec3 common_point = glm::linearRand(glm::vec3(-10.f, -10.f, -10.f),
                                             glm::vec3(10.f, 10.f, 10.f));
    glm::vec3 normal{1.f, 0.f, 0.f};
    auto tri1 = generateRandomTri(common_point, normal),
         tri2 = generateRandomTri(common_point, normal);
    EXPECT_TRUE(geom::Intersects(tri1, tri2));
  }
}

TEST(Geometry, NonIntersectingTriangles) {
  constexpr unsigned N = 100;
  for (unsigned i = 0; i < N; ++i) {
    glm::vec3 n = glm::sphericalRand(1.f);
    auto [u, v] = getBasis(n);
    auto generatePosVertex = [n, u, v]() {
      return n * glm::linearRand(0.001f, 10.f) +
             u * glm::linearRand(-10.f, 10.f) +
             v * glm::linearRand(-10.f, 10.f);
    };
    geom::Triangle tri1{generatePosVertex(), generatePosVertex(),
                        generatePosVertex()};
    auto generateNegVertex = [n, u, v]() {
      return n * glm::linearRand(-0.001f, -10.f) +
             u * glm::linearRand(-10.f, 10.f) +
             v * glm::linearRand(-10.f, 10.f);
    };
    geom::Triangle tri2{generateNegVertex(), generateNegVertex(),
                        generateNegVertex()};
    EXPECT_FALSE(geom::Intersects(tri1, tri2));
  }
}

TEST(Scene, RandomScene) {
  constexpr unsigned N = 10000;
  scene::Scene triangles;
  for (unsigned i = 0; i < N; ++i) {
    glm::vec3 center = glm::linearRand(glm::vec3(-10.f, -10.f, -10.f),
                                       glm::vec3(10.f, 10.f, 10.f));
    triangles.emplace_back(center + glm::ballRand(1.f),
                           center + glm::ballRand(1.f),
                           center + glm::ballRand(1.f));
  }
  scene::Collisions res1;
  for (scene::TriangleIdx i = 0; i < triangles.size(); ++i)
    for (scene::TriangleIdx j = i + 1; j < triangles.size(); ++j)
      if (geom::Intersects(triangles[i], triangles[j]))
        res1.insert({i, j});
  auto res2 = scene::findIntersectingTriangles(triangles);
  EXPECT_TRUE(res1 == res2);
}
