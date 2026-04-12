#include <SDL2/SDL_mouse.h>

#include <stdexcept>

#include <mouse/mouse.h>

Mouse::Mouse()
    : sensitivity(0.05f), locked(false), inverted(false) {
  lock();
}

Mouse::Mouse(float sensitivity, bool inverted)
    : sensitivity(sensitivity), locked(false), inverted(inverted) {
  lock();
}

void Mouse::lock() {
  if (SDL_SetRelativeMouseMode(SDL_TRUE)) {
    throw std::runtime_error("Failed to enable relative mouse mode");
  }
  locked = true;
}

void Mouse::unlock() {
  if (SDL_SetRelativeMouseMode(SDL_FALSE)) {
    throw std::runtime_error("Failed to disable relative mouse mode");
  }
  locked = false;
}

void Mouse::toggleInvert() {
  inverted = !inverted;
}

void Mouse::setInverted(bool invert) {
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
