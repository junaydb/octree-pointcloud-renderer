#include <algorithm>

#include <boundingbox/boundingbox.h>

static constexpr float screenScaleTarget = 300.0f;

BoundingBox::BoundingBox()
    : min(0), max(0), vao(0) {
}

BoundingBox::BoundingBox(const glm::vec3& min, const glm::vec3& max, bool uniform)
    : min(min), max(max), vao(0) {
  constexpr int numVerts = 8;
  constexpr int indexBufferLen = 24;

  if (uniform) {
    // make the box cubic so spatial hash grid cells stay uniform.
    float maxExtent = std::max(this->max.x - this->min.x,
                               std::max(this->max.y - this->min.y,
                                        this->max.z - this->min.z)) *
                      0.5f;
    glm::vec3 center = getCenter();
    this->min = center - maxExtent;
    this->max = center + maxExtent;
  }

  glm::vec3 lo = this->min;
  glm::vec3 hi = this->max;

  glm::vec3 positions[numVerts] = {
      {lo.x, lo.y, lo.z},
      {lo.x, lo.y, hi.z},
      {lo.x, hi.y, lo.z},
      {lo.x, hi.y, hi.z},
      {hi.x, lo.y, lo.z},
      {hi.x, lo.y, hi.z},
      {hi.x, hi.y, lo.z},
      {hi.x, hi.y, hi.z}};

  glm::u8vec3 colours[numVerts];
  for (int i = 0; i < numVerts; i++) {
    colours[i] = {0, 255, 0};
  }

  unsigned short int indices[indexBufferLen] = {
      0, 1, 1, 3, 3, 2, 2, 0,
      4, 5, 5, 7, 7, 6, 6, 4,
      0, 4, 1, 5, 2, 6, 3, 7};

  buffers = Buffers(positions, colours, indices, indexBufferLen, numVerts);
}

BoundingBox::BoundingBox(const BoundingBox& original)
    : buffers(original.buffers),
      min(original.min),
      max(original.max),
      vao(0) {
}

BoundingBox& BoundingBox::operator=(const BoundingBox& original) {
  if (this != &original) {
    min = original.min;
    max = original.max;
    buffers = original.buffers;

    if (vao != 0) {
      glDeleteVertexArrays(1, &vao);
      vao = 0;
    }
  }
  return *this;
}

BoundingBox::~BoundingBox() {
  if (vao != 0) {
    glDeleteVertexArrays(1, &vao);
  }
}

glm::vec3 BoundingBox::getCenter() const {
  return (min + max) * 0.5f;
}

glm::vec3 BoundingBox::getDimensions() const {
  return max - min;
}

glm::vec3 BoundingBox::getMin() const {
  return min;
}

glm::vec3 BoundingBox::getMax() const {
  return max;
}

float BoundingBox::getScale() const {
  return glm::length(getDimensions());
}

float BoundingBox::getScreenScaleFactor() const {
  return screenScaleTarget / getScale();
}

float BoundingBox::getBoundingSphereRadius() const {
  return getScale() * 0.5f;
}

void BoundingBox::buffer() {
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  buffers.uploadToGPU();
}

void BoundingBox::draw() const {
  glBindVertexArray(vao);
  glDrawElements(GL_LINES, buffers.getNumIndices(), GL_UNSIGNED_SHORT, 0);
}
