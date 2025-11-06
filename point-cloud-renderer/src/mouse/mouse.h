#pragma once
#include <glm/glm.hpp>

class Mouse {
 public:
  Mouse();
  Mouse(float sensitivity, bool inverted);
  ~Mouse();

  void lock();
  void unlock();
  void invert();
  void invert(bool invert);
  void setSens(float sensitivity);
  float getSens() const;
  bool isLocked() const;
  bool isInverted() const;

 private:
  float sensitivity;
  bool locked;
  bool inverted;
};
