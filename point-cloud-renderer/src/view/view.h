#pragma once

#include <glm/glm.hpp>

struct View {
  int width;
  int height;
  float FOV;
  float Z_NEAR_PLANE;
  float Z_FAR_PLANE;

  float getScreenProjectedSize(float radius, float distance) {
    float slope = tan(glm::radians(FOV) * 0.5f);
    return (float(height) * 0.5f) * (radius / (slope * distance));
  }
};
