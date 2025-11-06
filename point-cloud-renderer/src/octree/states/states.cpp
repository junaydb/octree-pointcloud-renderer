// This implementation was created by following this video by Michael Grieco:
// https://www.youtube.com/watch?v=L6aYpPAvalI&list=PLysLvOneEETPlOI_PI4mJnocqIpr2cSHS&index=26

#include <octree/states/states.h>

namespace states {
  /*
   * Returns a bool indicating whether the target state is active, i.e., returns
   * a bool indiciating whether the bit at `targetBit` is 1.
   *
   * e.g.
   * target = 4
   *
   * 1 = 00000001
   * 1 << target = 00010000
   *
   * 10001101   <--- states
   * 00010000 & <--- target
   * --------
   * 00000000
   *
   * returns false
   */
  bool isActive(unsigned char* states, int targetBit) {
    return (*states & (1 << targetBit)) == (1 << targetBit);
  }

  /*
   * Activates the target state, i.e., sets the bit at `targetBit` to 1.
   *
   * e.g.
   * target = 4
   *
   * 1 = 00000001
   * 1 << target = 00010000
   *
   * 10001101   <--- states
   * 00010000 | <--- target
   * --------
   * 10011101
   */
  void activateState(unsigned char* states, int targetBit) {
    *states |= 1 << targetBit;
  }
}  // namespace states
