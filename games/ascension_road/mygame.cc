// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// This file was generated with the assistance of Claude Code (claude.ai/code).

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "game_framework/stage.h"
#include "game_framework/util.h"

using namespace std;

#include "constants.h"
#include "player.h"
#include "board.h"
#include "skill.h"
#include "bot.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

class MainStage;
template <typename... SubStages> using SubGameStage = StageFsm<MainStage, SubStages...>;
template <typename... SubStages> using MainGameStage = StageFsm<void, SubStages...>;

const GameProperties k_properties {
    .name_ = "登仙路",
    .developer_ = "铁蛋",
    .description_ = "在九宫遗界中夺灵噬魂，一路修至合道飞升的多人厮杀游戏",
    .shuffled_player_id_ = true,
};

uint64_t MaxPlayerNum(const CustomOptions& options) { return MAX_PLAYER; }

uint32_t Multiple(const CustomOptions& options) { return 1; }

const MutableGenericOptions k_default_generic_options{
    .is_formal_ = false,
};

bool AdaptOptions(MsgSenderBase& reply, CustomOptions& game_options, const GenericOptions& generic_options_readonly, MutableGenericOptions& generic_options)
{
    if (generic_options_readonly.PlayerNum() < MIN_PLAYER) {
        reply() << "该游戏至少 " << MIN_PLAYER << " 人参加，当前玩家数为 " << generic_options_readonly.PlayerNum();
        return false;
    }
    return true;
}

const std::vector<InitOptionsCommand> k_init_options_commands = {
    InitOptionsCommand("独自一人开始游戏",
            [] (CustomOptions& game_options, MutableGenericOptions& generic_options)
            {
                // 补足到足以引出半仙的人数；若配置为永不改道，则退回人数上限
                const uint32_t threshold = GET_OPTION_VALUE(game_options, 半仙人数);
                generic_options.bench_computers_to_player_num_ = threshold > MAX_PLAYER ? MAX_PLAYER : threshold;
                return NewGameMode::SINGLE_USER;
            },
            VoidChecker("单机")),
};

// ========== 规则详情 ==========

static const std::string rule_details[8] = {
    R"EOF(《登仙路规则与机制细节》
使用「)EOF" META_COMMAND_SIGN R"EOF(规则 登仙路 <参数>」查看详情：
【法修】【体修】【邪修】【半仙】四条修行路线的完整能力表
【仙器】仙器降世流程与各件仙器的效果
【结算】同回合行动的结算顺序与伤害判定口径
【地图】九宫遗界的移动、摧毁与血阵机制)EOF",

    MakePathDetail(Path::FA),
    MakePathDetail(Path::TI),
    MakePathDetail(Path::XIE),
    MakePathDetail(Path::BAN),

    R"EOF(【仙器降世】
1、当场上出现掌握「夺天造化功」的修士（法修/邪修 化神、半仙 合道），天地大震。
2、自下一回合起，每回合有 10% 概率判定破碎仙器自中央区域天空陨落，判定成功将全场公示并公布仙器名字。
3、公示的下一回合，处于中央区域且境界达到化神及以上的修士（体修不可夺）可使用「夺天造化功」参与争夺。
4、所有争夺者以均等概率随机决出一位持有者，全场通报。若无人争夺，仙器消失。
5、一局游戏只会有一次仙器降世；仙器无法转移，持有者死去则仙器一同陨落（使用浑然一体逃脱后仙器同样不存）。

【仙器一览】
-万念璃花：得知同区域内所有人本回合的行动，并在全场最后追加一次行动。每回合限一次，使用三次后自动破碎。
-无垠寿果：服用寿果，增加 15 血。只可使用一次。
-水中镜（被动）：当你本回合受到的伤害结算后足以致命，则直接免去此次伤害，并额外回复 3 血。使用一次后自动破碎。
-墨杀仙剑：获得「墨杀」一式。指定一人，对其斩下 5 点无来源伤害；不指定目标则对全场除你以外所有人各斩 3 点。无来源之伤不为以命祭杀所反制，亦不引因果反噬之报。可反复施展。)EOF",

    R"EOF(【结算顺序】
同回合的行动共同进行，按以下次序判定：
1、回复：虚行寿果、无垠寿果最先结算。
2、代价：淬体、铁骨铜皮、祭灭血阵、以命祭杀、分魂、仙之势、万念璃花的自身扣血。自身代价不受自己的防御与无伤效果影响。
3、伤害：各类攻击、震爆、虚神指、万物皆虚、血阵、腐朽同时计算。
4、窃机：同区域内使用金丹元婴招数（浑然一体/以命祭杀/震爆）者被窃取金丹元婴，招数效果作废，但负面效果与消耗照常承担。
5、防御：金光护身减 4、护灵减 2、铁骨铜皮减 4、不动如山减 2 后溢出伤害减半、分魂与仙之势本回合无伤。
6、放大：使用猛击或暴击者，若本回合受到任何伤害，其受到的伤害总和乘 1.5。
7、因果反噬：按第 6 步得出的最终扣血量统计，同区域内造成伤害总量最多者受到 3 点伤害并公开播报姓名；无人造成伤害时该区域全员受到伤害。
8、以命祭杀：对本回合造成过伤害的修士各造成 5 点伤害。腐朽与因果反噬计入来源，血阵为无来源伤害不计入。第 7、8 步的伤害不再被防御减免。
9、死亡：血量降至 0 及以下判定死亡。免死判定次序为「不死不灭 → 水中镜 → 浑然一体」。
10、万物皆虚与仙之威强制将血量归零，无视一切防御与免死机制；其中万物皆虚仍可被浑然一体逃脱，仙之威不可。
　　万物皆虚乃无来源的湮灭：死于其中者算作凭空消失，无人因此得到击杀分，亦不会追溯是谁出的手。
11、追加行动：万念璃花的追加行动在全场结算完毕后单独结算，此时目标的防御减免已消耗，不再重复抵挡。持有者若已死亡则追加行动作废。

【伤害口径】
腐朽的固定扣血无法防御，不计入防御减免的计算，但计入修行失败判定、猛击放大与不死不灭的伤害总和。

【一次性招数】
浑然一体、以命祭杀、震爆一经施展便就此耗尽，无论本回合是否真的生效、是否被窃机作废。浑然一体与以命祭杀共用元婴，用过其一另一个也不复可用。)EOF",

    R"EOF(【九宫遗界】
一 二 三
四 五 六
七 八 九
1、九个区域八向互通，「五」为中央区域，可通往任意区域，且永远无法被摧毁或吞噬。
2、每回合的移动阶段可停留原地或前往相邻且未被摧毁的区域。任何区域都与中央区域相邻。
3、法修、邪修、半仙在中央区域聚气修行成功时，额外获得 0.5 修行值。
4、自第 4 回合起，若存在无人的普通区域，天道每回合随机摧毁其中一个。
5、吞天噬地会摧毁自身所处的空区域，施术者仍留在其中，但下一回合必须离开。该修士依然可以被攻击。

【血祭大阵】
1、祭灭血阵在布置的瞬间公开播报区域，并于下一回合自动触发，无论布置者是否存活。
2、触发时该区域内除各自施术者外的所有人受到 5 点无来源伤害，多座血阵可叠加。
3、公开播报只提示该区域存在血阵，不会提示数量。)EOF",
};

const std::vector<RuleCommand> k_rule_commands = {
    RuleCommand("查看修行方向和规则细节：「" META_COMMAND_SIGN "规则 登仙路 机制」查看可用列表帮助",
            [](const int type) { return rule_details[type].c_str(); },
            AlterChecker<int>({{"机制", 0}, {"法修", 1}, {"体修", 2}, {"邪修", 3}, {"半仙", 4},
                               {"仙器", 5}, {"结算", 6}, {"地图", 7}})),
};

// ========== GAME STAGES ==========

class PathStage;
class LandingStage;
class MoveStage;
class ActionStage;
class LihuaStage;

// 一次伤害事件
struct DmgEvent
{
    int from;       // 造成伤害的玩家，-1 表示无来源
    int to;
    Num value;
    DmgSource source;
    Act by;         // 造成该伤害的招式
};

class MainStage : public MainGameStage<PathStage, LandingStage, MoveStage, ActionStage, LihuaStage>
{
  public:
    MainStage(StageUtility&& utility)
        : StageFsm(std::move(utility),
            MakeStageCommand(*this, "查看当前遗界全景与所有修士的境界血量", &MainStage::Status_, VoidChecker("赛况")),
            MakeStageCommand(*this, "私信查看自己的修行方向、境界与当前可用行动", &MainStage::MyStatus_, VoidChecker("状态")),
            MakeStageCommand(*this, "查看四条修行路线的完整招式图鉴", &MainStage::Atlas_, VoidChecker("图鉴")))
        , round_(0)
        , player_scores_(Global().PlayerNum(), 0)
    {}

    virtual void FirstStageFsm(SubStageFsmSetter setter) override;
    virtual void NextStageFsm(PathStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(LandingStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(MoveStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(ActionStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;
    virtual void NextStageFsm(LihuaStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter) override;

    virtual int64_t PlayerScore(const PlayerID pid) const override { return player_scores_[pid]; }

    // 回合数
    int round_;
    // 遗界与全部修士
    Board board;
    // 玩家最终得分
    std::vector<int64_t> player_scores_;
    // 游戏是否已结束
    bool game_over_ = false;
    // 本回合的公屏结算摘要
    std::string round_report_;
    // 本回合陨落、待向框架发出淘汰通知的玩家
    std::vector<int> pending_eliminate_;
    // 仅剩两人后的决战截止回合，-1 表示尚未进入决战
    int duel_deadline_ = -1;
    // 移动阶段被略过的提示只播报一次
    bool skip_move_notified_ = false;

    int PlayerNum() const { return static_cast<int>(Global().PlayerNum()); }
    Player& P(const int pid) { return board.players[pid]; }
    const Player& P(const int pid) const { return board.players[pid]; }
    // 展示用的玩家编号，从 1 开始
    static std::string No(const int pid) { return PlayerNo(pid); }

    /* ===== 结算 ===== */
    void SettleActions(const bool extra);
    void SettleRoundEnd();
    void AdvanceArtifact();
    // 区域湮灭时，把随之消散的血阵一并写入战报
    void ReportVanishedArrays(const int count, const int region)
    {
        if (count > 0) {
            round_report_ += "🩸 区域「" + std::string(region_cn[region]) + "」的血祭大阵失去依托，" + std::to_string(count) + " 座尽数消散\n";
        }
    }
    void KillPlayer(const int pid, const int killer, const std::string& reason);
    void QuitPlayer(const int pid);
    void FinishRound(SubStageFsmSetter& setter);
    void FinishGame();

    /* ===== 消息与界面 ===== */
    void TellPlayerIdentity(const int pid);
    void BoardcastBoard(const std::string& phase);
    std::string BoardHtml(const std::string& phase) const { return board.BoardHtml(phase, round_, duel_deadline_); }
    static int ImageWidth() { return Board::ImageWidth(); }

  private:
    CompReqErrCode Status_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(BoardHtml(cur_phase_), ImageWidth());
        return StageErrCode::OK;
    }

    CompReqErrCode MyStatus_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 修行方向乃身家性命所系，岂可当众自陈？请私信裁判查看自身状态";
            return StageErrCode::FAILED;
        }
        reply() << Markdown(board.MyStatusHtml(pid, round_), ImageWidth());
        return StageErrCode::OK;
    }

    CompReqErrCode Atlas_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        reply() << Markdown(Board::AtlasHtml(), Board::AtlasWidth());
        return StageErrCode::OK;
    }

    // 当前阶段名，用于赛况图片的标题
    std::string cur_phase_ = "开局";

  public:
    void SetPhase(const std::string& phase) { cur_phase_ = phase; }
};

void MainStage::BoardcastBoard(const std::string& phase)
{
    SetPhase(phase);
    Global().Boardcast() << Markdown(BoardHtml(phase), ImageWidth());
}

// 私信告知修行方向与初始能力
void MainStage::TellPlayerIdentity(const int pid)
{
    Player& player = P(pid);
    Global().Tell(pid) << "【你的修行方向】" << path_icon[PathIndex(player.path)] << player.PathName() << "\n"
                       << path_desc[PathIndex(player.path)] << "\n\n"
                       << "随时私信「状态」可查看自身道途与招式图鉴，完整能力表见「"
                       << META_COMMAND_SIGN << "规则 登仙路 " << player.PathName() << "」";
}

// ========== 阶段一：选择修行方向 ==========

class PathStage : public SubGameStage<>
{
  public:
    PathStage(MainStage& main_stage)
        : StageFsm(main_stage, "选择修行方向",
            MakeStageCommand(*this, "选择自己的修行方向", &PathStage::Choose_, AlterChecker<Path>(path_map)))
    {}

    virtual void OnStageBegin() override
    {
        Main().SetPhase("选择修行方向");
        Global().Boardcast() << "仙路垂危，万千通行路崩塌。你们是仅存的一批低阶修士，被困在千万修士遗骸所化的遗界之中。\n"
                                "唯有杀尽其余修士，掠夺其根本、噬其灵性，方可破除此界，白日飞升，羽化登仙。\n\n"
                                "请所有修士私信裁判选择自己的修行方向：法修 / 体修 / 邪修\n"
                                "此过程全程保密，超时未选择者将由天道随机指定（时限 " << GAME_OPTION(时限) << " 秒）";
        for (int i = 0; i < Main().PlayerNum(); i++) {
            Global().Tell(i) << "请选择你的修行方向，直接私信下列之一：\n\n"
                             << "【法修】" << path_desc[PathIndex(Path::FA)] << "\n"
                             << "【体修】" << path_desc[PathIndex(Path::TI)] << "\n"
                             << "【邪修】" << path_desc[PathIndex(Path::XIE)] << "\n\n"
                                "发送「图鉴」可一图纵览四路招式与解锁境界";
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

    // 挂机者不再被等待，阶段可提前结束，此时同样需要代其择路
    virtual CheckoutErrCode OnStageOver() override
    {
        FillUnready_(false);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        FillUnready_(true);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Quit_(pid);
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        AssignRandom_(pid);
        return StageErrCode::READY;
    }

  private:
    void AssignRandom_(const int pid)
    {
        static const Path paths[3] = {Path::FA, Path::TI, Path::XIE};
        Main().P(pid).path = paths[Main().board.RandomInt(3)];
    }

    // 为迟迟未择路者代为择路，hook 为真时一并置为挂机
    void FillUnready_(const bool hook)
    {
        for (int i = 0; i < Main().PlayerNum(); i++) {
            if (Global().IsReady(i) || !Main().P(i).Alive()) {
                continue;
            }
            AssignRandom_(i);
            Global().Tell(i) << "你迟迟未定道心，天道代你择路：【" << Main().P(i).PathName() << "】";
            if (hook) {
                Global().Hook(i);
            }
        }
    }

    void Quit_(const int pid)
    {
        Player& player = Main().P(pid);
        if (player.out == 0) {
            player.out = 2;
            player.out_round = Main().round_;
            Global().Boardcast() << At(PlayerID(pid)) << " 离开了这片遗界";
        }
    }

    AtomReqErrCode Choose_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Path path)
    {
        if (is_public) {
            reply() << "[错误] 道途乃身家性命所系，岂能当众剖白？请私信裁判择定你的修行方向";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 道心已定，此生不改";
            return StageErrCode::FAILED;
        }
        Main().P(pid).path = path;
        reply() << "你踏上了【" << path_cn[PathIndex(path)] << "】之路，静待降临遗界";
        return StageErrCode::READY;
    }
};

// ========== 阶段二：降临遗界 ==========

class LandingStage : public SubGameStage<>
{
  public:
    LandingStage(MainStage& main_stage)
        : StageFsm(main_stage, "降临遗界",
            MakeStageCommand(*this, "选择降临的区域", &LandingStage::Land_, VoidChecker("降临"), AlterChecker<int>(region_map)),
            MakeStageCommand(*this, "选择降临的区域（可省略「降临」）", &LandingStage::LandShort_, AlterChecker<int>(region_map)))
    {}

    virtual void OnStageBegin() override
    {
        Main().SetPhase("降临遗界");
        Main().board.FreezeRegion();
        for (int i = 0; i < Main().PlayerNum(); i++) {
            if (!Main().P(i).Alive()) {
                Global().SetReady(i);
            }
        }
        Global().Boardcast() << "九片遗界呈九宫格相连：\n一 二 三\n四 五 六\n七 八 九\n\n"
                                "「五」为中央区域，可通往任意区域，永远无法被摧毁。\n"
                                "请所有修士私信裁判选择降临的区域（如：降临 5），超时默认降临中央区域「五」（时限 " << GAME_OPTION(时限) << " 秒）";
        // 降临之事需私信裁判，故也私下逐一提醒，免得有人只盯着群里
        for (int i = 0; i < Main().PlayerNum(); i++) {
            if (!Main().P(i).Alive()) {
                continue;
            }
            Global().Tell(i) << "遗界已在眼前，择一处落身吧。\n"
                                "发送「5」或「五」即可，九个区域皆可选，「五」为中央区域，四通八达而永不湮灭。";
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        FillUnready_(false);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        FillUnready_(true);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Player& player = Main().P(pid);
        if (player.out == 0) {
            player.out = 2;
            player.out_round = Main().round_;
            Global().Boardcast() << At(pid) << " 离开了这片遗界";
        }
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid)) {
            return StageErrCode::OK;
        }
        Main().P(pid).region = Main().board.RandomInt(REGION_NUM);
        return StageErrCode::READY;
    }

  private:
    // 为迟迟未择落身之地者送入中央区域，hook 为真时一并置为挂机
    void FillUnready_(const bool hook)
    {
        for (int i = 0; i < Main().PlayerNum(); i++) {
            if (Global().IsReady(i) || !Main().P(i).Alive()) {
                continue;
            }
            Main().P(i).region = CENTER_REGION;
            Global().Tell(i) << "你迟迟未择落身之地，身形随波逐流，跌入中央区域「五」";
            if (hook) {
                Global().Hook(i);
            }
        }
    }

    AtomReqErrCode Land_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int region)
    {
        if (is_public) {
            reply() << "[错误] 落身之处一旦声张，便是引刀自戮。请私信裁判择定你的降临之地";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 落身之处已定，不容反悔";
            return StageErrCode::FAILED;
        }
        Main().P(pid).region = region;
        reply() << "你择定了区域「" << region_cn[region] << "」，静待落身遗界";
        return StageErrCode::READY;
    }

    AtomReqErrCode LandShort_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int region)
    {
        return Land_(pid, is_public, reply, region);
    }
};

// ========== 阶段三：移动 ==========

class MoveStage : public SubGameStage<>
{
  public:
    MoveStage(MainStage& main_stage, const int round)
        : StageFsm(main_stage, "第 " + to_string(round) + " 回合 · 移动",
            MakeStageCommand(*this, "前往指定区域", &MoveStage::Move_, VoidChecker("前往"), AlterChecker<int>(region_map)),
            MakeStageCommand(*this, "前往指定区域（可省略「前往」）", &MoveStage::MoveShort_, AlterChecker<int>(region_map)),
            MakeStageCommand(*this, "停留在原地", &MoveStage::Stay_, VoidChecker("停留")))
    {}

    virtual void OnStageBegin() override
    {
        Main().SetPhase("第 " + to_string(Main().round_) + " 回合 · 移动");
        Main().board.FreezeRegion();
        for (int i = 0; i < Main().PlayerNum(); i++) {
            if (!Main().P(i).Alive()) {
                Global().SetReady(i);
            }
        }
        Global().Boardcast() << "请所有修士私信裁判选择前往的区域，或选择停留原地（时限 " << GAME_OPTION(时限) << " 秒，超时默认停留）";
        for (int i = 0; i < Main().PlayerNum(); i++) {
            Player& player = Main().P(i);
            if (!player.Alive()) {
                continue;
            }
            auto sender = Global().Tell(i);
            sender << "你当前位于区域「" << region_cn[player.region] << "」";
            if (player.must_leave) {
                sender << "\n此地已被你吞噬，本回合必须离开！";
            }
            sender << "\n可前往：";
            for (const int r : Main().board.MovableRegions(player)) {
                sender << region_cn[r] << (r == player.region ? "(原地) " : " ");
            }
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        FillUnready_(false);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        FillUnready_(true);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().QuitPlayer(pid);
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid) || !Main().P(pid).Alive()) {
            return StageErrCode::OK;
        }
        const std::vector<int> list = Main().board.MovableRegions(Main().P(pid));
        if (!list.empty()) {
            Main().P(pid).region = Main().board.RandomPick(list);
            Main().P(pid).must_leave = false;
        }
        return StageErrCode::READY;
    }

  private:
    // 超时或退出时的默认行为：停留原地；若必须离开则前往中央区域
    void Default_(const int pid)
    {
        Player& player = Main().P(pid);
        if (player.must_leave) {
            player.region = CENTER_REGION;
            Global().Tell(pid) << "此地已被你吞噬，脚下无处可立，你随大势退往中央区域「五」";
        } else {
            Global().Tell(pid) << "你迟迟未定去向，索性按兵不动，仍守在区域「" << region_cn[player.region] << "」";
        }
        player.must_leave = false;
    }

    // 为迟迟未定去向者补上默认去向，hook 为真时一并置为挂机
    void FillUnready_(const bool hook)
    {
        for (int i = 0; i < Main().PlayerNum(); i++) {
            if (Global().IsReady(i) || !Main().P(i).Alive()) {
                continue;
            }
            Default_(i);
            if (hook) {
                Global().Hook(i);
            }
        }
    }

    AtomReqErrCode Move_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int region)
    {
        if (is_public) {
            reply() << "[错误] 行踪泄于人前，与自寻死路无异。请私信裁判择定你的去向";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 去向已定，此念不可回";
            return StageErrCode::FAILED;
        }
        Player& player = Main().P(pid);
        if (!player.Alive()) {
            reply() << "[错误] 你已身死道消，天地虽大，再无你的去处";
            return StageErrCode::FAILED;
        }
        const int target = region;
        if (target < 0 || target >= REGION_NUM) {
            reply() << "[错误] 此界只有九域，并无此地";
            return StageErrCode::FAILED;
        }
        if (!Main().board.CanMoveTo(player, target)) {
            if (target == player.region && player.must_leave) {
                reply() << "[错误] 此地已被你吞入腹中，脚下无处可立，本回合必须离去";
            } else if (Main().board.Destroyed(target)) {
                reply() << "[错误] 区域「" << region_cn[target] << "」早已归于虚无，去无可去";
            } else {
                reply() << "[错误] 区域「" << region_cn[target] << "」远在天边，自「"
                        << region_cn[player.region] << "」一步难至";
            }
            return StageErrCode::FAILED;
        }
        const bool stay = (target == player.region);
        player.region = target;
        player.must_leave = false;
        reply() << (stay ? "你按兵不动，仍守在区域「" : "你悄然遁向区域「") << region_cn[target] << "」";
        return StageErrCode::READY;
    }

    AtomReqErrCode MoveShort_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const int region)
    {
        return Move_(pid, is_public, reply, region);
    }

    AtomReqErrCode Stay_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        return Move_(pid, is_public, reply, Main().P(pid).region);
    }
};

// ========== 阶段四：行动 ==========

class ActionStage : public SubGameStage<>
{
  public:
    ActionStage(MainStage& main_stage, const int round)
        : StageFsm(main_stage, "第 " + to_string(round) + " 回合 · 行动",
            MakeStageCommand(*this, "使用无需指定目标的行动（如：聚气修行）", &ActionStage::Simple_, AlterChecker<Act>(simple_act_map)),
            MakeStageCommand(*this, "使用需指定玩家的行动（如：火球术 3）", &ActionStage::Target_, AlterChecker<Act>(target_act_map),
                ArithChecker<int64_t>(1, main_stage.Global().PlayerNum(), "玩家编号")),
            MakeStageCommand(*this, "使用需指定区域的行动（如：万物皆虚 7）", &ActionStage::Region_, AlterChecker<Act>(region_act_map),
                AlterChecker<int>(region_map)),
            MakeStageCommand(*this, "使用需指定方向的行动（如：虚神指 右下）", &ActionStage::Direct_, AlterChecker<Act>(direct_act_map), AlterChecker<int>(direct_map)),
            MakeStageCommand(*this, "以招式代号快捷行动，目标可省略（如：HQ 3 / JG / WW 七）", &ActionStage::Code_,
                AlterChecker<Act>(code_act_map),
                OptionalDefaultChecker<BasicChecker<std::string>>(std::string(), "目标", "3")),
            MakeStageCommand(*this, "虚神指：全场只余一域时可不指方向", &ActionStage::DirectNoArg_,
                AlterChecker<Act>(direct_act_map)))
    {}

    virtual void OnStageBegin() override
    {
        Main().SetPhase("第 " + to_string(Main().round_) + " 回合 · 行动");
        for (int i = 0; i < Main().PlayerNum(); i++) {
            Player& player = Main().P(i);
            player.ClearRoundData();
            player.hp_start = player.hp;
            if (!player.Alive()) {
                Global().SetReady(i);
            }
        }
        Global().Boardcast() << "请所有修士私信裁判选择本回合的行动，全部行动将共同进行（时限 "
                             << GAME_OPTION(时限) << " 秒，超时默认修行）\n"
                                "私信「状态」可查看自己的境界与当前可用行动";
        for (int i = 0; i < Main().PlayerNum(); i++) {
            if (!Main().P(i).Alive()) {
                continue;
            }
            Global().Tell(i) << Markdown(Main().board.MyStatusHtml(i, Main().round_), Board::ImageWidth());
        }
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        FillUnready_(false);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        FillUnready_(true);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().QuitPlayer(pid);
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid) || !Main().P(pid).Alive()) {
            return StageErrCode::OK;
        }
        RandomAct(Main().board, pid, Main().P(pid).action);
        return StageErrCode::READY;
    }

  private:
    void Default_(const int pid)
    {
        Player& player = Main().P(pid);
        player.action.Clear();
        player.action.act = Act::XIULIAN;
        player.action.by_default = true;
    }

    // 为迟迟未出手者补上默认修行，hook 为真时一并置为挂机
    void FillUnready_(const bool hook)
    {
        for (int i = 0; i < Main().PlayerNum(); i++) {
            if (Global().IsReady(i) || !Main().P(i).Alive()) {
                continue;
            }
            Default_(i);
            Global().Tell(i) << "你迟迟未曾出手，索性闭目凝神，转而" << ActName(Main().P(i).path, Act::XIULIAN);
            if (hook) {
                Global().Hook(i);
            }
        }
    }

    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Action& action)
    {
        std::string err;
        if (!ValidateAction(Main().board, pid, action, err)) {
            reply() << "[错误] " << err;
            return StageErrCode::FAILED;
        }
        Main().P(pid).action = action;
        reply() << "心念已决：" << ActDesc(Main().P(pid), action) << "\n静待众修同参，此念不可再改";
        return StageErrCode::READY;
    }

    bool CheckReady_(const PlayerID pid, const bool is_public, MsgSenderBase& reply)
    {
        if (is_public) {
            reply() << "[错误] 出手之前先泄了机锋，还谈什么厮杀？请私信裁判决定本回合的行动";
            return false;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 招已出手，覆水难收";
            return false;
        }
        if (!Main().P(pid).Alive()) {
            reply() << "[错误] 你已身死道消，再无出手之机";
            return false;
        }
        return true;
    }

    AtomReqErrCode Simple_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act)
    {
        if (!CheckReady_(pid, is_public, reply)) {
            return StageErrCode::FAILED;
        }
        Action action;
        action.act = act;
        return Submit_(pid, is_public, reply, action);
    }

    AtomReqErrCode Target_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act, const int64_t target)
    {
        if (!CheckReady_(pid, is_public, reply)) {
            return StageErrCode::FAILED;
        }
        Action action;
        action.act = act;
        action.target = static_cast<int>(target) - 1;
        return Submit_(pid, is_public, reply, action);
    }

    AtomReqErrCode Region_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act, const int region)
    {
        if (!CheckReady_(pid, is_public, reply)) {
            return StageErrCode::FAILED;
        }
        Action action;
        action.act = act;
        action.region = region;
        return Submit_(pid, is_public, reply, action);
    }

    AtomReqErrCode Direct_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act, const int direct)
    {
        if (!CheckReady_(pid, is_public, reply)) {
            return StageErrCode::FAILED;
        }
        Action action;
        action.act = act;
        action.direct = direct;
        return Submit_(pid, is_public, reply, action);
    }

    AtomReqErrCode DirectNoArg_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act)
    {
        if (!CheckReady_(pid, is_public, reply)) {
            return StageErrCode::FAILED;
        }
        Action action;
        action.act = act;
        std::string err;
        if (!FillArg(Main().board, pid, action, "", err)) {
            reply() << "[错误] " << err;
            return StageErrCode::FAILED;
        }
        return Submit_(pid, is_public, reply, action);
    }

    AtomReqErrCode Code_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act, const std::string& arg)
    {
        if (!CheckReady_(pid, is_public, reply)) {
            return StageErrCode::FAILED;
        }
        Action action;
        action.act = act;
        std::string err;
        if (!FillArg(Main().board, pid, action, arg, err)) {
            reply() << "[错误] " << err;
            return StageErrCode::FAILED;
        }
        return Submit_(pid, is_public, reply, action);
    }
};

// ========== 阶段五：万念璃花追加行动 ==========

class LihuaStage : public SubGameStage<>
{
  public:
    LihuaStage(MainStage& main_stage, const int holder)
        : StageFsm(main_stage, "万念璃花 · 追加行动",
            MakeStageCommand(*this, "使用无需指定目标的追加行动", &LihuaStage::Simple_, AlterChecker<Act>(simple_act_map)),
            MakeStageCommand(*this, "使用需指定玩家的追加行动", &LihuaStage::Target_, AlterChecker<Act>(target_act_map),
                ArithChecker<int64_t>(1, main_stage.Global().PlayerNum(), "玩家编号")),
            MakeStageCommand(*this, "使用需指定区域的追加行动", &LihuaStage::Region_, AlterChecker<Act>(region_act_map),
                AlterChecker<int>(region_map)),
            MakeStageCommand(*this, "使用需指定方向的追加行动", &LihuaStage::Direct_, AlterChecker<Act>(direct_act_map), AlterChecker<int>(direct_map)),
            MakeStageCommand(*this, "以招式代号快捷追加行动，目标可省略", &LihuaStage::Code_,
                AlterChecker<Act>(code_act_map),
                OptionalDefaultChecker<BasicChecker<std::string>>(std::string(), "目标", "3")),
            MakeStageCommand(*this, "虚神指：全场只余一域时可不指方向", &LihuaStage::DirectNoArg_,
                AlterChecker<Act>(direct_act_map)))
        , holder_(holder)
    {}

    virtual void OnStageBegin() override
    {
        for (int i = 0; i < Main().PlayerNum(); i++) {
            if (i != holder_) {
                Global().SetReady(i);
            }
        }
        Global().Tell(holder_) << Markdown(Main().board.LihuaHtml(holder_), Board::ImageWidth());
        Global().Tell(holder_) << "请选择你的追加行动，招式详解请私信「状态」";
        Global().Boardcast() << "🌸 仙器【万念璃花】绽放，" << At(PlayerID(holder_)) << " 将于全场最后追加一次行动";
        Global().StartTimer(GAME_OPTION(时限));
    }

    virtual CheckoutErrCode OnStageOver() override
    {
        FillUnready_(false);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnStageTimeout() override
    {
        FillUnready_(true);
        return StageErrCode::CHECKOUT;
    }

    virtual CheckoutErrCode OnPlayerLeave(const PlayerID pid) override
    {
        Main().QuitPlayer(pid);
        return StageErrCode::CONTINUE;
    }

    virtual AtomReqErrCode OnComputerAct(const PlayerID pid, MsgSenderBase& reply) override
    {
        if (Global().IsReady(pid) || !Main().P(pid).Alive()) {
            return StageErrCode::OK;
        }
        RandomAct(Main().board, pid, Main().P(pid).extra_action);
        if (Main().P(pid).extra_action.act == Act::LIHUA) {
            Main().P(pid).extra_action.Clear();
        }
        return StageErrCode::READY;
    }

  private:
    // 持花者迟迟未决则散去花念，hook 为真时一并置为挂机
    void FillUnready_(const bool hook)
    {
        if (Global().IsReady(holder_) || !Main().P(holder_).Alive()) {
            return;
        }
        Main().P(holder_).extra_action.Clear();
        Global().Tell(holder_) << "你迟迟未决，花中一念散去，此番追加之机就此错过";
        if (hook) {
            Global().Hook(holder_);
        }
    }

    AtomReqErrCode Submit_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Action& action)
    {
        if (is_public) {
            reply() << "[错误] 璃花所照乃是众生之念，岂可宣于人前？请私信裁判决定追加的行动";
            return StageErrCode::FAILED;
        }
        if (Global().IsReady(pid)) {
            reply() << "[错误] 追加之念已定，不可更易";
            return StageErrCode::FAILED;
        }
        if (action.act == Act::LIHUA) {
            reply() << "[错误] 万念璃花一回合只绽放一次，花已谢，不再开";
            return StageErrCode::FAILED;
        }
        std::string err;
        if (!ValidateAction(Main().board, pid, action, err)) {
            reply() << "[错误] " << err;
            return StageErrCode::FAILED;
        }
        Main().P(pid).extra_action = action;
        reply() << "花中一念已定：" << ActDesc(Main().P(pid), action) << "\n此念将在全场之后落下";
        return StageErrCode::READY;
    }

    AtomReqErrCode Simple_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act)
    {
        Action action;
        action.act = act;
        return Submit_(pid, is_public, reply, action);
    }

    AtomReqErrCode Target_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act, const int64_t target)
    {
        Action action;
        action.act = act;
        action.target = static_cast<int>(target) - 1;
        return Submit_(pid, is_public, reply, action);
    }

    AtomReqErrCode Region_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act, const int region)
    {
        Action action;
        action.act = act;
        action.region = region;
        return Submit_(pid, is_public, reply, action);
    }

    AtomReqErrCode Direct_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act, const int direct)
    {
        Action action;
        action.act = act;
        action.direct = direct;
        return Submit_(pid, is_public, reply, action);
    }

    AtomReqErrCode DirectNoArg_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act)
    {
        return Code_(pid, is_public, reply, act, "");
    }

    AtomReqErrCode Code_(const PlayerID pid, const bool is_public, MsgSenderBase& reply, const Act act, const std::string& arg)
    {
        Action action;
        action.act = act;
        std::string err;
        if (!FillArg(Main().board, pid, action, arg, err)) {
            reply() << "[错误] " << err;
            return StageErrCode::FAILED;
        }
        return Submit_(pid, is_public, reply, action);
    }

    const int holder_;
};

// 玩家中途退出：立即离场，不再参与后续游戏
void MainStage::QuitPlayer(const int pid)
{
    Player& player = P(pid);
    if (player.out != 0) {
        return;
    }
    player.out = 2;
    player.out_round = round_;
    player.action.Clear();
    player.extra_action.Clear();
    player.artifact = Artifact::NONE;
    if (board.artifact_owner == pid) {
        board.artifact_owner = -1;
    }
    const bool fuxiu_stopped = (board.fuxiu_active && board.fuxiu_caster == pid);
    if (fuxiu_stopped) {
        board.fuxiu_active = false;
    }
    Global().Boardcast() << At(PlayerID(pid)) << " 的身形自遗界中消散，不再参与后续厮杀";
    if (fuxiu_stopped) {
        Global().Boardcast() << "腐朽的源头已然离去，万物腐败就此停止";
    }
}

// ========== 死亡处理 ==========

void MainStage::KillPlayer(const int pid, const int killer, const std::string& reason)
{
    Player& player = P(pid);
    if (player.out != 0) {
        return;
    }
    player.out = 1;
    player.out_round = round_;
    player.result = RoundResult::DEAD;
    round_report_ += "💀 " + No(pid) + " " + player.name + " " + reason;
    if (killer >= 0 && killer != pid) {
        P(killer).score.kills++;
        round_report_ += "（终结于 " + No(killer) + " 之手）";
    }
    round_report_ += "\n";
    if (player.artifact != Artifact::NONE) {
        round_report_ += std::string(artifact_icon[static_cast<int>(player.artifact)]) + " 其持有的仙器【" +
                         std::string(artifact_cn[static_cast<int>(player.artifact)]) + "】一同陨落\n";
        player.artifact = Artifact::NONE;
        board.artifact_owner = -1;
    }
    if (board.fuxiu_active && board.fuxiu_caster == pid) {
        board.fuxiu_active = false;
        round_report_ += "🍃 腐朽的源头已然消散，万物腐败就此停止\n";
    }
    // 淘汰通知延后到整轮结算完毕再统一发出
    pending_eliminate_.push_back(pid);
}

// ========== 行动结算 ==========

void MainStage::SettleActions(const bool extra)
{
    const int n = PlayerNum();
    std::vector<DmgEvent> events;
    std::vector<char> lethal(n, 0);              // 万物皆虚 / 仙之威 强制归零
    std::vector<char> lethal_absolute(n, 0);     // 仙之威：无视一切免死机制
    std::vector<char> lethal_void(n, 0);         // 万物皆虚：无来源的湮灭，不认凶手也不记功
    std::vector<Num> round_loss(n, 0);           // 本回合已结算的扣血，强制归零者要在归零之后再承受一次
    std::vector<char> skill_void(n, 0);          // 招数被窃机作废
    std::vector<std::vector<int>> forced_from(n);
    std::vector<std::map<int, Num>> credit(n);   // 每名玩家受到的伤害的来源与数值
    // 无来源伤害的暗账：不参与以命祭杀与因果反噬，仅在无人可归功时用于判定凶手
    std::vector<std::map<int, Num>> silent_credit(n);

    const auto act_of = [&](const int i) -> const Action& { return extra ? P(i).extra_action : P(i).action; };
    const auto acting = [&](const int i) { return P(i).Alive() && !act_of(i).Empty(); };

    // 1. 回复类行动最先结算
    for (int i = 0; i < n; i++) {
        if (!acting(i)) {
            continue;
        }
        Player& p = P(i);
        if (act_of(i).act == Act::XUXING) {
            p.hp += HEAL_XUXING;
            p.xuxing_round = round_ + XUXING_DELAY;
            p.round_detail += "虚行寿果：血量 +" + NumStr(HEAL_XUXING) + "，将于第 " +
                              to_string(p.xuxing_round) + " 回合结束时反噬 " + NumStr(COST_XUXING_LATER) + " 血\n";
        } else if (act_of(i).act == Act::SHOUGUO) {
            p.hp += HEAL_SHOUGUO;
            p.shouguo_used = true;
            p.round_detail += "无垠寿果：血量 +" + NumStr(HEAL_SHOUGUO) + "\n";
            round_report_ += "🍑 " + No(i) + " 服下了无垠寿果，气血奔涌\n";
        }
    }

    // 2. 自身行动的代价
    for (int i = 0; i < n; i++) {
        if (!acting(i)) {
            continue;
        }
        Player& p = P(i);
        switch (act_of(i).act) {
            case Act::CUITI:    p.self_cost += COST_CUITI; break;
            case Act::TIEGU:    p.self_cost += COST_TIEGU; break;
            case Act::XUEZHEN:  p.self_cost += COST_XUEZHEN; break;
            case Act::YIMING:   p.self_cost += COST_YIMING; break;
            case Act::FENHUN:   p.self_cost += COST_FENHUN; break;
            case Act::XIANSHI:  p.self_cost += COST_XIANSHI; break;
            default: break;
        }
    }

    // 2b. 腐朽自发起的回合起即刻生效，需先于伤害收集
    const bool fuxiu_was_active = board.fuxiu_active;
    for (int i = 0; i < n; i++) {
        if (acting(i) && act_of(i).act == Act::FUXIU && !board.fuxiu_active) {
            board.fuxiu_active = true;
            board.fuxiu_caster = i;
            P(i).used_fuxiu = true;
            round_report_ += "☠ 万物腐败，生机尽失！自本回合起，除半仙外所有修士每回合固定扣去 " +
                             NumStr(DMG_FUXIU) + " 血\n";
        }
    }

    // 3. 收集伤害
    if (!extra) {
        std::vector<int> array_regions;
        for (const BloodArray& array : board.arrays_active) {
            if (std::find(array_regions.begin(), array_regions.end(), array.region) == array_regions.end()) {
                array_regions.push_back(array.region);
            }
            for (int i = 0; i < n; i++) {
                if (P(i).Alive() && P(i).region == array.region && P(i).pid.Get() != array.caster.Get()) {
                    events.push_back({-1, i, DMG_XUEZHEN, DmgSource::ARRAY, Act::XUEZHEN});
                }
            }
        }
        // 血阵触发全场通报：同一区域无论叠了几座，只通报一次
        std::sort(array_regions.begin(), array_regions.end());
        for (const int region : array_regions) {
            round_report_ += "🩸 区域「" + std::string(region_cn[region]) + "」的血祭大阵轰然发动，除施术者外，身处其中者尽受血光之灾\n";
        }
    }
    if (board.fuxiu_active && (!extra || !fuxiu_was_active)) {
        for (int i = 0; i < n; i++) {
            if (P(i).Alive() && P(i).path != Path::BAN) {
                events.push_back({board.fuxiu_caster, i, DMG_FUXIU, DmgSource::FUXIU, Act::FUXIU});
            }
        }
    }
    for (int i = 0; i < n; i++) {
        if (!acting(i)) {
            continue;
        }
        const Action& a = act_of(i);
        const Player& p = P(i);
        switch (a.act) {
            case Act::HUOQIU:
                events.push_back({i, a.target, DMG_HUOQIU, DmgSource::ATTACK, a.act});
                break;
            case Act::MENGJI:
                events.push_back({i, a.target, DMG_MENGJI, DmgSource::ATTACK, a.act});
                break;
            case Act::BAOJI:
                events.push_back({i, a.target, DMG_BAOJI, DmgSource::ATTACK, a.act});
                break;
            case Act::MIEHUN:
                events.push_back({i, a.target, DMG_MIEHUN, DmgSource::ATTACK, a.act});
                break;
            case Act::YITONG:
                events.push_back({i, a.target, DMG_YITONG, DmgSource::ATTACK, a.act});
                break;
            case Act::MOSHA:
                if (a.target >= 0) {
                    if (P(a.target).Alive()) {
                        events.push_back({-1, a.target, DMG_MOSHA, DmgSource::RELIC, a.act});
                        silent_credit[a.target][i] += DMG_MOSHA;
                    }
                    P(i).round_detail += "墨杀：一剑指向 " + No(a.target) + "，斩下 " + NumStr(DMG_MOSHA) + " 点无来源伤害\n";
                    round_report_ += "🗡 仙剑出鞘，剑锋直指 " + No(a.target) + "！\n";
                } else {
                    for (int j = 0; j < n; j++) {
                        if (j != i && P(j).Alive()) {
                            events.push_back({-1, j, DMG_MOSHA_ALL, DmgSource::RELIC, a.act});
                            silent_credit[j][i] += DMG_MOSHA_ALL;
                        }
                    }
                    P(i).round_detail += "墨杀：剑气横扫遗界，除你以外人人各受 " + NumStr(DMG_MOSHA_ALL) + " 点无来源伤害\n";
                    round_report_ += "🗡 仙剑出鞘，剑气横扫整片遗界！\n";
                }
                break;
            case Act::XINGHUO:
                for (int j = 0; j < n; j++) {
                    if (j != i && P(j).Alive() && P(j).region == p.region) {
                        events.push_back({i, j, DMG_XINGHUO, DmgSource::ATTACK, a.act});
                    }
                }
                break;
            case Act::SIHOU:
                for (int j = 0; j < n; j++) {
                    if (j != i && P(j).Alive() && P(j).region == p.region) {
                        events.push_back({i, j, DMG_SIHOU, DmgSource::ATTACK, a.act});
                    }
                }
                break;
            case Act::ZHENBAO:
                for (int j = 0; j < n; j++) {
                    if (P(j).Alive() && P(j).region == p.region) {
                        events.push_back({i, j, DMG_ZHENBAO, j == i ? DmgSource::SELF : DmgSource::ATTACK, a.act});
                    }
                }
                break;
            case Act::XUSHEN: {
                const std::vector<int> regions = board.XushenRegions(p.region, a.direct);
                std::vector<int> victims;
                for (int j = 0; j < n; j++) {
                    if (j != i && P(j).Alive() &&
                            std::find(regions.begin(), regions.end(), P(j).region) != regions.end()) {
                        victims.push_back(j);
                    }
                }
                const Num value = victims.size() == 1 ? DMG_XUSHEN_SINGLE : DMG_XUSHEN;
                for (const int j : victims) {
                    events.push_back({i, j, value, DmgSource::ATTACK, a.act});
                }
                break;
            }
            case Act::WANWU:
                // 湮灭无来源可循：不记 forced_from，故既无人得功，亦无从追溯是谁出的手
                for (int j = 0; j < n; j++) {
                    if (P(j).Alive() && P(j).region == a.region) {
                        lethal[j] = 1;
                        lethal_void[j] = 1;
                    }
                }
                break;
            case Act::XIANWEI:
                lethal[a.target] = 1;
                lethal_absolute[a.target] = 1;
                forced_from[a.target].push_back(i);
                break;
            default:
                break;
        }
    }

    // 4. 窃机：窃取同区域内金丹元婴招数使用者的根本
    if (!extra) {
        std::vector<int> victims_all;
        for (int q = 0; q < n; q++) {
            if (!acting(q) || act_of(q).act != Act::QIEJI) {
                continue;
            }
            Num gain = 0;
            std::vector<int> victims;
            for (int j = 0; j < n; j++) {
                if (j == q || !acting(j) || P(j).region != P(q).region) {
                    continue;
                }
                const Act ja = act_of(j).act;
                if (ja != Act::HUNRAN && ja != Act::YIMING && ja != Act::ZHENBAO) {
                    continue;
                }
                victims.push_back(j);
                if (P(j).HasYuanying()) {
                    gain += CULT_QIEJI_YUANYING;
                }
                if (P(j).HasJindan()) {
                    gain += CULT_QIEJI_JINDAN;
                }
            }
            if (victims.empty()) {
                P(q).round_detail += "窃机：此区域内无人施展金丹元婴招数，无事发生\n";
                continue;
            }
            P(q).cult_gain += gain;
            P(q).round_detail += "窃机：夺得 " + to_string(victims.size()) + " 人的金丹元婴，修为 +" + NumStr(gain) + "\n";
            for (const int j : victims) {
                victims_all.push_back(j);
            }
        }
        for (const int j : victims_all) {
            if (skill_void[j]) {
                continue;
            }
            skill_void[j] = 1;
            P(j).jindan_lost = true;
            P(j).yuanying_lost = true;
            P(j).round_detail += "你的金丹元婴被窃机夺走，本回合的招数效果尽数落空\n";
            events.erase(std::remove_if(events.begin(), events.end(),
                    [j](const DmgEvent& e) { return e.from == j; }), events.end());
        }
    }

    // 5. 防御结算与伤害应用
    for (int i = 0; i < n; i++) {
        if (!P(i).Alive()) {
            continue;
        }
        Player& p = P(i);
        Num pool = 0;
        Num fuxiu_dmg = 0;
        Num xushen_dmg = 0;     // 虚神指造成的部分，金光护身对其无效
        bool attacked = false;
        for (const DmgEvent& e : events) {
            if (e.to != i) {
                continue;
            }
            if (e.by == Act::XUSHEN) {
                xushen_dmg += e.value;
            }
            switch (e.source) {
                case DmgSource::FUXIU:  fuxiu_dmg += e.value; break;
                case DmgSource::SELF:   p.self_cost += e.value; break;
                case DmgSource::ATTACK: pool += e.value; attacked = true; break;
                case DmgSource::ARRAY: case DmgSource::RELIC: pool += e.value; break;
            }
        }
        if (!forced_from[i].empty()) {
            attacked = true;
        }
        p.attacked = attacked;
        const Act a = acting(i) ? act_of(i).act : Act::NONE;
        Num after = pool;
        std::string defend_text;
        if (a == Act::FENHUN || a == Act::XIANSHI) {
            after = 0;
            defend_text = (a == Act::FENHUN ? "分魂" : "仙之势");
        } else if (a == Act::JINGUANG) {
            // 金光遇虚神指即被虚化，其余伤害照常抵挡
            const Num shieldable = pool - xushen_dmg;
            after = (shieldable > BLOCK_JINGUANG ? shieldable - BLOCK_JINGUANG : 0) + xushen_dmg;
            defend_text = "金光护身";
            if (xushen_dmg > 0) {
                p.round_detail += "金光遇虚神指而虚化，" + NumStr(xushen_dmg) + " 点伤害尽数穿身而过\n";
            }
        } else if (a == Act::HULING) {
            after = pool > BLOCK_HULING ? pool - BLOCK_HULING : 0;
            defend_text = "护灵";
        } else if (a == Act::TIEGU) {
            after = pool > BLOCK_TIEGU ? pool - BLOCK_TIEGU : 0;
            defend_text = "铁骨铜皮";
        } else if (a == Act::BUDONG) {
            after = pool > BLOCK_BUDONG ? NumHalf(pool - BLOCK_BUDONG) : 0;
            defend_text = "不动如山";
        }
        if (!defend_text.empty() && pool > 0) {
            p.round_detail += defend_text + "：抵挡了 " + NumStr(pool - after) + " 点伤害\n";
            p.result = RoundResult::DEFENDED;
        }
        if (a == Act::XIANSHI && !attacked) {
            p.self_cost += COST_XIANSHI_EXTRA;
            p.round_detail += "仙之势：本回合未被任何人攻击到，额外扣去 " + NumStr(COST_XIANSHI_EXTRA) + " 血\n";
        }
        Num total = after + fuxiu_dmg;
        Num fuxiu_part = fuxiu_dmg;     // 放大后仍需单独记账，不死不灭免不去这一份
        if ((a == Act::MENGJI || a == Act::BAOJI) && total > 0) {
            const Num raised = NumMul1_5(total);
            p.round_detail += std::string(a == Act::MENGJI ? "猛击" : "暴击") + "：拳锋在外门户大开，受到的伤害由 " +
                              NumStr(total) + " 放大至 " + NumStr(raised) + "\n";
            total = raised;
            fuxiu_part = NumMul1_5(fuxiu_part);
        }
        // 伤害归因：按各来源的原始占比分摊防御后的伤害
        if (pool > 0 && after > 0) {
            for (const DmgEvent& e : events) {
                if (e.to != i || e.source != DmgSource::ATTACK) {
                    continue;
                }
                const Num part = e.value * after / pool;
                if (part > 0) {
                    credit[i][e.from] += part;
                    P(e.from).dmg_dealt += part;
                }
            }
        }
        if (fuxiu_dmg > 0 && board.fuxiu_caster >= 0) {
            credit[i][board.fuxiu_caster] += fuxiu_dmg;
            P(board.fuxiu_caster).dmg_dealt += fuxiu_dmg;
        }
        p.dmg_taken += total;
        p.fuxiu_taken += fuxiu_part > total ? total : fuxiu_part;
        round_loss[i] = total + p.self_cost;
        p.hp -= round_loss[i];
        if (total > 0) {
            p.round_detail += "本回合共受到 " + NumStr(total) + " 点伤害\n";
            if (p.result == RoundResult::NONE) {
                p.result = RoundResult::DAMAGED;
            }
        }
        if (p.self_cost > 0) {
            p.round_detail += "自身招数的代价：扣去 " + NumStr(p.self_cost) + " 血\n";
        }
        p.self_cost = 0;
    }

    // 5a. 嘶吼：每确实震伤一人，便添一分修为
    for (int i = 0; i < n; i++) {
        if (!acting(i) || act_of(i).act != Act::SIHOU) {
            continue;
        }
        int hit = 0;
        for (int j = 0; j < n; j++) {
            const auto it = credit[j].find(i);
            if (it != credit[j].end() && it->second > 0) {
                hit++;
            }
        }
        if (hit > 0) {
            const Num raw = CULT_SIHOU * hit;
            const Num gain = raw > CULT_SIHOU_LIMIT ? CULT_SIHOU_LIMIT : raw;
            P(i).cult_gain += gain;
            P(i).round_detail += "嘶吼：震伤 " + to_string(hit) + " 人，修为 +" + NumStr(gain) +
                                 (raw > gain ? "（单次上限）" : "") + "\n";
        } else {
            P(i).round_detail += "嘶吼：吼声空荡，无一人被震伤\n";
        }
    }

    // 5b. 万物皆虚与仙之威：强制将血量归零
    for (int i = 0; i < n; i++) {
        if (!P(i).Alive() || !lethal[i]) {
            continue;
        }
        Player& p = P(i);
        const Num lost = p.hp > 0 ? p.hp : 0;
        // 先强制归零，再承受本回合区域内已结算的伤害，故血量仍可为负，终局排名据此分出高下
        p.hp = -round_loss[i];
        for (const int f : forced_from[i]) {
            credit[i][f] += lost;
            P(f).dmg_dealt += lost;
        }
        p.round_detail += std::string(lethal_absolute[i] ? "仙之威加身，一切防御与机制尽皆无用\n"
                                                         : "此方天地归虚，你被一同湮灭\n");
    }

    // 6. 因果反噬
    for (int i = 0; i < n; i++) {
        if (!acting(i) || act_of(i).act != Act::YINGUO) {
            continue;
        }
        const int region = P(i).region;
        Num max_dmg = -1;
        for (int j = 0; j < n; j++) {
            if (P(j).Alive() && P(j).region == region && P(j).dmg_dealt > max_dmg) {
                max_dmg = P(j).dmg_dealt;
            }
        }
        std::string names;
        for (int j = 0; j < n; j++) {
            if (!P(j).Alive() || P(j).region != region || P(j).dmg_dealt != max_dmg) {
                continue;
            }
            P(j).hp -= DMG_YINGUO;
            P(j).dmg_taken += DMG_YINGUO;
            credit[j][i] += DMG_YINGUO;
            names += No(j) + " " + P(j).name + " ";
            P(j).round_detail += "因果反噬：你在区域「" + std::string(region_cn[region]) + "」中造成的伤害最多，受到 " +
                                 NumStr(DMG_YINGUO) + " 点反噬伤害\n";
        }
        if (!names.empty()) {
            round_report_ += "☯ 因果反噬于区域「" + std::string(region_cn[region]) + "」显现，" + names + "受到反噬\n";
        }
    }
    // 7. 以命祭杀
    for (int i = 0; i < n; i++) {
        if (!acting(i) || act_of(i).act != Act::YIMING) {
            continue;
        }
        Player& p = P(i);
        p.used_yiming = true;
        p.yuanying_lost = true;
        if (skill_void[i]) {
            continue;
        }
        std::string names;
        for (const auto& [from, value] : credit[i]) {
            if (from < 0 || from == i || value <= 0 || !P(from).Alive()) {
                continue;
            }
            P(from).hp -= DMG_YIMING;
            P(from).dmg_taken += DMG_YIMING;
            p.dmg_dealt += DMG_YIMING;
            names += No(from) + " ";
            P(from).round_detail += "以命祭杀反噬：" + No(i) + " 献祭元婴，你受到 " + NumStr(DMG_YIMING) + " 点伤害\n";
        }
        if (names.empty()) {
            p.round_detail += "以命祭杀：本回合无人对你造成伤害，元婴白白献祭\n";
        } else {
            p.round_detail += "以命祭杀：向 " + names + "各降下 " + NumStr(DMG_YIMING) + " 点伤害\n";
            round_report_ += "🔥 " + No(i) + " 献祭元婴，以命祭杀降临于 " + names + "\n";
        }
    }

    // 8. 死亡判定与免死机制
    for (int i = 0; i < n; i++) {
        Player& p = P(i);
        if (!p.Alive() || p.hp > 0) {
            continue;
        }
        if (!lethal_absolute[i]) {
            // 以痛止戈会把本回合的免疫门槛收紧
            const Num busi_limit = (acting(i) && act_of(i).act == Act::YITONG) ? BUSI_DMG_LIMIT_YITONG
                                                                              : BUSI_DMG_LIMIT;
            if (p.BusiActive(round_) && p.hp_start <= BUSI_HP_LIMIT && p.dmg_taken < busi_limit && !lethal[i]) {
                // 腐朽源自天地朽坏，不朽之躯亦挡它不住，这一份扣血照旧
                const Num refund = p.dmg_taken > p.fuxiu_taken ? p.dmg_taken - p.fuxiu_taken : 0;
                p.hp += refund;
                p.dmg_taken -= refund;
                if (p.fuxiu_taken > 0) {
                    p.round_detail += "不死不灭：伤害未达 " + NumStr(busi_limit) +
                                      " 点，然腐朽之伤出自天地朽坏，" + NumStr(p.fuxiu_taken) + " 点扣血无法抵挡\n";
                } else {
                    p.round_detail += "不死不灭：伤害未达 " + NumStr(busi_limit) + " 点，你免疫了这次致命伤\n";
                }
                if (p.hp > 0) {
                    round_report_ += "🗿 " + No(i) + " 血肉不朽，硬生生扛下了这一击\n";
                    p.result = RoundResult::DEFENDED;
                    continue;
                }
            }
            if (p.artifact == Artifact::JINGJIAN && !p.jingjian_broken && !lethal[i]) {
                p.hp += p.dmg_taken;
                p.dmg_taken = 0;
                p.jingjian_broken = true;
                p.artifact = Artifact::NONE;
                board.artifact_owner = -1;
                p.hp += HEAL_JINGJIAN;
                p.round_detail += "水中镜：镜中映出你的死劫，此次伤害尽数免去，另有 " + NumStr(HEAL_JINGJIAN) +
                                  " 血自镜中涌回，仙器随之破碎\n";
                round_report_ += "🪞 " + No(i) + " 的水中镜应声而碎，死劫化为虚影\n";
                if (p.hp > 0) {
                    p.result = RoundResult::DEFENDED;
                    continue;
                }
            }
            if (acting(i) && act_of(i).act == Act::HUNRAN && !skill_void[i]) {
                p.used_hunran = true;
                p.yuanying_lost = true;
                p.hp = HUNRAN_RECOVER_HP;
                p.region = CENTER_REGION;
                p.must_leave = false;
                p.result = RoundResult::ESCAPED;
                p.round_detail += "浑然一体：元婴脱体而出，你以 " + NumStr(HUNRAN_RECOVER_HP) + " 血现身于中央区域\n";
                round_report_ += "✨ " + No(i) + " 的元婴自本体逃脱，现身于中央区域「五」\n";
                if (p.artifact != Artifact::NONE) {
                    p.round_detail += "本体既灭，你持有的仙器一同陨落\n";
                    p.artifact = Artifact::NONE;
                    board.artifact_owner = -1;
                }
                continue;
            }
        }
        int killer = -1;
        Num max_credit = 0;
        for (const auto& [from, value] : credit[i]) {
            if (from >= 0 && from != i && value > max_credit) {
                max_credit = value;
                killer = from;
            }
        }
        // 明面上无人下手，便追究无来源伤害的执刃者
        if (killer < 0) {
            for (const auto& [from, value] : silent_credit[i]) {
                if (from >= 0 && from != i && value > max_credit) {
                    max_credit = value;
                    killer = from;
                }
            }
        }
        // 随区域一同湮灭者，其死无从归咎于人
        KillPlayer(i, lethal_void[i] ? -1 : killer,
                lethal_absolute[i] ? "被仙之威一念抹去"
              : lethal_void[i]     ? "随区域一同归于虚无"
                                   : "气绝身亡");
    }

    // 9. 招数的后续影响与区域变化
    for (int i = 0; i < n; i++) {
        if (act_of(i).Empty()) {
            continue;
        }
        Player& p = P(i);
        switch (act_of(i).act) {
            case Act::TUNTIAN:
                round_report_ += "🌀 区域「" + std::string(region_cn[p.region]) + "」被吞噬，自遗界中消失\n";
                ReportVanishedArrays(board.DestroyRegion(p.region), p.region);
                p.must_leave = true;
                p.cult_gain += CULT_TUNTIAN;
                p.round_detail += "吞天噬地：此地已被你吞噬，修为 +" + NumStr(CULT_TUNTIAN) + "，下回合必须离开\n";
                break;
            case Act::WANWU: {
                const std::string tip = "🌑 区域「" + std::string(region_cn[act_of(i).region]) +
                                        "」归于虚无，其中的一切尽数湮灭\n";
                if (round_report_.find(tip) == std::string::npos) {
                    round_report_ += tip;
                }
                ReportVanishedArrays(board.DestroyRegion(act_of(i).region), act_of(i).region);
                break;
            }
            case Act::ZHENBAO:
                p.used_zhenbao = true;
                p.jindan_lost = true;
                break;
            case Act::HUNRAN:
                // 元婴一途只有一次机会：无论是否真的逃脱，浑然一体一经施展便就此耗尽
                p.used_hunran = true;
                p.yuanying_lost = true;
                break;
            case Act::XUEZHEN: {
                board.arrays_pending.push_back({p.pid, p.region});
                const std::string tip = "🩸 区域「" + std::string(region_cn[p.region]) +
                                        "」浮现血祭大阵，将于下回合自动触发\n";
                // 同一区域纵有数座血阵，也只通报一次
                if (round_report_.find(tip) == std::string::npos) {
                    round_report_ += tip;
                }
                break;
            }
            case Act::WEISHEN: {
                if (!p.Alive()) {
                    break;
                }
                Global().Tell(i) << Markdown(board.WeishenHtml(), ImageWidth());
                break;
            }
            case Act::LIHUA:
                p.lihua_left--;
                if (p.lihua_left <= 0) {
                    p.artifact = Artifact::NONE;
                    board.artifact_owner = -1;
                    p.round_detail += "万念璃花已用尽三次，就此破碎消散\n";
                    round_report_ += "🌸 " + No(i) + " 的万念璃花已然破碎\n";
                }
                break;
            default:
                break;
        }
    }
}

// ========== 回合末结算 ==========

void MainStage::SettleRoundEnd()
{
    const int n = PlayerNum();

    // 收魂炼灵：全场每有一人死亡便增长修为，不问远近
    for (int i = 0; i < n; i++) {
        Player& p = P(i);
        if (!p.Alive() || !HasShouhun(p.path)) {
            continue;
        }
        int deaths = 0;
        for (int j = 0; j < n; j++) {
            if (P(j).out == 1 && P(j).out_round == round_) {
                deaths++;
            }
        }
        if (deaths > 0) {
            const Num gain = CULT_SHOUHUN * deaths;
            p.cult_gain += gain;
            p.round_detail += "收魂炼灵：全场陨落 " + to_string(deaths) + " 人，你尽收其魂，修为 +" + NumStr(gain) + "\n";
        }
    }

    // 修行与修为结算
    for (int i = 0; i < n; i++) {
        Player& p = P(i);
        if (!p.Alive()) {
            continue;
        }
        const Action* actions[2] = {&p.action, &p.extra_action};
        for (const Action* a : actions) {
            if (a->Empty()) {
                continue;
            }
            if (a->act == Act::XIULIAN) {
                const Num limit = p.path == Path::XIE ? XIULIAN_FAIL_DMG_XIE : XIULIAN_FAIL_DMG;
                if (p.dmg_taken >= limit) {
                    p.round_detail += ActName(p.path, Act::XIULIAN) + "失败：本回合受到的伤害达到 " +
                                      NumStr(limit) + " 点，修为无从积攒\n";
                } else {
                    Num gain = p.path == Path::XIE ? CULT_XIULIAN_XIE : CULT_XIULIAN;
                    if (p.region == CENTER_REGION && p.path != Path::TI) {
                        gain += CULT_CENTER_BONUS;
                        p.round_detail += "于中央区域聚气，额外获得 " + NumStr(CULT_CENTER_BONUS) + " 修行值\n";
                    }
                    p.cult_gain += gain;
                    p.round_detail += ActName(p.path, Act::XIULIAN) + "：修为 +" + NumStr(gain) + "\n";
                }
            } else if (a->act == Act::MENGJI && p.dmg_taken > 0) {
                p.cult_gain += CULT_MENGJI;
                p.round_detail += "猛击见血：修为 +" + NumStr(CULT_MENGJI) + "\n";
            } else if (a->act == Act::CUITI) {
                Num gain = CULT_CUITI;
                if (p.dmg_taken >= CUITI_BONUS_DMG) {
                    gain += CULT_CUITI_BONUS;
                }
                p.cult_gain += gain;
                p.round_detail += "淬体：修为 +" + NumStr(gain) + "\n";
            }
        }
        if (p.cult_gain > 0) {
            const int before = p.realm;
            p.promoted = p.GainCultivation(p.cult_gain, board.cfg.cultivate_step);
            if (p.promoted > 0) {
                p.result = RoundResult::PROMOTED;
                p.round_detail += "境界晋升：" + RealmName(p.path, before) + " → " + p.RealmStr() + "\n";
                round_report_ += "⬆ " + No(i) + " 的境界攀升至【" + p.RealmStr() + "】\n";
                if (p.AtMaxRealm() && p.hedao_round < 0) {
                    p.hedao_round = round_;
                    // 回春：踏入合道的当回合，血肉重焕生机
                    if (HuichunRealm(p.path) >= 0) {
                        p.hp += HEAL_HUICHUN;
                        p.round_detail += "回春：初入" + p.RealmStr() + "，血肉重焕生机，回复 " + NumStr(HEAL_HUICHUN) + " 血\n";
                        round_report_ += "🌿 " + No(i) + " 初入合道，血肉重焕生机\n";
                    }
                }
            }
        }
    }

    // 虚行寿果的延迟反噬
    for (int i = 0; i < n; i++) {
        Player& p = P(i);
        if (!p.Alive() || p.xuxing_round != round_) {
            continue;
        }
        p.xuxing_round = -1;
        p.hp -= COST_XUXING_LATER;
        p.round_detail += "虚行寿果反噬：扣去 " + NumStr(COST_XUXING_LATER) + " 血\n";
        if (p.hp <= 0) {
            KillPlayer(i, -1, "被虚行寿果的反噬夺去了生机");
        }
    }

    // 天道摧毁
    if (round_ >= board.cfg.destroy_start_round) {
        const std::vector<int> list = board.DestroyableRegions();
        if (!list.empty()) {
            const int target = board.RandomPick(list);
            round_report_ += "⚡ 天道降罚，无人踏足的区域「" + std::string(region_cn[target]) + "」被彻底摧毁\n";
            ReportVanishedArrays(board.DestroyRegion(target), target);
        }
    }

    // 血阵转为下回合生效
    board.ActivatePendingArrays();

    // 仙器流程推进
    AdvanceArtifact();

    // 存活分
    for (int i = 0; i < n; i++) {
        if (P(i).Alive()) {
            P(i).score.survive_rounds++;
        }
    }
}

void MainStage::AdvanceArtifact()
{
    const int n = PlayerNum();
    if (board.artifact_state == ArtifactState::DESCENDED) {
        std::vector<int> grabbers;
        for (int i = 0; i < n; i++) {
            if (P(i).Alive() && P(i).region == CENTER_REGION &&
                    (P(i).action.act == Act::DUOTIAN || P(i).extra_action.act == Act::DUOTIAN)) {
                grabbers.push_back(i);
            }
        }
        board.artifact_state = ArtifactState::SETTLED;
        if (grabbers.empty()) {
            round_report_ += "✨ 无人夺天造化，破碎仙器自行消散于天地之间\n";
            board.falling_artifact = Artifact::NONE;
            return;
        }
        const int winner = board.RandomPick(grabbers);
        board.artifact_owner = winner;
        P(winner).artifact = board.falling_artifact;
        round_report_ += "✨ " + No(winner) + " " + P(winner).name + " 夺得仙器【" +
                         std::string(artifact_cn[static_cast<int>(board.falling_artifact)]) + "】！\n";
        Global().Tell(winner) << "你夺得了仙器【" << artifact_cn[static_cast<int>(board.falling_artifact)] << "】\n"
                              << artifact_desc[static_cast<int>(board.falling_artifact)];
        return;
    }
    if (board.artifact_state == ArtifactState::SETTLED) {
        return;
    }
    if (board.artifact_state == ArtifactState::NONE) {
        for (int i = 0; i < n; i++) {
            if (P(i).Alive() && P(i).CanUse(Act::DUOTIAN)) {
                board.artifact_state = ArtifactState::QUAKE;
                round_report_ += "🌩 已有修士掌握夺天造化功，天地大震，仙器随时可能自中央区域的天空陨落\n";
                return;
            }
        }
        return;
    }
    if (board.RandomInt(100) < board.cfg.artifact_chance) {
        board.falling_artifact = static_cast<Artifact>(1 + board.RandomInt(ARTIFACT_NUM));
        board.artifact_state = ArtifactState::DESCENDED;
        round_report_ += "✨ 天穹碎裂！下回合中央区域将有仙器降世：【" +
                         std::string(artifact_cn[static_cast<int>(board.falling_artifact)]) +
                         "】，唯有身处中央的化神及以上者可施展「夺天造化功」争夺\n";
    }
}

// ========== 阶段流转 ==========

void MainStage::FirstStageFsm(SubStageFsmSetter setter)
{
    board.cfg.init_hp = N(GAME_OPTION(血量));
    board.cfg.cultivate_step = NumFrom(GAME_OPTION(修为));
    board.cfg.banxian_threshold = GAME_OPTION(半仙人数);
    board.cfg.destroy_start_round = GAME_OPTION(摧毁回合);
    board.cfg.artifact_chance = GAME_OPTION(仙器概率);
    board.cfg.max_round = GAME_OPTION(回合数);

    board.players.reserve(PlayerNum());
    for (int i = 0; i < PlayerNum(); i++) {
        board.players.emplace_back(PlayerID(i), Global().PlayerName(i), Global().PlayerAvatar(i, 22),
                                   board.cfg.init_hp);
    }
    {
        auto sender = Global().Boardcast();
        sender << "【登仙路】\n本局共有 " << PlayerNum() << " 名修士被困于遗界之中";
        if (PlayerNum() >= board.cfg.banxian_threshold) {
            sender << "\n人数已达天数，选道完毕后，天道将随机择一人彻底改道";
        }
    }
    Global().Boardcast() << "📜 择道之前，不妨先观道途图鉴——发送「图鉴」即可一览法修、体修、邪修、半仙"
                            "四路的全部招式、解锁境界，以及全部 " << ARTIFACT_NUM << " 件仙器。";
    setter.Emplace<PathStage>(*this);
}

void MainStage::NextStageFsm(PathStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    if (PlayerNum() >= board.cfg.banxian_threshold) {
        std::vector<int> candidates;
        for (int i = 0; i < PlayerNum(); i++) {
            if (P(i).Alive()) {
                candidates.push_back(i);
            }
        }
        if (!candidates.empty()) {
            const int chosen = board.RandomPick(candidates);
            P(chosen).path = Path::BAN;
            P(chosen).realm = 0;
            P(chosen).cultivation = 0;
            Global().Tell(chosen) << "天道垂青，你的修行方向被彻底改变——你本是天上仙人，重伤跌入凡尘。\n"
                                     "自此，你的修行方向为【半仙】";
            Global().Boardcast() << "天道择一人彻底改道，其修行方向已然大变，然究竟是谁，无人知晓";
        }
    }
    for (int i = 0; i < PlayerNum(); i++) {
        if (P(i).Alive()) {
            TellPlayerIdentity(i);
        }
    }
    setter.Emplace<LandingStage>(*this);
}

void MainStage::NextStageFsm(LandingStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    board.UnfreezeRegion();
    for (int i = 0; i < PlayerNum(); i++) {
        if (P(i).region < 0) {
            P(i).region = CENTER_REGION;
        }
    }
    round_ = 1;
    BoardcastBoard("降临完毕");
    Global().Boardcast() << "众修士降临遗界，血量、境界与位置尽数公开。夺灵噬魂，就此开始";
    setter.Emplace<ActionStage>(*this, round_);
}

void MainStage::NextStageFsm(MoveStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    board.UnfreezeRegion();
    BoardcastBoard("第 " + to_string(round_) + " 回合 · 移动后");
    setter.Emplace<ActionStage>(*this, round_);
}

void MainStage::NextStageFsm(ActionStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    for (int i = 0; i < PlayerNum(); i++) {
        if (P(i).Alive() && P(i).action.act == Act::LIHUA && P(i).lihua_left > 0) {
            setter.Emplace<LihuaStage>(*this, i);
            return;
        }
    }
    FinishRound(setter);
}

void MainStage::NextStageFsm(LihuaStage& sub_stage, const CheckoutReason reason, SubStageFsmSetter setter)
{
    FinishRound(setter);
}

void MainStage::FinishRound(SubStageFsmSetter& setter)
{
    round_report_.clear();
    SettleActions(false);
    bool has_extra = false;
    for (int i = 0; i < PlayerNum(); i++) {
        if (P(i).Alive() && !P(i).extra_action.Empty()) {
            has_extra = true;
        }
    }
    if (has_extra) {
        round_report_ += "🌸 万念璃花的追加行动于全场最后生效\n";
        SettleActions(true);
    }
    SettleRoundEnd();

    // 仍有修士存活时才发出淘汰通知；全员同归于尽则不发
    if (board.AliveCount() > 0) {
        for (const int pid : pending_eliminate_) {
            Global().Eliminate(pid);
        }
    }
    pending_eliminate_.clear();

    BoardcastBoard("第 " + to_string(round_) + " 回合 · 结算");
    if (!round_report_.empty()) {
        Global().Boardcast() << "【第 " << round_ << " 回合战报】\n" << round_report_;
    } else {
        Global().Boardcast() << "【第 " << round_ << " 回合】风平浪静，无事发生";
    }
    for (int i = 0; i < PlayerNum(); i++) {
        Player& p = P(i);
        if (p.round_detail.empty()) {
            continue;
        }
        Global().Tell(i) << "【第 " << round_ << " 回合结算】\n" << p.round_detail
                         << "\n当前血量：" << NumStr(p.hp) << "　境界：" << p.RealmStr()
                         << "　修为：" << (p.AtMaxRealm() ? std::string("已至顶点")
                                 : NumStr(p.cultivation) + " / " + NumStr(board.cfg.cultivate_step));
    }

    if (board.AliveCount() <= 1) {
        game_over_ = true;
        FinishGame();
        return;
    }
    // 只余两人时进入决战倒计时，逾期未分胜负则以残存生机论高下
    if (board.AliveCount() == 2 && duel_deadline_ < 0) {
        duel_deadline_ = round_ + FINAL_DUEL_ROUNDS;
        Global().Boardcast() << "⚔ 遗界之中只余两名修士对峙。此后 " << FINAL_DUEL_ROUNDS
                             << " 回合内若仍分不出胜负，天道自会裁断——以残存的生机论高下";
    }
    if (duel_deadline_ > 0 && round_ >= duel_deadline_) {
        game_over_ = true;
        Global().Boardcast() << "⚔ 决战之期已至，两人皆无力斩落对方。天道裁断，以残存的生机论高下";
        FinishGame();
        return;
    }
    // 回合上限届满，无论余下几人，一律以残存生机论高下
    if (round_ >= board.cfg.max_round) {
        game_over_ = true;
        Global().Boardcast() << "⏳ 遗界天数已尽，第 " << board.cfg.max_round
                             << " 回合终了而众修仍在。天道裁断，以残存的生机论高下";
        FinishGame();
        return;
    }
    round_++;
    if (board.OnlyCenterLeft()) {
        for (int i = 0; i < PlayerNum(); i++) {
            Player& p = P(i);
            if (p.Alive() && p.region != CENTER_REGION) {
                p.region = CENTER_REGION;
            }
            p.must_leave = false;
        }
        if (!skip_move_notified_) {
            skip_move_notified_ = true;
            Global().Boardcast() << "八方遗界尽数湮灭，唯余中央一域立于虚空。众修无路可退，此后的移动阶段尽数略过";
        }
        setter.Emplace<ActionStage>(*this, round_);
        return;
    }
    setter.Emplace<MoveStage>(*this, round_);
}

void MainStage::FinishGame()
{
    const int n = PlayerNum();
    // 名次：存活者最优，其余按陨落回合与陨落时血量排序
    std::vector<int> order(n);
    for (int i = 0; i < n; i++) {
        order[i] = i;
    }
    const auto key = [this](const int i) {
        return std::make_pair(P(i).Alive() ? 1000000 : P(i).out_round, P(i).hp);
    };
    std::sort(order.begin(), order.end(), [&](const int a, const int b) { return key(a) > key(b); });
    // 名次逐档递增：并列者同名次
    int rank = 1;
    for (int i = 0; i < n; i++) {
        if (i > 0 && key(order[i]) != key(order[i - 1])) {
            rank++;
        }
        P(order[i]).score.rank = rank;
    }
    for (int i = 0; i < n; i++) {
        player_scores_[i] = P(i).score.FinalScore();
    }

    // 半仙本是天上仙人，其胜出乃是重归仙班，另有一番说法
    bool banxian_won = false;
    for (int i = 0; i < n; i++) {
        if (P(i).score.rank == 1 && P(i).path == Path::BAN) {
            banxian_won = true;
        }
    }

    auto sender = Global().Boardcast();
    if (banxian_won) {
        sender << (board.AliveCount() == 0
                       ? "遗界之中再无修士，然谪仙终究不曾真死。旧躯散尽，仙骨归位，重登天阙者："
                       : "遗界破除！跌落凡尘的谪仙拾回了旧日仙骨，伤愈还朝，重登天阙者：");
    } else if (board.AliveCount() == 0) {
        sender << "遗界之中再无活口，众修士同归于尽。以陨落时残存的生机论高下，胜出者：";
    } else {
        sender << "遗界破除！无视修为，白日飞升，羽化登仙者：";
    }
    for (int i = 0; i < n; i++) {
        if (P(i).score.rank == 1) {
            sender << At(PlayerID(i)) << " ";
        }
    }
    if (banxian_won) {
        sender << "\n云海之上仙班列队相迎，你拂去一身凡尘，朝着你从小敬仰的纸团大帝的方向进发";
    }
    BoardcastBoard("终局");
    Global().Boardcast() << Markdown(board.ScoreHtml(), ImageWidth());
}

auto* MakeMainStage(MainStageFactory factory) { return factory.Create<MainStage>(); }

} // namespace GAME_MODULE_NAME

} // namespace game

} // namespace lgtbot
