#pragma once

#include <optional>
#include <string>

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <boundingbox/boundingbox.h>
#include <buffers/buffers.h>

class PointCloud {
 public:
  static PointCloud build(const std::string& filepath,
                          std::optional<unsigned int> pointLimit);
  ~PointCloud();

  void rotate(float deltaX, float deltaY, float deltaTime, float sensitivity,
              bool inverted);
  void incrementPointSize();
  void decrementPointSize();
  float getPointSize() const;
  void buffer();
  void draw() const;
  const glm::mat4& getModelMatrix() const;

  const Buffers& getBuffers() const;
  const BoundingBox& getBoundingBox() const;

 private:
  PointCloud(Buffers&& buffers, const BoundingBox& bbox);

  static std::string getFileExtension(const std::string& filepath);
  static PointCloud loadPLY(const std::string& filepath,
                            std::optional<unsigned int> pointLimit);
  static BoundingBox createBoundingBox(const glm::vec3* positionBuffer,
                                       unsigned int numPoints);
  static void applyGradient(const glm::vec3* positionBuffer,
                            glm::u8vec3* colourBuffer,
                            unsigned int numPoints);

  Buffers buffers;
  BoundingBox bbox;
  glm::mat4 modelMatrix;
  float pointSize;
  unsigned int vao;
};
