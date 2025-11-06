#pragma once
#include <string>
#include <optional>
#include <boundingbox/boundingbox.h>
#include <buffers/buffers.h>

class PointCloud;  // forward  declaration to avoid circular dependency

namespace builder {
  enum Extension {
    PLY,
    LAS,
  };

  std::optional<Extension> getFileExt(const std::string& filepath);
  PointCloud fromPLY(const std::string& filepath);
  // PointCloud fromLAS(const std::string& filepath); // TODO: add LAS support
  BoundingBox createBoundingBox(glm::vec3* positionBuffer, unsigned int numPoints);
  void applyGradient(glm::vec3* positionBuffer, glm::u8vec3* colourBuffer, unsigned int numPoints);
}  // namespace builder
