#include <iostream>
#include <regex>
#include <miniply/miniply.h>
#include <point-cloud/point-cloud.h>  // include directly to avoid circular dependency
#include <point-cloud/builder/builder.h>

std::optional<builder::Extension> builder::getFileExt(const std::string& filepath) {
  std::regex plyRegex("\\.ply");
  std::regex lasRegex("\\.las");

  if (std::regex_search(filepath, plyRegex)) return builder::PLY;
  if (std::regex_search(filepath, lasRegex)) return builder::LAS;

  return std::nullopt;
}

PointCloud builder::fromPLY(const std::string& filepath) {
  miniply::PLYReader reader(filepath.c_str());
  if (!reader.valid()) {
    std::cerr << "Error: Failed to open " << filepath << std::endl;
    exit(EXIT_FAILURE);
  }

  unsigned int numPoints = 0;
  unsigned int vecSize = 3;
  glm::vec3* positions = nullptr;
  glm::u8vec3* colours = nullptr;
  uint32_t posIndexes[vecSize];
  uint32_t colourIndexes[vecSize];
  bool pointsProcessed = false;
  bool coloursProcessed = false;

  // while the vertex element has not been processed
  while (reader.has_element() && !pointsProcessed) {
    std::cout << "PLY reader: " << std::endl;
    // process vertex element
    if (reader.element_is(miniply::kPLYVertexElement) &&
        reader.load_element() && reader.find_pos(posIndexes)) {
      // get num points and allocate mem for buffers
      numPoints = reader.num_rows();
      positions = new glm::vec3[numPoints];
      colours = new glm::u8vec3[numPoints];
      std::cout << "  - Found position property" << std::endl;
      std::cout << "  - " << numPoints << " points" << std::endl;

      // check what data type positions are stored as, if positions are stored
      // as doubles, convert them to floats before putting them in `positions`
      if (reader.element()->properties[posIndexes[0]].type == miniply::PLYPropertyType::Float) {
        reader.extract_properties(posIndexes, vecSize, miniply::PLYPropertyType::Float, positions);
      } else {
        // exit if the position data is not stored as floats or doubles
        std::cerr << "Error whilst reading " << filepath << ": Position coordinates must be floats" << std::endl;
        exit(EXIT_FAILURE);
      }

      // get vertex colours if they exist
      if (reader.find_color(colourIndexes)) {
        std::cout << "  - Found colour property" << std::endl;
        reader.extract_properties(colourIndexes, vecSize, miniply::PLYPropertyType::UChar, colours);
        coloursProcessed = true;
      }

      pointsProcessed = true;
    }

    if (pointsProcessed) {
      break;
    }
  }

  if (!pointsProcessed) {
    std::cerr << "Error whilst reading " << filepath << ": No vertex element found" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (!coloursProcessed) {
    std::cerr << "Warning from reading PLY file: No colour attribute found" << std::endl;
    applyGradient(positions, colours, numPoints);
  }

  BoundingBox bbox = createBoundingBox(positions, numPoints);
  PointCloud pc(Buffers{positions, colours, numPoints}, bbox);

  return pc;
}

// PointCloud builder::fromLAS(const std::string& filepath) {
// TODO: add LAS support
// }

BoundingBox builder::createBoundingBox(glm::vec3* positionBuffer, unsigned int numPoints) {
  // initialise min and max points
  glm::vec3 min((float)(~0));   // bitwise complement of 0         = max float
  glm::vec3 max(-(float)(~0));  // neg of bitwise complement of 0  = min float

  for (unsigned int i = 0; i < numPoints; i++) {
    // if x|y|z is smaller than min, store it as the min x|y|z
    if (positionBuffer[i].x < min.x) min.x = positionBuffer[i].x;
    if (positionBuffer[i].y < min.y) min.y = positionBuffer[i].y;
    if (positionBuffer[i].z < min.z) min.z = positionBuffer[i].z;
    // if x|y|z is larger than max, store it as the max x|y|z
    if (positionBuffer[i].x > max.x) max.x = positionBuffer[i].x;
    if (positionBuffer[i].y > max.y) max.y = positionBuffer[i].y;
    if (positionBuffer[i].z > max.z) max.z = positionBuffer[i].z;
  }

  return BoundingBox(min, max, true);
}

// NOTE: temporary function, replace with eye-dome lighting
//
// apply a monochrome gradient when the imported point cloud has no colour
void builder::applyGradient(glm::vec3* positionBuffer, glm::u8vec3* colourBuffer, unsigned int numPoints) {
  float baseBrightness = 0.1f;
  unsigned char remapRange = 255;

  float min((float)(~0));   // bitwise complement of 0         = max float
  float max(-(float)(~0));  // neg of bitwise complement of 0  = min float

  // get min/max of x positions (for normalisation)
  for (unsigned int i = 0; i < numPoints; i++) {
    if (positionBuffer[i].z < min) {
      min = positionBuffer[i].z;
    }
    if (positionBuffer[i].z > max) {
      max = positionBuffer[i].z;
    }
  }

  for (unsigned int i = 0; i < numPoints; i++) {
    // normalise y value of current point
    float normalised = (positionBuffer[i].z - min) / (max - min);
    float withBaseBrightness = baseBrightness + (1 - baseBrightness) * normalised;

    // each point has a grayscale colour, so insert the same value for all
    // colour components.
    // i.e., insert the same value for r, g, and b.
    unsigned char colour = withBaseBrightness * remapRange;
    colourBuffer[i].r = colour;
    colourBuffer[i].g = colour;
    colourBuffer[i].b = colour;
  }
}
