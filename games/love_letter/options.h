EXTEND_OPTION("每回合行动时间限制（秒）", 时限, (ArithChecker<uint32_t>(10, 3600, "超时时间（秒）")), 90)
EXTEND_OPTION("目标分数（0=按人数自动：2人7❤/3人5❤/4+人4❤）", 爱心, (ArithChecker<uint32_t>(0, 10, "爱心数")), 0)
EXTEND_OPTION("牌堆版本（默认=自动根据人数，2-4标准、5-8豪华）", 牌堆, (AlterChecker<uint32_t>({{"默认", 0}, {"标准", 1}, {"豪华", 2}})), 0)
