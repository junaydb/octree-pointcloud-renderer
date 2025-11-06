// This implementation was created by following this video by Michael Grieco:
// https://www.youtube.com/watch?v=L6aYpPAvalI&list=PLysLvOneEETPlOI_PI4mJnocqIpr2cSHS&index=26

#pragma once

namespace states {
  bool isActive(unsigned char* states, int targetBit);
  void activateState(unsigned char* states, int targetBit);
}  // namespace states
