// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).

#include "game_framework/unittest_base.h"

namespace lgtbot {

namespace game {

namespace GAME_MODULE_NAME {

// 选择修行方向 + 降临，进入第 1 回合的行动阶段
#define REACH_ACTION_STAGE(n)                       \
    do {                                            \
        START_GAME();                               \
        for (uint64_t i = 0; i < (n); ++i) {        \
            ASSERT_PRI_MSG(i + 1 == (n) ? CHECKOUT : OK, i, "法修"); \
        }                                           \
        for (uint64_t i = 0; i < (n); ++i) {        \
            ASSERT_PRI_MSG(i + 1 == (n) ? CHECKOUT : OK, i, "降临 5"); \
        }                                           \
    } while (0)

GAME_TEST(5, player_num_too_few)
{
    ASSERT_FALSE(StartGame());
}

GAME_TEST(6, choose_path_must_be_private)
{
    START_GAME();
    ASSERT_PUB_MSG(FAILED, 0, "法修");
    ASSERT_PRI_MSG(OK, 0, "法修");
}

GAME_TEST(6, cannot_choose_path_twice)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "体修");
    ASSERT_PRI_MSG(FAILED, 0, "邪修");
}

GAME_TEST(6, cannot_choose_banxian_directly)
{
    START_GAME();
    ASSERT_PRI_MSG(NOT_FOUND, 0, "半仙");
}

GAME_TEST(6, path_timeout_is_assigned)
{
    START_GAME();
    ASSERT_PRI_MSG(OK, 0, "法修");
    ASSERT_TIMEOUT(CHECKOUT);
    // 选道结束后进入降临阶段，超时的 1~5 号已挂机，故 0 号一落地便直接结算
    ASSERT_PRI_MSG(CHECKOUT, 0, "降临 1");
}

GAME_TEST(6, landing_and_action_flow)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(OK, 0, "聚气修行");
    ASSERT_PRI_MSG(OK, 1, "金光护身");
    ASSERT_PRI_MSG(OK, 2, "火球术 1");
    ASSERT_PRI_MSG(OK, 3, "漫天星火");
    ASSERT_PRI_MSG(OK, 4, "聚气修行");
    ASSERT_PRI_MSG(CHECKOUT, 5, "聚气修行");
    // 结算完成后进入移动阶段
    ASSERT_PRI_MSG(OK, 0, "停留");
}

GAME_TEST(6, action_realm_limit)
{
    REACH_ACTION_STAGE(6);
    // 法修炼气期无法使用筑基及以上的招式
    ASSERT_PRI_MSG(FAILED, 0, "吞天噬地");
    ASSERT_PRI_MSG(FAILED, 0, "不动如山");
    ASSERT_PRI_MSG(FAILED, 0, "万物皆虚 7");
    // 也无法使用其他修行方向的招式
    ASSERT_PRI_MSG(FAILED, 0, "猛击 2");
    ASSERT_PRI_MSG(FAILED, 0, "灭魂 2");
}

GAME_TEST(6, action_target_check)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(FAILED, 0, "火球术 1");     // 目标为自己
    ASSERT_PRI_MSG(NOT_FOUND, 0, "火球术 7");  // 超出玩家编号范围，指令不匹配
    ASSERT_PRI_MSG(OK, 0, "火球术 2");
    ASSERT_PRI_MSG(FAILED, 0, "火球术 3");     // 已行动过
}

// 招式代号快捷行动：目标可省略，代号大小写不限
GAME_TEST(6, act_by_code)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(FAILED, 0, "HQ");           // 火球术须指明目标
    ASSERT_PRI_MSG(FAILED, 0, "HQ 1");         // 目标为自己
    ASSERT_PRI_MSG(FAILED, 0, "JG 3");         // 金光护身无需目标
    ASSERT_PRI_MSG(FAILED, 0, "MJ 2");         // 法修并无猛击
    ASSERT_PRI_MSG(FAILED, 0, "WW 7");         // 境界不足
    ASSERT_PRI_MSG(NOT_FOUND, 0, "ZZ");        // 并无此代号
    ASSERT_PRI_MSG(OK, 0, "hq 2");             // 小写代号同样可用
    ASSERT_PRI_MSG(OK, 1, "JG");
    ASSERT_PRI_MSG(OK, 2, "XL");
    ASSERT_PRI_MSG(OK, 3, "MT");
    ASSERT_PRI_MSG(OK, 4, "聚气修行");         // 原有的招式名方式仍然可用
    ASSERT_PRI_MSG(CHECKOUT, 5, "JQ");         // 修行的别名代号
    ASSERT_PRI_MSG(OK, 0, "停留");
}

// 尚未合道时，身份牌中的不死不灭应标注为未生效
GAME_TEST(6, ti_passive_locked)
{
    START_GAME();
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "体修");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "降临 5");
    }
    ASSERT_PRI_MSG(OK, 0, "状态");
}

// 体修新增的嘶吼（筑基）与以痛止戈（合道）
GAME_TEST(6, ti_new_skills)
{
    START_GAME();
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "体修");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "降临 5");
    }
    ASSERT_PRI_MSG(FAILED, 0, "SH");           // 炼气期尚无嘶吼
    ASSERT_PRI_MSG(FAILED, 0, "YT 2");         // 炼气期尚无以痛止戈
    // 全员安心修行五回合，体修每回合 1 点修为，五回合正好抵达合道
    for (int round = 1; round <= 5; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "捶打修行");
        }
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
        }
    }
    ASSERT_PRI_MSG(OK, 0, "嘶吼");             // 筑基即解锁
    ASSERT_PRI_MSG(OK, 1, "YT 3");             // 合道方可施展
    ASSERT_PRI_MSG(OK, 1, "状态");             // 技能表中应列全两式新招
}

// 邪修聚气修行提速：中央区域每回合 0.5 + 0.5 = 1 点修为，一回合即可晋升
GAME_TEST(6, xie_cultivate_speed)
{
    START_GAME();
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "邪修");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "降临 5");
    }
    ASSERT_PRI_MSG(FAILED, 0, "QJ");           // 炼气期尚无窃机
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
    }
    ASSERT_PRI_MSG(OK, 0, "QJ");               // 一回合即达筑基
    ASSERT_PRI_MSG(OK, 0, "状态");             // 身份牌中应列出收魂炼灵
}

GAME_TEST(6, action_must_be_private)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PUB_MSG(FAILED, 0, "聚气修行");
    ASSERT_PRI_MSG(OK, 0, "聚气修行");
}

GAME_TEST(6, move_check)
{
    REACH_ACTION_STAGE(6);
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    // 全员位于中央区域，可前往任意区域
    ASSERT_PRI_MSG(OK, 0, "前往 1");
    ASSERT_PRI_MSG(OK, 1, "停留");
    ASSERT_PRI_MSG(FAILED, 0, "前往 2");       // 已选择过去向
}

GAME_TEST(6, move_adjacent_only)
{
    START_GAME();
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "法修");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "降临 1");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    // 区域一与区域三、六、八、九均不相邻
    ASSERT_PRI_MSG(FAILED, 0, "前往 3");
    ASSERT_PRI_MSG(FAILED, 0, "前往 9");
    ASSERT_PRI_MSG(OK, 0, "前往 5");
}

GAME_TEST(6, move_accepts_chinese_numeral)
{
    REACH_ACTION_STAGE(6);
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    ASSERT_PRI_MSG(OK, 0, "前往 三");
    ASSERT_PRI_MSG(OK, 1, "七");
    ASSERT_PRI_MSG(OK, 2, "前往 5");
    ASSERT_PRI_MSG(NOT_FOUND, 3, "前往 十");
    ASSERT_PRI_MSG(OK, 3, "停留");
}

// 降临与万物皆虚的区域参数同样接受中文数字
GAME_TEST(6, landing_accepts_chinese_numeral)
{
    START_GAME();
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "法修");
    }
    ASSERT_PRI_MSG(OK, 0, "降临 五");
    ASSERT_PRI_MSG(OK, 1, "九");
    ASSERT_PRI_MSG(OK, 2, "降临 5");
    ASSERT_PRI_MSG(NOT_FOUND, 3, "降临 零");
    for (uint64_t i = 3; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "降临 五");
    }
    ASSERT_PRI_MSG(OK, 0, "聚气修行");
}

// 移动阶段进行中，赛况应只呈现阶段开始时的位置快照，不得泄露他人去向
GAME_TEST(6, status_during_move_is_frozen)
{
    REACH_ACTION_STAGE(6);
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    ASSERT_PRI_MSG(OK, 0, "前往 1");
    ASSERT_PRI_MSG(OK, 1, "前往 9");
    // 已有两人定好去向，此时的赛况仍应显示全员位于中央区域「五」
    ASSERT_PUB_MSG(OK, 2, "赛况");
    for (uint64_t i = 2; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
    }
    // 阶段结束后解冻，赛况呈现真实位置
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

// 八方区域尽数湮灭后，移动阶段应被跳过
GAME_TEST(6, skip_move_when_only_center_left)
{
    REACH_ACTION_STAGE(6);
    for (int round = 1; round <= 11; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
        }
        if (round <= 10) {
            for (uint64_t i = 0; i < 6; ++i) {
                ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
            }
        }
    }
    ASSERT_PRI_MSG(NOT_FOUND, 0, "停留");
    // 天地只余一域，虚神指指向何方都是同一批人，方向可以省略
    ASSERT_PRI_MSG(OK, 0, "XS");
    ASSERT_PRI_MSG(OK, 1, "虚神指");
}

GAME_TEST(6, promote_by_cultivating)
{
    REACH_ACTION_STAGE(6);
    // 全员在中央修行，法修每回合 1 + 0.5 修为，第 1 回合即可晋升至筑基
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    // 0 号独自前往区域一，其余留在中央
    ASSERT_PRI_MSG(OK, 0, "前往 1");
    for (uint64_t i = 1; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
    }
    ASSERT_PRI_MSG(FAILED, 0, "不动如山");     // 金丹能力仍未解锁
    ASSERT_PRI_MSG(OK, 0, "吞天噬地");         // 已至筑基，且区域一无他人
}

GAME_TEST(6, locked_actions_report_failure)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(FAILED, 0, "夺天造化功");   // 境界不足
    ASSERT_PRI_MSG(FAILED, 0, "仙之威 2");             // 法修并无此招
    ASSERT_PRI_MSG(FAILED, 0, "万念璃花");         // 未持有仙器
}

GAME_TEST(6, leave_at_action_stage)
{
    REACH_ACTION_STAGE(6);
    ASSERT_LEAVE(CONTINUE, 0);
    // 已离场的玩家不再占用行动名额，其余五人行动完即可结算
    for (uint64_t i = 1; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
}

GAME_TEST(6, timeout_defaults_to_cultivating)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(OK, 0, "金光护身");
    ASSERT_TIMEOUT(CHECKOUT);
    // 超时的 1~5 号已挂机，移动阶段不再等待他们
    ASSERT_PRI_MSG(CHECKOUT, 0, "停留");
}

// 超时者进入挂机，此后裁判不再等待；本人再发任意指令即恢复
GAME_TEST(6, timeout_hooks_unready_players)
{
    REACH_ACTION_STAGE(6);
    ASSERT_TIMEOUT(CHECKOUT);                  // 全员未出手，尽数挂机
    ASSERT_PRI_MSG(CHECKOUT, 0, "停留");       // 0 号解除挂机，余者仍挂机，一人行动即结算
    ASSERT_PRI_MSG(CHECKOUT, 0, "聚气修行");   // 下一回合同样只等 0 号
    ASSERT_PRI_MSG(CHECKOUT, 0, "停留");
}

// 挂机者恢复后重新纳入等待，未行动者仍会被等待
GAME_TEST(6, hooked_player_resumes)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(OK, 0, "聚气修行");
    ASSERT_TIMEOUT(CHECKOUT);                  // 1~5 号挂机，0 号未挂机
    ASSERT_PRI_MSG(OK, 1, "停留");             // 1 号恢复，仍需等待同样活跃的 0 号
    ASSERT_PRI_MSG(CHECKOUT, 0, "停留");
}

// 同归于尽时，湮灭先将血量归零，再承受本回合的伤害，负数血量方能分出名次
GAME_TEST(6, void_death_ranks_by_remaining_hp)
{
    ASSERT_PUB_MSG(OK, 0, "摧毁回合 99");
    REACH_ACTION_STAGE(6);
    // 五回合于中央聚气，同抵合道；第五回合末一并移往区域一
    for (int round = 1; round <= 5; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
        }
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, round == 5 ? "前往 1" : "停留");
        }
    }
    // 彼此消耗两回合，令各人残血互不相同
    for (int round = 1; round <= 2; ++round) {
        ASSERT_PRI_MSG(OK, 0, "聚气修行");
        ASSERT_PRI_MSG(OK, 1, "火球术 3");
        ASSERT_PRI_MSG(OK, 2, "漫天星火");
        ASSERT_PRI_MSG(OK, 3, "火球术 5");
        ASSERT_PRI_MSG(OK, 4, "火球术 6");
        ASSERT_PRI_MSG(CHECKOUT, 5, "火球术 5");
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
        }
    }
    // 0 号连同自己一并湮灭区域一；同回合仍有人出手，归零之后各自再吃一轮伤害
    ASSERT_PRI_MSG(OK, 0, "万物皆虚 1");
    ASSERT_PRI_MSG(OK, 1, "火球术 3");
    ASSERT_PRI_MSG(OK, 2, "漫天星火");
    ASSERT_PRI_MSG(OK, 3, "火球术 5");
    ASSERT_PRI_MSG(OK, 4, "聚气修行");
    ASSERT_PRI_MSG(CHECKOUT, 5, "聚气修行");
    ASSERT_FINISHED(true);
    // 归零后各自再吃 1/1/2/1/3/1 点伤害，血量遂为 -1/-1/-2/-1/-3/-1，
    // 名次 1/1/2/1/3/1，得 60/60/30/60/20/60 名次分，另加存活七回合的 14 分
    ASSERT_SCORE(74, 74, 44, 74, 34, 74);
}

// 万物皆虚：法修合道可施展，半仙一脉则无此法门
GAME_TEST(6, wanwu_not_for_banxian)
{
    ASSERT_PUB_MSG(OK, 0, "半仙人数 6");
    ASSERT_PUB_MSG(OK, 0, "摧毁回合 99");
    REACH_ACTION_STAGE(6);
    // 六人局已足以引出半仙，天道会随机改道一人
    for (int round = 1; round <= 5; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
        }
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
        }
    }
    // 五回合后法修已至合道，可湮灭区域；被改道的半仙则会因无此法门而被拒
    int allowed = 0;
    for (uint64_t i = 0; i < 6; ++i) {
        allowed += (PrivateRequest(i, "WW 一") != ::StageErrCode::FAILED);
    }
    ASSERT_EQ(5, allowed);      // 六人之中恰有一人被改道为半仙
}

// 伪神目：化神方可施展，私信呈上全场道途名录
GAME_TEST(6, weishen_roster_image)
{
    ASSERT_PUB_MSG(OK, 0, "摧毁回合 99");
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(FAILED, 0, "WS");            // 炼气期尚无伪神目
    // 法修中央聚气每回合 1.5 修为，三回合抵达化神
    for (int round = 1; round <= 3; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
        }
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
        }
    }
    ASSERT_PRI_MSG(OK, 0, "伪神目");
    for (uint64_t i = 1; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
    }
}

// 墨杀仙剑：可指名一人，也可不指名而横扫全场；未持剑者无从施展
GAME_TEST(6, mosha_needs_relic)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(FAILED, 0, "墨杀");        // 剑不在手
    ASSERT_PRI_MSG(FAILED, 0, "墨杀 2");
    ASSERT_PRI_MSG(FAILED, 0, "QM");
    ASSERT_PRI_MSG(OK, 0, "聚气修行");
}

// 图鉴为全局指令，公开私聊皆可查阅，且不占用行动
GAME_TEST(6, atlas_command)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PUB_MSG(OK, 0, "图鉴");
    ASSERT_PRI_MSG(OK, 0, "图鉴");
    ASSERT_PRI_MSG(OK, 0, "聚气修行");
}

GAME_TEST(6, status_command)
{
    REACH_ACTION_STAGE(6);
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 0, "状态");
    ASSERT_PUB_MSG(FAILED, 0, "状态");
}

// 境界攀升后的状态图应列出全部已解锁招式
GAME_TEST(6, status_image_high_realm)
{
    REACH_ACTION_STAGE(6);
    for (int round = 1; round <= 6; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
        }
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
        }
    }
    // 尚有多个区域存世时，虚神指必须指明方向
    ASSERT_PRI_MSG(FAILED, 0, "XS");
    ASSERT_PRI_MSG(FAILED, 0, "虚神指");
    ASSERT_PRI_MSG(OK, 0, "XS 右下");
    ASSERT_PRI_MSG(OK, 0, "状态");
}

// 尚未降临时，名录中的位置栏不得越界取区域名
GAME_TEST(6, status_before_landing)
{
    START_GAME();
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 0, "状态");
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "法修");
    }
    // 降临阶段：仍未有人定位
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 0, "状态");
    ASSERT_PRI_MSG(OK, 0, "降临 3");
    // 部分玩家已降临、部分未降临
    ASSERT_PUB_MSG(OK, 1, "赛况");
    for (uint64_t i = 1; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "降临 5");
    }
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

GAME_TEST(9, banxian_appears_and_game_runs)
{
    REACH_ACTION_STAGE(9);
    for (uint64_t i = 0; i < 9; ++i) {
        ASSERT_PRI_MSG(i + 1 == 9 ? CHECKOUT : OK, i, "聚气修行");
    }
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

// ========== 配置项 ==========

// 修为门槛调高后，原本一回合即可达到的境界需要多修一回合
GAME_TEST(6, option_cultivate_step)
{
    ASSERT_PUB_MSG(OK, 0, "修为 2");
    START_GAME();
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "邪修");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "降临 5");
    }
    // 中央聚气每回合得 1 点修为，默认一回合便可筑基，此处需修满两回合
    for (int round = 1; round <= 2; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
        }
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
        }
        ASSERT_PRI_MSG(round == 1 ? FAILED : OK, 0, "QJ");
    }
}

// 修为门槛可带小数：0.5 时，法修中央聚气一回合的 1.5 修为足以连升三境
GAME_TEST(6, option_cultivate_step_fraction)
{
    ASSERT_PUB_MSG(OK, 0, "修为 0.5");
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(FAILED, 0, "HR");           // 炼气期尚无浑然一体
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
    }
    ASSERT_PRI_MSG(OK, 0, "HR");               // 一回合直入元婴
}

// 血量调低后，赛况与状态图中的血条比例不应越界
GAME_TEST(6, option_low_hp)
{
    ASSERT_PUB_MSG(OK, 0, "血量 5");
    REACH_ACTION_STAGE(6);
    ASSERT_PUB_MSG(OK, 0, "赛况");
    ASSERT_PRI_MSG(OK, 0, "状态");
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

// 半仙人数设为上限之上时，九人局也不再有人被改道
GAME_TEST(9, option_banxian_never)
{
    ASSERT_PUB_MSG(OK, 0, "半仙人数 19");
    ASSERT_PUB_MSG(OK, 0, "摧毁回合 99");
    REACH_ACTION_STAGE(9);
    for (int round = 1; round <= 5; ++round) {
        for (uint64_t i = 0; i < 9; ++i) {
            ASSERT_PRI_MSG(i + 1 == 9 ? CHECKOUT : OK, i, "聚气修行");
        }
        for (uint64_t i = 0; i < 9; ++i) {
            ASSERT_PRI_MSG(i + 1 == 9 ? CHECKOUT : OK, i, "停留");
        }
    }
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

// 回合上限届满，众修仍在也照样收场，以残存血量论高下
GAME_TEST(6, option_max_round)
{
    ASSERT_PUB_MSG(OK, 0, "回合数 5");
    ASSERT_PUB_MSG(OK, 0, "摧毁回合 99");
    REACH_ACTION_STAGE(6);
    for (int round = 1; round <= 5; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
        }
        if (round == 5) {
            break;      // 第 5 回合行动结算完毕即达上限，游戏就此终局
        }
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
        }
    }
    ASSERT_FINISHED(true);
    // 全员一路修行未曾交手，血量相同，故并列第一
    // 存活 5 回合 +10，并列第一各得 60 名次分
    ASSERT_SCORE(70, 70, 70, 70, 70, 70);
}

// 区域被湮灭时，其上的血祭大阵随之消散，不会再行发动
GAME_TEST(6, array_vanishes_with_region)
{
    ASSERT_PUB_MSG(OK, 0, "摧毁回合 99");
    START_GAME();
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "邪修");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "降临 5");
    }
    // 中央聚气每回合 1 点修为，五回合抵达合道，万物皆虚方才可用
    for (int round = 1; round <= 5; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
        }
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
        }
    }
    // 0 号独自前往区域一
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    ASSERT_PRI_MSG(OK, 0, "前往 一");
    for (uint64_t i = 1; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
    }
    // 在区域一布下血阵后抽身返回中央
    ASSERT_PRI_MSG(OK, 0, "祭灭血阵");
    for (uint64_t i = 1; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    ASSERT_PRI_MSG(OK, 0, "前往 五");
    for (uint64_t i = 1; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
    }
    // 1、2 号同指区域一，此地只湮灭一次，血阵亦随之失了依托
    ASSERT_PRI_MSG(OK, 1, "万物皆虚 一");
    ASSERT_PRI_MSG(OK, 2, "万物皆虚 一");
    ASSERT_PRI_MSG(OK, 0, "聚气修行");
    for (uint64_t i = 3; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
    }
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

// 仙器三态：天地大震 → 降临中央 → 为人所得，顶部提示逐段更替
GAME_TEST(6, artifact_flow)
{
    ASSERT_PUB_MSG(OK, 0, "仙器概率 100");
    ASSERT_PUB_MSG(OK, 0, "摧毁回合 99");
    REACH_ACTION_STAGE(6);
    ASSERT_PRI_MSG(FAILED, 0, "DT");           // 炼气期尚无夺天造化功
    for (int round = 1; round <= 4; ++round) {
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
        }
        for (uint64_t i = 0; i < 6; ++i) {
            ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
        }
    }
    // 四回合后已至化神，仙器亦已降临中央，可出手争夺
    ASSERT_PRI_MSG(OK, 0, "DT");
    for (uint64_t i = 1; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "聚气修行");
    }
    for (uint64_t i = 0; i < 6; ++i) {
        ASSERT_PRI_MSG(i + 1 == 6 ? CHECKOUT : OK, i, "停留");
    }
    ASSERT_PUB_MSG(OK, 0, "赛况");
}

// 配置项的取值边界
GAME_TEST(6, option_range)
{
    ASSERT_PUB_MSG(FAILED, 0, "血量 4");
    ASSERT_PUB_MSG(OK, 0, "血量 100");
    ASSERT_PUB_MSG(OK, 0, "修为 0.5");
    ASSERT_PUB_MSG(OK, 0, "修为 2.25");
    ASSERT_PUB_MSG(FAILED, 0, "修为 0");
    ASSERT_PUB_MSG(FAILED, 0, "修为 10.5");
    ASSERT_PUB_MSG(FAILED, 0, "半仙人数 5");
    // 上限跟随人数上限，取值写成表达式以免日后改动人数上限时失准
    ASSERT_PUB_MSG(OK, 0, ("半仙人数 " + std::to_string(MAX_PLAYER + 1)).c_str());
    ASSERT_PUB_MSG(FAILED, 0, ("半仙人数 " + std::to_string(MAX_PLAYER + 2)).c_str());
    ASSERT_PUB_MSG(FAILED, 0, "摧毁回合 0");
    ASSERT_PUB_MSG(OK, 0, "仙器概率 0");
    ASSERT_PUB_MSG(FAILED, 0, "仙器概率 101");
    START_GAME();
}

} // namespace GAME_MODULE_NAME

} // namespace game

} // gamespace lgtbot

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    return RUN_ALL_TESTS();
}
