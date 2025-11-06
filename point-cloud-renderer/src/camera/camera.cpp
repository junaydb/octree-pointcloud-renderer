#include <camera/camera.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

Camera::Camera()
    : mouse(),
      speed(0.2f),
      // camera position starts a bit further back so the point cloud fits in
      // view on startup
      position(0.f, 0.f, 100.f),
      // position(67.7212f, -14.9501f, -34.3059f),
      // camera starts off facing forward in world space
      viewDirection(0.f, 0.f, -1.f),
      // viewDirection(-0.76121f, 0.551503f, 0.34118f),
      // +y axis is up
      up(0.f, 1.f, 0.f),
      // camera strafes along the vector perpendicular to
      // the view direction and up vector
      strafeDirection(1.f, 0.f, 0.f) {
  /* void */
}

Camera::~Camera() {
  /* void */
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
  if (movementSpeed > 0) {
    speed = movementSpeed;
  } else {
    speed = 0.1f;
  }
}

void Camera::tickSync(float deltaTime) {
  this->deltaTime = deltaTime;
}

float Camera::getSpeed() const {
  return speed;
}

void Camera::rotate(float deltaX, float deltaY) {
  // only rotate if the mouse is locked to the screen
  if (mouse.isLocked()) {
    // convert mouse movement to radians
    float xRadians = glm::radians((mouse.isInverted() ? deltaX : -deltaX) * mouse.getSens() * deltaTime);
    float yRadians = glm::radians((mouse.isInverted() ? deltaY : -deltaY) * mouse.getSens() * deltaTime);
    // create quaternions representing rotations around vertix and horizontal
    // axis
    glm::quat xQuat = glm::angleAxis(xRadians, up);
    glm::quat yQuat = glm::angleAxis(yRadians, strafeDirection);
    // combine quaternion rotations
    glm::quat xyQuat = xQuat * yQuat;

    // rotate the view vector with the quaternion
    viewDirection = xyQuat * viewDirection;
    // update strafe vector
    strafeDirection = glm::cross(viewDirection, up);
  }
}

const glm::vec3& Camera::getPosition() const {
  return position;
}

glm::mat4 Camera::getViewMatrix() const {
  return glm::lookAt(position, position + viewDirection, up);
}
