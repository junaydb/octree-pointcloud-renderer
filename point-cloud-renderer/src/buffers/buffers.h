#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

struct Buffers {
  Buffers();

  Buffers(glm::vec3* pointPositions, glm::u8vec3* pointColours,
          unsigned short int* indices, unsigned int numIndices,
          unsigned int numPoints);

  // constructor for geo that does not use an index buffer
  Buffers(glm::vec3* pointPositions, glm::u8vec3* pointColours,
          unsigned int numPoints);

  ~Buffers();

  Buffers(const Buffers& original);
  Buffers& operator=(const Buffers& original);

  Buffers(Buffers&& other) noexcept;
  Buffers& operator=(Buffers&& other) noexcept;

  const glm::vec3* getPositionBuffer() const;
  const glm::u8vec3* getColourBuffer() const;
  unsigned int getNumPoints() const;
  unsigned int getNumIndices() const;

  void uploadToGPU();

 private:
  void deallocate();

  glm::vec3* positionBuffer;
  glm::u8vec3* colourBuffer;
  unsigned short int* indexBuffer;
  unsigned int numIndices;
  unsigned int numPoints;
};
