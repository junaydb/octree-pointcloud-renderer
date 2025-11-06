#define SCREEN_SCALE_TARGET 300

#include <algorithm>
#include <boundingbox/boundingbox.h>

BoundingBox::BoundingBox()
    : min(0), max(0) {
  /* void */
}

BoundingBox::BoundingBox(glm::vec3& min, glm::vec3& max, bool uniform)
    : min(min), max(max) {
  int numVerts = 8;
  int vecLen = 3;
  int indexBufferLen = numVerts * vecLen;

  if (uniform) {
    /* ensure the bounding box is a cube (has uniform xyz lengths),
     * this simplifies spatial hash grid calculations */
    // get half the length of the longest component
    float maxExtent = std::max(max.x - min.x, std::max(max.y - min.y, max.z - min.z)) * 0.5f;
    glm::vec3 center = getCenter();
    // add/subtract the longest component to all components of the bounding box
    // center, resulting in a cube
    min = center - maxExtent;
    max = center + maxExtent;
    this->min = min;
    this->max = max;
  }

  // create bounding box vertices from min and max
  glm::vec3* positions = new glm::vec3[numVerts]{
      {min.x, min.y, min.z},  // 1
      {min.x, min.y, max.z},  // 2
      {min.x, max.y, min.z},  // 3
      {min.x, max.y, max.z},  // 4
      {max.x, min.y, min.z},  // 5
      {max.x, min.y, max.z},  // 6
      {max.x, max.y, min.z},  // 7
      {max.x, max.y, max.z}   // 8
  };

  glm::u8vec3* colours = new glm::u8vec3[numVerts];
  for (int i = 0; i < numVerts; i++) {
    colours[i].r = 0;
    colours[i].g = 255;
    colours[i].b = 0;
  }

  unsigned short int* indices = new unsigned short int[indexBufferLen]{
      0, 1, 1, 3, 3, 2, 2, 0,  // bottom face
      4, 5, 5, 7, 7, 6, 6, 4,  // top face
      0, 4, 1, 5, 2, 6, 3, 7   // connecting edges
  };

  buffers = Buffers(positions, colours, indices, indexBufferLen, numVerts);

  delete[] positions;
  delete[] colours;
  delete[] indices;
}

BoundingBox::BoundingBox(const BoundingBox& original) {
  min = original.min;
  max = original.max;
  buffers = original.buffers;
}

BoundingBox& BoundingBox::operator=(const BoundingBox& original) {
  if (this != &original) {
    min = original.min;
    max = original.max;
    buffers = original.buffers;
  }
  return *this;
}

BoundingBox::~BoundingBox() {
  /* void */
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
  return SCREEN_SCALE_TARGET / getScale();
}

float BoundingBox::getBoundingSphereRadius() {
  return getScale() * 0.5f;
}

void BoundingBox::buffer() {
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  buffers.buffer();
}

void BoundingBox::draw() const {
  glBindVertexArray(vao);
  glDrawElements(GL_LINES, buffers.getNumIndices(), GL_UNSIGNED_SHORT, 0);
}
