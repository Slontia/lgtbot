![Logo](./logo.svg)

![image](https://img.shields.io/badge/author-slontia-blue.svg) ![image](https://img.shields.io/badge/language-c++20-green.svg)

## 1 项目简介

LGTBot 是一个基于 C++ 实现的，用于在 **聊天室** 或 **其它通讯软件** 中，实现多人 **文字推理游戏** 的裁判机器人库。

- 提供了通用的 **聊天信息交互接口**，需要由已实现好的机器人框架调用使用
- 支持 **群组公开游戏** 和 **私密游戏** 两种方式，无需在群组内也可以进行游戏
- 内置了 3 种已实现好的游戏，同时提供了通用的**游戏框架**，便于后续开发新游戏
- 基于 CMake 编译，支持跨平台

其中，“LGT”源自日本漫画家甲斐谷忍创作的《Liar Game》中的虚构组织“**L**iar **G**ame **T**ournament事务所”。

本项目是一个面向开发者的机器人库，需要和已有的机器人框架进行结合，如您想直接在通讯工具中使用该机器人库，可以参考以下已结合好的解决方案：

- 基于 Mirai 框架，用于 QQ 通讯软件：[lgtbot-mirai](https://github.com/Slontia/lgtbot-mirai)

## 2 开始

### 2.1 安装依赖

- gflags + glog

		$ sudo yum install gflags-devel glog-devel

- qt

        $ sudo yum install qt5-qtwebkit

- MySQL Connector C++

### 2.2 编译

请确保您的编译器基于 **C++20** 语法，建议使用 g++10 以上版本

	# 完整克隆本项目
	$ git clone github.com/slontia/lgtbot

	# 构建二进制路径
	$ mkdir build

	# 编译项目
	$ cmake .. -DMYSQL_CONNECTOR_PATH=/usr/include/mysql-cppconn/ -DMYSQL_CONNECTOR_LIBRARIES=/usr/lib64/libmysqlcppconn.so
	$ make

## 3 项目组成

- `bot_core`：机器人的核心逻辑，包括开始游戏、结束游戏、查看信息等功能
- `bot_simulator`：用于模拟机器人与用户交互的客户端，可以在测试时使用
- `bot_unittest`：机器人单元测试
- `game_framework`：游戏框架，包括游戏的底层逻辑（如阶段切换等）
- `games`：已经实现好的游戏
- `utility`：通用组件

## 4 使用方法


### 4.1 使用机器人框架链接LGTBot库

**如您在理解下述使用方式中遇到困难，请参考 simulator 的调用方式[bot_simulaor](https://github.com/Slontia/lgtbot-mirai/blob/master/bot_simulaor/simulaor.cpp)，或参考基于Mirai框架实现的范例[lgtbot-mirai](https://github.com/Slontia/lgtbot-mirai/blob/master/src/main.cpp)。**

执行编译后，会在 `build`目录下生成`libbot_core.so`和`libbot_core_static.a`两文件，前者为LGTBot的动态库，后者为LGTBot的静态库。

#### 4.1.1 创建一个MsgSender

MsgSender是一个用来向用户发送消息的接口，客户需要继承`MsgSender`类，并完善以下成员函数的实现：

- `SendString`：用于记录一段需要发送给用户的字符串
- `SendAt`：用于记录At信息
- `~MsgSender`：将所记录的信息发送给用户

#### 4.1.2 调用LGTBot提供的接口

LGTBot 所提供的接口，定义在`bot_core/bot_core.h`中，包括：

- `Init`：进行LGTBot的初始化工作，需要插入供LGTBot回调的接口：
	- `this_uid`：当前机器人所属的用户ID
	- `new_msg_sender_cb`：创建一个用于回复消息的MsgSender
	- `delete_msg_sender_cb`：清理MsgSender，同时将消息发送给用户
	- `game_path`：游戏所在的路径
	- `admins | admin_count`：管理员用户ID列表 | 数量
- `Release`：释放机器人
- `HandlePrivateRequest`：当收到用户私信的消息时，需要调用该接口，将消息转发给 LGTBot
- `HandlePublicRequest`：当收到群组中用户发送的消息时，需要调用该接口，将消息转发给 LGTBot
- `ConnectDatabase`：连接数据库（目前只支持MySQL）

#### 4.1.3 支持配置项

LGTBot 本身不提供配置项，但您可以通过配置项更改 glog 输出日志的模式。详情请参考 glog 官方文档。

### 4.2 使用simulator测试机器人

执行编译后，会在`build`目录下生成`simulator`可执行文件。

可以通过以下参数控制`simulator`行为：

- `game_path`：游戏所在路径，默认为`./plugins"`
- `history_filename`：历史命令保存路径，默认为`./.simulator_history.txt`
- `color`：输出内容是否带颜色，默认为true
- `bot_uid`：机器人用户ID，默认为114514
- `admin_uid`：管理员用户ID，默认为0
- 数据库相关配置项：
	- `db_addr`：数据库地址（`<ip>:<port>`，如`127.0.0.1:3306`）。如果不配置这个数据项，则不会进行数据库连接操作。
	- `db_user`：数据库用户，默认为`root`
	- `db_passwd`：数据库密码，默认为空
	- `db_name`：数据库名称，默认为`lgtbot_simulator`
