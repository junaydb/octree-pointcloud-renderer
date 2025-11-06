#pragma once
#include <glm/glm.hpp>
#include <glad/gl.h>
#include <buffers/buffers.h>

class BoundingBox {
 public:
  Buffers buffers;

  BoundingBox();
  BoundingBox(glm::vec3& min, glm::vec3& max, bool uniform);
  BoundingBox(const BoundingBox& original);
  ~BoundingBox();

  BoundingBox& operator=(const BoundingBox& original);

  glm::vec3 getCenter() const;
  glm::vec3 getDimensions() const;
  glm::vec3 getMin() const;
  glm::vec3 getMax() const;
  float getScale() const;
  float getScreenScaleFactor() const;
  float getBoundingSphereRadius();
  void buffer();
  void draw() const;

 private:
  glm::vec3 min;
  glm::vec3 max;
  unsigned int vao;
};
