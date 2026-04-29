EXTEND_OPTION("每回合的时间限制", 时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 150)
EXTEND_OPTION("【配置游戏地图类型】<br>"
    "[小] 6\\*6面积 6宝藏 4炸弹，1生命，最多 4人<br>"
    "[中] 9\\*9面积 9宝藏 12炸弹，2生命，最多 6人<br>"
    "[大] 12\\*12面积 18宝藏 18炸弹，2生命，最多 9人<br>"
    "[特大] 15\\*15面积 36宝藏 24炸弹，2生命，最多 12人", 地图,
    AlterChecker<uint32_t>({{"小", 0}, {"中", 1}, {"大", 2}, {"特大", 3}}), 0)
EXTEND_OPTION("配置游戏的道具模式", 道具, AlterChecker<uint32_t>({{"经典", 0}, {"全部", 1}}), 0)
EXTEND_OPTION("自定义边长（默认根据地图类型变化）", 边长, (ArithChecker<int32_t>(4, 20, "边长")), -1)
EXTEND_OPTION("自定义生命值（默认根据地图类型变化）", 生命, (ArithChecker<int32_t>(1, 10, "生命")), -1)
EXTEND_OPTION("自定义宝藏 <font color='#00C8C8'>★</font> 数量（默认根据地图类型变化）", 宝藏, (ArithChecker<int32_t>(0, 200, "数量")), -1)
EXTEND_OPTION("自定义炸弹 ✹ 数量（默认根据地图类型变化）", 炸弹, (ArithChecker<int32_t>(0, 200, "数量")), -1)
EXTEND_OPTION("自定义加湿器 ♨ 数量（默认根据地图类型变化）", 加湿器, (ArithChecker<int32_t>(0, 200, "数量")), -1)
EXTEND_OPTION("自定义墨水瓶 ⚗ 数量（默认根据地图类型变化）", 墨水瓶, (ArithChecker<int32_t>(0, 200, "数量")), -1)
EXTEND_OPTION("自定义6种颜色的随机概率占比（如不足6项自动补零）<br>"
    "【配置顺序】R红、B蓝、Y黄、O橙、G绿、P紫", 配比,
    (RepeatableChecker<BasicChecker<int32_t>>("权重", "1 1 1 1 1 1")), (std::vector<int32_t>{1, 1, 1, 1, 1, 1}))