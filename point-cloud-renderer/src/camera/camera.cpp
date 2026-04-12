#include <camera/camera.h>
#include <glm/gtc/quaternion.hpp>

Camera::Camera()
    // start a bit further back so the point cloud fits in view on startup.
    : position(0.f, 0.f, 100.f),
      // the camera starts facing forward in world space.
      viewDirection(0.f, 0.f, -1.f),
      up(0.f, 1.f, 0.f),
      // strafe along the vector perpendicular to view direction and up.
      strafeDirection(1.f, 0.f, 0.f) {
}

void Camera::moveForward() {
  position += viewDirection * speed * deltaTime;
}

void Camera::moveBackward() {
  position -= viewDirection * speed * deltaTime;
}

void Camera::moveUp() {
  position += up * speed * deltaTime;
}

void Camera::moveDown() {
  position -= up * speed * deltaTime;
}

void Camera::strafeRight() {
  position += strafeDirection * speed * deltaTime;
}

void Camera::strafeLeft() {
  position -= strafeDirection * speed * deltaTime;
}

void Camera::reset() {
  position = glm::vec3(0.f, 0.f, 100.f);
  viewDirection = glm::vec3(0.f, 0.f, -1.f);
  strafeDirection = glm::vec3(1.f, 0.f, 0.f);
}

void Camera::setSpeed(float movementSpeed) {
  speed = (movementSpeed > 0) ? movementSpeed : 0.1f;
}

void Camera::setDeltaTime(float deltaTime) {
  this->deltaTime = deltaTime;
}

float Camera::getSpeed() const {
  return speed;
}

void Camera::rotate(float deltaX, float deltaY) {
  // only rotate when the mouse is locked to the window.
  if (!mouse.isLocked()) return;

  // convert mouse movement to radians and scale by sensitivity/frame time.
  float xRadians = glm::radians(
      (mouse.isInverted() ? deltaX : -deltaX) * mouse.getSens() * deltaTime);
  float yRadians = glm::radians(
      (mouse.isInverted() ? deltaY : -deltaY) * mouse.getSens() * deltaTime);

  // rotate around the vertical and horizontal camera axes.
  glm::quat xQuat = glm::angleAxis(xRadians, up);
  glm::quat yQuat = glm::angleAxis(yRadians, strafeDirection);
  glm::quat xyQuat = xQuat * yQuat;

  viewDirection = glm::normalize(xyQuat * viewDirection);
  strafeDirection = glm::normalize(glm::cross(viewDirection, up));
}

const glm::vec3& Camera::getPosition() const {
  return position;
}

glm::mat4 Camera::getViewMatrix() const {
  return glm::lookAt(position, position + viewDirection, up);
}
