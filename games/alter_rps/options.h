#ifndef ALTER_RPS_OPTIONS_H_
#define ALTER_RPS_OPTIONS_H_

enum class Type : int { ROCK = 0, PAPER = 1, SCISSOR = 2 };

struct Card
{
    Type type_;
    int point_;
    auto operator<=>(const Card&) const = default;
};

class CardChecker : public MsgArgChecker<Card>
{
  public:
    CardChecker() : arith_checker_(-9, 9, "点数") {}
    virtual ~CardChecker() {}

    virtual std::string FormatInfo() const override
    {
        return "石头|剪刀|布" + arith_checker_.FormatInfo();
    }

    virtual std::string EscapedFormatInfo() const override
    {
        return "石头|剪刀|布" + arith_checker_.EscapedFormatInfo();
    }

    virtual std::string ColoredFormatInfo() const override
    {
        return HTML_COLOR_FONT_HEADER(orange) "石头|剪刀|布" + arith_checker_.EscapedFormatInfo() + HTML_FONT_TAIL;
    }

    virtual std::string ExampleInfo() const override
    {
        return "剪刀" + arith_checker_.ExampleInfo();
    }

    virtual std::optional<Card> Check(MsgReader& reader) const
    {
        if (!reader.HasNext()) {
            return std::nullopt;
        }
        const auto str = reader.NextArg();
        const auto cut_prefix = [](const std::string& str, const std::string_view prefix)
            {
                return str.starts_with(prefix) ? str.substr(prefix.size()) : "";
            };
        std::optional<int> point = 0;
        if (const auto point_str = cut_prefix(str, "剪刀"); !point_str.empty() && (point = arith_checker_.Check(point_str)).has_value()) {
            return Card{.type_ = Type::SCISSOR, .point_ = *point};
        } else if (const auto point_str = cut_prefix(str, "石头"); !point_str.empty() && (point = arith_checker_.Check(point_str)).has_value()) {
            return Card{.type_ = Type::ROCK, .point_ = *point};
        } else if (const auto point_str = cut_prefix(str, "布"); !point_str.empty() && (point = arith_checker_.Check(point_str)).has_value()) {
            return Card{.type_ = Type::PAPER, .point_ = *point};
        }
        return std::nullopt;
    }

    virtual std::string ArgString(const Card& value) const
    {
        return (value.type_ == Type::ROCK ? "石头" :
                value.type_ == Type::PAPER ? "布" : "剪刀") + std::to_string(value.point_);
    }

  private:
    ArithChecker<int> arith_checker_;
};

#endif

EXTEND_OPTION("每回合落子时间", 时限, (ArithChecker<uint32_t>(10, 300, "时间（秒）")), 90)
EXTEND_OPTION("设置双方的 9 张手牌（空表示随机手牌）", 手牌, (OptionalChecker<FixedSizeRepeatableChecker<CardChecker>>(9)), std::nullopt)
