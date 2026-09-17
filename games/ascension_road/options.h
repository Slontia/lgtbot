#include "constants.h"

#ifdef INIT_OPTION_DEPEND
// 小数配置项：解析沿用 ArithChecker，仅重写提示文案。
class DecimalChecker : public ArithChecker<double>
{
  public:
    DecimalChecker(const double min, const double max, const std::string& meaning)
        : ArithChecker<double>(min, max, meaning)
        , info_(meaning + "：" + Trim(min) + "~" + Trim(max))
        , example_(Trim(min)) {}

    virtual std::string FormatInfo() const override { return "<" + info_ + ">"; }
    virtual std::string EscapedFormatInfo() const override { return HTML_ESCAPE_LT + info_ + HTML_ESCAPE_GT; }
    virtual std::string ColoredFormatInfo() const override
    {
        return HTML_COLOR_FONT_HEADER(green) + EscapedFormatInfo() + HTML_FONT_TAIL;
    }
    virtual std::string ExampleInfo() const override { return example_; }
    virtual std::string ArgString(const double& value) const override { return Trim(value); }

    // 去掉 std::to_string 补出的尾随零，如 0.500000 → 0.5
    static std::string Trim(const double value)
    {
        std::string text = std::to_string(value);
        if (text.find('.') != std::string::npos) {
            text.erase(text.find_last_not_of('0') + 1);
            if (!text.empty() && text.back() == '.') {
                text.pop_back();
            }
        }
        return text;
    }

  private:
    const std::string info_;
    const std::string example_;
};
#endif

EXTEND_OPTION("每位修士的初始血量", 血量, (ArithChecker<uint32_t>(5, 100, "血量")), DEFAULT_INIT_HP)
EXTEND_OPTION("晋升一个境界所需的修为，可带小数", 修为, (DecimalChecker(0.1, 10, "修为")), DEFAULT_CULTIVATE_STEP)
EXTEND_OPTION("玩家数达到该人数时，天道随机将一人改道为半仙；取满上限则永不改道", 半仙人数, (ArithChecker<uint32_t>(MIN_PLAYER, MAX_PLAYER + 1, "人数")), DEFAULT_BANXIAN_THRESHOLD)
EXTEND_OPTION("从该回合起，天道每回合摧毁一个无人区域；设为 99 则永不摧毁", 摧毁回合, (ArithChecker<uint32_t>(1, DESTROY_NEVER, "回合")), DEFAULT_DESTROY_START_ROUND)
EXTEND_OPTION("仙器降世后，每回合判定破碎仙器陨落的概率（百分比）", 仙器概率, (ArithChecker<uint32_t>(0, 100, "百分比")), DEFAULT_ARTIFACT_CHANCE)
EXTEND_OPTION("游戏回合上限，届满仍未分出胜负则以残存血量论高下", 回合数, (ArithChecker<uint32_t>(5, 100, "回合数")), DEFAULT_MAX_ROUND)
EXTEND_OPTION("每个阶段的行动时间限制", 时限, (ArithChecker<uint32_t>(30, 3600, "超时时间（秒）")), 120)
