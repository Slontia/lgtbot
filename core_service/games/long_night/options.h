#include "constants.h"

EXTEND_OPTION("[区块] 先根据模式从区块池抽取 12+4 组成随机池，再抽取地图区块<br>"
    "【经典】仅从**经典区块**中抽取区块　　　【幻变】将从**轮换区块**中随机抽取<br>"
    "【狂野】将从**所有区块**中随机抽取　　　【疯狂】随机池包含2个**特殊区块**<br>"
    "【主题模式】从**主题区块**中抽取12+4组成随机池，区块池具有不同的风格", 模式,
(AlterChecker(std::map<std::string, enum BlockMode>{
    {"经典", BlockMode::CLASSIC},
    {"幻变", BlockMode::TWIST},
    {"狂野", BlockMode::WILD},
    {"疯狂", BlockMode::CRAZY},
    {"按钮", BlockMode::BUTTON},
    {"陷阱", BlockMode::TRAP},
})), BlockMode::TWIST)
EXTEND_OPTION("[区块] 自定义游戏区块随机池：自定义区块时「模式」配置不生效", 区块,
    (RepeatableChecker<BasicChecker<std::string>>("区块", "1 5 6 16 34 38 E1 e7 S4 s1")), (std::vector<std::string>{"默认"}))

EXTEND_OPTION("[常规] 设置游戏地图边长", 边长, (AlterChecker<int32_t>({{"9", 9}, {"10", 10}, {"12", 12}})), 9)
EXTEND_OPTION("[常规] 游戏最大回合数限制", 回合数, (ArithChecker<uint32_t>(3, 40, "回合数")), 20)
EXTEND_OPTION("[常规] 捕捉目标：设置游戏中玩家的捕捉顺序", 捕捉目标, 
    (AlterChecker(std::map<std::string, enum Target>{{"上家", Target::PREVIOUS}, {"下家", Target::NEXT}})), Target::PREVIOUS)
EXTEND_OPTION("[常规] 停止私信：玩家主动停止或超时，得知私信墙壁信息", 停止私信, (BoolChecker("开启", "关闭")), false)

EXTEND_OPTION("[事件] 设置游戏特殊事件", 特殊事件, (AlterChecker(std::map<std::string, enum SpecialEvent>{
    {"无", SpecialEvent::NONE},
    {"随机", SpecialEvent::RANDOM},
    {"怠惰的园丁", SpecialEvent::LAZYGARDENER},
    {"营养过剩", SpecialEvent::OVERGROWTH},
    {"雨天小故事", SpecialEvent::RAINSTORY},
})), SpecialEvent::NONE)

EXTEND_OPTION("[模式] BOSS：具体规则详见「#规则 漫漫长夜 BOSS」", BOSS, (AlterChecker(std::map<std::string, enum BossType>{
    {"无", BossType::NONE},
    {"米诺陶斯", BossType::MINOTAUR},
    {"邦邦", BossType::BANGBANG},
})), BossType::NONE)
EXTEND_OPTION("[模式] 点杀：捕捉改为仅在回合结束时触发，路过不会触发捕捉", 点杀, (BoolChecker("开启", "关闭")), true)
EXTEND_OPTION("[模式] 隐匿：隐匿后私聊行动，可选回合和单步模式", 隐匿,
    (AlterChecker(std::map<std::string, enum HideMode>{{"关闭", HideMode::NONE}, {"回合", HideMode::TURN}, {"单步", HideMode::STEP}})), HideMode::NONE)
EXTEND_OPTION("[模式] 大乱斗：逃生舱改为随机传送", 大乱斗, (BoolChecker("开启", "关闭")), false)
EXTEND_OPTION("[模式] 谋定后动：每回合仅能执行一次移动，可使用多步行动指令", 谋定后动, (BoolChecker("开启", "关闭")), false)
EXTEND_OPTION("[模式] 炸弹人：公开安置炸弹，任何人经过炸弹并离开会引爆炸弹并出局", 炸弹, (ArithChecker<uint32_t>(0, 3, "数量")), 0)

EXTEND_OPTION("[时限] 行动前思考的时间限制", 思考时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("[时限] 开始行动后的总时间限制", 行动时限, (ArithChecker<uint32_t>(60, 3600, "超时时间（秒）")), 300)

EXTEND_OPTION("[纹理] 设置游戏使用的图片素材&纹理", 纹理,
    (AlterChecker(std::map<std::string, enum Texture>{{"经典", Texture::CLASSIC}, {"复古", Texture::RETRO}})), Texture::CLASSIC)
