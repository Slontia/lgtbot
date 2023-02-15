<div align="center">

![Logo](./images/logo_transparent_colorful.svg)

![image](https://img.shields.io/badge/author-slontia-blue.svg) ![image](https://img.shields.io/badge/language-c++20-green.svg)

</div>

## 1 项目简介

LGTBot 是一个基于 C++ 实现的，用于在 **聊天室** 或 **其它通讯软件** 中，实现多人 **文字推理游戏** 的裁判机器人库。

- 提供了通用的 **聊天信息交互接口**，需要由已实现好的机器人框架调用使用
- 支持 **群组公开游戏** 和 **私密游戏** 两种方式，无需在群组内也可以进行游戏
- 内置了 24 种已实现好的游戏，同时提供了通用的**游戏框架**，便于后续开发新游戏
- 基于 CMake 编译，支持跨平台

其中，「LGT」源自日本漫画家甲斐谷忍创作的《Liar Game》中的虚构组织「**L**iar **G**ame **T**ournament事务所」。

本项目是一个面向开发者的机器人库，需要和已有的机器人框架进行结合，如您想直接在通讯工具中使用该机器人库，可以参考以下已结合好的解决方案：

- 基于 Mirai 框架，用于 QQ 通讯软件：[lgtbot-mirai](https://github.com/Slontia/lgtbot-mirai)

## 2 开始

请确保您的编译器支持 **C++20** 语法，建议使用 g++10 以上版本

    # 安装依赖库（Ubuntu 系统）
	$ sudo apt-get install -y libgoogle-glog-dev libgflags-dev libgtest-dev libsqlite3-dev libqt5webkit5-dev

	# 完整克隆本项目
	$ git clone github.com/slontia/lgtbot

    # 安装子模块
    $ git submodule update --init --recursive

	# 构建二进制路径
	$ mkdir build

	# 编译项目
    $ cmake .. -DWITH_GCOV=OFF -DWITH_ASAN=OFF -DWITH_GLOG=OFF -DWITH_SQLITE=ON -DWITH_TEST=ON -DWITH_SIMULATOR=ON -DWITH_GAMES=ON
	$ make

## 3 项目组成

- `bot_core`：机器人的核心逻辑，包括开始游戏、结束游戏、查看信息等功能
- `bot_simulator`：用于模拟机器人与用户交互的客户端，可以在测试时使用
- `bot_unittest`：机器人单元测试
- `game_framework`：游戏框架，包括游戏的底层逻辑（如阶段切换等）
- `games`：已经实现好的游戏
- `utility`：通用组件
- `game_util`：通用的游戏组件

## 4 使用方法

### 4.1 使用机器人框架链接LGTBot库

**如您在理解下述使用方式中遇到困难，请参考 simulator 的调用方式[bot_simulaor](https://github.com/Slontia/lgtbot-mirai/blob/master/bot_simulaor/simulaor.cpp)，或参考基于Mirai框架实现的范例[lgtbot-mirai](https://github.com/Slontia/lgtbot-mirai/blob/master/src/main.cpp)。**

执行编译后，会在 `build` 目录下生成 `libbot_core.so` 和 `libbot_core_static.a` 两文件，前者为 LGTBot 的动态库，后者为 LGTBot 的静态库。

#### 4.1.1 实现接口，支持机器人发送消息

客户需要实现以下几个接口，用来让机器人可以发送消息。

- `void* OpenMessager(const char* id, bool is_uid)`：客户需要在此函数中创建一个「消息发送对象」，返回该对象的指针。参数：
    - `const char* id`：消息发送目标的 ID，可能为用户 ID（即私信用户）或群组 ID
    - `bool is_uid`：如果为 true，为用户 ID，否则为群组 ID
- `void MessagerPostText(void* p, const char* data, uint64_t len)`：记录一条文本消息。参数：
    - `void* p`：指向通过 `OpenMessager` 创建的对象
    - `const char* data`：需要发送的字符串
    - `uint64_t len`：字符串长度
- `void MessagerPostUser(void* p, const char* uid, bool is_at)`：记录一条用户相关的消息。参数：
    - `void* p`：指向通过 `OpenMessager` 创建的对象
    - `const char* uid`：用户 ID
    - `bool is_at`：如果为 true，表示 at 对应的用户，否则只需要获取该用户的名字
- `void MessagerPostImage(void* p, const std::filesystem::path::value_type* path)`：记录一条图片消息。参数：
    - `void* p`：指向通过 `OpenMessager` 创建的对象
    - `const std::filesystem::path::value_type* path`：要发送的图片所在路径
- `void MessagerFlush(void* p)`：将目前为止所记录的消息发送出去。参数：
    - `void* p`：指向通过 `OpenMessager` 创建的对象
- `void CloseMessager(void* p)`：释放消息发送对象。参数：
    - `void* p`：指向通过 `OpenMessager` 创建的对象
- `const char* GetUserName(const char* uid, const char* gid)`：返回玩家昵称或群昵称。参数：
    - `const char* uid`：用户 ID
    - `const char* gid`：群组 ID，可为空，若为空，表示获取用户的昵称；若非空，表示获取玩家在该群的群昵称
- `bool DownloadUserAvatar(const char* uid, const std::filesystem::path::value_type* dest_filename)`：下载玩家头像，返回是否下载成功。参数：
    - `const char* uid`：用户 ID
    - `const std::filesystem::path::value_type* path`：下载头像图片的目标路径

其中，`MessagerPost*` 的三个接口，并不负责实际发送消息，仅仅是将要发送的消息记录下来，真正发送是在 `MessagerFlush` 接口中进行。

函数实现后，不需要客户进行调用，函数的定义会在链接 LGTBot 库的过程中被载入，LGTBot 库负责进行函数的调用。

#### 4.1.2 调用接口，实现机器人接收消息

LGTBot 所提供的接口，定义在`bot_core/bot_core.h`中，包括：

- `Init`：进行 LGTBot 的初始化工作，并返回一个 bot handler，需要的参数在 BotOption 类中定义，包括：
	- `uint64_t this_uid_`：当前机器人所属的用户ID
    - `const char* game_path_`：游戏库所在路径（即编译出来的 plugins 目录的路径），建议为绝对路径，不可为空
    - `const char* image_path_`：（暂未使用）
	- `const uint64_t* admins_`：管理员用户 ID 列表（管理员可以使用管理命令），**末尾元素需为 0**，可为空，说明无管理员
    - `const char* db_path_`：SQLite 文件路径，用于存储对战数据，不可为空
- `Release`：释放机器人。参数：
    - `void* bot`：通过 `Init` 函数获取到的指针
- `HandlePrivateRequest`：当收到用户私信的消息时，需要调用该接口，将消息转发给 LGTBot。参数：
    - `void* bot`：通过 `Init` 函数获取到的指针
    - `const uint64_t uid`：发送消息的用户 ID
    - `const char* msg`：发送的消息
- `HandlePublicRequest`：当收到群组中用户发送的消息时，需要调用该接口，将消息转发给 LGTBot
    - `void* bot`：通过 `Init` 函数获取到的指针
    - `const uint64_t gid`：发送消息的群组 ID
    - `const uint64_t uid`：发送消息的用户 ID
    - `const char* msg`：发送的消息

#### 4.1.3 支持配置项

LGTBot 本身不提供配置项，但您可以通过配置项更改 glog 输出日志的模式。详情请参考 glog 官方文档。

### 4.2 使用simulator测试机器人

执行编译后，会在 `build` 目录下生成 `simulator` 可执行文件，执行后会进入命令行界面，可在此测试机器人行为。

- 命令格式：`<group_id> <user_id> <msg...>`
- 例如：「1 2 #新游戏 投标扑克」，表示用户 2 在群组 1 中发送了「#新游戏 投标扑克」指令给机器人
- 若为私信，则将 group\_id 置为 「-」，例如 「- 2 #新游戏 投标扑克」，表示用户 2 私信发送了「#新游戏 投标扑克」指令给机器人
- 通过 `quit` 命令退出

`simulator` 有如下附加参数，可在执行二进制时指定：

- `admin_uid`：管理员用户 ID，默认为 1
- `bot_uid`：机器人用户 ID，默认为 114514
- `color`：输出内容是否带颜色，默认为 true
- `db_path`：SQLite 数据文件路径，默认为 "simulator.db"
- `game_path`：游戏所在路径，默认为 "./plugins"
- `history_filename`：历史命令保存路径，默认为 `./.simulator_history.txt`

