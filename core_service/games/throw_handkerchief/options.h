EXTEND_OPTION("每回合时间限制", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 90)
EXTEND_OPTION("初始生命时间，先手方少15s", 生命, (ArithChecker<uint32_t>(60, 600, "生命时间（秒）")), 180)
EXTEND_OPTION("选择游戏模式 [生命时间被修改时，此配置项不生效]", 模式, AlterChecker<int>({{"快速", 0}, {"高级", 1}, {"实时", 2}}), 0)
EXTEND_OPTION("是否开启完美回头规则", 完美, (BoolChecker("开启", "关闭")), true)
