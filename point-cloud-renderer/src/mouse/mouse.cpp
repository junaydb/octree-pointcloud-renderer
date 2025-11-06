#include <SDL2/SDL_mouse.h>
#include <glm/gtc/matrix_transform.hpp>
#include <mouse/mouse.h>

Mouse::Mouse() : sensitivity(0.05f), inverted(false) {
  lock();
}

Mouse::Mouse(float sensitivity, bool inverted)
    : sensitivity(sensitivity), inverted(inverted) {
  lock();
}

Mouse::~Mouse() {
  /* void */
}

void Mouse::lock() {
  if (SDL_SetRelativeMouseMode(SDL_TRUE)) {
    exit(EXIT_FAILURE);
  }
  locked = true;
}

void Mouse::unlock() {
  if (SDL_SetRelativeMouseMode(SDL_FALSE)) {
    exit(EXIT_FAILURE);
  }
  locked = false;
}

void Mouse::invert() {
  inverted = !inverted;
}

void Mouse::invert(bool invert) {
  inverted = invert;
}

void Mouse::setSens(float sens) {
  if (sens > 0) {
    sensitivity = sens;
  } else {
    sensitivity = 0.025f;
  }
}

float Mouse::getSens() const {
  return sensitivity;
}

bool Mouse::isLocked() const {
  return locked;
}

bool Mouse::isInverted() const {
  return inverted;
}
