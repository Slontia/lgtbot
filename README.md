# cqsdk-vc [![Build status](https://ci.appveyor.com/api/projects/status/b45ik9dass1rnrnj?svg=true)](https://ci.appveyor.com/project/Coxxs/cqsdk-vc)
使用 Visual C++ 编写 酷Q V9 应用。

[下载](https://github.com/CoolQ/cqsdk-vc/archive/master.zip)

文件说明
--------
`CQPdemo.sln` - 示例项目，可以直接在此基础上编写应用

您可以编译为 `com.example.democ.dll`，与 `CQPdemo/com.example.democ.json` 一起放置在 酷Q 的 app 目录下测试

`CQPdemo/com.example.democ.json` - 样例应用的对应信息文件，包含应用的基础信息、事件列表等，请放置在 酷Q 的 app 目录下（无需使用的事件、菜单、权限请在此删除）

`CQPdemo/cqp.h` - 酷Q SDK 头文件，通常无需修改

`CQPdemo/CQP.lib` - CQP.dll 的动态连接 .lib 文件，便于 C、C++ 等调用 酷Q 的方法。

官方网站
--------
主站：https://cqp.cc

文库：https://d.cqp.me
