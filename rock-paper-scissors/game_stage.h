#include "msg_checker.h"
#include <memory>

class Game;

template <class MAIN_STAGE>
class GameStage
{
public:
  GameStage(Game& game, MAIN_STAGE& main_stage, std::vector<std::unique_ptr<MsgCheckerInterface>>&& msg_checkers) : 
    game_(game), main_stage_(main_stage), msg_checkers_(msg_checkers) {}

  bool Request(const std::string& msg)
  {
    bool matched = false;
    MsgReader reader(msg);
    for (auto& msg_checker : msg_checkers)
    {
      if (msg_checker->CheckAndCall(reader))
      {
        if (matched) { /* "Matched multiple checkers" */ }
        matched = true;
      }
    }
    return matched;
  }

  std::string GetMsgFormats()
  {
    std::string msg_formats;
    for (auto& msg_checker : msg_checkers)
    {
      msg_formats += msg_checker->Info();
    }
    return msg_formats;
  }

private:
  std::vector<std::unique_ptr<MsgCheckerInterface>> msg_checkers_;
  Game& game_;
  MAIN_STAGE& main_stage_;
};

template <typename MAIN_STAGE>
class CompStage : public GameStage<MAIN_STAGE>
{
 public:
  CompStage(Game& game, MAIN_STAGE& main_stage, std::vector<std::unique_ptr<MsgCheckerInterface>>&& msg_checkers) : 
    GameStage(game, main_stage, msg_checkers) {}


};

template <typename MAIN_STAGE, uint64_t SEC>
class TimeStage : public GameStage<MAIN_STAGE>
{
 public:
  TimeStage(Game& game, MAIN_STAGE& main_stage, std::vector<std::unique_ptr<MsgCheckerInterface>>&& msg_checkers) :
    GameStage(game, main_stage, msg_checkers) {}
};
