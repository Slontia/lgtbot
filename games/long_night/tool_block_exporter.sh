#!/usr/bin/env bash
# 【测试用工具】导出「漫漫长夜」全部区块的单区块例图（边界墙壁 + 内部 3x3，无外侧虚线边框）
#
# ============================== 如何运行 ==============================
# 在仓库根目录执行（WSL / Linux）：
#     bash games/long_night/tool_block_exporter.sh [build_dir] [texture] [out_dir]
#
# 参数（均可省略）：
#     build_dir  构建目录，默认 build（需已构建出其中的 markdown2image）
#     texture    材质，classic（默认）或 retro
#     out_dir    输出目录，默认 <build_dir>/long_night_blocks
#
# 示例：
#     bash games/long_night/tool_block_exporter.sh
#     bash games/long_night/tool_block_exporter.sh build retro build/long_night_blocks_retro
#
# 输出：<out_dir>/<编号>_<名称>.png，每张 190x190（4 道墙 10px + 3 个方格 50px）
#       普通区块如 45_提款机A.png；逃生舱带 E 前缀；特殊区块为 S 前缀
#
# 手动分步运行方式见 tool_block_exporter.cc 顶部注释。
# ====================================================================
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${1:-build}"
TEXTURE="${2:-classic}"
OUT_DIR="${3:-${BUILD_DIR}/long_night_blocks}"

BUILD_ABS="${REPO}/${BUILD_DIR}"
RESOURCE_DIR="${BUILD_ABS}/plugins/long_night/resource/"
MD2IMG="${BUILD_ABS}/markdown2image"
EXPORTER="${BUILD_ABS}/tool_block_exporter"
MD_DIR="${BUILD_ABS}/long_night_blocks_md"

# 单区块尺寸：3 个方格(50px) + 4 道墙壁(10px，含两侧边界墙) = 190px
# markdown2image 的 --width 实际输出宽度为 width-14，故需加 14
BLOCK_PX=$((50 * 3 + 10 * 4))
RENDER_WIDTH=$((BLOCK_PX + 14))

[[ -x "${MD2IMG}" ]] || { echo "[错误] 未找到 markdown2image：${MD2IMG}（请先构建）" >&2; exit 1; }

# 资源目录：优先用构建产物，缺失则回退到源码资源
if [[ ! -d "${RESOURCE_DIR}" ]]; then
    RESOURCE_DIR="${REPO}/games/long_night/resource/"
fi

echo "[1/3] 编译导出工具…"
g++ -std=c++23 -I"${REPO}" -I"${REPO}/third_party" -O1 \
    -o "${EXPORTER}" \
    "${REPO}/games/long_night/tool_block_exporter.cc" \
    "${REPO}/utility/html.cc"

echo "[2/3] 生成区块 HTML（材质：${TEXTURE}）…"
rm -rf "${MD_DIR}"
"${EXPORTER}" --resource "${RESOURCE_DIR}" --outdir "${MD_DIR}" --texture "${TEXTURE}" >/dev/null

echo "[3/3] 渲染 PNG（${BLOCK_PX}x${BLOCK_PX}）…"
rm -rf "${REPO}/${OUT_DIR}"
mkdir -p "${REPO}/${OUT_DIR}"
count=0
for md in "${MD_DIR}"/*.md; do
    name="$(basename "${md}" .md)"
    "${MD2IMG}" --input "${md}" --output "${REPO}/${OUT_DIR}/${name}.png" \
        --width "${RENDER_WIDTH}" --with_css=false >/dev/null 2>&1
    count=$((count + 1))
done
rm -rf "${MD_DIR}"

echo "完成：${count} 张区块图 → ${OUT_DIR}/"
