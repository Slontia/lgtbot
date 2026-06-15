# Match 重构总结

## 背景

原始 `Match` 类内部使用 `MatchLockedState` 大 struct 承载全部游戏状态（组队/对局中），通过 `std::shared_mutex` + 手动 lock/unlock 保护。游戏逻辑的 lobby 阶段和 running 阶段混在一起，Dispatch/Lock/Try 函数层层转发，callbacks_ 从 Lobby/Running 透传到 match_manager，维护困难。

## 目标

1. 将 lobby 和 running 拆分为独立类，各自持有与阶段相关的成员
2. 状态（state、users、players、game_child）归 Match 统一管理，用 `mutex_protect_wrapper` 保护
3. Lobby/Running 不再持有 callbacks_，bind/unbind 逻辑上提到 Match 层
4. 去掉手动 lock/unlock，全部通过 RAII guard 访问
5. 用 `std::get_if` / `std::visit` + `overloaded` 替代 state-based dispatch
6. `MatchBase` 从 bot_core 移出（Match 不再继承它，仅子进程 IpcMatchEnv 需要）

## 架构

```
Match
├── MatchData (mutex_protect_wrapper 保护)
│   ├── std::map<UserID, MatchParticipantUser> users
│   ├── std::vector<MatchPlayer> players
│   ├── std::unique_ptr<MatchChildClient> game_child
│   └── std::variant<Lobby, Running> phase
├── std::atomic<MatchState> state_   (仅用于状态转换，不用于 dispatch)
├── MatchContext ctx_
├── MatchMessaging messaging_
├── MatchHelpServices help_
└── ...
```

所有对 `MatchData` 的访问必须通过 `auto g = data_.lock()` 获取 RAII guard。

## 已完成的改动

### 1. 拆分 Lobby / Running（commit `cab2a84`）

- `MatchLockedState` 拆分为 `match_lobby.h/cc` 和 `match_running.h/cc`
- 引入 `MatchData` struct 收纳所有锁保护数据
- 用 `std::get_if` / `std::visit` + `overloaded` 做 phase 分派
- Lobby/Running 的 callbacks_ 移除，bind/unbind 由 Match 层完成
- `MatchBase` 从 `bot_core/` 移至 `game_framework/`
- Match 删除死代码：`StartTimer`、`StopTimer`、`Eliminate`、`Hook`、`Activate`、`IsInDeduction`、`GameConfigOver`
- `match_phase_guard.h` 精简为仅 `MatchPhaseMutex = std::mutex`

### 2. reproc 子进程修复（commit `135d406`）

`third_party/reproc` submodule：
- `sigdelset(SIGKILL)` + `sigdelset(SIGSTOP)`：macOS 上 `pthread_sigmask` 拒绝阻塞这两个信号，导致 `reproc_start` 返回 `EINVAL`
- `MAX_FD_LIMIT` 从 `INT_MAX` 降为 4096：macOS `getrlimit(RLIMIT_NOFILE)` 返回 `INT64_MAX`，子进程卡死在 fd 关闭循环

### 3. nlohmann::json 初始化修复（commit `4307660` / `8e00878`）

- `mutex_protect_wrapper` 的 variadic 构造函数使用 `obj_{args...}` 花括号初始化，触发 nlohmann::json 的 `initializer_list` 构造函数，将单个对象误包装成数组。改为 `obj_(args...)` 圆括号初始化。
- `LoadConfig` 增加空字符串检查：`conf_path` 为 `""` 时视为 `nullptr`，返回空 object
- `test_bot.cc` 的 `BotCtx` 初始化从 `nlohmann::json{}` 改为 `nlohmann::json::object()`

## 当前编译 & 测试状态

**编译**：`cmake -B ./build -DWITH_TEST=ON && cmake --build ./build --target test_bot` → **成功**

**测试（6/6 PASS）**：

| 测试名 | 验证内容 |
|--------|----------|
| `join_game_without_player_limit` | 加入无人数上限的 lobby |
| `pub_join_game_failed` | 公群加入不存在游戏 |
| `pri_join_game_failed` | 私聊加入不存在游戏 |
| `terminate_not_begin_match_when_new_game` | 未开始游戏时创建新游戏结束旧局 |
| `join_then_request` | 加入后向游戏插件发送请求 |
| `pub_join_pri_game_failed` / `pri_join_pub_game_failed` | 公/私游戏交叉加入 |

## 待解决（TODO）

### 1. test_bot.cc 的 conf_path 修改（未提交）

文件已修改但未 commit：
- `SetUp` 中创建随机临时配置路径 `/tmp/lgtbot_test_RANDOM/config.json`
- `TearDown` 中删除临时文件
- 目的：`UpdateGameConfig` 需要真实文件路径才能持久化游戏配置选项

### 2. 游戏执行测试挂起

以下测试涉及游戏在子进程中执行，目前**超时挂起**：

| 测试名 | 预期行为 | 实际 |
|--------|----------|------|
| `start_game_immediately_finish` | `%配置 直接结束` 后 `#新游戏 单机`，游戏立即结束 | 子进程游戏开始，stages 执行完成(Over)，但 `ASSERT_PRI_MSG` 超时 |
| `new_single_player_game` | 单人游戏开始后正常结束 | 同上 |
| `new_multi_players_game` | 多人游戏开始后正常结束 | 同上 |

**根因分析**：`%配置 测试游戏 直接结束` 执行路径：
1. `SetDefaultOption("直接结束", ...)` → config_runner 子进程 → 返回 `max_player`/`multiple`
2. `UpdateGameConfig(...)` → `SaveConfig_` → 写 JSON 到 `conf_path`
3. `#新游戏` → `Match::GameStart` → `match_game_runner` 子进程启动
4. `match_game_runner` 读取 game options → `GAME_OPTION(直接结束)` → game plugin 的 `OnStageBegin`

步骤 2 的 `conf_path` 原来为 `""`，`SaveConfig_` 直接 return 不写文件。修改为临时文件后 `SaveConfig` 已成功落盘。

**步骤 4 仍未生效**：game plugin 的 `OnStageBegin` 中没有出现 "游戏直接结束" 广播日志。需进一步追踪 `GameConfigClient::SetDefaultOption` → `match_game_runner` → `GameOptions` 的完整链路，定位配置选项未传达到 game plugin 的具体环节。

### 3. 全量 ctest 通过

`ctest` 会执行 `test_bot`、`test_test_game`、`test_db` 等。目前仅 `test_bot` 的部分用例通过，其他测试目标需单独验证。

## 改动文件一览

### 新增文件
- `bot_core/match_lobby.h/cc` — Lobby 阶段类
- `bot_core/match_running.h/cc` — Running 阶段类
- `bot_core/match_phase_common.h/cc` — Lobby/Running 共用基类
- `bot_core/match_env.h/cc` — MatchContext, MatchMessaging, MatchHelpServices
- `bot_core/match_internal.h/cc` — 内部工具函数
- `bot_core/match_types.h` — MatchState, LobbyStartSnapshot, PlayerInfo 等类型
- `bot_core/match_phase_guard.h` — MatchPhaseMutex typedef
- `game_framework/match_base.h` — 从 bot_core/ 移入
- `utility/mutex_guarded_ptr.h` — RAII 锁守卫
- `utility/overloaded.h` — std::visit 的 overloaded 模式
- `utility/atomic_weak_ptr.h` / `darwin.h` / `mutex.h` / `std.h` — 跨平台 atomic_weak_ptr
- `match_process/ipc_channels.h` — IPC 通信管道

### 修改文件
- `bot_core/match.h/cc` — 核心重构
- `bot_core/bot_core.cc` — 适配新 Match 接口
- `bot_core/bot_ctx.cc` — LoadConfig 空字符串修复
- `bot_core/CMakeLists.txt` — 新增源文件
- `bot_core/match_manager.cc` — 移除 EnsureCallbacks
- `bot_core/msg_sender.h/cc` — 适配接口
- `bot_core/test_bot.cc` — 配置路径、JSON 初始化（待提交）
- `game_framework/mock_match.h` — include 路径更新
- `game_framework/stage_utility.h/cc` — include 路径更新
- `match_process/ipc_match_env.h/cc` — include 路径更新
- `games/othello/mygame.cc` — include 路径更新
- `utility/lock_wrapper.h` — 花括号 → 圆括号修复
- `third_party/reproc` (submodule) — macOS 兼容修复