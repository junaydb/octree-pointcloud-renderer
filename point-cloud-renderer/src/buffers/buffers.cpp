#include <buffers/buffers.h>

Buffers::Buffers()
    : positionBuffer(nullptr),
      colourBuffer(nullptr),
      indexBuffer(nullptr),
      numIndices(0),
      numPoints(0) {
  /* void */
}

Buffers::Buffers(glm::vec3* pointPositions,
                 glm::u8vec3* pointColours,
                 unsigned short int* indices, unsigned int numIndices,
                 unsigned int numPoints)
    : positionBuffer(nullptr),
      colourBuffer(nullptr),
      indexBuffer(nullptr),
      numIndices(numIndices),
      numPoints(numPoints) {
  positionBuffer = new glm::vec3[numPoints];
  colourBuffer = new glm::u8vec3[numPoints];
  for (unsigned int i = 0; i < numPoints; i++) {
    positionBuffer[i] = pointPositions[i];
    colourBuffer[i] = pointColours[i];
  }

  if (indices != nullptr && numIndices > 0) {
    indexBuffer = new unsigned short int[numIndices];
    for (unsigned int i = 0; i < numIndices; i++) {
      indexBuffer[i] = indices[i];
    }
  }
}

// constructor for geo that does not use an index buffer
Buffers::Buffers(glm::vec3* pointPositions, glm::u8vec3* pointColours,
                 unsigned int numPoints)
    : positionBuffer(nullptr),
      colourBuffer(nullptr),
      indexBuffer(nullptr),
      numIndices(0),
      numPoints(numPoints) {
  positionBuffer = new glm::vec3[numPoints];
  colourBuffer = new glm::u8vec3[numPoints];
  for (unsigned int i = 0; i < numPoints; i++) {
    positionBuffer[i] = pointPositions[i];
    colourBuffer[i] = pointColours[i];
  }
}

Buffers::~Buffers() {
  deallocate();
}

Buffers::Buffers(const Buffers& original)
    : positionBuffer(nullptr),
      colourBuffer(nullptr),
      indexBuffer(nullptr),
      numIndices(original.numIndices),
      numPoints(original.numPoints) {
  if (numPoints > 0) {
    positionBuffer = new glm::vec3[numPoints];
    colourBuffer = new glm::u8vec3[numPoints];
    for (unsigned int i = 0; i < numPoints; i++) {
      positionBuffer[i] = original.positionBuffer[i];
      colourBuffer[i] = original.colourBuffer[i];
    }
  }

  if (numIndices > 0 && original.indexBuffer != nullptr) {
    indexBuffer = new unsigned short int[numIndices];
    for (unsigned int i = 0; i < numIndices; i++) {
      indexBuffer[i] = original.indexBuffer[i];
    }
  }
}

Buffers& Buffers::operator=(const Buffers& original) {
  if (this != &original) {
    deallocate();

    numPoints = original.numPoints;
    numIndices = original.numIndices;

    if (numPoints > 0) {
      positionBuffer = new glm::vec3[numPoints];
      colourBuffer = new glm::u8vec3[numPoints];
      for (unsigned int i = 0; i < numPoints; i++) {
        positionBuffer[i] = original.positionBuffer[i];
        colourBuffer[i] = original.colourBuffer[i];
      }
    }

    if (numIndices > 0 && original.indexBuffer != nullptr) {
      indexBuffer = new unsigned short int[numIndices];
      for (unsigned int i = 0; i < numIndices; i++) {
        indexBuffer[i] = original.indexBuffer[i];
      }
    }
  }
  return *this;
}

Buffers::Buffers(Buffers&& other) noexcept
    : positionBuffer(other.positionBuffer),
      colourBuffer(other.colourBuffer),
      indexBuffer(other.indexBuffer),
      numIndices(other.numIndices),
      numPoints(other.numPoints) {
  other.positionBuffer = nullptr;
  other.colourBuffer = nullptr;
  other.indexBuffer = nullptr;
  other.numIndices = 0;
  other.numPoints = 0;
}

Buffers& Buffers::operator=(Buffers&& other) noexcept {
  if (this != &other) {
    deallocate();

    positionBuffer = other.positionBuffer;
    colourBuffer = other.colourBuffer;
    indexBuffer = other.indexBuffer;
    numIndices = other.numIndices;
    numPoints = other.numPoints;

    other.positionBuffer = nullptr;
    other.colourBuffer = nullptr;
    other.indexBuffer = nullptr;
    other.numIndices = 0;
    other.numPoints = 0;
  }
  return *this;
}

const glm::vec3* Buffers::getPositionBuffer() const {
  return positionBuffer;
}

const glm::u8vec3* Buffers::getColourBuffer() const {
  return colourBuffer;
}

unsigned int Buffers::getNumPoints() const {
  return numPoints;
}

unsigned int Buffers::getNumIndices() const {
  return numIndices;
}

void Buffers::uploadToGPU() {
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

  deallocate();
}

void Buffers::deallocate() {
  delete[] positionBuffer;
  positionBuffer = nullptr;

  delete[] colourBuffer;
  colourBuffer = nullptr;

  if (indexBuffer != nullptr) {
    delete[] indexBuffer;
    indexBuffer = nullptr;
  }
}
