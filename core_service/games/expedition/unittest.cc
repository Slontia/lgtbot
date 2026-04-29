// Author:  Dva
// Date:    2022.10.5

#include "game_framework/unittest_base.h"

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
