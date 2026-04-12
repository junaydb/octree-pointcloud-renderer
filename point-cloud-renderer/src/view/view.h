#pragma once

#include <cmath>

#include <glm/glm.hpp>

struct View {
  int width = 0;
  int height = 0;
  float fov = 70.0f;
  float zNearPlane = 0.1f;
  float zFarPlane = 1000.0f;

  float getScreenProjectedSize(float radius, float distance) const {
    if (distance <= 0.0f) return 0.0f;
    float slope = std::tan(glm::radians(fov) * 0.5f);
    return (float(height) * 0.5f) * (radius / (slope * distance));
  }
};
