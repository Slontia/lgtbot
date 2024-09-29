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

#include "game_framework/stage.h"
#include "game_framework/util.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "远足",
    .developer_ = "dva",
    .description_ = "通过计算和放置数字，争取分数的游戏",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 0; } /* 0 means no max-player limits */
uint32_t Multiple(const CustomOptions& options) { return 2; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};
const std::array<const char*, 5> map_files = {"random", "wang_guo_bian_jing.txt",
                                              "feng_chao_zhi_xue.txt", "crystal_cave.txt",
                                              "fu_rao_zhi_di.txt"};
const std::array<const char*, 5> map_names = {"随机", "王国边境", "蜂巢之血", "水晶洞穴",
                                              "富饶之地"};

const std::map<int, char> op_char = {{0, '+'}, {1, '-'}, {2, '*'}, {3, '>'}, {4, '<'}};
std::map<std::string, int> char_op = {
    {"+", 0},  {"＋", 0}, {"加", 0}, {"-", 1},  {"－", 1}, {"–", 1},
    {"减", 1}, {"*", 2},  {"＊", 2}, {"乘", 2}, {"×", 2},  {">", 3},
    {"＞", 3}, {"大", 3}, {"<", 4},  {"＜", 4}, {"小", 4},
};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options) { return true; }

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                generic_options.bench_computers_to_player_num_ = 6;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== GameLogic ==========

char map[32][32], mappx[255], mappy[255];
int n, m;
char lines[128][32], lx[128][32], ly[128][32];
char zero_x, zero_y;
int line_cnt;
int random_seed;
int num_1, num_2;
struct board {
  int score;
  int opcnt[7];
  int num[32][32];
  board() {}
};
std::string map_name;
int offset, initial_random_cnt, turn;
std::string three_pos;

void generate_two_num(int& a, int& b, int* c) {
  int x = rand() % 180;
  if (x == 0) {
    a = 2, b = 9;
  } else if (x == 1) {
    a = 2, b = 11;
  } else if (x == 2) {
    a = 2, b = 13;
  } else if (x == 3) {
    a = 2, b = 17;
  } else if (x == 4) {
    a = 2, b = 18;
  } else if (x == 5) {
    a = 2, b = 10 + rand() % 10;
  } else if (x == 6) {
    a = 3, b = 9;
  } else if (x == 7) {
    a = 3, b = 10;
  } else if (x == 8) {
    a = 3, b = 11;
  } else if (x == 9) {
    a = 3, b = 12;
  } else if (x == 10) {
    a = 4, b = 8;
  } else if (x == 11) {
    a = 4, b = 9;
  } else if (x == 12) {
    if (rand() % 2 == 0) {
      a = 9, b = 25;
    } else {
      a = 10, b = 24;
    }
  } else if (x == 13) {
    if (rand() % 2 == 0) {
      a = 13, b = 16 + rand() % 2;
    } else {
      a = 11, b = 18 + rand() % 2;
    }
  } else if (x == 14) {
    a = 5, b = 8;
  } else if (x <= 20) {
    a = 1, b = x - 7;
  } else {
    a = rand() % 6 + 1;
    b = rand() % 6 + 1;
    if (rand() % 2 == 1) {
      a += rand() % 2;
      b += rand() % 2 + 1;
    }
    if (a > b) {
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
  zero_x = zero_y = -1;
  for (int i = 0; i < 31; i++) {
    for (int j = 0; j < 31; j++) {
      map[i][j] = '.';
    }
    map[i][31] = 0;
  }
  fin >> offset >> three_pos >> initial_random_cnt >> turn;
  fin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
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
      if (map[i][j] == '0') {
        zero_x = i;
        zero_y = j;
      } else if (map[i][j] != '.') {
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
  for (int i = 0; i < index; i++) {
    std::cout << "Line " << i << ": " << lines[i] << std::endl;
  }
  line_cnt = index;
}

void initBoard(board& b) {
  for (int i = 1; i <= n; i++) {
    for (int j = 1; j <= m; j++) {
      b.num[i][j] = -99;
    }
  }
  if (zero_x != -1) {
    b.num[zero_x][zero_y] = 0;
  }
  for (int i = 0; i < 5; i++) {
    b.opcnt[i] = 5;
  }

  b.score = 0;
}

bool is_three_point(char pos) { return three_pos.find(pos) != std::string::npos; }

int calc_inner(board& b) {
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
        sum += is_three_point(map[i][j]) ? 3 * num[i][j] : num[i][j];
      }
    }
  }
  for (int i = 0; i < line_cnt; i++) {
    int len = strlen(lines[i]);
    if (len < 2) continue;
    // check if this line is arthematic sequence
    int max = std::max(0, num[lx[i][0]][ly[i][0]]),
        d = num[lx[i][1]][ly[i][1]] - num[lx[i][0]][ly[i][0]],
        multiple = is_three_point(map[lx[i][0]][ly[i][0]]) ? 3 : 1;
    if (d == 1 || d == -1) {
      for (int j = 1; j < len; j++) {
        if (num[lx[i][j]][ly[i][j]] == -99 ||
            num[lx[i][j]][ly[i][j]] - num[lx[i][j - 1]][ly[i][j - 1]] != d) {
          max = 0;
          break;
        }
        max = std::max(max, num[lx[i][j]][ly[i][j]]);
        if (is_three_point(map[lx[i][j]][ly[i][j]])) multiple *= 3;
      }
      if (max > 0) {
        sum += multiple * max * len;
      }
    }
    // std::cout << lines[i] << " " << max << " " << len << " " << sum << std::endl;
  }
  return sum;
}

int calc(board& b) {
  if (zero_x == -1) return calc_inner(b);
  int max_score = 0, pos = 0;
  for (int i = 0; i <= 32; i++) {
    b.num[zero_x][zero_y] = i;
    int score = calc_inner(b);
    if (score > max_score) {
      max_score = score;
      pos = i;
    }
  }
  b.num[zero_x][zero_y] = pos;
  return max_score;
}

void initGame(const std::string& map_path, std::optional<int> seed = std::nullopt) {
  srand(random_seed = seed.value_or(clock() ^ rand()));
  readMap(map_path);
  std::cout << n << " " << m << std::endl;
  for (int i = 0; i < n + 2; i++) {
    std::cout << map[i] << std::endl;
  }
  memset(lines, 0, sizeof lines);
  memset(lx, 0, sizeof lx);
  memset(ly, 0, sizeof ly);
  getLines();
}

// ========== UI ==============

struct MyUI {
  MyUI(int player_num) : player_num_(player_num), boards(player_num), names(player_num) {}

  void SetBoard(int index, const board& bd) {
    std::stringstream html;
    html << "<p>" << names[index] << "</p>";
    html << "<p>"
         << "当前分数： " << bd.score << "</p>";
    for (int i = 1; i <= n; i++) {
      html << R"(<div style="display:inline-flex;margin-left:)" << (offset - 20 * i) << R"(px;">)";
      for (int j = 1; j <= m && map[i][j]; j++) {
        if (map[i][j] == '.') {
          html << R"(<div class="block"></div>)";
        } else if (map[i][j] == '0') {
          html << R"(<div class="block border purple"><span class="colorful">)" << bd.num[i][j]
               << R"(</span></div>)";
        } else if (bd.num[i][j] == -99) {
          if (is_three_point(map[i][j])) {
            html << R"(<div class="block border red">)" << map[i][j] << R"(</div>)";
          } else {
            html << R"(<div class="block border">)" << map[i][j] << R"(</div>)";
          }
        } else {
          if (is_three_point(map[i][j])) {
            html << R"(<div class="block border red">)" << bd.num[i][j] << R"(</div>)";
          } else {
            html << R"(<div class="block border purple">)" << bd.num[i][j] << R"(</div>)";
          }
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
    const short width = player_num_ == 1 ? 328 : 688;
    std::stringstream html;
    html
        << R"(<style>.colorful{background-image: linear-gradient(to top, yellow, orange, red, green, blue, purple); -webkit-background-clip: text; -moz-background-clip: text; background-clip: text; color: transparent; })"
        << R"(p{text-align: center;}.block{width: 40px; height: 36px; display: flex; justify-content: center; align-items: center; font-size: 20px;})"
        << R"(.border{outline: 2px solid #666; outline-offset: -1px; background-color: #fefeee;}.purple{background-color: #eeeefe;}.red{background-color: #fdedec;}</style>)"
        << R"(<p style="width: )" << width
        << R"(px;font-size: 30px;font-weight: bold;letter-spacing: 5px;">)" << map_name << "</p>"
        << R"(<div style="width: )" << width
        << R"(px; display: flex; flex-wrap: wrap; justify-content: space-between; overflow: hidden;">)";
    for (int i = 0; i < player_num_; i++) {
      html << R"(<div style="width: 300px; background-color: #ebfcfd; padding: 15px;">)"
           << boards[i] << R"(</div>)";
    }
    html << R"(</div>)";
    std::string result = html.str();
    // std::cout << result << std::endl;
    return result;
  }

  void SetName(int index, const std::string& name) {
    std::string tmp = "";
    for (auto c : name) {
      if (c == '>')
        tmp += "&gt;";
      else if (c == '<')
        tmp += "&lt;";
      else if (c == '&')
        tmp += "&amp;";
      else
        tmp += c;
    }
    names[index] = tmp;
  }

 private:
  int player_num_;
  std::vector<std::string> names, boards;
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage> {
 public:
  MainStage(StageUtility&& utility);
  virtual void FirstStageFsm(SubStageFsmSetter setter) override;
  virtual void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
  int64_t PlayerScore(const PlayerID pid) const;

  bool JudgeOver();
  void Print();

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
      : StageFsm(main_stage, "第" + std::to_string(++main_stage.turn_) + "回合",
                  MakeStageCommand(*this, "设置数字", &RoundStage::Set_, AnyArg("位置", "a"),
                                   AlterChecker<int>(char_op)),
                  MakeStageCommand(*this, "跳过", &RoundStage::Pass_, VoidChecker("pass"))) {}

  virtual void OnStageBegin() override {
    generate_two_num(num_1, num_2, number);
    for (int i = 0; i < Main().num_player_; i++) {
      Main().ui_.SetName(i, Global().PlayerName(i));
      Main().ui_.SetBoard(i, Main().boards_[i]);
    }
    Main().Print();
    Global().Boardcast() << ("本回合数字：" + std::to_string(num_1) + " " + std::to_string(num_2));
    Global().StartTimer(GAME_OPTION(时限));
  }

  virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) {
    Global().Boardcast() << "玩家 " << Global().PlayerName(pid) << " 中途退出。";
    return StageErrCode::CONTINUE;
  }

  virtual CheckoutErrCode OnStageTimeout() override {
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
      if (!Global().IsReady(pid)) {
        Global().Boardcast() << At(pid) << "超时未做选择，跳过回合";
      }
    }
    return StageErrCode::CHECKOUT;
  }

  virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override {
    return StageErrCode::READY;
  }

  AtomReqErrCode Pass_(const PlayerID pid, const bool is_public, MsgSenderBase& reply) {
    if (Global().IsReady(pid)) {
      reply() << "跳过失败：您已经完成落子，无法跳过";
      return StageErrCode::FAILED;
    }
    reply() << "跳过成功";
    return StageErrCode::READY;
  }

  AtomReqErrCode Set_(const PlayerID pid, const bool is_public, MsgSenderBase& reply,
                      const std::string& pos, const int op) {
    if (Global().IsReady(pid)) {
      reply() << "您已经设置过，无法重复设置";
      return StageErrCode::FAILED;
    }
    auto& board = Main().boards_[pid];
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
    const auto point = calc(board) - Main().score_[pid];
    auto sender = reply();
    sender << "设置数字" << number[op] << "成功";
    if (point > 0) {
      sender << "，本次操作收获" << point << "点积分";
      Main().score_[pid] += point;
      board.score += point;
    }
    return StageErrCode::READY;
  }

 private:
  int number[7];
};

MainStage::MainStage(StageUtility&& utility)
    : StageFsm(std::move(utility)), ui_(Global().PlayerNum()), turn_(0), num_player_(Global().PlayerNum()) {
  auto map_file = map_files[GAME_OPTION(地图)];
  map_name = map_names[GAME_OPTION(地图)];
  if (map_file == "random") {
    int index = rand() % (map_files.size() - 1) + 1;
    map_file = map_files[index];
    map_name = map_names[index];
  }
  initGame(std::string(Global().ResourceDir()) + "/" + map_file);
  boards_.resize(Global().PlayerNum());
  score_.resize(Global().PlayerNum(), 0);
  for (int i = 0; i < Global().PlayerNum(); i++) {
    initBoard(boards_[i]);
  }
  for (int i = 0; i < initial_random_cnt; i++) {
    int val = 3 + rand() % 10;
    int x = 1 + rand() % n, y = 1 + rand() % m;
    if ('a' <= map[x][y] && map[x][y] <= 'z') {
      for (int i = 0; i < Global().PlayerNum(); i++) {
        boards_[i].num[x][y] = val;
      }
    } else
      i--;
  }
}

void MainStage::FirstStageFsm(SubStageFsmSetter setter) { setter.Emplace<RoundStage>(*this); }

void MainStage::NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) {
  if (JudgeOver()) {
    return;
  }
  setter.Emplace<RoundStage>(*this);
}

int64_t MainStage::PlayerScore(const PlayerID pid) const { return score_.at(pid); }

bool MainStage::JudgeOver() { return turn_ >= turn; }

void MainStage::Print() {
  const int width = Global().PlayerNum() == 1 ? 400 : 700;
  Global().Boardcast() << Markdown(ui_.ToHtml(), width);
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

