#include "constants.h"

EXTEND_OPTION("黑白：公布每个格子周边通路数的奇偶，黑为奇数、白为偶数<br>"
              "边走边画：自己回合可私信往自己的迷宫里继续加墙，与黑白互斥", 模式,
    (AlterChecker(std::map<std::string, enum GameMode>{
        {"普通", GameMode::NORMAL}, {"黑白", GameMode::BLACK_WHITE}, {"边走边画", GameMode::DRAW_WHILE_WALK},
    })), GameMode::NORMAL)
EXTEND_OPTION("迷雾：3 种黑白信息的可见范围，仅在黑白模式下生效", 迷雾,
    (AlterChecker(std::map<std::string, enum FogMode>{
        {"无", FogMode::NONE}, {"当前", FogMode::CURRENT}, {"四周", FogMode::AROUND},
    })), FogMode::NONE)
EXTEND_OPTION("配置迷宫的边长", 边长, (ArithChecker<uint32_t>(5, 9, "边长")), 5)
EXTEND_OPTION("每位玩家可绘制的墙壁数量上限，实际绘制可以少于该值", 墙数, (ArithChecker<uint32_t>(5, 64, "墙壁数量")), 16)
EXTEND_OPTION("绘制迷宫阶段的时间限制", 绘制时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 360)
EXTEND_OPTION("对战阶段每一步移动的时间限制", 行动时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 90)
