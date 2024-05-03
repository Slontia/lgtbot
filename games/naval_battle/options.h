EXTEND_OPTION("放置阶段时间限制（总时间限制）", 放置时限, (ArithChecker<uint32_t>(60, 3600, "超时时间（秒）")), 300)
EXTEND_OPTION("进攻阶段时间限制（进攻操作每一步的时间限制）", 进攻时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 120)
EXTEND_OPTION("设置地图大小", 边长, (ArithChecker<uint32_t>(8, 15, "边长")), 10)
EXTEND_OPTION("设置飞机数量", 飞机, (ArithChecker<uint32_t>(1, 8, "数量")), 3)
EXTEND_OPTION("是否允许飞机互相重叠", 重叠, (BoolChecker("允许", "不允许")), false)
EXTEND_OPTION("设置命中要害是否有提示", 要害, AlterChecker<int>({{"有", 0}, {"无", 1}, {"首次", 2}}), 0)
EXTEND_OPTION("连发次数", 连发, (ArithChecker<uint32_t>(1, 10, "次数")), 3)
EXTEND_OPTION("初始随机侦察区域大小（默认为随机）", 侦察, (ArithChecker<uint32_t>(0, 30, "面积")), 100)
EXTEND_OPTION("【BOSS挑战配置】</br>"
              "设置时须按照下方的顺序，不能跳过其中的某一项，未设置则为默认</br>"
              "[第1项] 挑战的BOSS类型：仅支持 1-2，输入其他为随机</br>"
              "[第2项] 重叠：0 为不允许，大于0 均为允许</br>"
              "[第3项] 要害：0 为有要害提示，1 为无要害提示，大于1 均为首要害提示</br>"
              "[第4项] 连发次数：范围1-10，其他输入均为默认</br>"
              "[第5项] 侦察区域大小：范围0-30，其他输入均为默认",
              BOSS挑战, (RepeatableChecker<ArithChecker<int32_t>>(0, 100, "配置")), (std::vector<int32_t>{0}))
