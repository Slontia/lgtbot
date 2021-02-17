#include <memory>
#include <optional>
#include <gtest/gtest.h>
#include "game_framework/game.h"
#include "game_framework/game_stage.h"
#include "utility/msg_sender.h"

inline std::ostream& operator<<(std::ostream& os, const ErrCode e) { return os << errcode2str(e); }

class MyMsgSender : public MsgSender
{
 public:
  MyMsgSender() {}
  MyMsgSender(const uint64_t uid) : uid_(uid) {}
  virtual ~MyMsgSender()
  {
    if (uid_.has_value())
    {
      std::cout << "[BOT -> " << *uid_ << "]" << std::endl << ss_.str() << std::endl;
    }
    else
    {
      std::cout << "[BOT -> GROUP]" << std::endl << ss_.str() << std::endl;
    }
  }
  virtual void SendString(const char* const str, const size_t len) override
  {
    ss_ << std::string_view(str, len);
  }
  virtual void SendAt(const uint64_t uid) override
  {
    ss_ << "@" << uid;
  }

 private:
  const std::optional<uint64_t> uid_;
  std::stringstream ss_;
};

MsgSender* NewBoardcastMsgSender(void*) { return new MyMsgSender(); }
MsgSender* NewTellMsgSender(void*, const uint64_t pid) { return new MyMsgSender(pid); }
void DeleteMsgSender(MsgSender* const msg_sender) { delete msg_sender; };
void GameOver(void* p, const int64_t scores[]);
void StartTimer(void*, const uint64_t) {}
void StopTimer(void*) {}

template <uint64_t k_player_num>
class TestGame : public testing::Test
{
 public:
  using ScoreArray = std::array<int64_t, k_player_num>;
  static void SetUpTestCase()
  {
    Init(NewBoardcastMsgSender, NewTellMsgSender, DeleteMsgSender, GameOver, StartTimer, StopTimer);
  }

  virtual void SetUp()
  {
    game_ = std::make_unique<Game>(this);
  }

  virtual void SetDown() {}

  bool StartGame() { return game_->StartGame(false /* is_public */, k_player_num); }

  Game& game() { return *game_; }
  auto& expected_scores() const { return expected_scores_; }

 private:
  static void GameOver(void* p, const int64_t src_scores[])
  {
    auto& dst_scores = static_cast<TestGame<k_player_num>*>(p)->expected_scores_.emplace();
    for (size_t i = 0; i < dst_scores.size(); ++ i) { dst_scores.at(i) = src_scores[i]; }
  }

  std::unique_ptr<Game> game_;
  std::optional<ScoreArray> expected_scores_;
};


#define ASSERT_SCORE(scores...)\
do\
{\
  ASSERT_TRUE(expected_scores().has_value()) << "Game not finish";\
  ASSERT_EQ((ScoreArray{ scores }), *expected_scores()) << "Score not match";\
} while (0)

#define START_GAME()\
do\
{\
  std::cout << "[GAME START]" << std::endl;\
  ASSERT_TRUE(StartGame()) << "Start game failed";\
} while (0)

#define TIMEOUT()\
do\
{\
  std::cout << "[TIMEOUT]" << std::endl;\
  bool stage_is_over = false;\
  game().HandleTimeout(&stage_is_over);\
} while (0)

#define PUB_MSG(uid, msg)\
[&] {\
  std::cout << "[USER_" << uid <<  " -> GROUP]" << std::endl << msg << std::endl;\
  return game().HandleRequest((uid), true, (msg));\
}()

#define ASSERT_PUB_MSG(ret, uid, msg) ASSERT_EQ(EC_GAME_REQUEST_##ret, (PUB_MSG(uid, msg))) << "Handle request failed"

#define CHECK_PUB_MSG(ret, uid, msg) (EC_GAME_REQUEST_##ret == PUB_MSG(uid, msg))

#define PRI_MSG(uid, msg)\
[&] {\
  std::cout << "[USER_" << uid <<  " -> BOT]" << std::endl << msg << std::endl;\
  return game().HandleRequest((uid), false, (msg));\
}()

#define ASSERT_PRI_MSG(ret, uid, msg) ASSERT_EQ(EC_GAME_REQUEST_##ret, (PRI_MSG(uid, msg))) << "Handle request failed"

#define CHECK_PRI_MSG(ret, uid, msg) (EC_GAME_REQUEST_##ret == PRI_MSG(uid, msg))

#define GAME_TEST(player_num, test_name)\
using TestGame_##player_num##_##test_name = TestGame<player_num>;\
TEST_F(TestGame_##player_num##_##test_name, test_name)
