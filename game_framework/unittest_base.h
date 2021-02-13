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
  std::optional<std::array<int64_t, k_player_num>> expected_scores_;
};


#define CHECK_SCORE(scores) ASSERT_EQ((scores), game().expected_scores()) << "Score not match"

#define START_GAME()\
do\
{\
  std::cout << "[GAME START]" << std::endl;\
  ASSERT_TRUE(StartGame()) << "Start game failed";\
} while (0)

#define PUB_MSG(ret, uid, msg)\
do\
{\
  std::cout << "[USER_" << uid <<  " -> GROUP]" << std::endl << msg << std::endl;\
  ASSERT_EQ(EC_GAME_REQUEST_##ret, game().HandleRequest((uid), true, (msg))) << "Handle request failed";\
} while (0)

#define PRI_MSG(ret, uid, msg)\
do\
{\
  std::cout << "[USER_" << uid <<  " -> BOT]" << std::endl << msg << std::endl;\
  ASSERT_EQ(EC_GAME_REQUEST_##ret, game().HandleRequest((uid), false, (msg))) << "Handle request failed";\
} while (0)

#define GAME_TEST(player_num, test_name)\
using TestGame_##player_num##_##test_name = TestGame<player_num>;\
TEST_F(TestGame_##player_num##_##test_name, test_name)
