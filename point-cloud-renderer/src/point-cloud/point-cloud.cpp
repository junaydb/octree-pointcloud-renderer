#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <point-cloud/point-cloud.h>
#include <point-cloud/builder/builder.h>

PointCloud::PointCloud(const Buffers& buffers, const BoundingBox& bbox)
    : buffers(buffers), bbox(bbox), mouse(0.02f, true), pointSize(3.f) {
  /* center and scale the point cloud so it fits in view */
  float scaleAdjustment = bbox.getScreenScaleFactor();
  // translate to center
  modelMatrix = glm::translate(
      glm::mat4(1), glm::vec3(-bbox.getCenter().x * scaleAdjustment,
                              -bbox.getCenter().y * scaleAdjustment,
                              -bbox.getCenter().z * scaleAdjustment));
  // scale
  modelMatrix = glm::scale(modelMatrix, glm::vec3(scaleAdjustment));
}

PointCloud::~PointCloud() {
  /* void */
}

PointCloud PointCloud::build(const std::string& filepath) {
  std::optional<builder::Extension> fileExt = builder::getFileExt(filepath);

  if (fileExt == builder::PLY) {
    return builder::fromPLY(filepath);
  } else if (fileExt == builder::LAS) {
    // return builder::fromLAS(filepath);  // TODO: add LAS support
    std::cerr << "Error whilst reading file: LAS files are not yet supported." << std::endl;
    exit(EXIT_FAILURE);
  } else {
    std::cerr << "Error whilst reading file: Unrecognised file extension.\nMust one of the following: .ply .las" << std::endl;
    exit(EXIT_FAILURE);
  }
}

void PointCloud::rotate(float deltaX, float deltaY) {
  // convert mouse movement to radians
  float xRadians = glm::radians((mouse.isInverted() ? deltaX : -deltaX) * mouse.getSens() * deltaTime);
  float yRadians = glm::radians((mouse.isInverted() ? deltaY : -deltaY) * mouse.getSens() * deltaTime);
  // create quaternions representing rotations around vertix and horizontal
  // axis
  glm::quat xQuat = glm::angleAxis(xRadians, glm::vec3(0.f, 1.f, 0.f));
  glm::quat yQuat = glm::angleAxis(yRadians, glm::vec3(1.f, 0.f, 0.f));
  // combine quaternions
  glm::quat xyQuat = xQuat * yQuat;

  modelMatrix = glm::mat4_cast(xyQuat) * modelMatrix;
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
  buffers.buffer();
}

void PointCloud::draw() const {
  glBindVertexArray(vao);
  glDrawArrays(GL_POINTS, 0, buffers.getNumPoints());
}

void PointCloud::tickSync(float deltaTime) {
  this->deltaTime = deltaTime;
}

const glm::mat4& PointCloud::getModelMatrix() const {
  return modelMatrix;
}
