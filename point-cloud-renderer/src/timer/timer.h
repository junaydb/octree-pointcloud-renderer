#pragma once
#include <SDL2/SDL_timer.h>

class Timer {
 public:
  Timer() = default;
  ~Timer() = default;

  void start();
  void end();
  void updateAverages();
  int getFPS() const;
  float getMS() const;
  int getAvgFPS() const;
  float getAvgMS() const;

 private:
  Uint64 startTime = 0;
  Uint64 endTime = 0;
  float elapsedMS = 0.f;
  int FPS = 0;
  int ticks = 0;
  float sumMS = 0.f;
};
