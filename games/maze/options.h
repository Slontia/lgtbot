EXTEND_OPTION("配置迷宫的边长", 边长, (ArithChecker<uint32_t>(5, 9, "边长")), 5)
EXTEND_OPTION("每位玩家可绘制的墙壁数量上限，实际绘制可以少于该值", 墙数, (ArithChecker<uint32_t>(5, 64, "墙壁数量")), 16)
EXTEND_OPTION("绘制迷宫阶段的时间限制", 绘制时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 360)
EXTEND_OPTION("对战阶段每一步移动的时间限制", 行动时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 90)
