// Author:  Dva
// Date:    2022.10.5

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// The first parameter is player number. It is a one-player game test.
GAME_TEST(1, player_not_enough) {
  ASSERT_FALSE(StartGame());  // according to |GameOption::ToValid|, the mininum player number is 2
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  return RUN_ALL_TESTS();
}
