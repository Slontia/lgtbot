// Copyright (c) 2018-present, JiaQi Yu <github.com/tiedanGH>. All rights reserved.
//
// This source code is licensed under LGPLv2 (found in the LICENSE file).
//
// 【测试用工具】导出「漫漫长夜」全部区块的单区块例图（边界墙壁 + 内部 3x3，无外侧虚线边框）
//
// ============================== 如何运行 ==============================
//
// 【方式一（推荐）】一键脚本，自动完成编译 + 导出 + 渲染 PNG：
//     bash games/long_night/tool_block_exporter.sh                       # 默认 build / classic
//     bash games/long_night/tool_block_exporter.sh build retro         # 复古材质
//     bash games/long_night/tool_block_exporter.sh build classic 输出目录
//   输出：<输出目录>/<编号>_<名称>.png，默认为 build/long_night_blocks/
//   前置条件：已构建出 <build_dir>/markdown2image（构建过 long_night 即有）
//
// 【方式二】手动分步（本文件只负责生成 HTML，渲染交给 markdown2image）：
//   1. 编译（在仓库根目录执行）
//        g++ -std=c++23 -I. -Ithird_party -O1 -o build/tool_block_exporter \
//            games/long_night/tool_block_exporter.cc utility/html.cc
//   2. 生成每个区块的 HTML
//        ./build/tool_block_exporter \
//            --resource "$(pwd)/build/plugins/long_night/resource/" \
//            --outdir   ./build/blocks_md \
//            --texture  classic            # 可选：classic（默认）/ retro
//   3. 批量渲染为 PNG（--width 需为 区块尺寸+14，见下方 BLOCK_PX 说明）
//        for f in build/blocks_md/*.md; do
//            ./build/markdown2image --input "$f" \
//                --output "build/long_night_blocks/$(basename "$f" .md).png" \
//                --width 204 --with_css=false
//        done
//
// 【关于图片尺寸】单区块 = 4 道墙(10px) + 3 个方格(50px) = 190px 见方。
//   markdown2image 的 --width 实际输出宽度为 width-14，故需传 190+14=204；
//   同时本工具在 HTML 顶部注入 margin/padding 归零样式，避免页面留白。
//
// 【命名规则】普通区块 `<编号>_<名称>.png`（如 45_提款机A.png）；
//   逃生舱带 E 前缀（E15_花园密道A.png）；特殊区块为 S 前缀（S4_黑洞.png）。
// ====================================================================

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <span>
#include <string>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "bot_core/id.h"
#include "utility/html.h"

using namespace std;

// 游戏头文件依赖顺序与 mygame.cc 保持一致
#include "games/long_night/constants.h"
#include "games/long_night/player.h"
#include "games/long_night/grid.h"
#include "games/long_night/map.h"
#include "games/long_night/boss.h"
#include "games/long_night/board.h"

namespace {

string Arg(int argc, char** argv, const string& key, const string& fallback)
{
    for (int i = 1; i + 1 < argc; i++) {
        if (argv[i] == key) return argv[i + 1];
    }
    return fallback;
}

// 文件名安全化：去掉路径分隔符等非法字符
string SafeName(const string& s)
{
    string out;
    for (char c : s) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') continue;
        out += c;
    }
    return out;
}

} // namespace

int main(int argc, char** argv)
{
    const string resource_dir = Arg(argc, argv, "--resource", "./resource/");
    const string out_dir = Arg(argc, argv, "--outdir", "./block_md/");
    const bool retro = Arg(argc, argv, "--texture", "classic") == "retro";

    filesystem::create_directories(out_dir);

    // 使用自定义模式构造 Board，避免抽取区块池；仅用于调用渲染函数
    Board board(resource_dir, retro ? Texture::RETRO : Texture::CLASSIC, BlockMode::CUSTOM, {});

    struct Entry { string file_id; string id; string name; bool is_exit; };
    vector<Entry> entries;
    for (const auto& m : board.unitMaps.all_maps)         entries.push_back({m.id, m.id, m.title, false});
    for (const auto& m : board.unitMaps.all_exits)        entries.push_back({"E" + m.id, m.id, m.title, true});
    for (const auto& m : board.unitMaps.all_special_maps) entries.push_back({m.id, m.id, m.title, false});

    // 归零页面外边距，使渲染结果恰好等于区块本身尺寸（无任何边界留白）
    const string reset_css = "<style>html,body{margin:0;padding:0;}"
        "table{border-collapse:collapse;border-spacing:0;}</style>";

    int ok = 0;
    for (const auto& e : entries) {
        const string html = reset_css + board.GetSingleBlockNoBorder(e.id, e.is_exit, SpecialEvent::NONE);
        const string path = out_dir + "/" + SafeName(e.file_id + "_" + e.name) + ".md";
        ofstream ofs(path, ios::binary);
        if (!ofs) {
            cerr << "[错误] 无法写入：" << path << endl;
            continue;
        }
        ofs << html;
        ofs.close();
        cout << path << endl;
        ok++;
    }
    // 单区块尺寸：3 个方格 + 4 道墙壁（含两侧边界墙）
    const int block_px = GRID_SIZE * 3 + WALL_SIZE * 4;
    cerr << "共导出 " << ok << " / " << entries.size() << " 个区块（材质："
         << (retro ? "retro" : "classic") << "）\n"
         << "区块尺寸 " << block_px << "x" << block_px
         << "，渲染时请使用 --width " << (block_px + 14) << endl;
    return ok == static_cast<int>(entries.size()) ? 0 : 1;
}
