#pragma once
#include <string>
#include <glm/glm.hpp>
#include <buffers/buffers.h>
#include <boundingbox/boundingbox.h>
#include <point-cloud/builder/builder.h>
#include <mouse/mouse.h>

class PointCloud {
  friend PointCloud builder::fromPLY(const std::string& filepath);
  // friend PointCloud builder::fromLAS(const std::string& filepath);

 public:
  Buffers buffers;
  BoundingBox bbox;
  Mouse mouse;

  static PointCloud build(const std::string& filepath);
  ~PointCloud();

  void rotate(float deltaX, float deltaY);
  void incrementPointSize();
  void decrementPointSize();
  float getPointSize() const;
  void buffer();
  void draw() const;
  void tickSync(float deltaTime);
  const glm::mat4& getModelMatrix() const;

 private:
  float deltaTime;
  glm::mat4 modelMatrix;
  float pointSize;
  unsigned int vao;

  PointCloud(const Buffers& buffers, const BoundingBox& bbox);
};
