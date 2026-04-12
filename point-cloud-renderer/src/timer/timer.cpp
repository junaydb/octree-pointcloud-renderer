#include <timer/timer.h>

void Timer::start() {
  startTime = SDL_GetPerformanceCounter();
}

void Timer::end() {
  endTime = SDL_GetPerformanceCounter();
  elapsedMS =
      static_cast<float>(endTime - startTime) /
      static_cast<float>(SDL_GetPerformanceFrequency()) * 1000.f;
  FPS = static_cast<int>(1000.f / elapsedMS);
}

void Timer::updateAverages() {
  Uint64 now = SDL_GetPerformanceCounter();
  float deltaTime =
      static_cast<float>(now - startTime) /
      static_cast<float>(SDL_GetPerformanceFrequency()) * 1000.f;
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
  if (ticks == 0) return 0;
  return static_cast<int>(1000.f / (sumMS / ticks));
}

float Timer::getAvgMS() const {
  if (ticks == 0) return 0.f;
  return sumMS / ticks;
}
