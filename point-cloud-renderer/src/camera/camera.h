#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <mouse/mouse.h>

class Camera {
 public:
  Mouse mouse;

  Camera();

  void moveForward();
  void moveBackward();
  void moveUp();
  void moveDown();
  void strafeLeft();
  void strafeRight();
  void rotate(float deltaX, float deltaY);
  void setSpeed(float speed);
  void reset();
  void setDeltaTime(float deltaTime);
  float getSpeed() const;
  const glm::vec3& getPosition() const;
  glm::mat4 getViewMatrix() const;

 private:
  float speed = 0.2f;
  float deltaTime = 0.f;
  glm::vec3 position;
  glm::vec3 viewDirection;
  glm::vec3 up;
  glm::vec3 strafeDirection;
};
