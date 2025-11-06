#include <timer/timer.h>

Timer::Timer()
    : ticks(1), sumMS(100) {
  /* void */
}

Timer::~Timer() {
  /* void */
}

void Timer::start() {
  startTime = SDL_GetPerformanceCounter();
}

void Timer::end() {
  endTime = SDL_GetPerformanceCounter();
  elapsedMS = ((endTime - startTime) / (float)SDL_GetPerformanceFrequency()) * 1000.f;
  FPS = (int)(1000.f / elapsedMS);
}

void Timer::updateAverages() {
  float endTime = SDL_GetPerformanceCounter();
  float deltaTime = ((endTime - startTime) / (float)SDL_GetPerformanceFrequency()) * 1000.f;
  ticks++;
  sumMS += deltaTime;
}

int Timer::getFPS() const {
  return FPS;
}

float Timer::getMS() const {
  return elapsedMS;
}

int Timer::getAvgFPS() const {
  return (int)(1000.f / (sumMS / ticks));
}

float Timer::getAvgMS() const {
  return sumMS / ticks;
}
