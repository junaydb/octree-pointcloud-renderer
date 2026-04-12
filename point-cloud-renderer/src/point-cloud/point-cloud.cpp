#include "point-cloud.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <miniply/miniply.h>

PointCloud::PointCloud(Buffers&& buffers, const BoundingBox& bbox)
    : buffers(std::move(buffers)), bbox(bbox), pointSize(3.f), vao(0) {
  // Center and scale the point cloud so it starts in view.
  float scale = bbox.getScreenScaleFactor();
  glm::vec3 center = bbox.getCenter();

  // translate point cloud to center
  modelMatrix = glm::translate(glm::mat4(1.f),
                               glm::vec3(-center.x * scale,
                                         -center.y * scale,
                                         -center.z * scale));
  modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
}

PointCloud::~PointCloud() {
  if (vao != 0) {
    glDeleteVertexArrays(1, &vao);
  }
}

PointCloud PointCloud::build(const std::string& filepath,
                             std::optional<unsigned int> pointLimit) {
  std::string ext = getFileExtension(filepath);

  if (ext == ".ply") {
    return loadPLY(filepath, pointLimit);
  }

  std::cerr << "Error: Unrecognised file extension '" << ext
            << "'. Supported formats: .ply" << std::endl;
  std::exit(EXIT_FAILURE);
}

void PointCloud::rotate(float deltaX, float deltaY, float deltaTime,
                        float sensitivity, bool inverted) {
  float xRadians = glm::radians(
      (inverted ? deltaX : -deltaX) * sensitivity * deltaTime);
  float yRadians = glm::radians(
      (inverted ? deltaY : -deltaY) * sensitivity * deltaTime);

  glm::quat xQuat = glm::angleAxis(xRadians, glm::vec3(0.f, 1.f, 0.f));
  glm::quat yQuat = glm::angleAxis(yRadians, glm::vec3(1.f, 0.f, 0.f));

  modelMatrix = glm::mat4_cast(xQuat * yQuat) * modelMatrix;
}

void PointCloud::incrementPointSize() {
  pointSize += 0.1f;
}

void PointCloud::decrementPointSize() {
  if (pointSize > 1.f) {
    pointSize -= 0.1f;
  } else {
    pointSize = 1.f;
  }
}

float PointCloud::getPointSize() const {
  return pointSize;
}

void PointCloud::buffer() {
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  buffers.uploadToGPU();
}

void PointCloud::draw() const {
  glBindVertexArray(vao);
  glDrawArrays(GL_POINTS, 0, buffers.getNumPoints());
}

const glm::mat4& PointCloud::getModelMatrix() const {
  return modelMatrix;
}

const Buffers& PointCloud::getBuffers() const {
  return buffers;
}

const BoundingBox& PointCloud::getBoundingBox() const {
  return bbox;
}

std::string PointCloud::getFileExtension(const std::string& filepath) {
  std::size_t dot = filepath.rfind('.');
  if (dot == std::string::npos) {
    return "";
  }
  return filepath.substr(dot);
}

PointCloud PointCloud::loadPLY(const std::string& filepath,
                               std::optional<unsigned int> pointLimit) {
  miniply::PLYReader reader(filepath.c_str());
  if (!reader.valid()) {
    std::cerr << "Error: Failed to open " << filepath << std::endl;
    std::exit(EXIT_FAILURE);
  }

  unsigned int numPoints = 0;
  std::unique_ptr<glm::vec3[]> positions;
  std::unique_ptr<glm::u8vec3[]> colours;
  uint32_t posIndexes[3];
  uint32_t colourIndexes[3];
  bool hasColours = false;

  while (reader.has_element()) {
    if (!reader.element_is(miniply::kPLYVertexElement)) {
      reader.next_element();
      continue;
    }

    const unsigned int filePointCount = reader.num_rows();
    if (pointLimit && *pointLimit < filePointCount) {
      miniply::PLYElement* vertexElement =
          reader.get_element(reader.find_element(miniply::kPLYVertexElement));
      vertexElement->count = *pointLimit;
    }

    if (!reader.load_element() || !reader.find_pos(posIndexes)) {
      reader.next_element();
      continue;
    }

    numPoints = reader.num_rows();
    positions = std::make_unique<glm::vec3[]>(numPoints);
    colours = std::make_unique<glm::u8vec3[]>(numPoints);

    std::cout << "PLY reader:" << std::endl;
    std::cout << "  - Found position property" << std::endl;
    std::cout << "  - " << filePointCount << " points" << std::endl;
    if (numPoints < filePointCount) {
      std::cout << "  - Reading first " << numPoints
                << " points due to point buffer budget" << std::endl;
    }

    if (reader.element()->properties[posIndexes[0]].type !=
        miniply::PLYPropertyType::Float) {
      std::cerr << "Error: Position coordinates in " << filepath
                << " must be floats" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    reader.extract_properties(posIndexes, 3, miniply::PLYPropertyType::Float,
                              positions.get());

    if (reader.find_color(colourIndexes)) {
      std::cout << "  - Found colour property" << std::endl;
      reader.extract_properties(colourIndexes, 3,
                                miniply::PLYPropertyType::UChar,
                                colours.get());
      hasColours = true;
    }

    break;
  }

  if (numPoints == 0) {
    std::cerr << "Error: No vertex element found in " << filepath << std::endl;
    std::exit(EXIT_FAILURE);
  }

  if (!hasColours) {
    std::cerr << "Warning: No colour attribute found in PLY file" << std::endl;
    applyGradient(positions.get(), colours.get(), numPoints);
  }

  BoundingBox bbox = createBoundingBox(positions.get(), numPoints);
  Buffers buffers(positions.get(), colours.get(), numPoints);

  return PointCloud(std::move(buffers), bbox);
}

BoundingBox PointCloud::createBoundingBox(const glm::vec3* positionBuffer,
                                          unsigned int numPoints) {
  float fmax = std::numeric_limits<float>::max();
  float fmin = std::numeric_limits<float>::lowest();

  glm::vec3 min(fmax);
  glm::vec3 max(fmin);

  for (unsigned int i = 0; i < numPoints; i++) {
    if (positionBuffer[i].x < min.x) min.x = positionBuffer[i].x;
    if (positionBuffer[i].y < min.y) min.y = positionBuffer[i].y;
    if (positionBuffer[i].z < min.z) min.z = positionBuffer[i].z;
    if (positionBuffer[i].x > max.x) max.x = positionBuffer[i].x;
    if (positionBuffer[i].y > max.y) max.y = positionBuffer[i].y;
    if (positionBuffer[i].z > max.z) max.z = positionBuffer[i].z;
  }

  return BoundingBox(min, max, true);
}

// temporary fallback until point clouds without colour can be lit properly.
void PointCloud::applyGradient(const glm::vec3* positionBuffer,
                               glm::u8vec3* colourBuffer,
                               unsigned int numPoints) {
  float baseBrightness = 0.1f;

  float zMin = std::numeric_limits<float>::max();
  float zMax = std::numeric_limits<float>::lowest();

  for (unsigned int i = 0; i < numPoints; i++) {
    if (positionBuffer[i].z < zMin) zMin = positionBuffer[i].z;
    if (positionBuffer[i].z > zMax) zMax = positionBuffer[i].z;
  }

  const float zRange = zMax - zMin;
  for (unsigned int i = 0; i < numPoints; i++) {
    float normalised =
        zRange > 0.f ? (positionBuffer[i].z - zMin) / zRange : 0.f;
    float brightness = baseBrightness + (1.f - baseBrightness) * normalised;
    unsigned char colour = static_cast<unsigned char>(brightness * 255);
    colourBuffer[i] = glm::u8vec3(colour);
  }
}
