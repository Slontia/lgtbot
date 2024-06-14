<div align="center">

![Logo](./images/logo_transparent_colorful.svg)

![image](https://img.shields.io/badge/author-slontia-blue.svg) ![image](https://img.shields.io/badge/language-c++20-green.svg)

</div>

## 1 项目简介

LGT-Bot 是一个基于 C++ 实现的，用于在 **网络聊天室** 或 **其它通讯软件** 中，实现多人 **互动游戏** 的裁判机器人库。您可以将 LGT-Bot 接入到聊天室的一个账户中，这样一来聊天室中其他用户便可以通过与向该机器人账户发送文字消息，以创建和参与各种类型的游戏。

其中，「LGT」源自日本漫画家甲斐谷忍创作的《Liar Game》中的虚构组织「**L**iar **G**ame **T**ournament 事务所」。

LGT-Bot 具备以下特性：

- 内置 **36** 款已实现好的游戏，游戏涵盖心理、棋类、扑克、麻将等多个领域
- 支持**群组公开比赛**和**私信比赛**两种参与方式
- 每场比赛结束都会变动玩家的积分情况，积分分为「零和分」、「头名分」和「等级分」三种
    - 零和分：每场比赛的零和分变动，是将每名玩家的游戏得分做**线性变化**后得到的结果（同一局游戏中所有玩家的零和分变动总和为 0，且对于倍率相同的比赛，玩家零和分变动绝对值的平均值相同）
    - 头名分：只有每场比赛中**游戏得分最高和最低的玩家**会增加和减少头名分（对于倍率相同的比赛，玩家头名分变动绝对值的平均值相同）
    - 等级分：每种游戏独立计算等级分，等级分的计算基于 **ELO 等级分算法**，每场比赛结束后根据玩家的当前排名和各个玩家的历史等级分计算出等级分的变动
- 部分游戏提供了成就系统，玩家在某场比赛中满足了对应条件后，即可在成就栏中点亮对应成就

另外对于游戏开发者，LGT-Bot 提供了通用的**游戏框架**。开发者可以以较低的开发成本，实现一个新的用于 LGT-Bot 的游戏。

## 2 使用场景

LGT-Bot 是一个面向开发者的机器人库，需要和已有的机器人框架进行结合，才可接入到对应的聊天室平台。被 LGT-Bot 接入到的聊天室平台，需要提供相应的消息收发接口。

您可能需要编写程序，将聊天室平台提供的接口和 LGT-Bot 提供的接口进行对接，以便您的机器人账户可以执行 LGT-Bot 的逻辑进行消息的收发。您可以参考以下已结合好的解决方案：

- 项目内置的 Simulator 工具：tools/simulator.cc
- 基于 Mirai 框架，用于 QQ 通讯软件：[lgtbot-mirai](https://github.com/Slontia/lgtbot-mirai)
- 基于 khl.py 框架，用于 Kook 通讯软件：[lgtbot-khl](https://github.com/Slontia/lgtbot-khl)

## 3 项目组成

- `bot_core`：机器人内核，包括游戏模块的载入、比赛的管理、数据的持久化等
- `game_framework`：游戏框架，即所有游戏通用的底层逻辑，包括阶段的切换、玩家状态的变化等
- `games`：已经实现好的游戏
- `game_util`：一些会被游戏用到的组件
- `utility`：一些可能会被内核和游戏用到的通用组件，例如计时器、HTML 常用组件、消息校验器等
- `tools`：供研发或运维使用的工具
    - simulator.cc：用于模拟用户发送消息的工具 Simulator，开发者可以利用该工具测试内核逻辑或游戏逻辑的正确性
    - score_updater.cc：分数更新工具，当分数的计算方式发生改变的时候，运维可利用该工具更新数据库中各个历史赛事的用户积分变动情况
- `game_template`：游戏模板，游戏开发者可以将 `game_template` 拷贝到 `games` 目录下以开始实现一个新的游戏
- `third_party`：第三方库

## 4 部署方法

在开始之前：

- 请确保您的编译器支持 **C++20** 语法，例如您可以使用 g++11 及以上版本的编译器。
- 请确保您已经为您的 GitHub 账户正确配置了 SSH 秘钥，以支持远程仓库的拉取。如果您尚未配置，可参考[新增 SSH 密钥到 GitHub 帐户](https://docs.github.com/zh/authentication/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account)。

请在 Bash 或其他适当 shell 环境执行以下命令，以部署 LGT-Bot 开发环境：

    # 安装依赖库（Windows 系统 + MSYS2 MinGW 终端，注意不要使用 MSYS2 MSYS 终端）

    # 完整克隆本项目
    git clone git@github.com:slontia/lgtbot.git
    cd lgtbot

    # 安装子模块
    git submodule update --init --recursive

    # 构建二进制路径
    mkdir build
    cd build

如果您是 Ubuntu 系统用户：

    # 安装依赖库
    sudo apt-get install -y libgoogle-glog-dev libgflags-dev libgtest-dev libsqlite3-dev libqt5webkit5-dev

    # 编译项目
    cmake .. -DWITH_GCOV=OFF -DWITH_ASAN=OFF -DWITH_GLOG=OFF -DWITH_SQLITE=ON -DWITH_TEST=ON -DWITH_GAMES=ON
    make

如果您是 Windows 系统用户，建议使用 MSYS2 MinGW 作为开发环境：

    # 安装依赖库
    pacman -Su git mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-gcc mingw-w64-x86_64-qtwebkit mingw-w64-x86_64-gflags mingw-w64-x86_64-gtest mingw-w64-x86_64-glog

    # 编译项目
    cmake -G "MSYS Makefiles" .. -DWITH_GCOV=OFF -DWITH_ASAN=OFF -DWITH_GLOG=OFF -DWITH_SQLITE=ON -DWITH_TEST=ON -DWITH_GAMES=ON -DCMAKE_MAKE_PROGRAM=mingw32-make.exe
    mingw32-make

## 5 链接 LGT-Bot 库以对接聊天室平台

**如您在理解下述使用方式中遇到困难，可以参考 [Simulator](https://github.com/Slontia/lgtbot-mirai/blob/master/bot_simulaor/simulaor.cpp) 的实现，或参考基于 Mirai 框架实现的范例 [lgtbot-mirai](https://github.com/Slontia/lgtbot-mirai/blob/master/src/main.cpp)。**

执行编译指令后，`libbot_core.so` 和 `libbot_core_static.a` 两文件会在 `build` 目录下会生成，分别为 LGT-Bot 内核的动态库和静态库。开发者可以根据需要选择其一进行链接。

LGT-Bot 所提供的接口，被定义在`bot_core/bot_core.h` 文件中。请参考文件中各个接口函数的注释，了解其具体作用和参数说明。

### 5.1 LGT-Bot 对象的创建和销毁

`LGTBot_Create` 负责创建一个 LGT-Bot 对象，`LGTBot_Release` 负责销毁一个 LGT-Bot 对象。

当调用 `LGTBot_Create` 的时候，开发者需要构造一个 `LGTBot_Option` 对象，该 `LGTBot_Option` 对象包含 LGT-Bot 所必需的配置信息。这个配置信息中，最重要的是 `game_path_` 和 `db_path_`。前者是存放**游戏动态库**的路径（也就是编译生成的 `plugins` 目录对应的路径），后者是 **SQLite 数据库文件**的路径（用来持久化用户数据）。此外，`callbacks_` 中声明了若干个回调函数的函数指针，开发者需要实现对应的函数供 LGT-Bot 回调，这一点我们在 5.3 节做具体说明。

`LGTBot_Create` 返回非空 `void*` 指针标志着 LGT-Bot 对象的创建成功，该 `void*` 指针即指向创建出来的 LGT-Bot 对象。此后每需要调用 LGT-Bot 的其它接口时，都需要将该 `void*` 指针作为接口的第一个参数传入。

### 5.2 LGT-Bot 接收用户发送的消息

`LGTBot_HandlePrivateRequest` 负责接收用户私信发送的消息，`LGTBot_HandlePublicRequest` 负责接收用户在群组中公开发送的消息。

对于大多数聊天室平台提供的开发框架，当收到用户发送的消息时，特定的函数会被回调。开发者需要在该回调函数中去调用 `LGTBot_HandlePrivateRequest` 或 `LGTBot_HandlePublicRequest`，以将用户发送的消息传达给 LGT-Bot。

`LGTBot_HandlePrivateRequest` 接收 `user_id` 和 `msg` 两个参数，`user_id` 唯一标识发送消息的用户，`msg` 是该用户所发送的消息。`LGTBot_HandlePublicRequest` 比 `LGTBot_HandlePrivateRequest` 多一个 `group_id` 参数，唯一标识用户发送消息时所在的群组。

### 5.3 实现回调接口，供 LGT-Bot 发送消息

`LGTBot_Callback` 是一个结构体，它的每个成员变量都是一个函数指针。开发者需要实现若干函数，将函数的指针赋值给 `LGTBot_Callback` 的对应函数指针成员变量，再将 `LGTBot_Callback` 对象赋值给 `LGTBot_Option` 的 `callbacks_` 成员，并传入 `LGTBot_Create`，以注册这些回调函数。

其中，`get_user_name`、`get_user_name_in_group` 和 `download_user_avatar` 负责获取用户的昵称、群内昵称和头像，而 `handle_messages` 则负责给用户发送消息。

### 5.4 示例

假设 `ChatRoom` 是第三方聊天室提供的消息收发接口。

``` c++

void GetUserName(void* handler, char* const buffer, const size_t size, const char* const user_id)
{
    strncpy(buffer, user_id, size); // 将 User ID 直接作为昵称
}

void GetUserNameInGroup(void* handler, char* const buffer, const size_t size, const char* group_id, const char* const user_id)
{
    snprintf(buffer, size, "%s(gid=%s)", user_id, group_id); // 将 Group ID 拼接在 User ID 后面作为群内昵称
}

int DownloadUserAvatar(void* handler, const char* const user_id, const char* const dest_filename)
{
    // 这里的 handler 参数的值，与配置中的 handler_ 成员变量的值相等
    return static_cast<ChatRoom*>(handler)->download_user_avatar(user_id, dest_filename);
}

void HandleMessages(void* handler, const char* const id, const int is_uid, const LGTBot_Message* messages, const size_t size)
{
    // 将要给用户或群组发送的信息直接标准输出出来
    printf("message to %s %s: ", is_uid ? "user" : "group", id);
    for (size_t i = 0; i < size; ++i) {
        const auto& msg = messages[i];
        const char* const format =
            msg.type_ == LGTBOT_MSG_TEXT         ? "%s"              :
            msg.type_ == LGTBOT_MSG_USER_MENTION ? "@%s"             :
            msg.type_ == LGTBOT_MSG_IMAGE        ? "[image_path=%s]" : (assert(false), "");
        printf(format, msg.str_);
    }
}

bool RunLgtBot(ChatRoom& chat_room)
{
    // 初始化配置
    LGTBot_Option options = LGTBot_InitOptions();  // 获取空的配置对象
    options.game_path_ = "./plugins";  // plugins 目录下存放游戏动态库
    options.db_path_ = "./lgtbot.db";  // lgtbot.db 是存放用户数据的 SQLite 数据库文件
    options.handler_ = &chat_room;     // 便于在回调函数中，从 chat_room 获取信息
    options.callbacks_.get_user_name = GetUserName;
    options.callbacks_.get_user_name_in_group = GetUserNameInGroup;
    options.callbacks_.download_user_avatar = DownloadUserAvatar;
    options.callbacks_.handle_messages = HandleMessages;

    // 创建 LGT-Bot
    const char* errmsg = nullptr;
    void* const bot = LGTBot_Create(&option, &errmsg);
    if (!bot) {
        std::cerr << errmsg << std:endl;
        return false;
    }

    // 账号收到消息之后，将消息传递给 LGT-Bot
    chat_room.register_message_handler(
        [&](const char* const user_id, const char* const group_id, const char* const message)
        {
            if (group_id) {
                LGTBot_HandlePublicRequest(bot, group_id, user_id, message);
            } else {
                LGTBot_HandlePrivateRequest(bot, user_id, message);
            }
        });

    // 账号提供服务，直到按下任意按键
    getchar();

    // 释放 LGT-Bot
    LGTBot_Release(bot);
}

```

## 6 贡献者

感谢以下开发者对本项目的贡献！

<div>
  <a href="https://github.com/slontia/lgtbot/graphs/contributors">
    <img src="https://contrib.rocks/image?repo=slontia/lgtbot" />
  </a>
</div>
