#pragma once

class Mouse {
 public:
  Mouse();
  Mouse(float sensitivity, bool inverted);
  ~Mouse() = default;

  void lock();
  void unlock();
  void toggleInvert();
  void setInverted(bool invert);
  void setSens(float sensitivity);
  float getSens() const;
  bool isLocked() const;
  bool isInverted() const;

 private:
  float sensitivity;
  bool locked;
  bool inverted;
};
