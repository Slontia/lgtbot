// Author:  Dva
// Date:    2022.10.5

#include <array>
#include <cstring>
#include <ctime>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <random>

#include "game_framework/game_main.h"
#include "game_framework/game_options.h"
#include "game_framework/game_stage.h"
#include "utility/msg_checker.h"

const std::string k_game_name = "远足";
const uint64_t k_max_player = 0; /* 0 means no max-player limits */
const uint64_t k_multiple = 0;
const std::string k_developer = "dva";
const std::string k_description = "通过计算和放置数字，争取分数的游戏";

const std::map<int, char> op_char = {{0, '+'}, {1, '-'}, {2, '*'}, {3, '>'}, {4, '<'}};

std::string GameOption::StatusInfo() const {
  std::stringstream ss;
  ss << "每回合时间限制：" << GET_VALUE(时限) << "秒";
  return ss.str();
}

bool GameOption::ToValid(MsgSenderBase& reply) { return true; }

uint64_t GameOption::BestPlayerNum() const { return 6; }

// ========== GameLogic ==========

char map[32][32], mappx[255], mappy[255];
int n, m, turn = 1;
char lines[128][32], lx[128][32], ly[128][32];
int line_cnt;
int random_seed;
struct board {
  int score;
  int opcnt[7];
  int num[32][32];
  board() {}
};

void generate_two_num(int& a, int& b, int* c) {
  int x = rand() % 100;
  if (x < 2) {
    a = 2, b = 13;
  } else if (x < 4) {
    a = 2, b = 17;
  } else if (x < 6) {
    a = 2, b = 18;
  } else if (x < 8) {
    a = 3, b = 11;
  } else if (x < 10) {
    a = 3, b = 12;
  } else {
    a = rand() % 6 + 1;
    b = rand() % 6 + 1;
    if (a == b && (rand() % 2 == 1)) {
      b = 7;
    } else if (a > b) {
      std::swap(a, b);
    }
  }
  c[0] = a + b;
  c[1] = b - a;
  c[2] = a * b;
  c[3] = b;
  c[4] = a;
}

void readMap(const std::string& path) {
  std::ifstream fin(path);
  turn = 0;
  for (int i = 0; i < 31; i++) {
    for (int j = 0; j < 31; j++) {
      map[i][j] = '.';
    }
    map[i][31] = 0;
  }
  for (int i = 1; i < 32; i++) {
    int _m = 0;
    map[i][0] = '.';
    for (int j = 1; j < 32; j++) {
      map[i][j] = fin.get();
      if (map[i][j] == EOF) {
        map[i][j] = '.';
        n = i;
        for (int i = 0; i < 31; i++) map[0][i] = map[n + 1][i] = '.';
        map[0][31] = map[n + 1][31] = 0;
        return;
      }
      if (map[i][j] == '\n') {
        map[i][j] = '.';
        break;
      }
      if (map[i][j] != '.') {
        turn++;
        if (map[i][j] >= 'A' && map[i][j] <= 'Z')
          map[i][j] -= 'A' - 'a';
        else if (map[i][j] < 'a' || map[i][j] > 'z') {
          std::cout << "Invalid map file! Position (" << i << "," << j << ") can not be "
                    << map[i][j] << std::endl;
        }
        mappx[map[i][j]] = i;
        mappy[map[i][j]] = j;
      }
      m = std::max(m, ++_m);
    }
  }
}

void getLines() {
  int index = 0;
  for (int i = 1; i <= n; i++) {
    for (int j = 0; map[i][j]; j++) {
      if (map[i][j] == '.') continue;
      if (map[i - 1][j] == '.' && map[i + 1][j] != '.') {
        for (int k = i; map[k][j] != '.'; k++) {
          lines[index][k - i] = map[k][j];
          lx[index][k - i] = k;
          ly[index][k - i] = j;
        }
        index++;
      }
      if (map[i][j - 1] == '.' && map[i][j + 1] != '.') {
        for (int k = j; map[i][k] != '.'; k++) {
          lines[index][k - j] = map[i][k];
          lx[index][k - j] = i;
          ly[index][k - j] = k;
        }
        index++;
      }
      if (map[i - 1][j - 1] == '.' && map[i + 1][j + 1] != '.') {
        for (int k = i, l = j; map[k][l] != '.'; k++, l++) {
          lines[index][k - i] = map[k][l];
          lx[index][k - i] = k;
          ly[index][k - i] = l;
        }
        index++;
      }
    }
  }
  line_cnt = index;
}

void initBoard(board& b) {
  for (int i = 1; i <= n; i++) {
    for (int j = 1; j <= m; j++) {
      b.num[i][j] = -99;
    }
  }
  for (int i = 0; i < 5; i++) {
    b.opcnt[i] = 5;
  }
  b.score = 0;
}

int calc(const board& b) {
  auto& num = b.num;
  int sum = 0;
  for (int i = 1; i <= n; i++) {
    for (int j = 1; map[i][j]; j++) {
      if (map[i][j] == '.') continue;
      if (num[i][j] == -99) continue;
      if ((map[i - 1][j] != '.' && num[i - 1][j] == num[i][j]) ||
          (map[i][j - 1] != '.' && num[i][j - 1] == num[i][j]) ||
          (map[i - 1][j - 1] != '.' && num[i - 1][j - 1] == num[i][j]) ||
          (map[i + 1][j] != '.' && num[i + 1][j] == num[i][j]) ||
          (map[i][j + 1] != '.' && num[i][j + 1] == num[i][j]) ||
          (map[i + 1][j + 1] != '.' && num[i + 1][j + 1] == num[i][j])) {
        sum += num[i][j];
      }
    }
  }
  for (int i = 0; i < line_cnt; i++) {
    int len = strlen(lines[i]);
    if (len < 2) continue;
    // check if this line is arthematic sequence
    int max = std::max(0, num[lx[i][0]][ly[i][0]]),
        d = num[lx[i][1]][ly[i][1]] - num[lx[i][0]][ly[i][0]];
    if (d == 1 || d == -1) {
      for (int j = 1; j < len; j++) {
        if (num[lx[i][j]][ly[i][j]] == -99 ||
            num[lx[i][j]][ly[i][j]] - num[lx[i][j - 1]][ly[i][j - 1]] != d) {
          max = 0;
          break;
        }
        max = std::max(max, num[lx[i][j]][ly[i][j]]);
      }
      sum += max * len;
    }
    // std::cout << lines[i] << " " << max << " " << len << " " << sum << std::endl;
  }
  return sum;
}

void initGame(const std::string& map_path, std::optional<int> seed = std::nullopt) {
  srand(::random_seed = seed.value_or(clock() ^ rand()));
  readMap(map_path);
  std::cout << n << " " << m << std::endl;
  for (int i = 0; i < n + 2; i++) {
    std::cout << map[i] << std::endl;
  }
  getLines();
}

// ========== UI ==============

struct MyUI {
  MyUI(int player_num) : player_num_(player_num), boards(player_num), names(player_num) {}

  void SetBoard(int index, const board& bd) {
    std::stringstream html;
    html << "<p>" + names[index] + " 当前分数： " << bd.score << "</p>";
    for (int i = 1; i <= n; i++) {
      html << R"(<div style="display:inline-flex;margin-left:)" << (40 - 20 * i) << R"(px;">)";
      for (int j = 1; j <= m && map[i][j]; j++) {
        if (map[i][j] == '.') {
          html << R"(<div class="block"></div>)";
        } else if (bd.num[i][j] == -99) {
          html << R"(<div class="block border">)" << map[i][j] << R"(</div>)";
        } else {
          html << R"(<div class="block border purple">)" << bd.num[i][j] << R"(</div>)";
        }
      }
      html << R"(</div>)";
    }
    html << "<p style=\"font-family: 黑体; font-size: 18px;\">剩余运算：+ " << bd.opcnt[0]
         << "&nbsp; – " << bd.opcnt[1] << "&nbsp; × " << bd.opcnt[2] << "&nbsp; &gt; "
         << bd.opcnt[3] << "&nbsp; &lt; " << bd.opcnt[4] << "</p>";

    boards[index] = html.str();
  }

  std::string ToHtml() {
    std::stringstream html;
    html
        << R"(<style>p{text-align: center;}.block{width: 40px; height: 36px; display: flex; justify-content: center; align-items: center; font-size: 20px;})"
        << R"(.border{outline: 2px solid #666; outline-offset: -1px; background-color: #fefeee;}.purple{background-color: #f6f6f6;}</style>)"
        << R"(<div style="width: 700px; display: flex; flex-wrap: wrap; justify-content: space-between; overflow: hidden;">)";
    for (int i = 0; i < player_num_; i++) {
      html << R"(<div style="width: 300px; background-color: #ebfcfd; padding: 15px;">)"
           << boards[i] << R"(</div>)";
    }
    html << R"(</div>)";
    std::string result = html.str();
    // std::cout << result << std::endl;
    return result;
  }

  void SetName(int index, const std::string& name) { names[index] = name.substr(0, 24); }

 private:
  int player_num_;
  std::vector<std::string> names, boards;
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage> {
 public:
  MainStage(const GameOption& option, MatchBase& match);
  virtual VariantSubStage OnStageBegin() override;
  virtual VariantSubStage NextSubStage(RoundStage& sub_stage, const CheckoutReason reason) override;
  int64_t PlayerScore(const PlayerID pid) const;

  bool JudgeOver();

  MyUI ui_;
  int turn_;
  int num_player_;
  std::vector<int> side_;
  std::vector<int64_t> score_;
  std::vector<board> boards_;
};

class RoundStage : public SubGameStage<> {
 public:
  RoundStage(MainStage& main_stage)
      : GameStage(main_stage, "第" + std::to_string(++main_stage.turn_) + "回合",
                  MakeStageCommand(
                      "设置数字", &RoundStage::Set_, AnyArg("位置", "a"),
                      AlterChecker<int>({{"+", 0}, {"-", 1}, {"*", 2}, {">", 3}, {"<", 4}}))) {}

  virtual void OnStageBegin() override {
    generate_two_num(num_1, num_2, number);
    for (int i = 0; i < main_stage().num_player_; i++) {
      main_stage().ui_.SetBoard(i, main_stage().boards_[i]);
      main_stage().ui_.SetName(i, PlayerName(i));
    }
    Boardcast() << Markdown(main_stage().ui_.ToHtml(), 700);
    Boardcast() << ("本回合数字：" + std::to_string(num_1) + " " + std::to_string(num_2));
    StartTimer(option().GET_VALUE(时限));
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) {
    Boardcast() << "玩家 " << PlayerName(pid) << " 中途退出。";
    return StageErrCode::CHECKOUT;
  }

  virtual CheckoutErrCode OnTimeout() override {
    for (PlayerID pid = 0; pid < option().PlayerNum(); ++pid) {
      if (!IsReady(pid)) {
        Boardcast() << At(pid) << "超时未做选择被淘汰，谢谢你垫底侠";
        Eliminate(pid);
      }
    }
    return StageErrCode::CHECKOUT;
  }

  virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override {
    return StageErrCode::READY;
  }

  AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                      const std::string& pos, const int op) {
    if (IsReady(pid)) {
      reply() << "您已经设置过，无法重复设置";
      return StageErrCode::FAILED;
    }
    auto& board = main_stage().boards_[pid];
    if (pos.length() != 1) {
      reply() << "位置格式错误，应为一位字母，如 a";
      return StageErrCode::FAILED;
    }
    if (board.opcnt[op] <= 0) {
      reply() << "运算符 " << op_char.at(op) << " 已经用完";
      return StageErrCode::FAILED;
    }
    int p = pos[0];
    if (pos[0] >= 'A' && pos[0] <= 'Z')
      p -= 'A' - 'a';
    else if (pos[0] < 'a' || pos[0] > 'z') {
      reply() << "位置格式错误，应为一位字母，如 a";
      return StageErrCode::FAILED;
    }
    int px = mappx[p], py = mappy[p];
    if (board.num[px][py] != -99) {
      reply() << "该位置已经被填过了，试试其它位置吧";
      return StageErrCode::FAILED;
    }
    board.num[px][py] = number[op];
    board.opcnt[op]--;
    const auto point = calc(board) - main_stage().score_[pid];
    auto sender = reply();
    sender << "设置数字" << number[op] << "成功";
    if (point > 0) {
      sender << "，本次操作收获" << point << "点积分";
      main_stage().score_[pid] += point;
      board.score += point;
    }
    return StageErrCode::READY;
  }

 private:
  int num_1, num_2;
  int number[7];
};

MainStage::MainStage(const GameOption& option, MatchBase& match)
    : GameStage(option, match), ui_(option.PlayerNum()), turn_(0), num_player_(option.PlayerNum()) {
  initGame(std::string(option.ResourceDir()) + "/map.txt");
  boards_.resize(option.PlayerNum());
  score_.resize(option.PlayerNum(), 0);
  for (int i = 0; i < option.PlayerNum(); i++) {
    initBoard(boards_[i]);
  }
}

MainStage::VariantSubStage MainStage::OnStageBegin() { return std::make_unique<RoundStage>(*this); }

MainStage::VariantSubStage MainStage::NextSubStage(RoundStage& sub_stage,
                                                   const CheckoutReason reason) {
  if (JudgeOver()) {
    return {};
  }
  return std::make_unique<RoundStage>(*this);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const { return score_.at(pid); }

bool MainStage::JudgeOver() {
  // Boardcast() << Markdown(ui_.ToHtml(), 700);
  return turn_ >= ::turn;
}

MainStageBase* MakeMainStage(MsgSenderBase& reply, GameOption& options, MatchBase& match) {
  if (!options.ToValid(reply)) {
    return nullptr;
  }
  return new MainStage(options, match);
}
