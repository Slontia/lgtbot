// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include <random>
#include <queue>
#include <unordered_set>

#include "game_framework/stage.h"
#include "game_framework/util.h"
#include "utility/html.h"

using namespace std;

#include "constants.h"
#include "player.h"
#include "grid.h"
#include "map.h"
#include "boss.h"
#include "board.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;
const GameProperties k_properties { 
    .name_ = "漫漫长夜",
    .developer_ = "铁蛋",
    .description_ = "在漆黑的迷宫中探索，根据有限的线索展开追击与逃生",
    .shuffled_player_id_ = true,
};
uint64_t MaxPlayerNum(const CustomOptions& options) { return 8; }
uint32_t Multiple(const CustomOptions& options) {
    if (GET_OPTION_VALUE(options, 区块).size() != 1)
        return 0;
    return 1;
}
const MutableGenericOptions k_default_generic_options;

const char* const boss_details[1] = {
    R"EOF(《BOSS 规则和技能》
    【👹米诺陶斯】
1、随机生成在地图中，每回合最后行动，首回合锁定最近玩家为目标（公屏显示）
2、每回合移动步数递增，发现更近玩家则更换目标并重置步数。
3、BOSS无视地形，移动结束时会发出巨响，如果玩家在其周围会听到喘息声。
4、BOSS踩到玩家则玩家出局，玩家经过米诺陶斯不会出局。
    【💣邦邦】
1、所有玩家移动后，BOSS以固定速度（3-5 随机）跟着距离最近玩家移动。接触玩家后停止移动，但无法捕捉玩家；追到玩家后，会转移目标到第二近的玩家。
2、BOSS每回合结束时下一个[炸弹]，[炸弹]的四墙信息将会公开。
3、BOSS一定每回合都移动，无视地形，转换目标将公屏显示。)EOF",
};
const char* const rule_details[5] = {
    R"EOF(《游戏规则和机制细节》
可选参数列表，使用「#规则 漫漫长夜 <参数>」查看详情：
【地形】地形&附着&墙壁机制和特殊情况
【传送】开局随机和随机传送机制
【成就】成就触发判定和特殊情况
【区块】开局区块随机相关机制)EOF",

    R"EOF(【地形&附着】
出生在任何地形都不会触发其效果，所有效果均在移动时触发。
【墙壁】
相邻区块组合时，边界墙壁发生冲突则只保留优先级更高的墙壁。
优先级：门(开) > 门 > 墙壁 > 空
【炸弹相关】
炸弹必须要有[进入]并[离开]两步才能引爆。炸弹下到玩家上因为没有[进入]的过程，[离开]不会引爆。同理，随机传送到炸弹上，[离开]也不会引爆。
[拆弹]也需要[进入]并[停止行动]，放置炸弹直接撞墙，因为没有[进入]的过程，不会拆弹。同理，随机传送到炸弹上，直接[停止行动]也不会拆弹。
特殊：抓人时脚下有炸弹不会触发拆弹；进入逃生舱不会引爆/拆弹；隐匿状态一样会引爆炸弹；亚空间内下包会放置于传送门入口处
【传送门】
如果传送门被四周封闭，对应的传送门将会失效，仅发出啪啪声不会传送（相当于水洼）
【热源相关】
玩家出生在热浪区域，如果第1步还在热浪范围会收到提示。如果第1步进入热源，在第2步才会收到热浪提示。)EOF",

    R"EOF(【玩家随机传送】
传送至最大联通区域，不会传送至[有效逃生舱、热源周围8格]、[存活玩家周围8格]、[BOSS当前步数可到达的区域]，但可能传送进[失效逃生舱]区块。如果没有有效候选点，直接在最大联通区域内随机。
【开局玩家随机】
所有位置均在同一个最大联通区域内，按照顺序依次尝试（BFS会被[墙壁]、[热源]、[箱子]阻挡，但不考虑传送门）：
- 方案1：使用非逃生舱区块，相邻玩家之间的BFS路径距离≥5；每个玩家落点到所有逃生舱的BFS路径距离≥5
- 方案2：使用非逃生舱区块，仅要求相邻玩家之间的BFS路径距离≥5（不检查逃生舱距离）
- 方案3：直接使用非逃生舱区块随机分配（不检查距离）
- 方案4：允许使用逃生舱区块
- 保险方案：直接从最大连通区域中随机选取位置)EOF",

    R"EOF(【成就相关】
[乒铃乓啷]撞箱子不会计数，必须要成功推动箱子。引爆/拆弹均会计数，但是算作同一种类型。“单向传送门”和“普通传送门”视为同一种地形
隐匿状态进入树丛等声响地形，仍可以获得[无声]相关成就。
[守株待兔]可以直接停止也可以第1步撞墙，[谋定后动模式]走1步抓不会触发此成就。)EOF",

    R"EOF(【开局区块随机】
游戏开始时，将根据不同是游戏模式从不同的区块池抽取区块。
默认情况下，逃生舱数量为本局玩家人数的一半。但12*12地图逃生舱固定为4个
【自定义模式】
因自定义模式逃生舱池可能为任意数量，根据不同情况采用如下的随机方案：
- 情况1：逃生舱数量不足[默认数量]，使用全部逃生舱
- 情况2：逃生舱数量充足，同时区块数量充足，逃生舱为[默认数量]
- 情况3：当逃生舱为[默认数量]时，普通区块数量不足。使用全部普通区块，并抽取额外的逃生舱补足至需要的区块总数)EOF",
};
const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("查看所有 BOSS 的规则和技能",
            []() { return boss_details[0]; },
            VoidChecker("BOSS")),
    RuleCommand("游戏部分隐藏机制：「#规则 漫漫长夜 机制」查看可用列表帮助",
            [](const int type) { return rule_details[type]; },
            AlterChecker<int>({{"机制", 0}, {"地形", 1}, {"传送", 2}, {"成就", 3}, {"区块", 4}})),
};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    auto& custom_blocks = GET_OPTION_VALUE(game_options, 区块);
    if (custom_blocks.empty()) {
        reply() << "[错误] 区块参数为空：必须包含至少 9 个有效区块代号。形如：1 1 6 16 34 38 E1 e7 S4 s1";
        return false;
    }
    if (custom_blocks[0] != "默认") {
        UnitMaps unitMaps;
        vector<string> vaild_blocks;
        bool has_invalid = false;
        for (auto& block : custom_blocks) {
            bool is_valid;
            std::transform(block.begin(), block.end(), block.begin(), [](unsigned char c) { return std::toupper(c); });
            if (!block.empty() && block[0] == 'E') {
                is_valid = unitMaps.IsBlockExist(block.substr(1), true);
            } else {
                is_valid = unitMaps.IsBlockExist(block, false);
            }
            if (is_valid) vaild_blocks.push_back(block); else has_invalid = true;
        }
        if (generic_options_readonly.PlayerNum() > 6 && vaild_blocks.size() < 16) {
            reply() << "[错误] 区块参数不足：当前玩家数为 " << generic_options_readonly.PlayerNum() << "，必须包含至少 16 个有效区块代号，当前数量为：" << vaild_blocks.size();
            return false;
        }
        if (vaild_blocks.size() < 9) {
            reply() << "[错误] 区块参数不足：必须包含至少 9 个有效区块代号，当前数量为：" << vaild_blocks.size();
            return false;
        }
        GET_OPTION_VALUE(game_options, 模式) = BlockMode::CUSTOM;
        std::sort(vaild_blocks.begin(), vaild_blocks.end(), CompareMapId);
        custom_blocks = vaild_blocks;
    }

    if (GET_OPTION_VALUE(game_options, 特殊事件) == SpecialEvent::RANDOM) {
        GET_OPTION_VALUE(game_options, 特殊事件) = UnitMaps::GetRandomSpecialEvent();
    }
    if (generic_options_readonly.PlayerNum() > 6 && GET_OPTION_VALUE(game_options, 边长) < 12) {
        GET_OPTION_VALUE(game_options, 边长) = 12;
        reply() << "[警告] 玩家数 " << generic_options_readonly.PlayerNum() << " 超出普通地图限制，自动将地图边长调整为 12*12";
    }
    return true;
}

enum class InitOption {
    // ===== 区块模式 =====
    BLOCK_CLASSIC,
    BLOCK_TWIST,
    BLOCK_WILD,
    BLOCK_CRAZY,
    BLOCK_BUTTON,
    BLOCK_TRAP,

    // ===== 边长 =====
    BOARD_10,
    BOARD_12,

    // ===== 特殊事件 =====
    EVENT_RANDOM,
    EVENT_LAZYGARDENER,
    EVENT_OVERGROWTH,
    EVENT_RAINSTORY,

    // ===== 游戏模式 =====
    MODE_BATTLEROYALE,
    MODE_HIDDEN_TURN,
    MODE_HIDDEN_STEP,
    MODE_POINTKILL,
    MODE_NON_POINTKILL,
    MODE_PLANNING,
    MODE_BOMBER,

    // ===== BOSS =====
    BOSS_MINOTAUR,
    BOSS_BANGBANG,

    // ===== 其他配置 =====
    TARGET_PREVIOUS,
    TARGET_NEXT,
    STOP_PRIVATE,
    TEXTURE_RETRO,

    // ===== 启动模式 =====
    SINGLE_USER,
};

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("一键设定特殊事件或游戏模式：空格分隔，冲突配置以靠后的为准",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options, const vector<InitOption>& init_options)
            {
                bool single_user = false;
                for (const InitOption& option : init_options) {
                    switch (option) {
                        case InitOption::BLOCK_CLASSIC: GET_OPTION_VALUE(game_options, 模式) = BlockMode::CLASSIC; break;
                        case InitOption::BLOCK_TWIST:   GET_OPTION_VALUE(game_options, 模式) = BlockMode::TWIST; break;
                        case InitOption::BLOCK_WILD:    GET_OPTION_VALUE(game_options, 模式) = BlockMode::WILD; break;
                        case InitOption::BLOCK_CRAZY:   GET_OPTION_VALUE(game_options, 模式) = BlockMode::CRAZY; break;
                        case InitOption::BLOCK_BUTTON:  GET_OPTION_VALUE(game_options, 模式) = BlockMode::BUTTON; break;
                        case InitOption::BLOCK_TRAP:    GET_OPTION_VALUE(game_options, 模式) = BlockMode::TRAP; break;

                        case InitOption::BOARD_10:  GET_OPTION_VALUE(game_options, 边长) = 10; break;
                        case InitOption::BOARD_12:  GET_OPTION_VALUE(game_options, 边长) = 12; break;

                        case InitOption::EVENT_RANDOM:          GET_OPTION_VALUE(game_options, 特殊事件) = SpecialEvent::RANDOM; break;
                        case InitOption::EVENT_LAZYGARDENER:    GET_OPTION_VALUE(game_options, 特殊事件) = SpecialEvent::LAZYGARDENER; break;
                        case InitOption::EVENT_OVERGROWTH:      GET_OPTION_VALUE(game_options, 特殊事件) = SpecialEvent::OVERGROWTH; break;
                        case InitOption::EVENT_RAINSTORY:       GET_OPTION_VALUE(game_options, 特殊事件) = SpecialEvent::RAINSTORY; break;

                        case InitOption::MODE_BATTLEROYALE:     GET_OPTION_VALUE(game_options, 大乱斗) = true; break;
                        case InitOption::MODE_HIDDEN_TURN:      GET_OPTION_VALUE(game_options, 隐匿) = HideMode::TURN; break;
                        case InitOption::MODE_HIDDEN_STEP:      GET_OPTION_VALUE(game_options, 隐匿) = HideMode::STEP; break;
                        case InitOption::MODE_POINTKILL:        GET_OPTION_VALUE(game_options, 点杀) = true; break;
                        case InitOption::MODE_NON_POINTKILL:    GET_OPTION_VALUE(game_options, 点杀) = false; break;
                        case InitOption::MODE_PLANNING:         GET_OPTION_VALUE(game_options, 谋定后动) = true; break;
                        case InitOption::MODE_BOMBER:           GET_OPTION_VALUE(game_options, 炸弹) = 1; break;

                        case InitOption::BOSS_MINOTAUR:     GET_OPTION_VALUE(game_options, BOSS) = BossType::MINOTAUR; break;
                        case InitOption::BOSS_BANGBANG:     GET_OPTION_VALUE(game_options, BOSS) = BossType::BANGBANG; break;

                        case InitOption::TARGET_PREVIOUS:   GET_OPTION_VALUE(game_options, 捕捉目标) = Target::PREVIOUS; break;
                        case InitOption::TARGET_NEXT:       GET_OPTION_VALUE(game_options, 捕捉目标) = Target::NEXT; break;
                        case InitOption::STOP_PRIVATE:      GET_OPTION_VALUE(game_options, 停止私信) = true; break;
                        case InitOption::TEXTURE_RETRO:     GET_OPTION_VALUE(game_options, 纹理) = Texture::RETRO; break;

                        case InitOption::SINGLE_USER:       single_user = true; break;
                        default:;
                    }
                }
                if (single_user) return NewGameMode::SINGLE_USER;
                return NewGameMode::MULTIPLE_USERS;
            },
            RepeatableChecker<AlterChecker<InitOption>>(map<string, enum InitOption>{
                {"经典", InitOption::BLOCK_CLASSIC},
                {"幻变", InitOption::BLOCK_TWIST},
                {"狂野", InitOption::BLOCK_WILD},
                {"疯狂", InitOption::BLOCK_CRAZY},
                {"按钮", InitOption::BLOCK_BUTTON},
                {"陷阱", InitOption::BLOCK_TRAP},

                {"10*10", InitOption::BOARD_10},
                {"12*12", InitOption::BOARD_12},

                {"随机", InitOption::EVENT_RANDOM},
                {"怠惰的园丁", InitOption::EVENT_LAZYGARDENER},
                {"营养过剩", InitOption::EVENT_OVERGROWTH},
                {"雨天小故事", InitOption::EVENT_RAINSTORY},

                {"大乱斗", InitOption::MODE_BATTLEROYALE},
                {"回合隐匿", InitOption::MODE_HIDDEN_TURN},
                {"单步隐匿", InitOption::MODE_HIDDEN_STEP},
                {"点杀", InitOption::MODE_POINTKILL},
                {"关闭点杀", InitOption::MODE_NON_POINTKILL},
                {"谋定后动", InitOption::MODE_PLANNING},
                {"炸弹人", InitOption::MODE_BOMBER},

                {"米诺陶斯", InitOption::BOSS_MINOTAUR},
                {"邦邦", InitOption::BOSS_BANGBANG},

                {"上家", InitOption::TARGET_PREVIOUS},
                {"下家", InitOption::TARGET_NEXT},
                {"停止私信", InitOption::STOP_PRIVATE},
                {"复古", InitOption::TEXTURE_RETRO},

                {"单机", InitOption::SINGLE_USER},
            })),
};

// ========== GAME STAGES ==========

class RoundStage;

class MainStage : public MainGameStage<RoundStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看所有区块信息和图例", &MainStage::BlockInfo_, VoidChecker("地图")),
            MakeStageCommand(*this, "指定区块生成地图预览，逃生舱区块需要加 'E' 前缀，未知区块用 0 代替", &MainStage::MapPreview_,
                VoidChecker("预览"), RepeatableChecker<BasicChecker<string>>("序号", "2 3 0 11 E1 0 E2 7 9")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
        , board(Global().ResourceDir(), GAME_OPTION(纹理), GAME_OPTION(模式), GAME_OPTION(区块))
    {}

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    std::vector<int64_t> player_scores_;

    // 回合数 
	int round_;
    // 地图
    Board board;

    // 多人全员停止/超时结束游戏判定
    bool all_active_stop = true;
    // 无逃生舱最后生还胜利判定
    bool withoutE_win_ = false;
    // 无逃生舱最后生还判定
    bool withE_win_ = false;

    // 剩余玩家数
    int Alive_() const { return std::count_if(board.players.begin(), board.players.end(), [](const auto& player){ return player.out == 0; }); }
    
  private:
    CompReqErrCode BlockInfo_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        auto sender = reply();
        if (GAME_OPTION(特殊事件) != SpecialEvent::NONE) {
            sender << UnitMaps::ShowSpecialEvent(GAME_OPTION(特殊事件)) << "\n";
        }
        if (GAME_OPTION(边长) > 9) {
            sender << "本局游戏地图为 " << GAME_OPTION(边长) << "x" << GAME_OPTION(边长) << "\n";
        }
        sender << Markdown(board.GetAllBlocksInfo(GAME_OPTION(特殊事件), GAME_OPTION(炸弹) > 0), (GRID_SIZE + WALL_SIZE) * 16 + 40);
        return StageErrCode::OK;
    }

    CompReqErrCode MapPreview_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const vector<string>& map_string)
    {
        vector<string> map_str = map_string;
        map_str.resize(board.unitMaps.pos.size(), "0");
        reply() << Markdown(board.MapPreview(map_str), (GRID_SIZE + WALL_SIZE) * (GAME_OPTION(边长) + 1));
        return StageErrCode::OK;
    }
    
    void FirstStageFsm(SubStageFsmSetter setter) override
    {
        srand((unsigned int)time(NULL));

        // 调试：用于生成全部地图列表
        // Global().SaveMarkdown(board.GetAllBlocksInfo(SpecialEvent::NONE, true, BlockMode::CRAZY), ((GRID_SIZE + WALL_SIZE) * 16 + 40) * 2);
        // board.unitMaps.SampleBlockPoolsFromIds(board.unitMaps.twist_mode_ids);  // [幻变]模式
        // Global().SaveMarkdown(board.GetAllBlocksInfo(SpecialEvent::NONE, true, BlockMode::TWIST), ((GRID_SIZE + WALL_SIZE) * 16 + 40));
        // board.unitMaps.SampleBlockPoolsFromIds(board.unitMaps.button_mode_ids); // [按钮]模式
        // Global().SaveMarkdown(board.GetAllBlocksInfo(SpecialEvent::NONE, true, BlockMode::BUTTON), ((GRID_SIZE + WALL_SIZE) * 16 + 40));
        // board.unitMaps.SampleBlockPoolsFromIds(board.unitMaps.trap_mode_ids);   // [陷阱]模式
        // Global().SaveMarkdown(board.GetAllBlocksInfo(SpecialEvent::NONE, true, BlockMode::TRAP), ((GRID_SIZE + WALL_SIZE) * 16 + 40));

        auto sender = Global().Boardcast();
        if (GAME_OPTION(捕捉目标) == Target::NEXT) {
            sender << "【捕捉顺位】本局玩家的捕捉顺位为相反顺序，捕捉目标变更为下家\n\n";
        }
        if (GAME_OPTION(特殊事件) != SpecialEvent::NONE) {      // 特殊事件
            switch (GAME_OPTION(特殊事件)) {
                case SpecialEvent::LAZYGARDENER:    board.unitMaps.SpecialEvent1(); break;
                case SpecialEvent::OVERGROWTH:      board.unitMaps.SpecialEvent2(); break;
                case SpecialEvent::RAINSTORY:       board.unitMaps.SpecialEvent3(); break;
                case SpecialEvent::NONE: case SpecialEvent::RANDOM: break;
            }
            sender << UnitMaps::ShowSpecialEvent(GAME_OPTION(特殊事件)) << "\n\n";
        }
        if (GAME_OPTION(边长) == 10) {      // 边长10
            if (board.unitMaps.RandomizeBlockPosition(GAME_OPTION(边长))) {
                sender << "【10*10】本局游戏地图将更改为 " << GAME_OPTION(边长) << "x" << GAME_OPTION(边长) << " 大地图。9个区块在大地图随机排列，区块不会重叠。没有区块覆盖的地图空隙将变成普通道路。\n";
            } else {
                sender << "[未知错误] 生成10*10地图时发生错误：未能成功随机布局，游戏无法正常开始！";
                return;
            }
        }
        if (GAME_OPTION(边长) == 12) {      // 边长12
            board.unitMaps.SetMapPosition12();
            board.exit_num = 4;
            sender << "【12*12】本局游戏地图将更改为 " << GAME_OPTION(边长) << "x" << GAME_OPTION(边长) << " 大地图。使用 16 个区块铺满地图，逃生舱固定为 4 个。\n";
        }
        if (GAME_OPTION(点杀)) {    // 点杀
            sender << "【点杀模式】捕捉改为仅在回合结束时触发，路过不会触发捕捉\n";
        }
        if (GAME_OPTION(隐匿) == HideMode::TURN) {   // 隐匿
            sender << "【隐匿模式】回合隐匿：隐匿效果持续1回合，**仅可使用1次**，隐匿后当回合的行动转为私聊进行，不会发出声响，不会触发捕捉。\n";
        } else if (GAME_OPTION(隐匿) == HideMode::STEP) {
            sender << "【隐匿模式】单步隐匿：隐匿效果仅作用于下一步，**可使用" << HIDE_LIMIT << "次**，隐匿后在私聊行动一步，不会发出声响，不会触发捕捉。\n";
        }
        if (GAME_OPTION(大乱斗)) {  // 大乱斗
            sender << "【大乱斗模式】所有的逃生舱改为随机传送！但仍会统计逃生分\n";
        }
        if (GAME_OPTION(炸弹) > 0) {    // 炸弹人
            sender << "【炸弹人模式】玩家可在公屏安置炸弹，经过并离开会爆炸立即出局并-100分。在炸弹上结束行动可拆除炸弹\n";
        }

        board.size = GAME_OPTION(边长);
        // 初始化玩家
        board.playerNum = Global().PlayerNum();
        board.players.reserve(board.playerNum);
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            board.players.emplace_back(pid, Global().PlayerName(pid), Global().PlayerAvatar(pid, 40), board.size, board.unitMaps.pos);
            if (GAME_OPTION(隐匿) == HideMode::TURN) {   // 隐匿
                board.players[pid].hide_remaining = 1;
            } else if (GAME_OPTION(隐匿) == HideMode::STEP) {
                board.players[pid].hide_remaining = HIDE_LIMIT;
            }
            if (GAME_OPTION(炸弹) > 0) board.players[pid].bomb = GAME_OPTION(炸弹); // 炸弹人
        }
        board.UpdatePlayerTarget(GAME_OPTION(捕捉目标));
        // 出口数
        if (GAME_OPTION(边长) != 12) {
            if (Global().PlayerNum() > 1) {
                board.exit_num = Global().PlayerNum() / 2;
            } else {
                board.exit_num = 1;
            }
        }
        // 单机模式
        if (Global().PlayerNum() == 1) board.players[0].target = 100;
        // 初始化地图
        board.Initialize();
        board.exit_num = board.TypeCount(GridType::EXIT);

        if (GAME_OPTION(BOSS) != BossType::NONE) {
            board.boss.BossInitialize(GAME_OPTION(BOSS));   // 初始化BOSS
            board.boss.InitBossStartRecord();
            sender << "【BOSS】" + board.boss.GetBossStartInfo() + "\n";
            sender << "当前 BOSS 锁定的玩家为 " << At(board.boss.target) << "\n";
        }

        board.SaveGameStartMap();   // 保存初始盘面

        sender << "[提示] 本局游戏人数为 " << board.playerNum << " 人，逃生舱数量为 " << board.exit_num << " 个。请留意私信发送的开局墙壁信息\n\n";
        sender << "【区块模式】" << mode_str[static_cast<int>(GAME_OPTION(模式))] << "\n";
        sender << "本局可能出现的区块详见下图所示：\n";
        sender << Markdown(board.GetAllBlocksInfo(GAME_OPTION(特殊事件), GAME_OPTION(炸弹) > 0), (GRID_SIZE + WALL_SIZE) * 16 + 40);

        setter.Emplace<RoundStage>(*this, ++round_);
    }

    void NextStageFsm(RoundStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override
    {
        bool game_end =
            ((Alive_() <= 1 || all_active_stop) && Global().PlayerNum() > 1) ||
            ((Alive_() <= 0 || board.TypeCount(GridType::EXIT) == 0) && Global().PlayerNum() == 1);
        if (!game_end && round_ < GAME_OPTION(回合数)) {
            setter.Emplace<RoundStage>(*this, ++round_);
            return;
        }

        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            player_scores_[pid] = board.players[pid].score.FinalScore();
        }

        if (game_end) {
            if (Global().PlayerNum() > 1) {
                if (all_active_stop) {
                    Global().Boardcast() << "所有玩家均主动停止，游戏结束！";
                } else {
                    Global().Boardcast() << "玩家剩余 " + to_string(Alive_()) + " 人，游戏结束！";
                }
            } else if (board.players[0].out == 2) {
                Global().Boardcast() << "经过 " + to_string(round_) + " 回合，您成功抵达了逃生舱！";
            }
        } else {
            Global().Boardcast() << "回合数到达上限，游戏结束";
        }
        Global().Boardcast() << "完整行动轨迹：\n" << Markdown(board.GetAllRecordHtml(-1, false), 500);
        Global().Boardcast() << "玩家分数细则：\n" << Markdown(board.GetAllScore(), 800);
        Global().Boardcast() << Markdown(board.GetFinalBoard(), (GRID_SIZE + WALL_SIZE) * 2 * (GAME_OPTION(边长) + 1));

        // 成就结算
        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            const PlayerAchievement& achievement = board.players[pid].achievement;
            if (achievement.exit_without_sound)         Global().Achieve(pid, Achievement::悄无声息);
            if (achievement.explore_all_map)            Global().Achieve(pid, Achievement::环游世界);
            if (achievement.explore_all_map_silently()) Global().Achieve(pid, Achievement::游荡幽灵);
            if (achievement.exit_first_round)           Global().Achieve(pid, Achievement::我赶时间);
            if (achievement.catch_first_round)          Global().Achieve(pid, Achievement::饥渴难耐);
            if (achievement.visit_five_grid_type())     Global().Achieve(pid, Achievement::乒铃乓啷);
            if (achievement.catch_everyone_4p)          Global().Achieve(pid, Achievement::嗜杀成性);
            if (achievement.catch_without_moving)       Global().Achieve(pid, Achievement::守株待兔);
            if (achievement.boss_chase_four_steps)      Global().Achieve(pid, Achievement::牛头魅魔);
        }
    }
};

class RoundStage : public SubGameStage<>
{
   public:
    RoundStage(MainStage& main_stage, const uint64_t round)
            : StageFsm(main_stage, "第 " + std::to_string(round) + " 回合" ,
                MakeStageCommand(*this, "选择方向，在迷宫中探路", &RoundStage::MakeMove_, AlterChecker<Direct>(direction_map)),
                MakeStageCommand(*this, "主动停止行动，并结束回合", &RoundStage::Stop_, VoidChecker("停止")),
                MakeStageCommand(*this, "[隐匿模式] 使用“隐匿”技能", &RoundStage::Hide_, VoidChecker("隐匿")),
                MakeStageCommand(*this, "[炸弹人模式] 在当前位置安放炸弹", &RoundStage::SetBomb_, VoidChecker("下包")),
                MakeStageCommand(*this, "查看当前回合进展情况", &RoundStage::Status_, VoidChecker("赛况"), OptionalDefaultChecker<BoolChecker>(true, "图片", "文字")),
                MakeStageCommand(*this, "查看所有玩家完整行动轨迹", &RoundStage::AllStatus_, VoidChecker("完整赛况"), OptionalDefaultChecker<BoolChecker>(true, "图片", "文字")),
                MakeStageCommand(*this, "多步行动：一次性输入多个方向，自动在迷宫中移动", &RoundStage::MakeMultipleMove_, AnyArg("连续多个方向", "上下左右sxzyUDLR")))
    {
        main_stage.all_active_stop = true;

        currentPlayer = 0;
        if (Main().Alive_() == 0) {
            Global().Boardcast() << "[错误] 发生了意料之外的错误：无可行动玩家但游戏仍未判定结束，请联系管理员或中断游戏（切勿退出强制）！";
            return;
        }
        while (Main().board.players[currentPlayer].out > 0) {
            currentPlayer = currentPlayer + 1;
        }
        step = 0;
        hide = false;
        active_stop = false;
        is_acting = false;
        door_modified = 0;
    }

    // 当前行动玩家
    PlayerID currentPlayer;
    // 玩家行动临时变量
    int step;           // 行动步数
    bool hide;          // 隐匿状态
    bool active_stop;   // 主动停止或超时
    bool is_acting;     // 玩家是否开始行动（时限/挂机判定）
    
    // 记录门的状态发生改变的次数
    int door_modified;

    virtual void OnStageBegin() override
    {
        Global().SaveMarkdown(Main().board.GetBoard(Main().board.grid_map), (GRID_SIZE + WALL_SIZE) * (GAME_OPTION(边长) + 1));

        for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
            Main().board.players[pid].ClearMoveRecord();
            if (pid != currentPlayer) {
                Global().SetReady(pid);
            }
        }
        Global().Boardcast() << Markdown(Main().board.GetPlayerTable(Main().round_));

        if (GAME_OPTION(谋定后动)) {
            uint32_t think_time = Main().board.players[currentPlayer].hook_status ? 30 : GAME_OPTION(行动时限);
            Global().Boardcast() << "请 " << At(currentPlayer) << " 在公屏选择方向移动，仅能移动一次（时限 " << think_time << " 秒）";
            Global().StartTimer(think_time);
        } else {
            StartNextPlayerTurn(Main().board.players[currentPlayer]);
        }

        // 首轮信息播报
        if (Main().round_ == 1) {
            // 开局私信墙壁信息
            for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
                auto [info, md] = Main().board.GetSurroundingWalls(pid);
                Main().board.players[pid].private_record = "【开局】\n您所在位置的四周墙壁信息，按照 上下左右 顺序分别是：\n" + info;
                Global().Tell(pid) << Main().board.players[pid].private_record << "\n" << Markdown(md, (GRID_SIZE + WALL_SIZE * 2) + 40);
            }
            // [BOSS-米诺陶斯] 开局声音
            if (GAME_OPTION(BOSS) == BossType::MINOTAUR) {
                SendSoundMessage(Main().board.boss.x, Main().board.boss.y, Sound::BOSS, true);
            }
            // 开局帮助和模式信息播报
            Global().Boardcast() << "指令「预览」可生成自定义地图来记录草稿，格式例如：预览 2 3 0 11 E1 0 E2（E前缀表示逃生舱）\n\n"
                                 << "私信「完整赛况」可查询完整的私信信息汇总，包括其他玩家的声响方向历史记录\n\n"
                                 << (GAME_OPTION(谋定后动) ? "【谋定后动】每回合仅能执行一次移动，可使用多步行动指令\n" : "")
                                 << (GAME_OPTION(停止私信) 
                                    ? "【有停止私信】主动停止或超时可以获得私信四周墙壁信息"
                                    : "【无停止私信】主动停止或超时将无法得知四周墙壁信息");
        }
    }

   private:
    void StartNextPlayerTurn(const Player& player)
    {
        uint32_t think_time = player.hook_status ? 30 : GAME_OPTION(思考时限);
        Global().Boardcast() << "请 " << At(currentPlayer) << " 在公屏选择方向移动（等待时间 " << think_time << " 秒，执行任意指令后获得额外时间 "
                             << GAME_OPTION(行动时限) << " 秒，剩余加时卡 " << player.extra_time_card << " 张）"
                             << (player.bomb > 0 ? "，剩余炸弹 " + to_string(player.bomb) + " 个" : "");
        Global().StartTimer(think_time);
    }

    void ActivatePlayerMovingTimer(const PlayerID pid)
    {
        // 解除挂机状态
        Main().board.players[pid].hook_status = false;
        // 如果是当前玩家，视为开始行动
        if (pid == currentPlayer && !is_acting) {
            is_acting = true;
            Global().StartTimer(TimerLeft() + GAME_OPTION(行动时限));
        }
    }

    bool CheckCommon(const PlayerID pid, MsgSenderBase& reply)
    {
        Player& player = Main().board.players[pid];
        ActivatePlayerMovingTimer(pid);
        if (player.out == 2) {
            reply() << "您已乘坐逃生舱撤离，无需继续行动";
            return false;
        }
        if (pid != currentPlayer) {
            reply() << "[错误] 不是您的回合，当前正在行动玩家是：" << Global().PlayerName(currentPlayer);
            return false;
        }
        return true;
    }

    AtomReqErrCode MakeMove_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, Direct direction)
    {
        if (!CheckCommon(pid, reply)) return StageErrCode::FAILED;

        Player& player = Main().board.players[pid];
        if (!is_public && Global().PlayerNum() > 1) {
            if (GAME_OPTION(隐匿) == HideMode::NONE) {
                reply() << "[错误] 多人游戏下，请全程在公屏进行行动";
                return StageErrCode::FAILED;
            }
            if (!hide) {
                reply() << "[错误] 您未使用隐匿技能，请在公屏进行行动";
                return StageErrCode::FAILED;
            }
        }
        
        bool success = Main().board.MakeMove(pid, direction, hide);
        step++;

        // 撞墙直接切换下一个玩家
        if (!success) {
            if (step == 1) {
                reply() << "[第 1 步] 尝试向 " << dir_cn[static_cast<int>(direction)] << " 移动\n"
                        << GetRandomHint(firststep_wall_hints) << "\n移动时碰撞【墙壁】，本回合结束！**请留意机器人私信发送的四周墙壁信息**";
            } else {
                reply() << "[第 " << step << " 步] 尝试向 " << dir_cn[static_cast<int>(direction)] << " 移动\n"
                        << GetRandomHint(wall_hints) << "\n移动时碰撞【墙壁】，本回合结束！请留意机器人私信发送的四周墙壁信息";
            }
            return StageErrCode::READY;
        }

        auto sender = reply();
        sender << "[第 " << step << " 步] 向 " << dir_cn[static_cast<int>(direction)] << " 移动";

        bool ready_status = HandleGridInteraction(player, sender, false);
        if (ready_status) return StageErrCode::READY;

        if (GAME_OPTION(谋定后动)) {
            step++;
            active_stop = true;
            return StageErrCode::READY;
        }

        // 继续行动
        sender << "\n\n请继续行动（剩余时间 " << TimerLeft() << " 秒）";
        // 单步隐匿：消除隐匿状态
        if (GAME_OPTION(隐匿) == HideMode::STEP && hide) { hide = false; }
        return StageErrCode::OK;
    }

    AtomReqErrCode MakeMultipleMove_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const string& direction_str)
    {
        if (!CheckCommon(pid, reply)) return StageErrCode::FAILED;

        Player& player = Main().board.players[pid];
        if (hide && GAME_OPTION(隐匿) == HideMode::STEP) {
            reply() << "[错误] 您正处于单步隐匿状态，无法使用多步移动指令，请私信裁判使用常规移动";
            return StageErrCode::FAILED;
        }
        if (!is_public && Global().PlayerNum() > 1) {
            reply() << "[错误] 多人游戏下，多步行动指令只能在公屏使用";
            return StageErrCode::FAILED;
        }

        vector<Direct> directions;
        string result = Board::parseDirections(direction_str, directions);

        if (!result.empty()) {
            reply() << "[错误] 解析失败：检测到未知字符 \"" + result + "\"，仅支持包含：\n上/s/U、下/x/D、左/z/L、右/y/R";
            return StageErrCode::FAILED;
        }

        for (auto it = directions.begin(); it != directions.end(); ++it) {
            const Direct& direct = *it;
            bool success = Main().board.MakeMove(pid, direct, hide);
            step++;
            // 中途撞墙直接停止行动
            if (!success) {
                if (step == 1) {
                    reply() << "[第 1 步] 尝试向 " << dir_cn[static_cast<int>(direct)] << " 移动\n"
                            << GetRandomHint(firststep_wall_hints) << "\n移动时碰撞【墙壁】，本回合结束！**请留意机器人私信发送的四周墙壁信息**";
                } else {
                    reply() << "[第 " << step << " 步] 尝试向 " << dir_cn[static_cast<int>(direct)] << " 移动\n"
                            << GetRandomHint(wall_hints) << "\n移动时碰撞【墙壁】，本回合结束！请留意机器人私信发送的四周墙壁信息";
                }
                return StageErrCode::READY;
            }

            auto sender = reply();
            sender << "[第 " << step << " 步] 向 " << dir_cn[static_cast<int>(direct)] << " 移动";

            bool ready_status = HandleGridInteraction(player, sender, true);
            if (ready_status) return StageErrCode::READY;

            if (std::next(it) == directions.end()) {
                if (GAME_OPTION(谋定后动)) {
                    step++;
                    active_stop = true;
                    return StageErrCode::READY;
                }

                sender << "\n\n请继续行动（剩余时间 " << TimerLeft() << " 秒）";
            }
            // 每步随机延迟
            std::uniform_int_distribution<int> dist(0, 1000);
            int delay_ms = 1000 + dist(Main().board.g);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode Stop_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (!CheckCommon(pid, reply)) return StageErrCode::FAILED;

        Player& player = Main().board.players[pid];
        if (!is_public && Global().PlayerNum() > 1) {
            if (GAME_OPTION(隐匿) == HideMode::NONE) {
                reply() << "[错误] 请全程在公屏进行行动";
            } else {
                reply() << "[错误] 您未使用隐匿技能，请在公屏进行行动";
            }
            return StageErrCode::FAILED;
        }

        step++;
        active_stop = true;
        if (!hide) player.NewContentRecord("(停止)");
        reply() << "[第 " << step << " 步] 您选择主动停止行动，本回合结束！"
                << (GAME_OPTION(停止私信) ? "请留意机器人私信发送的四周墙壁信息" : "主动停止无法获得四周墙壁信息");
        return StageErrCode::READY;
    }

    AtomReqErrCode Hide_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (GAME_OPTION(隐匿) == HideMode::NONE) {
            reply() << "[错误] 本局游戏未开启隐匿技能";
            return StageErrCode::FAILED;
        }

        if (!CheckCommon(pid, reply)) return StageErrCode::FAILED;

        Player& player = Main().board.players[pid];
        if (hide) {
            reply() << "[错误] 您已经处于隐匿状态，请在私信选择行动";
            return StageErrCode::FAILED;
        }
        if (player.hide_remaining == 0) {
            reply() << "[错误] 隐匿技能次数已耗尽";
            return StageErrCode::FAILED;
        }
        if (!is_public && Global().PlayerNum() > 1) {
            reply() << "[错误] 请在公屏使用隐匿技能";
            return StageErrCode::FAILED;
        }
        
        hide = true;
        player.hide_remaining--;
        if (GAME_OPTION(隐匿) == HideMode::TURN) {
            player.NewContentRecord("<隐匿行动>", "hide");
            reply() << "使用隐匿技能，本回合剩余时间转为私聊行动，不会发出声响，不会触发捕捉。剩余次数：" << player.hide_remaining;
        } else if (GAME_OPTION(隐匿) == HideMode::STEP) {
            player.NewContentRecord("�", "hide");
            reply() << "使用隐匿技能，下一步请在私聊行动，不会发出声响，不会触发捕捉。剩余次数：" << player.hide_remaining;
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode SetBomb_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (GAME_OPTION(炸弹) == 0) {
            reply() << "[错误] 本局游戏未开启炸弹人模式";
            return StageErrCode::FAILED;
        }

        if (!CheckCommon(pid, reply)) return StageErrCode::FAILED;

        Player& player = Main().board.players[pid];
        if (player.bomb == 0) {
            reply() << "[错误] 炸弹已用尽。但脚下地雷遍布，请谨慎前行";
            return StageErrCode::FAILED;
        }
        if (!is_public && Global().PlayerNum() > 1) {
            reply() << "[错误] 请在公屏使用下包技能";
            return StageErrCode::FAILED;
        }

        Grid& grid = Main().board.grid_map[player.x][player.y];
        if (grid.Attach() == AttachType::EMPTY) grid.SetAttach(AttachType::BOMB);

        player.bomb--;
        player.NewContentRecord("[下包]", "bomb");
        reply() << "在当前所在位置执行下包操作。剩余：" << player.bomb;
        return StageErrCode::OK;
    }

    AtomReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool show_image)
    {
        ActivatePlayerMovingTimer(pid);
        auto sender = reply();
        if (show_image) {
            sender << Markdown(Main().board.GetPlayerTable(Main().round_));
        } else {
            sender << Main().board.GetPlayerString() << "\n";
        }

        if (is_public) {
            int query_pid_current = currentPlayer == pid ? -1 : pid.Get();
            for (PlayerID pid = 0; pid < currentPlayer.Get(); ++pid) {
                if (Main().board.players[pid].out == 0) {
                    sender << "\n[" << pid.Get() << "号]本回合行动轨迹：\n" << Main().board.players[pid].move_record.GetMoveRecord(query_pid_current, is_public);
                }
            }
            sender << "\n[" << currentPlayer.Get() << "号]正在行动中：\n" << Main().board.players[currentPlayer].move_record.GetMoveRecord(query_pid_current, is_public);
        } else {
            sender << "\n" << Main().board.players[pid].private_record;
        }
        return StageErrCode::OK;
    }

    AtomReqErrCode AllStatus_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const bool show_image)
    {
        ActivatePlayerMovingTimer(pid);
        auto sender = reply();
        if (GAME_OPTION(特殊事件) != SpecialEvent::NONE) {
            sender << UnitMaps::ShowSpecialEvent(GAME_OPTION(特殊事件)) << "\n";
        }
        if (GAME_OPTION(边长) > 9) {
            sender << "本局游戏地图为 " << GAME_OPTION(边长) << "x" << GAME_OPTION(边长) << "\n";
        }

        int query_pid = pid.Get();
        if (show_image) {
            sender << Markdown(Main().board.GetAllRecordHtml(query_pid, is_public), 550);
        } else {
            sender << Main().board.GetAllRecordString(query_pid, is_public);
        }

        int query_pid_current = currentPlayer == pid ? -1 : pid.Get();
        sender << "\n[" << currentPlayer.Get() << "号]正在行动中：\n" << Main().board.players[currentPlayer].move_record.GetMoveRecord(query_pid_current, is_public);
        if (!is_public) {
            sender << "\n\n" << Main().board.players[pid].private_record;
        }
        return StageErrCode::OK;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        Player& player = Main().board.players[currentPlayer];
        if (!is_acting) {
            if (!player.hook_status) {
                Global().Tell(currentPlayer) << "您已进入挂机状态，等待时间将缩减至 30 秒，执行游戏指令可恢复至原状态";
            }
            player.hook_status = true;
            Global().Boardcast() << "玩家 " << At(PlayerID(currentPlayer)) << " 超时未行动，已进入挂机状态，再次行动前仅有 30 秒等待时间";
        } else {
            if (player.extra_time_card > 0) {
                player.extra_time_card--;
                Global().Boardcast() << "行动超时，自动使用加时卡，剩余时间延长 " + to_string(EXTRATIMECRAD_TIME) + " 秒，剩余 " + to_string(player.extra_time_card) + " 张加时卡";
                Global().StartTimer(EXTRATIMECRAD_TIME);
                return StageErrCode::CONTINUE;
            }
            Global().Boardcast() << "玩家 " << At(PlayerID(currentPlayer)) << " 行动超时，切换下一个玩家";
        }
        player.NewContentRecord("(超时)");
        active_stop = true;
        Global().SetReady(currentPlayer);
        return HandleStageOver();
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        if (Main().board.players[pid].out > 0) {
            return StageErrCode::CONTINUE;
        }
        Main().board.players[pid].out = 1;
        Main().board.players[pid].score.quit_score -= 300;  // 退出分

        auto sender = Global().Boardcast();
        sender << "玩家 " << At(pid) << " 退出游戏";
        if (Main().Alive_() > 1) {
            Main().board.UpdatePlayerTarget(GAME_OPTION(捕捉目标));   // 捕捉顺位变更
            sender << "，捕捉目标顺位发生变更！\n";
            sender << Markdown(Main().board.GetPlayerTable(Main().round_));
            return StageErrCode::CONTINUE;
        }
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        return HandleStageOver();
    }

    CheckoutErrCode HandleStageOver()
    {
        Player& player = Main().board.players[currentPlayer];
        // 回合结束始终检查捕捉
        vector<PlayerID> list = Main().board.player_map[player.x][player.y];
        if (find(list.begin(), list.end(), player.target) != list.end() && !hide && player.out == 0) {
            auto sender = Global().Boardcast();
            PlayerCatch(player, sender);
        }
        Grid& grid = Main().board.grid_map[player.x][player.y];

        Global().Boardcast() << "[" << currentPlayer.Get() << "号]玩家本回合的完整行动轨迹：\n" << player.move_record.GetMoveRecord(-1, true);
        // 记录历史行动轨迹
        player.all_record.push_back(player.move_record);
        // 更新全员停止状态（全员停止/超时结束游戏）
        if (!active_stop) Main().all_active_stop = false;
        // 仅剩1玩家，游戏结束
        if ((Main().Alive_() == 1 && Global().PlayerNum() > 1) || (Main().Alive_() == 0 && Global().PlayerNum() == 1)) {
            if (Main().withoutE_win_) {     // 无逃生舱最后生还胜利
                Global().Boardcast() << At(currentPlayer) << "\n" << GetRandomHint(withoutE_win_hints);
            }
            if (Main().withE_win_) {        // 有逃生舱但死斗取胜
                Global().Boardcast() << At(currentPlayer) << "\n" << GetRandomHint(withE_win_hints);
            }
            return StageErrCode::CHECKOUT;
        }
        // 私信发送四周墙壁信息（主动停止或超时不发送）
        if (player.out == 0 && (!active_stop || GAME_OPTION(停止私信))) {
            auto [info, md] = Main().board.GetSurroundingWalls(currentPlayer);
            player.private_record = "【第 " + to_string(Main().round_) + " 回合】\n您所在位置的四周墙壁信息，按照 上下左右 顺序分别是：\n" + info;
            Global().Tell(currentPlayer) << player.private_record << "\n" << Markdown(md, (GRID_SIZE + WALL_SIZE * 2) + 40);
        }
        // 已触发炸弹才能拆除炸弹
        if (grid.Attach() == AttachType::BOMB && player.bomb_trigger) {
            player.bomb_trigger = false;
            grid.SetAttach(AttachType::EMPTY);
            Global().Tell(currentPlayer) << "引线熄灭，爆炸未曾发生，你成功拆除了脚下的【炸弹】";
            player.private_record += "\n回合结束时拆除【炸弹】";
        }
        // 重置玩家临时变量
        step = 0;
        hide = false;
        active_stop = false;
        is_acting = false;
        // 下一个玩家行动
        do {
            currentPlayer = (currentPlayer + 1) % Global().PlayerNum();
            if (currentPlayer == 0) break;
        } while (Main().board.players[currentPlayer].out > 0);
        if (currentPlayer != 0) {
            if (GAME_OPTION(谋定后动)) {
                uint32_t think_time = Main().board.players[currentPlayer].hook_status ? 30 : GAME_OPTION(行动时限);
                Global().Boardcast() << "请 " << At(currentPlayer) << " 在公屏选择方向移动，仅能移动一次（时限 " << think_time << " 秒）";
                Global().StartTimer(think_time);
            } else {
                StartNextPlayerTurn(Main().board.players[currentPlayer]);
            }
            Global().ClearReady(currentPlayer);
            return StageErrCode::CONTINUE;
        }

        // [回合结束] 所有玩家都行动后结束本回合
        // 门变更过进行提示
        if (door_modified > 0) {
            Global().Boardcast() << "【注意】在本回合内，门曾被按钮触发，共发生 " + to_string(door_modified) + " 次变化";
            Main().board.all_extra_record += "<br>【第 " + to_string(Main().round_) + " 回合】门曾被按钮触发，发生 " + to_string(door_modified) + " 次变化";
            door_modified = 0;
        }
        // BOSS相关结算
        if (GAME_OPTION(BOSS) != BossType::NONE) {
            string boss_record = "【第 " + to_string(Main().round_) + " 回合】";
            Boss& boss = Main().board.boss;
            boss.NewRecord("");
            auto sender = Global().Boardcast();
            sender << "【回合结束[BOSS行动]】";

            if (boss.Is(BossType::MINOTAUR)) HandleMinotaurBossAction(boss, boss_record, sender);
            if (boss.Is(BossType::BANGBANG)) HandleBangBangBossAction(boss, boss_record, sender);

            boss.UpdateContentRecord(boss_record);
        }

        return StageErrCode::CHECKOUT;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        Main().board.players[pid].out = 1;
        Main().board.players[pid].score.quit_score -= 300;  // 退出分

        auto sender = Global().Boardcast();
        sender << "笨笨的机器人退出了游戏";
        if (Main().Alive_() > 1) {
            Main().board.UpdatePlayerTarget(GAME_OPTION(捕捉目标));   // 捕捉顺位变更
            sender << "，捕捉目标顺位发生变更！\n";
            sender << Markdown(Main().board.GetPlayerTable(Main().round_));
        }
        return StageErrCode::READY;
    }

    int TimerLeft() const { return std::chrono::duration_cast<std::chrono::seconds>(*Global().TimerFinishTime() - std::chrono::steady_clock::now()).count(); }

    // ========== 成员函数 ==========
    bool HandleGridInteraction(Player& player, MsgSenderBase::MsgSenderGuard& sender, const bool multiple_mode);
    bool PlayerCatch(Player& player, MsgSenderBase::MsgSenderGuard& sender);
    void SendSoundMessage(const int fromX, const int fromY, const Sound sound, const bool to_all);
    void HandleMinotaurBossAction(Boss& boss, string& boss_record, MsgSenderBase::MsgSenderGuard& sender);
    void HandleBangBangBossAction(Boss& boss, string& boss_record, MsgSenderBase::MsgSenderGuard& sender);
};


// 处理区块效果（返回玩家回合是否结束）
bool RoundStage::HandleGridInteraction(Player& player, MsgSenderBase::MsgSenderGuard& sender, const bool multiple_mode)
{
    const string prefix = "\n";

    // [亚空间] * 优先级必须最高 *
    Grid& former_grid = Main().board.grid_map[player.x][player.y];
    // 亚空间内不影响地图
    if (player.InSubspace()) return false;  
    // 离开亚空间，传送门传送
    bool this_move_teleport = false;
    if (player.subspace == 0) {
        Main().board.RemovePlayerFromMap(player.pid);
        former_grid.PortalTeleport(player);
        this_move_teleport = true;
        Main().board.player_map[player.x][player.y].push_back(player.pid);
    }
    
    // 玩家当前所在格子
    Grid& grid = Main().board.grid_map[player.x][player.y];

    // 成就[乒铃乓啷]辅助
    player.achievement.visitGrid(grid.Type());
    player.achievement.visitAttach(grid.Attach());
    /* ========== AttachType ========== */
    // [按钮]
    if (grid.Attach() == AttachType::BUTTON) {
        vector<Grid::ButtonTarget> targets = grid.ButtonTargetPos();
        const int s = Main().board.size;
        for (auto const &target : targets) {
            // 切换[门]状态
            if (target.dir.has_value()) {
                int tx = player.x + target.dx;
                int ty = player.y + target.dy;
                const Direct dir = target.dir.value();
                Main().board.grid_map[tx][ty].switchDoor(dir);
                int d = static_cast<int>(dir);
                int nx = (tx + k_DX_Direct[d] + s) % s;
                int ny = (ty + k_DY_Direct[d] + s) % s;
                Main().board.grid_map[nx][ny].switchDoor(opposite(dir));
            }
        }
        door_modified++;
    }
    // [炸弹]
    if (player.bomb_trigger) {
        // 炸弹爆炸
        player.UpdateEndRecord("炸飞出局");
        player.out = 1;
        player.score.catch_score -= 100;
        grid.SetAttach(AttachType::EMPTY);
        sender << prefix << GetRandomHint(bomb_hints) << "\n\n尝试移动时触发【炸弹】，被炸飞出局，失去行动能力！";
        if (Global().PlayerNum() > 1) Global().Eliminate(player.pid);
        if (Main().Alive_() > 1) {
            Main().board.UpdatePlayerTarget(GAME_OPTION(捕捉目标));   // 捕捉顺位变更
            sender << "\n\n剩余玩家捕捉目标顺位发生变更！\n";
            sender << Markdown(Main().board.GetPlayerTable(Main().round_));
        }
        return true;
    }
    if (grid.Attach() == AttachType::BOMB) {
        player.bomb_trigger = true;
    }
    /* ========== GridType ========== */
    // [逃生舱]
    if (grid.Type() == GridType::EXIT) {
        player.NewContentRecord("(逃生)", "escape");
        int exited = Main().board.exit_num - Main().board.TypeCount(GridType::EXIT);
        player.score.exit_score += Score::exit_order[exited];   // 逃生分
        grid.SetType(GridType::EMPTY);
        if (!player.achievement.trigger_sound) player.achievement.exit_without_sound = true;    // 成就【悄无声息】
        if (Main().round_ == 1) player.achievement.exit_first_round = true; // 成就【我赶时间】

        if (GAME_OPTION(大乱斗)) {
            player.NewContentRecord("[传送]", "teleport");
            Main().board.TeleportPlayer(player.pid);  // 大乱斗随机传送
            sender << prefix << GetRandomHint(exit_hints) << "\n\n您已抵达【逃生舱】！此逃生舱已失效，" << At(player.pid) << " 被随机传送至地图其他地方！";
        } else {
            player.out = 2;
            sender << prefix << GetRandomHint(exit_hints) << "\n\n您已抵达【逃生舱】！不再参与后续游戏，此逃生舱已失效";
            if (Main().Alive_() > 1) {
                Main().board.UpdatePlayerTarget(GAME_OPTION(捕捉目标));   // 捕捉顺位变更
                sender << "，剩余玩家捕捉目标顺位发生变更！\n";
                sender << Markdown(Main().board.GetPlayerTable(Main().round_));
            }
        }
        // 隐匿状态公屏提示
        if (hide) {
            Global().Boardcast() << At(currentPlayer) << " 已抵达【逃生舱】！" << (GAME_OPTION(大乱斗) ? "被随机传送至地图其他地方" : "不再参与后续游戏") << "，此逃生舱已失效\n"
                                 << Markdown(Main().board.GetPlayerTable(Main().round_));
        }
        return true;
    }
    // [传送门] 仅本回合未传送时触发
    if (grid.Type() == GridType::PORTAL && !this_move_teleport) {
        // 进入亚空间
        player.subspace = 2;
        // 出口是[单向传送门]则交换
        pair<int, int> pRelPos = Main().board.grid_map[player.x][player.y].PortalPos();
        Grid& target_grid = Main().board.grid_map[player.x + pRelPos.first][player.y + pRelPos.second];
        if (target_grid.Type() == GridType::ONEWAYPORTAL) {
            grid.SetType(GridType::ONEWAYPORTAL);
            target_grid.SetType(GridType::PORTAL);
        }
    }
    // [陷阱]
    if (grid.Type() == GridType::TRAP) {
        grid.TrapTrigger();
        if (grid.TrapStatus()) {
            step++; // 成就【守株待兔】：强制停止计入额外步数
            player.NewContentRecord("(陷阱)");
            sender << prefix << GetRandomHint(trap_hints) << "\n\n移动触发【陷阱】，本回合被强制停止行动！";
            return true;
        }
    }
    /* ========== Sound ========== */
    Sound sound = Main().board.GetSound(grid, GAME_OPTION(特殊事件));
    if (sound == Sound::SHASHA) {
        if (hide) {
            sender << prefix << "移动进入【树丛】（隐匿中，不会向其他人发出声响）";
        } else {
            player.UpdateSoundRecord(sound);
            sender << prefix << GetRandomHint(grass_hints) << "\n移动进入【树丛】，请其他玩家留意私信声响信息！";
            SendSoundMessage(player.x, player.y, sound, false);
            player.achievement.trigger_sound = true;
        }
    } else if (sound == Sound::PAPA) {
        if (hide) {
            sender << prefix << "移动发出【啪啪声】（隐匿中，不会向其他人发出声响）";
        } else {
            player.UpdateSoundRecord(sound);
            sender << prefix << GetRandomHint(papa_hints) << "\n移动发出【啪啪声】，请其他玩家留意私信声响信息！";
            SendSoundMessage(player.x, player.y, sound, false);
            player.achievement.trigger_sound = true;
        }
    }
    // [热源]
    string step_info, heat_message;
    if (Main().board.HeatNotice(player.pid)) {
        step_info = "[热浪(第" + to_string(step) + "步)]";
        heat_message = GetRandomHint(heat_wave_hints) + "\n移动进入【热浪范围】，当前位置附近存在热源";
        player.NewExtraPriContent("热浪",  "heat-wave");
    }
    if (grid.Type() == GridType::HEAT) {
        if (player.heated) {
            step++; // 成就【守株待兔】：强制停止计入额外步数
            player.NewContentRecord("(热源)");
            sender << prefix << GetRandomHint(heat_active_hints) << "\n\n您本局游戏已进入过【热源】，高温难耐，本回合无法继续前进！";
            return true;
        } else {
            player.heated = true;
            step_info = "[热源(第" + to_string(step) + "步)]";
            heat_message = GetRandomHint(heat_core_hints) + "\n移动进入【热源】！请注意，在下一次进入热源时，将公开热源并强制停止行动";
            player.UpdateExtraPriContent("热源", "heat-core");
        }
    }
    if (heat_message != "") {
        Global().Tell(player.pid) << step_info << heat_message;
    }

    // 非点杀模式检测玩家捕捉（隐匿状态不能捕捉）
    if (!GAME_OPTION(点杀)) {
        if (PlayerCatch(player, sender)) {
            return true;
        }
    }
    // 玩家可继续行动
    return false;
}

// 捕捉：坐标重合，玩家没有隐匿且未出局
bool RoundStage::PlayerCatch(Player& player, MsgSenderBase::MsgSenderGuard& sender)
{
    vector<PlayerID> list = Main().board.player_map[player.x][player.y];
    PlayerID t = player.target;

    if (find(list.begin(), list.end(), t) == list.end() || hide || player.out != 0) {
        return false;
    }

    active_stop = false;    // 捕捉成功不视为主动停止

    sender << GetRandomHint(catch_hints) << "\n\n";
    sender << At(t) << " 被捕捉！";

    if (step == 1) player.achievement.catch_without_moving = true;  // 成就【守株待兔】

    if (Main().round_ == 1) {
        player.NewContentRecord("(首轮捕捉)", "catch");
        player.NewContentRecord("[传送]", "teleport");

        Player& target = Main().board.players[t];
        target.NewContentRecord("[首轮被抓传送]", "teleport");
        if (t < currentPlayer) {    // 被抓玩家在前面，强制刷新完整赛况
            target.all_record.back() = target.move_record;
        }

        Main().board.TeleportPlayer(t);     // 随机传送被捉方
        Main().board.TeleportPlayer(player.pid);    // 随机传送捕捉方
        player.achievement.catch_first_round = true;    // 成就【饥渴难耐】
        sender << "\n【首轮玩家保护】\n首轮捕捉不生效：双方均被随机传送至地图其他地方！";
        return true;
    }

    player.NewContentRecord("(捕捉)", "catch");
    Main().board.players[t].out = 1;
    Global().Eliminate(t);
    player.score.catch_score += 100;        // 抓人分
    Main().board.players[t].score.catch_score -= 100;
    player.achievement.recordCatch(Global().PlayerNum());   // 成就[嗜杀成性]辅助

    if (Main().Alive_() > 1) {
        player.NewContentRecord("[传送]", "teleport");
        Main().board.TeleportPlayer(player.pid);  // 随机传送捕捉方
        Main().board.UpdatePlayerTarget(GAME_OPTION(捕捉目标));   // 捕捉顺位变更
        sender << "\n" << At(player.pid) << " 被随机传送至地图其他地方，捕捉目标顺位发生变更！\n";
        sender << Markdown(Main().board.GetPlayerTable(Main().round_));
    }
    
    if (Main().Alive_() == 1) {
        // 无逃生舱最后生还判定
        if (Main().board.TypeCount(GridType::EXIT) == 0) Main().withoutE_win_ = true;
        // 有逃生舱但死斗取胜判定
        if (Main().board.TypeCount(GridType::EXIT) > 0) Main().withE_win_ = true;
    }
    return true;
}

// 私信其他玩家发送声响信息
void RoundStage::SendSoundMessage(const int fromX, const int fromY, const Sound sound, const bool to_all)
{
    for (PlayerID pid = 0; pid < Global().PlayerNum(); ++pid) {
        if ((pid != currentPlayer || to_all) && Main().board.players[pid].out == 0) {
            string direction = Main().board.GetSoundDirection(fromX, fromY, Main().board.players[pid]);
            string step_info = "[" + to_string(currentPlayer) + "号(第" + to_string(step) + "步)]";
            string sound_message;
            if (direction == "同格") {
                if (Main().board.players[currentPlayer].target != pid || GAME_OPTION(点杀)) {
                    switch (sound) {
                        case Sound::SHASHA: sound_message = step_info + "\n" + GetRandomHint(grass_sound_hints); break;
                        case Sound::PAPA:   sound_message = step_info + "\n" + GetRandomHint(papa_sound_hints); break;
                        default:            sound_message = "[错误] 未知声音类型：相同格子的未知声音";
                    }
                }
            } else {
                switch (sound) {
                    case Sound::SHASHA: sound_message = step_info + "你听见了来自【" + direction + "方】的沙沙声！"; break;
                    case Sound::PAPA:   sound_message = step_info + "你听见了来自【" + direction + "方】的啪啪声！"; break;
                    case Sound::BOSS:   sound_message = "[BOSS-米诺陶斯] 你听见了来自【" + direction + "方】的巨大响声！"; break;
                    default:            sound_message = "[错误] 未知声音类型：不同格子来自【" + direction + "方】的未知声音";
                }
            }
            if (sound == Sound::BOSS) {
                Main().board.boss.AddSoundPropagation(direction);
            } else {
                Main().board.players[currentPlayer].AddSoundPropagation(direction);
            }
            Global().Tell(pid) << sound_message;
        } else {
            Main().board.players[currentPlayer].AddSoundPropagation("错误");
        }
    }
}

// [BOSS-米诺陶斯] 行动
void RoundStage::HandleMinotaurBossAction(Boss& boss, string& boss_record, MsgSenderBase::MsgSenderGuard& sender)
{
    if (boss.BossChangeTarget(false)) {
        // 更换目标，重置步数
        boss_record += "发现更近的目标，变更目标至 [" + to_string(boss.target) + "号]";
        sender << "\n[BOSS-米诺陶斯] 发现了距离更近的玩家，变更锁定目标至 " << At(boss.target);
    } else {
        // 未更换目标，执行移动
        if (boss.BossMove()) {
            // 抓住玩家
            for (const auto pid: Main().board.player_map[boss.x][boss.y]) {
                Player& catched_player = Main().board.players[pid];
                if (catched_player.out > 0) continue;
                catched_player.NewContentRecord("(BOSS捕捉出局)", "end");
                catched_player.all_record.back() = catched_player.move_record;  // 回合已经结束，需强制更新完整赛况
                catched_player.out = 1;
                if (Global().PlayerNum() > 1) Global().Eliminate(pid);
                catched_player.score.catch_score -= 100;        // 抓人分
                boss_record += "[" + to_string(pid) + "号] ";
                sender << "\n" << At(pid);
            }
            boss_record += "被BOSS捕捉出局！";
            sender << "\n被BOSS捕捉出局！";
            if (Main().Alive_() > 1) {
                boss.BossChangeTarget(true);    // 重置锁定目标
                Main().board.UpdatePlayerTarget(GAME_OPTION(捕捉目标));     // 捕捉顺位变更
                boss_record += "变更目标至 [" + to_string(boss.target) + "号]";
                sender << "\n\nBOSS更换锁定目标至 " << At(boss.target) << "，同时玩家捕捉目标顺位发生变更！\n";
                sender << Markdown(Main().board.GetPlayerTable(Main().round_));
            }
        } else {
            // 未抓住玩家
            boss_record += "向 [" + to_string(boss.target) + "号] 移动了 " + to_string(boss.steps) + " 步";
            sender << "\n[BOSS-米诺陶斯] 向 " << At(boss.target) << " 移动了 " << boss.steps << " 步";
            if (boss.steps == 3) Main().board.players[boss.target].achievement.boss_chase_four_steps = true;    // 成就【牛头魅魔】
        }
        // BOSS移动后发出巨响
        if (Main().Alive_() > 1 || (Global().PlayerNum() == 1 && Main().board.players[0].out == 0)) {
            boss.UpdateSoundRecord(Sound::BOSS);
            sender << "\n\nBOSS发出震耳欲聋的巨响！请所有玩家留意私信声响信息！";
            SendSoundMessage(boss.x, boss.y, Sound::BOSS, true);
        }
    }
    // BOSS周围8格内获得喘息提示
    for (auto& player : Main().board.players) {
        if (boss.IsBossNearby(player) && player.out == 0) {
            player.private_record += "\n[BOSS-米诺陶斯] 你听到来自BOSS沉重的喘息声！";
            Global().Tell(player.pid) << "「呼……呼……」你听到来自[米诺陶斯]沉重的喘息声！";
        }
    }
}

// [BOSS-邦邦] 行动
void RoundStage::HandleBangBangBossAction(Boss& boss, string& boss_record, MsgSenderBase::MsgSenderGuard& sender)
{
    // 更新目标
    if (boss.BossChangeTarget(false)) {
        boss_record += "变更目标至 [" + to_string(boss.target) + "号]，";
        sender << "\nBOSS发现了距离更近的玩家，变更锁定目标至 " << At(boss.target);
    }
    // 每回合一定移动
    boss_record += "BOSS 移动中...";
    sender << "\n[BOSS-邦邦] 移动中...";
    if (boss.BossMove()) {
        // 到达玩家位置（不会捕捉）
        boss_record += "追上了玩家 [" + to_string(boss.target) + "号]，";
        sender << "\n【邦邦】追到你了 [" + to_string(boss.target) + "号]！你说邦邦不邦邦！";
        boss.BossChangeTarget(true);    // 重置锁定目标
        boss_record += "变更目标至 [" + to_string(boss.target) + "号]，";
        sender << "\nBOSS抵达目标位置，更换新目标 " << At(boss.target);
    }
    // 放置炸弹
    Grid& grid = Main().board.grid_map[boss.x][boss.y];
    if (grid.Attach() == AttachType::EMPTY) grid.SetAttach(AttachType::BOMB);
    // 公屏展示炸弹墙壁信息
    auto [info, md] = Main().board.GetBangBangSurroundingWalls(boss.x, boss.y);
    string wall_info = "BOSS所在位置的四周墙壁信息，按照 上下左右 顺序分别是：\n" + info;
    boss_record += "放置炸弹（" + info + "）";
    sender << "\n\n【邦邦】哈哈，炸弹来喽~"
           << "\n" << wall_info
           << "\n" << Markdown(md, (GRID_SIZE + WALL_SIZE * 2) + 40);
}


auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot

