#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

struct Buffers {
  Buffers()
      : positionBuffer(nullptr),
        colourBuffer(nullptr),
        indexBuffer(nullptr),
        numIndices(0),
        numPoints(0) {
    /* void */
  }

  Buffers(glm::vec3* pointPositions, glm::u8vec3* pointColours,
          unsigned short int* indices, unsigned int numIndices,
          unsigned int numPoints)
      : positionBuffer(pointPositions),
        colourBuffer(pointColours),
        indexBuffer(indices),
        numIndices(numIndices),
        numPoints(numPoints) {
    /* void */
  }

  // constructor for geo that does not use an index buffer
  Buffers(glm::vec3* pointPositions, glm::u8vec3* pointColours, unsigned int numPoints)
      : positionBuffer(pointPositions),
        colourBuffer(pointColours),
        indexBuffer(nullptr),
        numIndices(0),
        numPoints(numPoints) {
    /* void */
  }

  Buffers(const Buffers& original) {
    numPoints = original.numPoints;
    numIndices = original.numIndices;

    positionBuffer = new glm::vec3[numPoints];
    colourBuffer = new glm::u8vec3[numPoints];
    for (int i = 0; i < numPoints; i++) {
      positionBuffer[i] = original.positionBuffer[i];
      colourBuffer[i] = original.colourBuffer[i];
    }

    indexBuffer = new unsigned short int[numIndices];
    for (int i = 0; i < numIndices; i++) {
      indexBuffer[i] = original.indexBuffer[i];
    }
  }

  Buffers& operator=(const Buffers& original) {
    if (this != &original) {
      numPoints = original.numPoints;
      numIndices = original.numIndices;

      positionBuffer = new glm::vec3[numPoints];
      colourBuffer = new glm::u8vec3[numPoints];
      for (int i = 0; i < numPoints; i++) {
        positionBuffer[i] = original.positionBuffer[i];
        colourBuffer[i] = original.colourBuffer[i];
      }

      indexBuffer = new unsigned short int[numIndices];
      for (int i = 0; i < numIndices; i++) {
        indexBuffer[i] = original.indexBuffer[i];
      }
    }
    return *this;
  }

  const glm::vec3* getPositionBuffer() const {
    return positionBuffer;
  }
  const glm::u8vec3* getColourBuffer() const {
    return colourBuffer;
  }

  unsigned int getNumPoints() const {
    return numPoints;
  }
  unsigned int getNumIndices() const {
    return numIndices;
  }

  void buffer() {
    /* VAO spec:
     *    3 VBOs:
     *      - VBO for position data
     *      - VBO for colour data
     *      - VBO for index data (bounding boxes only)
     *
     *    Attribute 1 - Position:
     *      - tightly packed in position VBO
     *      - 3 floats for x,y,z coords
     *
     *    Attribute 2 - Colour:
     *      - tightly packed in colour VBO
     *      - 3 unsigned bytes for 8-bit rgb channels
     */
    unsigned int posBuffer;
    glGenBuffers(1, &posBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, posBuffer);
    glBufferData(GL_ARRAY_BUFFER, numPoints * sizeof(glm::vec3), positionBuffer, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

    unsigned int colBuffer;
    glGenBuffers(1, &colBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, colBuffer);
    glBufferData(GL_ARRAY_BUFFER, numPoints * sizeof(glm::u8vec3), colourBuffer, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(glm::u8vec3), 0);

    if (indexBuffer != nullptr) {
      unsigned int bboxIndexBuffer;
      glGenBuffers(1, &bboxIndexBuffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bboxIndexBuffer);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(GLushort), indexBuffer, GL_STATIC_DRAW);
    }

    free();
  }

  void free() {
    delete[] positionBuffer;
    delete[] colourBuffer;
    if (indexBuffer != nullptr) {
      delete[] indexBuffer;
    }
  }

 private:
  glm::vec3* positionBuffer;
  glm::u8vec3* colourBuffer;
  unsigned short int* indexBuffer;
  int numIndices;
  int numPoints;
};
