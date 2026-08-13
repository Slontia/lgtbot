// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <regex>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

#include "board.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "彩虹奇兵", // the game name which should be unique among all the games
    .developer_ = "铁蛋",
    .description_ = "挖宝游戏。推理宝藏、炸弹位置。分高获胜！",
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 12; }
uint32_t Multiple(const CustomOptions& options) { return 1; }
const MutableGenericOptions k_default_generic_options;
const std::vector<RuleCommand> k_rule_commands = {};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    uint32_t& map_option = GET_OPTION_VALUE(game_options, 地图);
    const int32_t size_option = GET_OPTION_VALUE(game_options, 边长);
    const int32_t treasure_option = GET_OPTION_VALUE(game_options, 宝藏);
    const int32_t bomb_option = GET_OPTION_VALUE(game_options, 炸弹);
    const int32_t humidifier_option = GET_OPTION_VALUE(game_options, 加湿器);
    const int32_t ink_option = GET_OPTION_VALUE(game_options, 墨水瓶);
    
    if (size_option == -1) {
        const char* map_names[] = {"中地图", "大地图", "特大地图"};
        uint32_t player_num = generic_options_readonly.PlayerNum();
        for (int i = 0; i < 3; i++) {
            if (player_num > maps[i].limit && player_num <= maps[i + 1].limit && map_option < i + 1) {
                map_option = i + 1;
                reply() << "[警告] 玩家数 " << player_num << " 超出当前地图人数限制，自动将大小调整为 " << map_names[i];
                break;
            }
        }
    } else {
        reply() << "[警告] 本局游戏使用自定义边长配置，地图大小为 " << size_option << "*" << size_option;
    }

    vector<int32_t> origin_proportion= {1, 1, 1, 1, 1, 1};
    vector<int32_t> &modified = GET_OPTION_VALUE(game_options, 配比);
    if (modified != origin_proportion) {
        if (modified.size() < 6) {
            modified.insert(modified.end(), 6 - modified.size(), 0);
        } else if (modified.size() > 6) {
            modified.resize(6);
        }
        if (all_of(modified.begin(), modified.end(), [](int n) { return n == 0; })) {
            reply() << "[错误] 无效配置：配比不能全部为 0，请修正配置！";
            return false;
        }
        reply() << "[警告] 本局6种颜色的配比非默认配置\n"
                << "·当前随机权重为：\n"
                << "R红(" << modified[0] << ")、B蓝(" << modified[1] << ")、Y黄(" << modified[2] << ")\n"
                << "O橙(" << modified[3] << ")、G绿(" << modified[4] << ")、P紫(" << modified[5] << ")";
    }
    
    srand((unsigned int)time(NULL));
    const int32_t max_limit = size_option >= 0 ? size_option * size_option : maps[map_option].size * maps[map_option].size;
    const int32_t treasure = treasure_option >= 0 ? treasure_option : maps[map_option].treasure;
    const int32_t bomb = bomb_option >= 0 ? bomb_option : maps[map_option].bomb;
    const int32_t humidifier = humidifier_option >= 0 ? humidifier_option : 0;
    const int32_t ink = ink_option >= 0 ? ink_option : 0;

    int32_t total_special = 0;
    if (GET_OPTION_VALUE(game_options, 道具) == 1) {
        total_special = max(humidifier + ink, maps[map_option].special);
    }
    int32_t total_num = treasure + bomb + total_special;
    if (total_num > max_limit) {
        reply() << "[错误] 无效配置：当前地图大小最多可容纳非空地元素 " << max_limit << " 个，当前为：" << total_num << "（" << treasure << "+" << bomb << "+" << total_special << "），请修正配置！";
        return false;
    }

    if (generic_options_readonly.PlayerNum() < 2) {
        reply() << "该游戏至少 2 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("使用预设地图类型",
        [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const uint32_t& map_option, const uint32_t& item_mode) {
            GET_OPTION_VALUE(game_options, 地图) = map_option;
            GET_OPTION_VALUE(game_options, 道具) = item_mode;
            return NewGameMode::MULTIPLE_USERS;
        },
        AlterChecker<uint32_t>({{"小地图", 0}, {"中地图", 1}, {"大地图", 2}, {"特大地图", 3}}),
        OptionalDefaultChecker<AlterChecker<uint32_t>>(0, map<string, uint32_t>{{"经典", 0}, {"全部", 1}})),
    InitOptionsCommand("自定义游戏配置（边长、生命、道具数量）",
        [] (
            CustomOptions& game_options,
            MutableGenericOptions& generic_options,
            const int32_t& size_option,
            const int32_t& hp_option,
            const int32_t& treasure_option,
            const int32_t& bomb_option,
            const int32_t& humidifier_option,
            const int32_t& ink_option
        ) {
            uint32_t& map_option = GET_OPTION_VALUE(game_options, 地图);
            for (int i = 3; i >= 0; i--) {
                if (size_option >= maps[i].size && map_option < i) {
                    map_option = i;
                    break;
                }
            }
            GET_OPTION_VALUE(game_options, 边长) = size_option;
            GET_OPTION_VALUE(game_options, 生命) = hp_option;
            GET_OPTION_VALUE(game_options, 宝藏) = treasure_option;
            GET_OPTION_VALUE(game_options, 炸弹) = bomb_option;
            GET_OPTION_VALUE(game_options, 加湿器) = humidifier_option;
            GET_OPTION_VALUE(game_options, 墨水瓶) = ink_option;
            if (humidifier_option >= 0 || ink_option >= 0) {
                GET_OPTION_VALUE(game_options, 道具) = 1;
            }
            return NewGameMode::MULTIPLE_USERS;
        },
        ArithChecker<int32_t>(4, 20, "边长"),
        OptionalDefaultChecker<ArithChecker<int32_t>>(-1, 1, 10, "生命"),
        OptionalDefaultChecker<ArithChecker<int32_t>>(-1, 0, 200, "宝藏"),
        OptionalDefaultChecker<ArithChecker<int32_t>>(-1, 0, 200, "炸弹"),
        OptionalDefaultChecker<ArithChecker<int32_t>>(-1, 0, 200, "加湿器"),
        OptionalDefaultChecker<ArithChecker<int32_t>>(-1, 0, 200, "墨水瓶")),
    InitOptionsCommand("独自一人开始游戏",
        [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const uint32_t& map_option, const uint32_t& item_mode)
        {
            GET_OPTION_VALUE(game_options, 地图) = map_option;
            GET_OPTION_VALUE(game_options, 道具) = item_mode;
            generic_options.bench_computers_to_player_num_ = maps[map_option].limit;
            return NewGameMode::SINGLE_USER;
        },
        VoidChecker("单机"),
        OptionalDefaultChecker<AlterChecker<uint32_t>>(1, map<string, uint32_t>{{"小地图", 0}, {"中地图", 1}, {"大地图", 2}, {"特大地图", 3}}),
        OptionalDefaultChecker<AlterChecker<uint32_t>>(0, map<string, uint32_t>{{"经典", 0}, {"全部", 1}})),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看当前游戏进展情况", &MainStage::Status_, VoidChecker("赛况")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
        , board(Global().ResourceDir(), Global().PlayerNum())
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 回合数 
	int round_;
    // 地图
    Board board;
    // 回合赛况
    string round_status_;
    
  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply){
        reply() << Markdown(round_status_, max(600, (board.size + 2) * 43));
        return StageErrCode::OK;
    }

    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        srand((unsigned int)time(NULL));
        const int32_t hp_option = GAME_OPTION(生命) >= 0 ? GAME_OPTION(生命) : maps[GAME_OPTION(地图)].hp;
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            board.players.emplace_back(Global().PlayerName(pid), Global().PlayerAvatar(pid, 40), hp_option);
        }
        board.Initialize(
            GAME_OPTION(地图),
            GAME_OPTION(配比),
            GAME_OPTION(道具),
            GAME_OPTION(边长),
            {
                GAME_OPTION(宝藏),
                GAME_OPTION(炸弹),
                GAME_OPTION(加湿器),
                GAME_OPTION(墨水瓶),
            }
        );

        setter.Emplace<RoundStage>(*this, ++round_);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        if (board.countLeftGridType(GridType::TREASURE) > 0 && board.AliveCount() > 1) {
            setter.Emplace<RoundStage>(*this, ++round_);
            return;
        }

        Global().Boardcast() << Markdown(board.GetMarkdown(round_), max(600, (board.size + 2) * 43));
        board.ClearRoundStatus();
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            player_scores_[pid] = board.players[pid].score;
        }

        if (board.AliveCount() == 0) {
            Global().Boardcast() << "所有玩家均被淘汰，游戏结束！";
        } else if (board.countLeftGridType(GridType::TREASURE) == 0) {
            Global().Boardcast() << "地图内无剩余宝藏，游戏结束！";
        } else {
            int left = board.countLeftGridType(GridType::TREASURE);
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                if (board.players[pid].hp > 0) {
                    player_scores_[pid] += left * 60;
                }
            }
            Global().Boardcast() << "存活人数仅剩一人，游戏结束！剩余的 " << left << " 个宝藏将直接计入存活玩家的得分";
        }
        Global().Boardcast() << Markdown(board.GetBoard(true), (board.size + 2) * 43);
    }
};

class RoundStage : public SubGameStage<>
{
  public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合" ,
                MakeStageCommand(*this, "选择坐标挖掘", &RoundStage::Action_, AnyArg("坐标", "A1")))
    {}

    virtual void OnStageBegin() override
    {
        Main().round_status_ = Main().board.GetMarkdown(Main().round_);
        Global().Boardcast() << Markdown(Main().round_status_, max(600, (Main().board.size + 2) * 43));
        Main().board.ClearRoundStatus();

        Global().Boardcast() << "【剩余宝藏】" << Main().board.countLeftGridType(GridType::TREASURE) << " 个\n"
                             << "请私信裁判选择坐标进行挖掘" << (Main().round_ <= 2 ? "，前 2 回合炸弹不生效。" : "。")
                             << "时限 " << GAME_OPTION(时限) << " 秒，超时未行动扣除生命值";
        Global().StartTimer(GAME_OPTION(时限));
    }

  private:
    AtomReqErrCode Action_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const string str)
    {
        if (is_public) {
            reply() << "[错误] 请私信裁判进行行动";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 本回合您已经行动完成";
            return StageErrCode::FAILED;
        }
        
        auto [success, result] = Main().board.PlayerAction(pid, str, Main().round_);
        reply() << result;
        if (!success) return StageErrCode::FAILED;

        if (Main().board.players[pid].hp <= 0 && Main().board.AliveCount() > 0) {
            Global().Eliminate(pid);
        }

        return StageErrCode::READY;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        return HandleStageOver();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().board.players[pid].hp = 0;
        return StageErrCode::CONTINUE;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return HandleStageOver();
    }

    CheckoutErrCode HandleStageOver()
    {
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            if (!Main().board.players[pid].has_dug && Main().board.players[pid].hp > 0) {
                Main().board.players[pid].hp -= 1;
                Main().board.players[pid].change_hp = -1;
                if (Main().board.players[pid].hp <= 0 && Main().board.AliveCount() > 0) {
                    Global().Eliminate(pid);
                }
            }
        }

        Main().board.UpdatePlayerStatus();
        return StageErrCode::CHECKOUT;
    }
    
    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        pair<int, int> pos;
        int attempt = 0;
        bool success;
        do {
            pos = { rand() % Main().board.size, rand() % Main().board.size };
            success = Main().board.Action(pid, pos.first, pos.second, Main().round_).first;
        } while (!success && ++attempt <= 500);

        if (Main().board.players[pid].hp <= 0 && Main().board.AliveCount() > 0) {
            Global().Eliminate(pid);
        }
        return StageErrCode::READY;
    }
};

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

