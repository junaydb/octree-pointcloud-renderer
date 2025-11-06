#pragma once
#include <SDL2/SDL_timer.h>

class Timer {
 public:
  Timer();
  ~Timer();

  void start();
  void end();
  void updateAverages();
  int getFPS() const;
  float getMS() const;
  int getAvgFPS() const;
  float getAvgMS() const;

 private:
  Uint64 startTime;
  Uint64 endTime;
  float elapsedMS;
  int FPS;
  int ticks;
  float sumMS;
};
